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
#include "toast_kernel/events/key_event.hpp"
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
	bool                     visible{true};
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
	static constexpr uint32            maxDrawCalls{1028u * 1028u * 10u};
	static constexpr gpu::ESampleCount msaaSamples{gpu::ESampleCount::e4};

	struct alignas(16u) ObjectData
	{
		uint32 material;
		uint32 vertexBufferOffset;
		uint32 indexBufferOffset;

		uint8 vertexPageId;
		uint8 indexPageId;

		uint16 _padd;
	};

	auto onInit() -> void override
	{
		m_textureManager  = makeUnique<render::TextureManager>(m_renderCtx);
		m_materialManager = makeUnique<render::MaterialManager>(m_textureManager.get(), 1028u);
		m_meshManager     = makeUnique<render::MeshManager>(m_renderCtx);

		std::filesystem::current_path("../test");

		m_textureImporter = makeUnique<asset::TextureImporter>(m_textureManager.get());
		m_meshImporter    = makeUnique<asset::MeshImporter>(m_meshManager.get(), m_materialManager.get(), m_textureImporter.get());

		{
			render::StaticMeshHandle orbo_mesh{m_meshManager->registerStaticMesh()};
			m_meshImporter->asyncLoadStaticMeshFromFile(orbo_mesh, "resources/meshes/Orbo_Geo.gltf");

			m_orboEntity = m_scene.createEntity();
			m_scene.addComponent<StaticMeshComponent>(m_orboEntity, orbo_mesh);
		}
		{
			render::StaticMeshHandle level_mesh{m_meshManager->registerStaticMesh()};
			m_meshImporter->asyncLoadStaticMeshFromFile(level_mesh, "resources/meshes/Backrooms.fbx");

			m_levelEntity = m_scene.createEntity();
			m_scene.addComponent<StaticMeshComponent>(m_levelEntity, level_mesh);
		}

		gpu::SamplerDesc   sampler_desc{};
		gpu::SamplerHandle sampler{gpu::createSampler(sampler_desc)};

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

		{
			gpu::TextureDesc depth_desc{};
			depth_desc.extent = {m_app->getWindow().getSize(), 1u};
			depth_desc.format = gpu::EFormat::eD32Sfloat;
			depth_desc.usage  = gpu::ETextureUsageFlagBits::eDepthStencilAttachment;
			m_depthAttachment = gpu::createTexture(depth_desc);
		}

		{
			gpu::TextureDesc msaa_depth_desc{};
			msaa_depth_desc.extent      = {m_app->getWindow().getSize(), 1u};
			msaa_depth_desc.format      = gpu::EFormat::eD32Sfloat;
			msaa_depth_desc.usage       = gpu::ETextureUsageFlagBits::eDepthStencilAttachment | gpu::ETextureUsageFlagBits::eTransient;
			msaa_depth_desc.sampleCount = msaaSamples;
			m_msaaDepthAttachment       = gpu::createTexture(msaa_depth_desc);
		}

		{
			gpu::TextureDesc msaa_colour_desc{};
			msaa_colour_desc.extent      = {m_app->getWindow().getSize(), 1u};
			msaa_colour_desc.format      = gpu::EFormat::eR8G8B8A8Srgb;
			msaa_colour_desc.usage       = gpu::ETextureUsageFlagBits::eColourAttachment | gpu::ETextureUsageFlagBits::eTransient;
			msaa_colour_desc.sampleCount = msaaSamples;
			m_msaaColourAttachment       = gpu::createTexture(msaa_colour_desc);
		}
	}

	TestLayer()           = default;
	~TestLayer() override = default;

	auto onDestroy() -> void override
	{
		gpu::destroyTexture(m_depthAttachment);
		gpu::destroyTexture(m_msaaDepthAttachment);
		gpu::destroyTexture(m_msaaColourAttachment);

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
		m_materialManager.reset();
		m_textureImporter.reset();
		m_textureManager.reset();
	}

	auto onUpdate(float32 p_dt) -> void override
	{
		if (m_inputCtx->isKeyDown(EKeyCode::eLeftControl) && m_inputCtx->isKeyPressed(EKeyCode::eE))
			m_app->close();

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
		m_materialManager->pollMaterialTextureUploads();
		m_materialManager->updateDirtyMaterials(m_app->getFrameIndex());

		for (auto &list: m_secondaryBuffers[m_app->getFrameIndex()])
			gpu::resetCommandList(list);

		auto &secondary_cmd{m_secondaryBuffers[m_app->getFrameIndex()][0]};

		gpu::TextureHandle render_tex{m_app->getWindow().getCurrentTexture()};
		gpu::TextureDesc   render_tex_desc{gpu::getTextureDesc(render_tex)};

		gpu::RenderingInfo rendering_info{};
		rendering_info.colourAttachments = {
			gpu::RenderingAttachmentInfo{
				gpu::ClearColourValue{1.0f, 0.0f, 1.0f, 1.0f},
				m_msaaColourAttachment,
				render_tex,
				gpu::EAttachmentUsageOP::eClearStore,
				gpu::EAttachmentResolveMode::eAverage
			}
		};
		rendering_info.depthAttachment = gpu::RenderingAttachmentInfo{
			gpu::ClearDepthStencilValue{},
			m_msaaDepthAttachment,
			m_depthAttachment,
			gpu::EAttachmentUsageOP::eClearStore,
			gpu::EAttachmentResolveMode::eMin
		};
		rendering_info.renderArea = tsm::Rect{m_app->getWindow().getSize()};

		gpu::bindResourceHeap(p_cmd, m_renderCtx->getResourceHeap());
		gpu::bindSamplerHeap(p_cmd, m_renderCtx->getSamplerHeap());

		gpu::beginRendering(p_cmd, rendering_info);

		gpu::CommandListInheritanceInfo inheritance_info{};
		inheritance_info.resourceHeap            = m_renderCtx->getResourceHeap();
		inheritance_info.samplerHeap             = m_renderCtx->getSamplerHeap();
		inheritance_info.colourAttachmentFormats = {render_tex_desc.format};
		inheritance_info.depthAttachmentFormat   = gpu::EFormat::eD32Sfloat;
		inheritance_info.samples                 = msaaSamples;
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

		gpu::setRasterizationSamples(secondary_cmd, msaaSamples);

		gpu::setDepthState(secondary_cmd, true);
		gpu::setStencilState(secondary_cmd, false);

		uint32 draw_count{0u};

		gpu::DrawIndirectCommand *mapped_cmd{static_cast<gpu::DrawIndirectCommand *>(gpu::getBufferMappedData(m_indirectBuffers[m_app->getFrameIndex()]))};
		ObjectData *              mapped_object_data{static_cast<ObjectData *>(gpu::getBufferMappedData(m_objectDataBuffers[m_app->getFrameIndex()]))};

		const auto view{m_scene.getRegistry().view<StaticMeshComponent>()};
		view.each([this, &draw_count, mapped_cmd, mapped_object_data]([[maybe_unused]] entt::entity p_entity, const StaticMeshComponent &p_smc) -> void
		{
			const render::StaticMesh *mesh{m_meshManager->tryGetStaticMesh(p_smc.mesh)};

			if (mesh && p_smc.visible && gpu::upload::isStateTrackerReady(mesh->stateTracker))
			{
				for (const auto &submesh: mesh->submeshes)
				{
					mapped_object_data[draw_count].material           = submesh.material.getId();
					mapped_object_data[draw_count].vertexBufferOffset = mesh->vertexBufferOffset();
					mapped_object_data[draw_count].indexBufferOffset  = mesh->indexBufferOffset();
					mapped_object_data[draw_count].vertexPageId       = mesh->vertexBufferAllocation.heapSlot;
					mapped_object_data[draw_count].indexPageId        = mesh->indexBufferAllocation.heapSlot;

					mapped_cmd[draw_count] = gpu::DrawIndirectCommand{submesh.indexCount, 1u, submesh.indexOffset, draw_count};
					++draw_count;
				}
			}
		});

		struct PushData
		{
			uintptr cameraBuffer;
			uintptr objectDataBuffer;
			uintptr materialBuffer;

			uint32 _padd[1];
			uint32 samplerId;
		};
		PushData push_data{};
		push_data.cameraBuffer     = gpu::getBufferAddress(m_cameraBuffers[m_app->getFrameIndex()]);
		push_data.objectDataBuffer = gpu::getBufferAddress(m_objectDataBuffers[m_app->getFrameIndex()]);
		push_data.materialBuffer   = m_materialManager->getMaterialBufferAddress(m_app->getFrameIndex());
		push_data.samplerId        = m_samplerHeapSlot;

		gpu::pushData(secondary_cmd, push_data);
		gpu::bindIndexBuffer(secondary_cmd, nullptr);

		if (draw_count)
			gpu::drawIndirect(secondary_cmd, m_indirectBuffers[m_app->getFrameIndex()], 0u, draw_count);

		gpu::closeCommandList(secondary_cmd);
		gpu::executeCommandLists(p_cmd, secondary_cmd);

		gpu::endRendering(p_cmd);
	}

	auto onEvent(Event &p_event) -> void override
	{
		EventDispatcher ed{p_event};

		// This has to happen here, so it gets called outside of the frame loop because the swapchain will experience issues
		ed.dispatch<KeyPressEvent>([this](KeyPressEvent &p_e) -> bool
		{
			if (p_e.getKeyCode() == EKeyCode::eF11)
			{
				if (m_app->getWindow().isFullscreen())
				{
					m_app->getWindow().setWindowed();
					m_app->getWindow().maximiseWindow();
				}
				else
					m_app->getWindow().setFullscreen();
			}

			return false;
		});

		ed.dispatch<WindowResizeEvent>([this](WindowResizeEvent &p_e)-> bool
		{
			// Recreate the depth buffers
			{
				gpu::destroyTexture(m_depthAttachment);
				gpu::TextureDesc depth_desc{};
				depth_desc.extent = {p_e.getSize(), 1u};
				depth_desc.format = gpu::EFormat::eD32Sfloat;
				depth_desc.usage  = gpu::ETextureUsageFlagBits::eDepthStencilAttachment;
				m_depthAttachment = gpu::createTexture(depth_desc);
			}

			{
				gpu::destroyTexture(m_msaaDepthAttachment);
				gpu::TextureDesc msaa_depth_desc{};
				msaa_depth_desc.extent      = {p_e.getSize(), 1u};
				msaa_depth_desc.format      = gpu::EFormat::eD32Sfloat;
				msaa_depth_desc.usage       = gpu::ETextureUsageFlagBits::eDepthStencilAttachment | gpu::ETextureUsageFlagBits::eTransient;
				msaa_depth_desc.sampleCount = msaaSamples;
				m_msaaDepthAttachment       = gpu::createTexture(msaa_depth_desc);
			}

			// Recreate the MSAA colour
			{
				gpu::destroyTexture(m_msaaColourAttachment);
				gpu::TextureDesc colour_desc{};
				colour_desc.extent      = {p_e.getSize(), 1u};
				colour_desc.format      = gpu::EFormat::eR8G8B8A8Srgb;
				colour_desc.usage       = gpu::ETextureUsageFlagBits::eColourAttachment | gpu::ETextureUsageFlagBits::eTransient;
				colour_desc.sampleCount = msaaSamples;
				m_msaaColourAttachment  = gpu::createTexture(colour_desc);
			}

			m_camera.onResize(p_e.getAspectRatio());

			return true;
		});
	}

private :
	gpu::TextureHandle m_depthAttachment{nullptr};
	gpu::TextureHandle m_msaaDepthAttachment{nullptr};
	gpu::TextureHandle m_msaaColourAttachment{nullptr};

	entt::entity m_levelEntity{entt::null};
	entt::entity m_orboEntity{entt::null};

	uint32 m_samplerHeapSlot{UINT32_MAX};

	gpu::ShaderHandle m_vs{nullptr};
	gpu::ShaderHandle m_ps{nullptr};

	Camera                         m_camera;
	std::vector<gpu::BufferHandle> m_cameraBuffers;

	std::vector<std::vector<gpu::CommandListHandle> > m_secondaryBuffers;

	UniquePtr<render::MaterialManager> m_materialManager{nullptr};
	UniquePtr<render::MeshManager>     m_meshManager{nullptr};
	UniquePtr<asset::MeshImporter>     m_meshImporter{nullptr};

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
