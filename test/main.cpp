#include <future>
#include <print>
#include <toast_os/console.hpp>
#include <toast_os/entry_points.hpp>

#include <toast_kernel/application.hpp>

#include <stb/stb_image.h>

#include "toast_asset/mesh_importer.hpp"
#include "toast_asset/texture_importer.hpp"
#include "toast_kernel/camera.hpp"
#include "toast_kernel/events/key_event.hpp"
#include "toast_kernel/events/window_event.hpp"
#include "toast_render/mesh.hpp"
#include "toast_render/texture.hpp"
#include "toast_render/transform_system.hpp"
#include "toast_scene/scene.hpp"

using namespace toaster;

#include <object_culling.comp.h>
#include <meshlet_culling.task.h>
#include <tst_pbr_static.mesh.h>
#include <tst_pbr_static.frag.h>

class TestLayer : public IAppLayer
{
public:
	static constexpr uint32            maxDrawCalls{1028u * 1028u * 10u};
	static constexpr gpu::ESampleCount msaaSamples{gpu::ESampleCount::e4};

	struct ObjectData
	{
		uint32 meshId;
		uint32 transformId;
	};

	struct DrawMeshTasksIndirectCountCommand
	{
		uint32 groupCountX{0u};
		uint32 groupCountY{0u};
		uint32 groupCountZ{0u};

		uint32 objectId{0u}; // Index into the object data buffer
	};

