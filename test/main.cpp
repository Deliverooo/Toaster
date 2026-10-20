#include <iostream>
#include <toast_os/console.hpp>
#include <toast_os/entry_points.hpp>

#include <toast_kernel/input.hpp>
#include <toast_kernel/window.hpp>

#include <toast_gpu/api.hpp>
#include <toast_render/render_context.hpp>

using namespace toaster;
#if 0
struct TestLayer
{
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

	ke::Camera                     m_camera;
	std::vector<gpu::BufferHandle> m_cameraBuffers;

	UniquePtr<rd::MaterialManager> m_materialManager{nullptr};
	UniquePtr<rd::MeshManager>     m_meshManager{nullptr};
	UniquePtr<asset::MeshImporter> m_meshImporter{nullptr};

	UniquePtr<rd::TransformSystem> m_transformSystem{nullptr};
	rd::TextureManager *           m_textureMngr{nullptr};

	std::vector<gpu::BufferHandle> m_indirectBuffers;
	std::vector<gpu::BufferHandle> m_objectDataBuffers;
	std::vector<gpu::BufferHandle> m_countBuffers;

	scene::Scene m_scene;

	tf::Executor m_executor{};
};auto testLayerInit(ke::Application *p_app, TestLayer *p_layer) -> void
{
	p_layer->m_textureMngr     = rd::texture_mngr_create(&p_app->renderCtx);
	p_layer->m_materialManager = makeUnique<rd::MaterialManager>(p_layer->m_textureMngr, 1028u);
	p_layer->m_meshManager     = makeUnique<rd::MeshManager>(&p_app->renderCtx);
	p_layer->m_transformSystem = makeUnique<rd::TransformSystem>();

	std::filesystem::current_path("../test");

	p_layer->m_meshImporter = makeUnique<asset::MeshImporter>(p_layer->m_meshManager.get(), p_layer->m_materialManager.get(), p_layer->m_textureMngr);

	{
		rd::StaticMeshHandle orbo_mesh{p_layer->m_meshManager->registerStaticMesh()};
		p_layer->m_meshImporter->asyncLoadStaticMeshFromFile(orbo_mesh, "resources/meshes/Orbo_Geo.gltf", p_layer->m_executor);

		p_layer->m_orboEntity = p_layer->m_scene.createEntity();
		p_layer->m_scene.addComponent<scene::StaticMeshComponent>(p_layer->m_orboEntity, orbo_mesh);
		p_layer->m_scene.addComponent<scene::GPUTransformComponent>(p_layer->m_orboEntity, p_layer->m_transformSystem->createTransform());
		p_layer->m_scene.addComponent<scene::TransformComponent>(p_layer->m_orboEntity);
	}
	{
		rd::StaticMeshHandle level_mesh{p_layer->m_meshManager->registerStaticMesh()};
		// p_layer->m_meshImporter->asyncLoadStaticMeshFromFile(level_mesh, R"(C:\Users\Oliver\Downloads\main_sponza\main_sponza\NewSponza_Main_glTF_003.gltf)",
		// p_layer->m_executor);
		p_layer->m_meshImporter->asyncLoadStaticMeshFromFile(level_mesh, "resources/meshes/Backrooms.fbx", p_layer->m_executor);

		p_layer->m_levelEntity = p_layer->m_scene.createEntity();
		p_layer->m_scene.addComponent<scene::StaticMeshComponent>(p_layer->m_levelEntity, level_mesh);
		p_layer->m_scene.addComponent<scene::GPUTransformComponent>(p_layer->m_levelEntity, p_layer->m_transformSystem->createTransform());
	}

	gpu::SamplerDesc sampler_desc{};
	sampler_desc.minFilter    = gpu::EFilter::eLinear;
	sampler_desc.magFilter    = gpu::EFilter::eLinear;
	sampler_desc.mipmapMode   = gpu::ESamplerMipmapMode::eLinear;
	sampler_desc.addressModeU = gpu::ESamplerAddressMode::eRepeat;
	sampler_desc.addressModeV = gpu::ESamplerAddressMode::eRepeat;
	sampler_desc.addressModeW = gpu::ESamplerAddressMode::eRepeat;

	gpu::SamplerHandle sampler{gpu::sampler_create(sampler_desc)};

	p_layer->m_samplerHeapSlot = gpu::sampler_descriptor_heap_alloc_sampler_slot(p_app->renderCtx.samplerHeap);
	gpu::sampler_descriptor_heap_write_sampler(p_app->renderCtx.samplerHeap, p_layer->m_samplerHeapSlot, sampler);

	p_layer->m_objectCullingShader = gpu::shader_create(gpu::ShaderDesc{
															"main",
															c_object_culling_comp_bytecode,
															sizeof(c_object_culling_comp_bytecode) / sizeof(uint32),
															gpu::EShaderStageFlagBits::eCompute
														});

	p_layer->m_ms = gpu::shader_create(gpu::ShaderDesc{
										   "main",
										   c_tst_pbr_static_mesh_bytecode,
										   sizeof(c_tst_pbr_static_mesh_bytecode) / sizeof(uint32),
										   gpu::EShaderStageFlagBits::eMesh,
										   gpu::EShaderStageFlagBits::ePixel,
										   true
									   });

	p_layer->m_ts = gpu::shader_create(gpu::ShaderDesc{
										   "main",
										   c_meshlet_culling_task_bytecode,
										   sizeof(c_meshlet_culling_task_bytecode) / sizeof(uint32),
										   gpu::EShaderStageFlagBits::eTask,
										   gpu::EShaderStageFlagBits::eMesh
									   });

	p_layer->m_ps = gpu::shader_create(gpu::ShaderDesc{
										   "main",
										   c_tst_pbr_static_frag_bytecode,
										   sizeof(c_tst_pbr_static_frag_bytecode) / sizeof(uint32),
										   gpu::EShaderStageFlagBits::ePixel,
										   gpu::EShaderStageFlagBits::eNone
									   });

	p_layer->m_camera = ke::Camera{90.0f, ke::window_get_aspect_ratio(&p_app->window)};
	{
		gpu::BufferDesc camera_buffer_desc{};
		camera_buffer_desc.size       = sizeof(ke::CameraCB);
		camera_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eUniformBuffer;
		camera_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

		p_layer->m_cameraBuffers.resize(p_app->maxFramesInFlight);
		for (auto &buffer: p_layer->m_cameraBuffers)
			buffer = gpu::buffer_create(camera_buffer_desc);
	}

	gpu::BufferDesc indirect_buffer_desc{};
	indirect_buffer_desc.size       = sizeof(TestLayer::DrawMeshTasksIndirectCountCommand) * p_layer->maxDrawCalls;
	indirect_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eIndirectBuffer | gpu::EBufferUsageFlagBits::eStorageBuffer;
	indirect_buffer_desc.memoryType = gpu::EMemoryType::eDeviceLocal;

	gpu::BufferDesc object_buffer_desc{};
	object_buffer_desc.size       = sizeof(TestLayer::ObjectData) * p_layer->maxDrawCalls;
	object_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eStorageBuffer;
	object_buffer_desc.memoryType = gpu::EMemoryType::eHostVisibleCoherent;

	gpu::BufferDesc count_buffer_desc{};
	count_buffer_desc.size       = sizeof(uint32);
	count_buffer_desc.usage      = gpu::EBufferUsageFlagBits::eIndirectBuffer | gpu::EBufferUsageFlagBits::eStorageBuffer | gpu::EBufferUsageFlagBits::eTransferDst;
	count_buffer_desc.memoryType = gpu::EMemoryType::eDeviceLocal;

	p_layer->m_indirectBuffers.resize(p_app->maxFramesInFlight);
	p_layer->m_objectDataBuffers.resize(p_app->maxFramesInFlight);
	p_layer->m_countBuffers.resize(p_app->maxFramesInFlight);
	for (uint32 i{0u}; i < p_app->maxFramesInFlight; ++i)
	{
		p_layer->m_indirectBuffers[i]   = gpu::buffer_create(indirect_buffer_desc);
		p_layer->m_objectDataBuffers[i] = gpu::buffer_create(object_buffer_desc);
		p_layer->m_countBuffers[i]      = gpu::buffer_create(count_buffer_desc);
	}

	tsm::uint2 window_size{ke::window_get_size(&p_app->window)};

	{
		gpu::TextureDesc depth_desc{};
		depth_desc.extent          = {window_size, 1u};
		depth_desc.format          = gpu::EFormat::eD32Sfloat;
		depth_desc.usage           = gpu::ETextureUsageFlagBits::eDepthStencilAttachment;
		p_layer->m_depthAttachment = gpu::texture_create(depth_desc);
	}

	{
		gpu::TextureDesc msaa_depth_desc{};
		msaa_depth_desc.extent         = {window_size, 1u};
		msaa_depth_desc.format         = gpu::EFormat::eD32Sfloat;
		msaa_depth_desc.usage          = gpu::ETextureUsageFlagBits::eDepthStencilAttachment | gpu::ETextureUsageFlagBits::eTransient;
		msaa_depth_desc.sampleCount    = p_layer->msaaSamples;
		p_layer->m_msaaDepthAttachment = gpu::texture_create(msaa_depth_desc);
	}

	{
		gpu::TextureDesc msaa_colour_desc{};
		msaa_colour_desc.extent         = {window_size, 1u};
		msaa_colour_desc.format         = gpu::EFormat::eR8G8B8A8Srgb;
		msaa_colour_desc.usage          = gpu::ETextureUsageFlagBits::eColourAttachment | gpu::ETextureUsageFlagBits::eTransient;
		msaa_colour_desc.sampleCount    = p_layer->msaaSamples;
		p_layer->m_msaaColourAttachment = gpu::texture_create(msaa_colour_desc);
	}
}auto testLayerDestroy(ke::Application *p_app, TestLayer *p_layer) -> void
{
	p_layer->m_executor.wait_for_all();

	gpu::texture_destroy(p_layer->m_depthAttachment);
	gpu::texture_destroy(p_layer->m_msaaDepthAttachment);
	gpu::texture_destroy(p_layer->m_msaaColourAttachment);

	gpu::shader_destroy(p_layer->m_ps);
	gpu::shader_destroy(p_layer->m_ms);
	gpu::shader_destroy(p_layer->m_ts);
	gpu::shader_destroy(p_layer->m_objectCullingShader);

	gpu::sampler_descriptor_heap_free_sampler_slot(p_app->renderCtx.samplerHeap, p_layer->m_samplerHeapSlot);

	for (uint32 i{0u}; i < p_app->maxFramesInFlight; ++i)
	{
		gpu::buffer_destroy(p_layer->m_countBuffers[i]);
		gpu::buffer_destroy(p_layer->m_objectDataBuffers[i]);
		gpu::buffer_destroy(p_layer->m_indirectBuffers[i]);
	}

	p_layer->m_transformSystem.reset();
	p_layer->m_meshImporter.reset();
	p_layer->m_meshManager.reset();
	p_layer->m_materialManager.reset();
	rd::texture_mngr_destroy(p_layer->m_textureMngr);
}auto testLayerOnUpdate(ke::Application *p_app, TestLayer *p_layer, float32 p_dt) -> void
{
	auto &input_ctx{p_app->window.cbData.inputCtx};

	if (ke::input_ctx_is_key_down(&input_ctx, ke::EKeyCode::eLeftControl) && ke::input_ctx_is_key_pressed(&input_ctx, ke::EKeyCode::eE))
		ke::application_close(p_app);

	if (ke::input_ctx_is_mouse_button_down(&input_ctx, ke::EMouseButton::eRight))
	{
		if (ke::input_ctx_get_cursor_mode(&input_ctx) != ke::ECursorMode::eDisabled)
			ke::input_ctx_set_cursor_mode(&input_ctx, ke::ECursorMode::eDisabled);
		p_layer->m_camera.onUpdate(&input_ctx, p_dt);
	}
	else
	{
		if (ke::input_ctx_get_cursor_mode(&input_ctx) != ke::ECursorMode::eNormal)
			ke::input_ctx_set_cursor_mode(&input_ctx, ke::ECursorMode::eNormal);
	}

	XMMATRIX camera_view{p_layer->m_camera.getViewMatrix()};
	XMMATRIX camera_proj{p_layer->m_camera.getProjectionMatrix()};
	XMMATRIX camera_inverse_proj{XMMatrixInverse(nullptr, camera_proj)};
	XMVECTOR camera_position{p_layer->m_camera.getPosition()};

	ke::CameraCB camera_cb{};
	XMStoreFloat4x4(&camera_cb.view, camera_view);
	XMStoreFloat4x4(&camera_cb.proj, camera_proj);
	XMStoreFloat4x4(&camera_cb.invProj, camera_inverse_proj);
	XMStoreFloat4(&camera_cb.position, camera_position);

	const auto planes{p_layer->m_camera.getFrustumPlanes()};
	for (uint32 i{0u}; i < 6u; ++i)
		XMStoreFloat4(&camera_cb.frustumPlanes[i], planes[i]);

	gpu::buffer_write_data(p_layer->m_cameraBuffers[p_app->frameIndex], &camera_cb, sizeof(ke::CameraCB));

	auto &   tc{p_layer->m_scene.getRegistry().get<scene::TransformComponent>(p_layer->m_orboEntity)};
	XMVECTOR orbo_translation{tc.getTranslation()};

	if (ke::input_ctx_is_key_down(&input_ctx, ke::EKeyCode::eUp))
		orbo_translation += XMVectorSet(1.0f * p_dt, 0.0f, 0.0f, 0.0f);
	if (ke::input_ctx_is_key_down(&input_ctx, ke::EKeyCode::eDown))
		orbo_translation -= XMVectorSet(1.0f * p_dt, 0.0f, 0.0f, 0.0f);
	if (ke::input_ctx_is_key_down(&input_ctx, ke::EKeyCode::eLeft))
		orbo_translation += XMVectorSet(0.0f, 0.0f, 1.0f * p_dt, 0.0f);
	if (ke::input_ctx_is_key_down(&input_ctx, ke::EKeyCode::eRight))
		orbo_translation -= XMVectorSet(0.0f, 0.0f, 1.0f * p_dt, 0.0f);

	tc.setTranslation(orbo_translation);
	p_layer->m_transformSystem->updateTransform(p_layer->m_scene.getRegistry().get<scene::GPUTransformComponent>(p_layer->m_orboEntity).transformId, p_app->frameIndex,
												tc.getTransform());
}auto testLayerOnRender(ke::Application *p_app, TestLayer *p_layer, gpu::CommandListHandle p_cmd) -> void
{
	uint32 frame_index{p_app->frameIndex};

	p_layer->m_materialManager->pollMaterialTextureUploads();
	p_layer->m_materialManager->updateDirtyMaterials(frame_index);
	rd::texture_mngr_poll_uploads(p_layer->m_textureMngr, p_cmd);

	gpu::cmd_bind_resource_heap(p_cmd, p_app->renderCtx.resourceHeap);
	gpu::cmd_bind_sampler_heap(p_cmd, p_app->renderCtx.samplerHeap);

	uint32 active_object_count{0u};
	{
		TestLayer::ObjectData *mapped_object_data{static_cast<TestLayer::ObjectData *>(gpu::buffer_get_mapped_data(p_layer->m_objectDataBuffers[frame_index]))};

		const auto view{p_layer->m_scene.getRegistry().view<scene::StaticMeshComponent, scene::GPUTransformComponent>()};
		view.each([p_layer, &active_object_count, mapped_object_data]([[maybe_unused]] entt::entity       p_entity, const scene::StaticMeshComponent &p_smc,
																	  const scene::GPUTransformComponent &p_gpu_tc) -> void
		{
			const rd::StaticMesh *mesh{p_layer->m_meshManager->tryGetStaticMesh(p_smc.mesh)};
			TST_ASSERT(p_gpu_tc.transformId != UINT32_MAX);

			if (mesh && p_smc.visible && p_layer->m_meshManager->isStaticMeshReady(p_smc.mesh))
			{
				auto &obj_data{mapped_object_data[active_object_count]};

				obj_data.meshId      = p_smc.mesh.getId();
				obj_data.transformId = p_gpu_tc.transformId;
				++active_object_count;
			}
		});

		gpu::cmd_fill_buffer(p_cmd, p_layer->m_countBuffers[frame_index], 0u, sizeof(uint32), 0u); // Reset the count to 0

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
		push_data.cameraBuffer       = gpu::buffer_get_address(p_layer->m_cameraBuffers[frame_index]);
		push_data.objectDataBuffer   = gpu::buffer_get_address(p_layer->m_objectDataBuffers[frame_index]);
		push_data.transformBuffer    = gpu::buffer_get_address(p_layer->m_transformSystem->getTransformBuffer(frame_index));
		push_data.meshMetadataBuffer = p_layer->m_meshManager->getStaticMeshMetadataBufferAddress();
		push_data.indirectBuffer     = gpu::buffer_get_address(p_layer->m_indirectBuffers[frame_index]);
		push_data.countBuffer        = gpu::buffer_get_address(p_layer->m_countBuffers[frame_index]);

		push_data.activeObjectCount = active_object_count;

		gpu::cmd_push_data(p_cmd, push_data);

		gpu::cmd_bind_shaders(p_cmd, p_layer->m_objectCullingShader);

		gpu::cmd_dispatch(p_cmd, (active_object_count + 31u) / 32u, 1u, 1u);
	}

	std::array<gpu::BufferMemoryBarrier, 2u> buffer_memory_barriers;
	buffer_memory_barriers[0u].buffer        = p_layer->m_indirectBuffers[frame_index];
	buffer_memory_barriers[0u].size          = sizeof(TestLayer::DrawMeshTasksIndirectCountCommand) * p_layer->maxDrawCalls;
	buffer_memory_barriers[0u].srcAccessMask = gpu::EAccessFlagBits::eShaderWrite;
	buffer_memory_barriers[0u].dstAccessMask = gpu::EAccessFlagBits::eIndirectCommandRead | gpu::EAccessFlagBits::eShaderRead;
	buffer_memory_barriers[0u].srcStageMask  = gpu::EPipelineStageFlagBits::eComputeShader;
	buffer_memory_barriers[0u].dstStageMask  = gpu::EPipelineStageFlagBits::eDrawIndirect | gpu::EPipelineStageFlagBits::eTaskShader;

	buffer_memory_barriers[1u].buffer        = p_layer->m_countBuffers[frame_index];
	buffer_memory_barriers[1u].size          = sizeof(uint32);
	buffer_memory_barriers[1u].srcAccessMask = gpu::EAccessFlagBits::eShaderWrite;
	buffer_memory_barriers[1u].dstAccessMask = gpu::EAccessFlagBits::eIndirectCommandRead | gpu::EAccessFlagBits::eShaderRead;
	buffer_memory_barriers[1u].srcStageMask  = gpu::EPipelineStageFlagBits::eComputeShader;
	buffer_memory_barriers[1u].dstStageMask  = gpu::EPipelineStageFlagBits::eDrawIndirect | gpu::EPipelineStageFlagBits::eTaskShader;
	gpu::cmd_pipeline_barrier(p_cmd, buffer_memory_barriers, {});

	gpu::TextureHandle render_tex{p_app->window.currentTexture};

	gpu::RenderingInfo rendering_info{};
	rendering_info.colourAttachments = {
		gpu::RenderingAttachmentInfo{
			gpu::ClearColourValue{1.0f, 0.0f, 1.0f, 1.0f},
			p_layer->m_msaaColourAttachment,
			render_tex,
			gpu::EAttachmentUsageOP::eClearStore,
			gpu::EAttachmentResolveMode::eAverage
		}
	};
	rendering_info.depthAttachment = gpu::RenderingAttachmentInfo{
		gpu::ClearDepthStencilValue{},
		p_layer->m_msaaDepthAttachment,
		p_layer->m_depthAttachment,
		gpu::EAttachmentUsageOP::eClearStore,
		gpu::EAttachmentResolveMode::eMin
	};
	tsm::uint2 window_size{ke::window_get_size(&p_app->window)};

	rendering_info.renderArea = tsm::Rect{window_size};

	gpu::cmd_begin_rendering(p_cmd, rendering_info);

	gpu::cmd_bind_shaders(p_cmd, {p_layer->m_ts, p_layer->m_ms, p_layer->m_ps});

	gpu::cmd_set_primitive_topology(p_cmd, gpu::EPrimitiveTopology::eTriangleList);
	gpu::cmd_set_primitive_restart(p_cmd, false);

	gpu::cmd_set_viewport(p_cmd, tsm::Viewport{rendering_info.renderArea});
	gpu::cmd_set_scissor(p_cmd, rendering_info.renderArea);

	gpu::cmd_set_rasterizer_discard_enable(p_cmd, false);
	gpu::cmd_set_polygon_mode(p_cmd, gpu::EPolygonMode::eFill);
	gpu::cmd_set_cull_mode(p_cmd, gpu::ECullMode::eBack);
	gpu::cmd_set_front_face(p_cmd, gpu::EFrontFace::eCCW);
	gpu::cmd_set_depth_bias(p_cmd, false);
	gpu::cmd_set_line_width(p_cmd, 1.0f);

	gpu::cmd_set_rasterization_samples(p_cmd, p_layer->msaaSamples);

	gpu::cmd_set_depth_state(p_cmd, true);
	gpu::cmd_set_stencil_state(p_cmd, false);

	gpu::cmd_set_colour_write_enable(p_cmd, {true});
	gpu::cmd_set_colour_write_mask(p_cmd, {gpu::EColourComponentFlagBits::eAll});

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
	push_data.cameraBuffer       = gpu::buffer_get_address(p_layer->m_cameraBuffers[frame_index]);
	push_data.objectDataBuffer   = gpu::buffer_get_address(p_layer->m_objectDataBuffers[frame_index]);
	push_data.materialBuffer     = p_layer->m_materialManager->getMaterialBufferAddress(frame_index);
	push_data.transformBuffer    = gpu::buffer_get_address(p_layer->m_transformSystem->getTransformBuffer(frame_index));
	push_data.meshMetadataBuffer = p_layer->m_meshManager->getStaticMeshMetadataBufferAddress();
	push_data.indirectBuffer     = gpu::buffer_get_address(p_layer->m_indirectBuffers[frame_index]);
	push_data.samplerId          = p_layer->m_samplerHeapSlot;

	gpu::cmd_push_data(p_cmd, push_data);

	if (active_object_count > 0u)
		gpu::cmd_draw_mesh_tasks_indirect_count(p_cmd, p_layer->m_indirectBuffers[frame_index], 0u, p_layer->m_countBuffers[frame_index], 0u, active_object_count,
												sizeof(TestLayer::DrawMeshTasksIndirectCountCommand));

	gpu::cmd_end_rendering(p_cmd);
}auto testLayerOnEvent(ke::Application *p_app, TestLayer *p_layer, ke::Event &p_event) -> void
{
	ke::EventDispatcher ed{p_event};

	// This has to happen here, so it gets called outside of the frame loop because the swapchain will experience issues
	ed.dispatch<ke::KeyPressEvent>([p_app](ke::KeyPressEvent &p_e)-> bool
	{
		if (p_e.getKeyCode() == ke::EKeyCode::eF11)
		{
			if (ke::window_is_fullscreen(&p_app->window))
			{
				ke::window_set_windowed(&p_app->window);
				ke::window_maximise(&p_app->window);
			}
			else
				ke::window_set_fullscreen(&p_app->window);
		}
		return false;
	});

	ed.dispatch<ke::WindowResizeEvent>([p_layer](ke::WindowResizeEvent &p_e)-> bool
	{
		// Recreate the depth buffers
		{
			gpu::texture_destroy(p_layer->m_depthAttachment);
			gpu::TextureDesc depth_desc{};
			depth_desc.extent          = {p_e.getSize(), 1u};
			depth_desc.format          = gpu::EFormat::eD32Sfloat;
			depth_desc.usage           = gpu::ETextureUsageFlagBits::eDepthStencilAttachment;
			p_layer->m_depthAttachment = gpu::texture_create(depth_desc);
		}
		{
			gpu::texture_destroy(p_layer->m_msaaDepthAttachment);
			gpu::TextureDesc msaa_depth_desc{};
			msaa_depth_desc.extent         = {p_e.getSize(), 1u};
			msaa_depth_desc.format         = gpu::EFormat::eD32Sfloat;
			msaa_depth_desc.usage          = gpu::ETextureUsageFlagBits::eDepthStencilAttachment | gpu::ETextureUsageFlagBits::eTransient;
			msaa_depth_desc.sampleCount    = p_layer->msaaSamples;
			p_layer->m_msaaDepthAttachment = gpu::texture_create(msaa_depth_desc);
		}

		// Recreate the MSAA colour
		{
			gpu::texture_destroy(p_layer->m_msaaColourAttachment);
			gpu::TextureDesc colour_desc{};
			colour_desc.extent              = {p_e.getSize(), 1u};
			colour_desc.format              = gpu::EFormat::eR8G8B8A8Srgb;
			colour_desc.usage               = gpu::ETextureUsageFlagBits::eColourAttachment | gpu::ETextureUsageFlagBits::eTransient;
			colour_desc.sampleCount         = p_layer->msaaSamples;
			p_layer->m_msaaColourAttachment = gpu::texture_create(colour_desc);
		}
		p_layer->m_camera.onResize(p_e.getAspectRatio());

		return true;
	});
}

#endif

TST_OS_WINMAIN()
{
	os::output_console_create("Toaster-3.0 Test Debug Output");
	{
		rnd::RenderContext render_ctx;
		rnd::render_context_create(&render_ctx, {});

		rnd::render_context_destroy(&render_ctx);
	}
	os::output_console_destroy();

	return 0;
}
