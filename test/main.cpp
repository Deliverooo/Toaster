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
#include "toast_scene/scene.hpp"

using namespace toaster;

#include <test.vert.h>
#include <test.frag.h>

struct StaticMeshComponent
{
	render::StaticMeshHandle mesh{nullptr};
};

struct TransformComponent
{
	XMFLOAT3 translation{0.0f, 0.0f, 0.0f};
	XMFLOAT4 orientation{1.0f, 0.0f, 0.0f, 0.0f};
	XMFLOAT3 scale{0.0f, 0.0f, 0.0f};

	[[nodiscard]] auto XM_CALLCONV getTransform() const -> XMMATRIX
	{
		XMVECTOR simd_orientation{XMLoadFloat4(&orientation)};
		XMVECTOR simd_translation{XMLoadFloat3(&translation)};
		XMVECTOR simd_scale{XMLoadFloat3(&scale)};

		XMMATRIX transformation{XMMatrixTransformation(XMVectorZero(), XMVectorZero(), simd_scale, XMVectorZero(), simd_orientation, simd_translation)};
		return transformation;
	}

	[[nodiscard]] auto XM_CALLCONV getTranslation() const -> XMVECTOR { return XMLoadFloat3(&translation); }
	[[nodiscard]] auto XM_CALLCONV getOrientation() const -> XMVECTOR { return XMLoadFloat4(&orientation); }
	[[nodiscard]] auto XM_CALLCONV getScale() const -> XMVECTOR { return XMLoadFloat3(&scale); }

	auto XM_CALLCONV setTranslation(FXMVECTOR p_translation) -> void { XMStoreFloat3(&translation, p_translation); }
	auto XM_CALLCONV setOrientation(FXMVECTOR p_orientation) -> void { XMStoreFloat4(&orientation, p_orientation); }
	auto XM_CALLCONV setScale(FXMVECTOR p_scale) -> void { XMStoreFloat3(&scale, p_scale); }
};

class TestLayer : public IAppLayer
{
public:
	static constexpr uint32 maxDrawCalls{1028u * 1028u * 10u};

	struct ObjectData
	{
		XMFLOAT4X4 model;
		uintptr    materialAddress;
		uint32     vertexBufferOffset;
		uint32     indexBufferOffset;
	};

	auto onInit() -> void override
	{
		m_textureManager = makeUnique<render::TextureManager>(m_renderCtx);
		m_meshManager    = makeUnique<render::MeshManager>();

		std::filesystem::current_path("../test");

		m_whiteTexture = m_textureManager->createTexture(gpu::TextureDesc{
															 tsm::uint3{1u, 1u, 1u},
															 1u,
															 1u,
															 gpu::ETextureType::e2D,
															 gpu::ESampleCount::e1,
															 gpu::EFormat::eR8G8B8A8Srgb,
															 gpu::ETextureUsageFlagBits::eSampled | gpu::ETextureUsageFlagBits::eTransferDst
														 });

		uint32 white_texture_data{0xFFFFFFFF};
		m_textureManager->setData(m_whiteTexture, &white_texture_data, sizeof(uint32));

		gpu::upload::flushUploadsAndWait();
		m_textureManager->pollTextureUploads();

		m_textureImporter = makeUnique<asset::TextureImporter>();
		m_textureReal     = m_textureManager->registerTexture();
		m_textureImporter->asyncLoadTextureFromFile(m_textureManager.get(), m_textureReal, "resources/textures/brick_wall_001_diffuse_8k.png");

		m_meshImporter = makeUnique<asset::MeshImporter>();

		{
			render::StaticMeshHandle orbo_mesh{m_meshManager->registerStaticMesh()};
			m_meshImporter->asyncLoadStaticMeshFromFile(m_meshManager.get(), orbo_mesh, "resources/meshes/Orbo_Geo.gltf");

			m_orboEntity = m_scene.createEntity();
			m_scene.addComponent<StaticMeshComponent>(m_orboEntity, orbo_mesh);
		}

		{
			render::StaticMeshHandle level_mesh{m_meshManager->registerStaticMesh()};
			m_meshImporter->asyncLoadStaticMeshFromFile(m_meshManager.get(), level_mesh, "resources/meshes/Backrooms.fbx");

			m_levelEntity = m_scene.createEntity();
			m_scene.addComponent<StaticMeshComponent>(m_levelEntity, level_mesh);
		}

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

		gpu::BufferDesc indirect_buffer_desc{};
		indirect_buffer_desc.size       = sizeof(gpu::DrawIndirectCommand) * maxDrawCalls;
		indirect_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eIndirectBuffer;
		indirect_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		gpu::BufferDesc object_buffer_desc{};
		object_buffer_desc.size       = sizeof(ObjectData) * maxDrawCalls;
		object_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer;
		object_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		m_indirectBuffers.resize(Application::maxFramesInFlight);
		m_objectDataBuffers.resize(Application::maxFramesInFlight);
		for (uint32 i{0u}; i < Application::maxFramesInFlight; ++i)
		{
			m_indirectBuffers[i]   = gpu::createBuffer(indirect_buffer_desc);
			m_objectDataBuffers[i] = gpu::createBuffer(object_buffer_desc);
		}

		gpu::TextureDesc depth_desc{};
		depth_desc.extent = {m_app->getWindow().getSize(), 1u};
		depth_desc.format = gpu::EFormat::eD32Sfloat;
		depth_desc.usage  = gpu::ETextureUsageFlagBits::eDepthStencilAttachment;
		m_depthAttachment = gpu::createTexture(depth_desc);
	}

	TestLayer() = default;

	~TestLayer() override = default;