	auto onInit() -> void override
	{
		m_textureManager  = makeUnique<render::TextureManager>(m_renderCtx);
		m_materialManager = makeUnique<render::MaterialManager>(m_textureManager.get(), 1028u);
		m_meshManager     = makeUnique<render::MeshManager>(m_renderCtx);
		m_transformSystem = makeUnique<rd::TransformSystem>();

		std::filesystem::current_path("../test");

		m_meshImporter = makeUnique<asset::MeshImporter>(m_meshManager.get(), m_materialManager.get(), m_textureManager.get());

		{
			render::StaticMeshHandle orbo_mesh{m_meshManager->registerStaticMesh()};
			m_meshImporter->asyncLoadStaticMeshFromFile(orbo_mesh, "resources/meshes/Orbo_Geo.gltf", m_executor);

			m_orboEntity = m_scene.createEntity();
			m_scene.addComponent<scene::StaticMeshComponent>(m_orboEntity, orbo_mesh);
			m_scene.addComponent<scene::GPUTransformComponent>(m_orboEntity, m_transformSystem->createTransform());
			m_scene.addComponent<scene::TransformComponent>(m_orboEntity);
		}
		{
			render::StaticMeshHandle level_mesh{m_meshManager->registerStaticMesh()};
			// m_meshImporter->asyncLoadStaticMeshFromFile(level_mesh, R"(C:\Users\Oliver\Downloads\main_sponza\main_sponza\NewSponza_Main_glTF_003.gltf)", m_executor);
			m_meshImporter->asyncLoadStaticMeshFromFile(level_mesh, "resources/meshes/Backrooms.fbx", m_executor);

			m_levelEntity = m_scene.createEntity();
			m_scene.addComponent<scene::StaticMeshComponent>(m_levelEntity, level_mesh);
			m_scene.addComponent<scene::GPUTransformComponent>(m_levelEntity, m_transformSystem->createTransform());
		}

		gpu::SamplerDesc sampler_desc{};
		sampler_desc.minFilter    = gpu::EFilter::eLinear;
		sampler_desc.magFilter    = gpu::EFilter::eLinear;
		sampler_desc.mipmapMode   = gpu::ESamplerMipmapMode::eLinear;
		sampler_desc.addressModeU = gpu::ESamplerAddressMode::eRepeat;
		sampler_desc.addressModeV = gpu::ESamplerAddressMode::eRepeat;
		sampler_desc.addressModeW = gpu::ESamplerAddressMode::eRepeat;

		gpu::SamplerHandle sampler{gpu::createSampler(sampler_desc)};

		m_samplerHeapSlot = gpu::allocSamplerHeapSlot(m_renderCtx->getSamplerHeap());
		gpu::writeSamplerDescriptor(m_renderCtx->getSamplerHeap(), m_samplerHeapSlot, sampler);

		m_objectCullingShader = gpu::createShader(gpu::ShaderDesc{
													  "main",
													  c_object_culling_comp_bytecode,
													  sizeof(c_object_culling_comp_bytecode) / sizeof(uint32),
													  gpu::EShaderStageFlagBits::eCompute
												  });

		m_ms = gpu::createShader(gpu::ShaderDesc{
									 "main",
									 c_tst_pbr_static_mesh_bytecode,
									 sizeof(c_tst_pbr_static_mesh_bytecode) / sizeof(uint32),
									 gpu::EShaderStageFlagBits::eMesh,
									 gpu::EShaderStageFlagBits::ePixel,
									 true
								 });

		m_ts = gpu::createShader(gpu::ShaderDesc{
									 "main",
									 c_meshlet_culling_task_bytecode,
									 sizeof(c_meshlet_culling_task_bytecode) / sizeof(uint32),
									 gpu::EShaderStageFlagBits::eTask,
									 gpu::EShaderStageFlagBits::eMesh
								 });

		m_ps = gpu::createShader(gpu::ShaderDesc{
									 "main",
									 c_tst_pbr_static_frag_bytecode,
									 sizeof(c_tst_pbr_static_frag_bytecode) / sizeof(uint32),
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

		gpu::BufferDesc indirect_buffer_desc{};
		indirect_buffer_desc.size       = sizeof(DrawMeshTasksIndirectCountCommand) * maxDrawCalls;
		indirect_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eIndirectBuffer | gpu::EBufferUsageFlagBits::eStorageBuffer;
		indirect_buffer_desc.memoryType = gpu::EMemoryType::eDeviceLocal;

		gpu::BufferDesc object_buffer_desc{};
		object_buffer_desc.size       = sizeof(ObjectData) * maxDrawCalls;
		object_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer;
		object_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		gpu::BufferDesc count_buffer_desc{};
		count_buffer_desc.size       = sizeof(uint32);
		count_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eIndirectBuffer | gpu::EBufferUsageFlagBits::eStorageBuffer | gpu::EBufferUsageFlagBits::eTransferDst;
		count_buffer_desc.memoryType = gpu::EMemoryType::eDeviceLocal;

		m_indirectBuffers.resize(Application::maxFramesInFlight);
		m_objectDataBuffers.resize(Application::maxFramesInFlight);
		m_countBuffers.resize(Application::maxFramesInFlight);
		for (uint32 i{0u}; i < Application::maxFramesInFlight; ++i)
		{
			m_indirectBuffers[i]   = gpu::createBuffer(indirect_buffer_desc);
			m_objectDataBuffers[i] = gpu::createBuffer(object_buffer_desc);
			m_countBuffers[i]      = gpu::createBuffer(count_buffer_desc);
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
		m_executor.wait_for_all();

		gpu::destroyTexture(m_depthAttachment);
		gpu::destroyTexture(m_msaaDepthAttachment);
		gpu::destroyTexture(m_msaaColourAttachment);

		gpu::destroyShader(m_ps);
		gpu::destroyShader(m_ms);
		gpu::destroyShader(m_ts);
		gpu::destroyShader(m_objectCullingShader);

		gpu::freeSamplerHeapSlot(m_renderCtx->getSamplerHeap(), m_samplerHeapSlot);

		for (uint32 i{0u}; i < Application::maxFramesInFlight; ++i)
		{
			gpu::destroyBuffer(m_countBuffers[i]);
			gpu::destroyBuffer(m_objectDataBuffers[i]);
			gpu::destroyBuffer(m_indirectBuffers[i]);
		}

		m_transformSystem.reset();
		m_meshImporter.reset();
		m_meshManager.reset();
		m_materialManager.reset();
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

		XMMATRIX camera_view{m_camera.getViewMatrix()};
		XMMATRIX camera_proj{m_camera.getProjectionMatrix()};
		XMMATRIX camera_inverse_proj{XMMatrixInverse(nullptr, camera_proj)};
		XMVECTOR camera_position{m_camera.getPosition()};

		CameraCB camera_cb{};
		XMStoreFloat4x4(&camera_cb.view, camera_view);
		XMStoreFloat4x4(&camera_cb.proj, camera_proj);
		XMStoreFloat4x4(&camera_cb.invProj, camera_inverse_proj);
		XMStoreFloat4(&camera_cb.position, camera_position);

		const auto planes{m_camera.getFrustumPlanes()};
		for (uint32 i{0u}; i < 6u; ++i)
			XMStoreFloat4(&camera_cb.frustumPlanes[i], planes[i]);

		gpu::writeBufferData(m_cameraBuffers[m_app->getFrameIndex()], &camera_cb, sizeof(CameraCB));

		auto &   tc{m_scene.getRegistry().get<scene::TransformComponent>(m_orboEntity)};
		XMVECTOR orbo_translation{tc.getTranslation()};

		if (m_inputCtx->isKeyDown(EKeyCode::eUp))
			orbo_translation += XMVectorSet(1.0f * p_dt, 0.0f, 0.0f, 0.0f);
		if (m_inputCtx->isKeyDown(EKeyCode::eDown))
			orbo_translation -= XMVectorSet(1.0f * p_dt, 0.0f, 0.0f, 0.0f);
		if (m_inputCtx->isKeyDown(EKeyCode::eLeft))
			orbo_translation += XMVectorSet(0.0f, 0.0f, 1.0f * p_dt, 0.0f);
		if (m_inputCtx->isKeyDown(EKeyCode::eRight))
			orbo_translation -= XMVectorSet(0.0f, 0.0f, 1.0f * p_dt, 0.0f);

		tc.setTranslation(orbo_translation);
		m_transformSystem->updateTransform(m_scene.getRegistry().get<scene::GPUTransformComponent>(m_orboEntity).transformId, m_app->getFrameIndex(), tc.getTransform());
	}

	auto onRender(gpu::CommandListHandle p_cmd) -> void override
	{
		m_materialManager->pollMaterialTextureUploads();
		m_materialManager->updateDirtyMaterials(m_app->getFrameIndex());
		m_textureManager->pollTextureUploads(p_cmd);

		gpu::bindResourceHeap(p_cmd, m_renderCtx->getResourceHeap());
		gpu::bindSamplerHeap(p_cmd, m_renderCtx->getSamplerHeap());

		uint32 active_object_count{0u};
		{
			ObjectData *mapped_object_data{static_cast<ObjectData *>(gpu::getBufferMappedData(m_objectDataBuffers[m_app->getFrameIndex()]))};

			const auto view{m_scene.getRegistry().view<scene::StaticMeshComponent, scene::GPUTransformComponent>()};
			view.each([this, &active_object_count, mapped_object_data]([[maybe_unused]] entt::entity       p_entity, const scene::StaticMeshComponent &p_smc,
																	   const scene::GPUTransformComponent &p_gpu_tc) -> void
			{
				const render::StaticMesh *mesh{m_meshManager->tryGetStaticMesh(p_smc.mesh)};
				TST_ASSERT(p_gpu_tc.transformId != UINT32_MAX);

				if (mesh && p_smc.visible && m_meshManager->isStaticMeshReady(p_smc.mesh))
				{
					auto &obj_data{mapped_object_data[active_object_count]};

					obj_data.meshId      = p_smc.mesh.getId();
					obj_data.transformId = p_gpu_tc.transformId;
					++active_object_count;
				}
			});

			gpu::fillBuffer(p_cmd, m_countBuffers[m_app->getFrameIndex()], 0u, sizeof(uint32), 0u); // Reset the count to 0

			struct PushData
			{
				uintptr cameraBuffer;
				uintptr objectDataBuffer;
				uintptr transformBuffer;
				uintptr meshMetadataBuffer;
				uintptr indirectBuffer;
				uintptr countBuffer;

				uint32 activeObjectCount;
			};

			PushData push_data{};
			push_data.cameraBuffer       = gpu::getBufferAddress(m_cameraBuffers[m_app->getFrameIndex()]);
			push_data.objectDataBuffer   = gpu::getBufferAddress(m_objectDataBuffers[m_app->getFrameIndex()]);
			push_data.transformBuffer    = gpu::getBufferAddress(m_transformSystem->getTransformBuffer(m_app->getFrameIndex()));
			push_data.meshMetadataBuffer = m_meshManager->getStaticMeshMetadataBufferAddress();
			push_data.indirectBuffer     = gpu::getBufferAddress(m_indirectBuffers[m_app->getFrameIndex()]);
			push_data.countBuffer        = gpu::getBufferAddress(m_countBuffers[m_app->getFrameIndex()]);

			push_data.activeObjectCount = active_object_count;

			gpu::pushData(p_cmd, push_data);

			gpu::bindShaders(p_cmd, m_objectCullingShader);

			gpu::dispatch(p_cmd, (active_object_count + 31u) / 32u, 1u, 1u);
		}

		std::array<gpu::BufferMemoryBarrier, 2u> buffer_memory_barriers;
		buffer_memory_barriers[0u].buffer = m_indirectBuffers[m_app->getFrameIndex()];
		buffer_memory_barriers[0u].size   = sizeof(DrawMeshTasksIndirectCountCommand) * maxDrawCalls;

		buffer_memory_barriers[1u].buffer = m_countBuffers[m_app->getFrameIndex()];
		buffer_memory_barriers[1u].size   = sizeof(uint32);
		gpu::pipelineBarrier(p_cmd, buffer_memory_barriers, {});

		gpu::TextureHandle render_tex{m_app->getWindow().getCurrentTexture()};

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

		gpu::beginRendering(p_cmd, rendering_info);

		gpu::bindShaders(p_cmd, {m_ts, m_ms, m_ps});

		gpu::setPrimitiveTopology(p_cmd, gpu::EPrimitiveTopology::eTriangleList);
		gpu::setPrimitiveRestart(p_cmd, false);

		gpu::setViewport(p_cmd, tsm::Viewport{rendering_info.renderArea});
		gpu::setScissor(p_cmd, rendering_info.renderArea);

		gpu::setRasterizerDiscardEnable(p_cmd, false);
		gpu::setPolygonMode(p_cmd, gpu::EPolygonMode::eFill);
		gpu::setCullMode(p_cmd, gpu::ECullMode::eBack);
		gpu::setFrontFace(p_cmd, gpu::EFrontFace::eCCW);
		gpu::setDepthBias(p_cmd, false);
		gpu::setLineWidth(p_cmd, 1.0f);

		gpu::setRasterizationSamples(p_cmd, msaaSamples);

		gpu::setDepthState(p_cmd, true);
		gpu::setStencilState(p_cmd, false);

		gpu::setColourWriteEnable(p_cmd, {true});
		gpu::setColourWriteMask(p_cmd, {gpu::EColourComponentFlagBits::eAll});

		struct PushData
		{
			uintptr cameraBuffer;
			uintptr objectDataBuffer;
			uintptr materialBuffer;
			uintptr transformBuffer;
			uintptr meshMetadataBuffer;
			uintptr indirectBuffer;

			uint32 samplerId;
			uint32 _padd[1];
		};
		PushData push_data{};
		push_data.cameraBuffer       = gpu::getBufferAddress(m_cameraBuffers[m_app->getFrameIndex()]);
		push_data.objectDataBuffer   = gpu::getBufferAddress(m_objectDataBuffers[m_app->getFrameIndex()]);
		push_data.materialBuffer     = m_materialManager->getMaterialBufferAddress(m_app->getFrameIndex());
		push_data.transformBuffer    = gpu::getBufferAddress(m_transformSystem->getTransformBuffer(m_app->getFrameIndex()));
		push_data.meshMetadataBuffer = m_meshManager->getStaticMeshMetadataBufferAddress();
		push_data.indirectBuffer     = gpu::getBufferAddress(m_indirectBuffers[m_app->getFrameIndex()]);
		push_data.samplerId          = m_samplerHeapSlot;

		gpu::pushData(p_cmd, push_data);

		if (active_object_count > 0u)
			gpu::drawMeshTasksIndirectCount(p_cmd, m_indirectBuffers[m_app->getFrameIndex()], 0u, m_countBuffers[m_app->getFrameIndex()], 0u, active_object_count,
											sizeof(DrawMeshTasksIndirectCountCommand));

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

private:
	gpu::TextureHandle m_depthAttachment{nullptr};
	gpu::TextureHandle m_msaaDepthAttachment{nullptr};
	gpu::TextureHandle m_msaaColourAttachment{nullptr};

	entt::entity m_levelEntity{entt::null};
	entt::entity m_orboEntity{entt::null};

	uint32 m_samplerHeapSlot{UINT32_MAX};

	gpu::ShaderHandle m_objectCullingShader{nullptr};

	gpu::ShaderHandle m_ts{nullptr};
	gpu::ShaderHandle m_ms{nullptr};
	gpu::ShaderHandle m_ps{nullptr};

	Camera                         m_camera;
	std::vector<gpu::BufferHandle> m_cameraBuffers;

	UniquePtr<render::MaterialManager> m_materialManager{nullptr};
	UniquePtr<render::MeshManager>     m_meshManager{nullptr};
	UniquePtr<asset::MeshImporter>     m_meshImporter{nullptr};

	UniquePtr<rd::TransformSystem>    m_transformSystem{nullptr};
	UniquePtr<render::TextureManager> m_textureManager{nullptr};

	std::vector<gpu::BufferHandle> m_indirectBuffers;
	std::vector<gpu::BufferHandle> m_objectDataBuffers;
	std::vector<gpu::BufferHandle> m_countBuffers;

	scene::Scene m_scene;

	tf::Executor m_executor{};
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
