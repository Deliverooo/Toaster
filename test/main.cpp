#include <future>
#include <print>
#include <toast_os/console.hpp>
#include <toast_os/entry_points.hpp>

#include <toast_kernel/application.hpp>

#include <stb/stb_image.h>

#include "toast_asset/mesh_importer.hpp"
#include "toast_asset/texture_importer.hpp"
#include "toast_gpu/upload.hpp"
#include "toast_kernel/camera.hpp"
#include "toast_kernel/events/window_event.hpp"
#include "toast_render/mesh.hpp"
#include "toast_render/texture.hpp"

using namespace toaster;

#include <test.vert.h>
#include <test.frag.h>

class TestLayer : public IAppLayer
{
public:
	TestLayer()           = default;
	~TestLayer() override = default;

	auto onInit() -> void override
	{
		m_textureManager = makeUnique<render::TextureManager>(m_renderCtx);
		m_meshManager    = makeUnique<render::MeshManager>();

		asset::TextureImporter texture_importer{m_textureManager.get()};
		m_textureReal = texture_importer.importFromFile("../test/resources/textures/doorbell_pig.jpg");

		m_cpuMeshData = std::async(std::launch::async, []()
		{
			return asset::MeshImporter::importStaticFromFile(R"(C:\dev\Toaster-2.0\resources\meshes\Backrooms.fbx)");
		});
		// auto cpu_mesh_data{asset::MeshImporter::importStaticFromFile("../test/resources/meshes/Orbo_Geo.gltf")};

		gpu::SamplerHandle sampler{gpu::createSampler(gpu::SamplerDesc{})};

		m_samplerHeapSlot = gpu::allocSamplerHeapSlot(m_renderCtx->getSamplerHeap());
		gpu::writeSamplerDescriptor(m_renderCtx->getSamplerHeap(), m_samplerHeapSlot, sampler);

		m_vs = gpu::createShader(gpu::ShaderDesc{
									 "main",
									 c_test_vert_bytecode,
									 sizeof(c_test_vert_bytecode) / sizeof(uint32),
									 gpu::EShaderStageFlagBits::eVertex,
									 gpu::EShaderStageFlagBits::ePixel
								 });

		m_ps = gpu::createShader(gpu::ShaderDesc{
									 "main",
									 c_test_frag_bytecode,
									 sizeof(c_test_frag_bytecode) / sizeof(uint32),
									 gpu::EShaderStageFlagBits::ePixel,
									 gpu::EShaderStageFlagBits::eNone
								 });

		m_camera = Camera{90.0f, m_app->getWindow().getAspectRatio()};
		{
			gpu::BufferDesc camera_buffer_desc{};
			camera_buffer_desc.size       = sizeof(CameraCB);
			camera_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eUniformBuffer;
			camera_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

			m_cameraBuffers.resize(Application::maxFramesInFlight);
			for (auto &buffer: m_cameraBuffers)
				buffer = gpu::createBuffer(camera_buffer_desc);
		}

		m_secondaryBuffers.resize(Application::maxFramesInFlight);
		for (auto &cmd: m_secondaryBuffers)
			cmd.emplace_back(gpu::getOrCreateCommandList(gpu::EQueueType::eGraphics, true));

		gpu::TextureDesc depth_desc{};
		depth_desc.extent = {m_app->getWindow().getSize(), 1u};
		depth_desc.format = gpu::EFormat::eD32Sfloat;
		depth_desc.usage  = gpu::ETextureUsageFlagBits::eDepthStencilAttachment;
		m_depthAttachment = gpu::createTexture(depth_desc);
	}

	auto onDestroy() -> void override
	{
		gpu::destroyTexture(m_depthAttachment);

		gpu::destroyShader(m_ps);
		gpu::destroyShader(m_vs);

		gpu::freeSamplerHeapSlot(m_renderCtx->getSamplerHeap(), m_samplerHeapSlot);

		for (auto &cmd_vec: m_secondaryBuffers)
			for (auto &cmd: cmd_vec)
				gpu::freeCommandList(cmd);

		m_meshManager.reset();
		m_textureManager.reset();
	}