	auto onDestroy() -> void override
	{
		gpu::destroyTexture(m_depthAttachment);

		gpu::destroyShader(m_ps);
		gpu::destroyShader(m_vs);

		gpu::freeSamplerHeapSlot(m_renderCtx->getSamplerHeap(), m_samplerHeapSlot);

		for (auto &cmd_vec: m_secondaryBuffers)
			for (auto &cmd: cmd_vec)
				gpu::freeCommandList(cmd);

		for (uint32 i{0u}; i < Application::maxFramesInFlight; ++i)
		{
			gpu::destroyBuffer(m_objectDataBuffers[i]);
			gpu::destroyBuffer(m_indirectBuffers[i]);
		}

		m_meshImporter.reset();
		m_meshManager.reset();
		m_textureImporter.reset();
		m_textureManager.reset();
	}

	auto onUpdate(float32 p_dt) -> void override
	{
		if (m_inputCtx->isKeyDown(EKeyCode::eLeftControl) && m_inputCtx->isKeyPressed(EKeyCode::eE))
			m_app->close();

		if (m_inputCtx->isKeyPressed(EKeyCode::eF11))
		{
			if (m_app->getWindow().isFullscreen())
				m_app->getWindow().setWindowed();
			else
				m_app->getWindow().setFullscreen();
		}

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
		m_textureManager->pollTextureUploads();
		m_meshManager->pollMeshUploads();

		for (auto &list: m_secondaryBuffers[m_app->getFrameIndex()])
			gpu::resetCommandList(list);

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

				render::TextureHandle texture_to_render{m_textureReal};

				if (m_textureManager->getTextureState(m_textureReal) != render::ETextureState::eReady)
					texture_to_render = m_whiteTexture;

				uint32 draw_count{0u};

				gpu::DrawIndirectCommand *mapped_cmd{static_cast<gpu::DrawIndirectCommand *>(gpu::getBufferMappedData(m_indirectBuffers[m_app->getFrameIndex()]))};
				ObjectData *              mapped_object_data{static_cast<ObjectData *>(gpu::getBufferMappedData(m_objectDataBuffers[m_app->getFrameIndex()]))};

				const auto view{m_scene.getRegistry().view<StaticMeshComponent>()};
				view.each([this, &draw_count, mapped_cmd, mapped_object_data]([[maybe_unused]] entt::entity p_entity, const StaticMeshComponent &p_smc) -> void
				{
					const auto mesh_data{m_meshManager->tryGetStaticMeshThreadData(p_smc.mesh)};
					if (mesh_data.has_value() && mesh_data->state == render::EMeshState::eReady)
					{
						const uint32 vertex_buffer_offset{
							static_cast<uint32>(gpu::alloc::getAllocationOffset(mesh_data->vertexBufferAllocation) / sizeof(render::StaticMeshVertex))
						};
						const uint32 index_buffer_offset{static_cast<uint32>(gpu::alloc::getAllocationOffset(mesh_data->indexBufferAllocation) / sizeof(uint32))};

						for (const auto &submesh: mesh_data->submeshes)
						{
							mapped_object_data[draw_count] = ObjectData{{}, 0u, vertex_buffer_offset, index_buffer_offset};
							mapped_cmd[draw_count]         = gpu::DrawIndirectCommand{submesh.indexCount, 1u, submesh.indexOffset, draw_count};
							++draw_count;
						}
					}
				});

				struct PushData
				{
					uintptr cameraBuffer;
					uintptr vertexBuffer;
					uintptr indexBuffer;
					uintptr objectDataBuffer;

					uint32 texture;
					uint32 sampler;
				};
				PushData push_data{};
				push_data.cameraBuffer     = gpu::getBufferAddress(m_cameraBuffers[m_app->getFrameIndex()]);
				push_data.vertexBuffer     = gpu::getBufferAddress(m_meshManager->getStaticMeshVertexBuffer());
				push_data.indexBuffer      = gpu::getBufferAddress(m_meshManager->getStaticMeshIndexBuffer());
				push_data.objectDataBuffer = gpu::getBufferAddress(m_objectDataBuffers[m_app->getFrameIndex()]);
				push_data.sampler          = m_samplerHeapSlot;
				push_data.texture          = m_textureManager->getTexture(texture_to_render).shaderReadHeapSlot;

				gpu::pushData(secondary_cmd, push_data);
				gpu::bindIndexBuffer(secondary_cmd, nullptr);

				if (draw_count)
					gpu::drawIndirect(secondary_cmd, m_indirectBuffers[m_app->getFrameIndex()], 0u, draw_count);

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

	render::TextureHandle m_whiteTexture{nullptr};
	render::TextureHandle m_textureReal{nullptr};

	entt::entity m_levelEntity{entt::null};
	entt::entity m_orboEntity{entt::null};

	uint32 m_samplerHeapSlot{UINT32_MAX};

	gpu::ShaderHandle m_vs{nullptr};
	gpu::ShaderHandle m_ps{nullptr};

	Camera                         m_camera;
	std::vector<gpu::BufferHandle> m_cameraBuffers;

	std::vector<std::vector<gpu::CommandListHandle> > m_secondaryBuffers;

	UniquePtr<render::MeshManager> m_meshManager{nullptr};
	UniquePtr<asset::MeshImporter> m_meshImporter{nullptr};

	UniquePtr<render::TextureManager> m_textureManager{nullptr};
	UniquePtr<asset::TextureImporter> m_textureImporter{nullptr};

	std::vector<gpu::BufferHandle> m_indirectBuffers;
	std::vector<gpu::BufferHandle> m_objectDataBuffers;

	scene::Scene m_scene;
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