	auto onUpdate(float32 p_dt) -> void override
	{
		if (m_inputCtx->isMouseButtonDown(EMouseButton::eRight))
		{
			if (m_inputCtx->getCursorMode() != ECursorMode::eDisabled)
				m_inputCtx->setCursorMode(ECursorMode::eDisabled);
			m_camera.onUpdate(*m_inputCtx, p_dt);
		}
		else
		{
			if (m_inputCtx->getCursorMode() != ECursorMode::eNormal)
				m_inputCtx->setCursorMode(ECursorMode::eNormal);
		}

		CameraCB camera_cb{};
		m_camera.populateConstantBuffer(camera_cb);
		gpu::writeBufferData(m_cameraBuffers[m_app->getFrameIndex()], &camera_cb, sizeof(CameraCB));
	}

	auto onRender(gpu::CommandListHandle p_cmd) -> void override
	{
		for (auto &list: m_secondaryBuffers[m_app->getFrameIndex()])
			gpu::resetCommandList(list);

		if (m_cpuMeshData.valid() && m_cpuMeshData.wait_for(std::chrono::seconds(0u)) == std::future_status::ready && !m_mesh)
		{
			auto data{m_cpuMeshData.get()};
			m_mesh = m_meshManager->createStaticMesh(data.vertices, data.indices, data.submeshes);
		}

		auto &secondary_cmd{m_secondaryBuffers[m_app->getFrameIndex()][0]};

		gpu::TextureHandle render_tex{m_app->getWindow().getCurrentTexture()};
		gpu::TextureDesc   render_tex_desc{gpu::getTextureDesc(render_tex)};

		gpu::RenderingInfo rendering_info{};
		rendering_info.colourAttachments = {
			gpu::RenderingAttachmentInfo{gpu::ClearColourValue{1.0f, 0.0f, 1.0f, 1.0f}, render_tex, nullptr, gpu::EAttachmentUsageOP::eClearStore}
		};
		rendering_info.depthAttachment = gpu::RenderingAttachmentInfo{gpu::ClearDepthStencilValue{}, m_depthAttachment};
		rendering_info.renderArea      = tsm::Rect{m_app->getWindow().getSize()};

		gpu::bindResourceHeap(p_cmd, m_renderCtx->getResourceHeap());
		gpu::bindSamplerHeap(p_cmd, m_renderCtx->getSamplerHeap());

		gpu::beginRendering(p_cmd, rendering_info);

		std::future<void> future{
			std::async(std::launch::async, [this, secondary_cmd, render_tex_desc, rendering_info]() -> void
			{
				gpu::CommandListInheritanceInfo inheritance_info{};
				inheritance_info.resourceHeap            = m_renderCtx->getResourceHeap();
				inheritance_info.samplerHeap             = m_renderCtx->getSamplerHeap();
				inheritance_info.colourAttachmentFormats = {render_tex_desc.format};
				inheritance_info.depthAttachmentFormat   = gpu::EFormat::eD32Sfloat;
				inheritance_info.samples                 = gpu::ESampleCount::e1;
				gpu::openCommandList(secondary_cmd, &inheritance_info);

				gpu::bindShaders(secondary_cmd, {m_vs, m_ps});

				gpu::setPrimitiveTopology(secondary_cmd, gpu::EPrimitiveTopology::eTriangleList);
				gpu::setPrimitiveRestart(secondary_cmd, false);

				gpu::setViewport(secondary_cmd, tsm::Viewport{rendering_info.renderArea});
				gpu::setScissor(secondary_cmd, rendering_info.renderArea);

				gpu::setRasterizerDiscardEnable(secondary_cmd, false);
				gpu::setPolygonMode(secondary_cmd, gpu::EPolygonMode::eFill);
				gpu::setCullMode(secondary_cmd, gpu::ECullMode::eBack);
				gpu::setFrontFace(secondary_cmd, gpu::EFrontFace::eCCW);
				gpu::setDepthBias(secondary_cmd, false);
				gpu::setLineWidth(secondary_cmd, 1.0f);

				gpu::setRasterizationSamples(secondary_cmd, gpu::ESampleCount::e1);

				gpu::setDepthState(secondary_cmd, true);
				gpu::setStencilState(secondary_cmd, false);

				struct PushData
				{
					uintptr cameraBuffer;
					uintptr vertexBuffer;
					uintptr indexBuffer;

					uint32 vertexBufferOffset;
					uint32 indexBufferOffset;

					uint32 texture;
					uint32 sampler;
				};

				if (m_mesh)
				{
					const render::StaticMesh &static_mesh{m_meshManager->getStaticMesh(m_mesh)};

					PushData push_data{};
					push_data.cameraBuffer = gpu::getBufferAddress(m_cameraBuffers[m_app->getFrameIndex()]);

					push_data.vertexBuffer = gpu::getBufferAddress(m_meshManager->getStaticMeshVertexBuffer());
					push_data.indexBuffer  = gpu::getBufferAddress(m_meshManager->getStaticMeshIndexBuffer());

					push_data.vertexBufferOffset = gpu::alloc::getAllocationOffset(static_mesh.vertexBufferAllocation) / sizeof(render::StaticMeshVertex);
					push_data.indexBufferOffset  = gpu::alloc::getAllocationOffset(static_mesh.indexBufferAllocation) / sizeof(uint32);

					push_data.texture = m_textureManager->getTexture(m_textureReal).shaderReadHeapSlot;
					push_data.sampler = m_samplerHeapSlot;
					gpu::pushData(secondary_cmd, push_data);

					gpu::bindIndexBuffer(secondary_cmd, nullptr);

					for (const auto &submesh: static_mesh.submeshes)
					{
						gpu::draw(secondary_cmd, submesh.indexCount, 1u, submesh.indexOffset);
					}
				}

				gpu::closeCommandList(secondary_cmd);
			})
		};
		future.wait();
		gpu::executeCommandLists(p_cmd, secondary_cmd);

		gpu::endRendering(p_cmd);
	}

	auto onEvent(Event &p_event) -> void override
	{
		EventDispatcher ed{p_event};
		ed.dispatch<WindowResizeEvent>([this](WindowResizeEvent &p_e)-> bool
		{
			// Recreate the depth buffer
			gpu::destroyTexture(m_depthAttachment);
			gpu::TextureDesc depth_desc{};
			depth_desc.extent = {p_e.getSize(), 1u};
			depth_desc.format = gpu::EFormat::eD32Sfloat;
			depth_desc.usage  = gpu::ETextureUsageFlagBits::eDepthStencilAttachment;
			m_depthAttachment = gpu::createTexture(depth_desc);

			m_camera.onResize(p_e.getAspectRatio());

			return true;
		});
	}

private:
	gpu::TextureHandle m_depthAttachment{nullptr};

	render::TextureHandle m_textureReal{nullptr};

	std::future<asset::MeshImportData> m_cpuMeshData;
	render::StaticMeshHandle           m_mesh{nullptr};

	uint32 m_samplerHeapSlot{UINT32_MAX};

	gpu::ShaderHandle m_vs{nullptr};
	gpu::ShaderHandle m_ps{nullptr};

	Camera                         m_camera;
	std::vector<gpu::BufferHandle> m_cameraBuffers;

	std::vector<std::vector<gpu::CommandListHandle> > m_secondaryBuffers;

	UniquePtr<render::MeshManager>    m_meshManager{nullptr};
	UniquePtr<render::TextureManager> m_textureManager{nullptr};
};

TST_WINMAIN()
{
	os::createOutputConsole();
	{
		Application app{};
		app.addLayer<TestLayer>();
		app.run();
	}

	os::destroyOutputConsole();

	return 0;
}
