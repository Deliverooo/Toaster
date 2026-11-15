#include <filesystem>
#include <iostream>
#include <toast_os/console.hpp>
#include <toast_os/entry_points.hpp>

#include <toast_gpu/api.hpp>
#include <toast_gpu/upload.hpp>

#include "toast_render/mesh.hpp"
#include "toast_gpu/allocation.hpp"
#include "toast_gpu/frame.hpp"
#include "toast_kernel/window.hpp"
#include "toast_kernel/events/event.hpp"
#include "toast_render/material.hpp"

using namespace toaster;

#include <test.mesh.h>
#include <test.frag.h>

#include <ktx.h>

#pragma comment(lib, "winmm.lib")

TST_OS_WINMAIN()
{
	#ifndef NDEBUG
	os::output_console_create("Toaster Test Debug Output");
	#endif

	rpmalloc_interface_t malloc_interface{};
	malloc_interface.error_callback = +[](CString p_message) -> void
	{
		printf("rpmalloc Error: %s\n", p_message);
	};
	rpmalloc_initialize(&malloc_interface);

	#if 1

	struct Application;

	struct AppLayer
	{
		using InitFn     = void(*)(Application &, void *);
		using DestroyFn  = void(*)(Application &, void *);
		using OnRenderFn = void(*)(Application &, void *, gpu::CommandListHandle);
		using OnUpdateFn = void(*)(Application &, void *, float32);
		using OnEventFn  = void(*)(Application &, void *, knl::Event &);

		InitFn     initFn{nullptr};
		DestroyFn  destroyFn{nullptr};
		OnRenderFn onRenderFn{nullptr};
		OnUpdateFn onUpdateFn{nullptr};
		OnEventFn  onEventFn{nullptr};

		void *userData{nullptr}; // This could be a stack of heap pointer, as it is managed by the end-user
	};

	static constexpr uint32 c_max_frames_in_flight{3u};

	struct Application
	{
		gpu::GPUContext *                 gpuCtx{nullptr};
		gpu::FrameContext *               frameCtx{nullptr};
		gpu::AllocationContext *          allocCtx{nullptr};
		gpu::UploadContext *              uploadCtx{nullptr};
		gpu::ResourceDescriptorHeapHandle resourceHeap{nullptr};
		gpu::SamplerDescriptorHeapHandle  samplerHeap{nullptr};

		knl::Window window;

		Array<AppLayer, 1u> layers;

		uint32  frameIndex{0u};
		uint32  maxFramesInFlight{c_max_frames_in_flight};
		float32 deltaTime{0.0f};
	};

	Application app;

	{
		// Create the GPU contexts
		gpu::GPUContextDesc gpu_ctx_desc{};
		#ifndef NDEBUG
		gpu_ctx_desc.enableDebugInfo = true;
		#else
		gpu_ctx_desc.enableDebugInfo = false;
		#endif
		gpu_ctx_desc.usingSwapchain                  = true;
		gpu_ctx_desc.maxConcurrentSwapchainWorkloads = c_max_frames_in_flight;
		app.gpuCtx                                   = gpu::gpu_ctx_create(gpu_ctx_desc);
		app.frameCtx                                 = gpu::frame_ctx_create(app.gpuCtx, gpu::FrameContextDesc{.maxFramesInFlight = c_max_frames_in_flight});
		app.allocCtx                                 = gpu::allocation_ctx_create();
		app.uploadCtx                                = gpu::upload_ctx_create(app.gpuCtx, app.allocCtx, gpu::UploadContextDesc{.maxAllocationCommandLists = 2u});

		app.resourceHeap = gpu::resource_descriptor_heap_create(app.gpuCtx, {128u, 128u});
		app.samplerHeap  = gpu::sampler_descriptor_heap_create(app.gpuCtx, {8u});

		knl::window_init_api();

		knl::WindowDesc window_desc{};
		window_desc.title          = "Toaster Test";
		window_desc.size           = {1920u, 1080u};
		window_desc.startMaximized = false;
		knl::window_create(app.window, app.gpuCtx, window_desc);

		knl::window_set_event_callback_user_data(app.window, &app);
		knl::window_set_event_callback(app.window, +[](knl::Event &p_event, void *p_user_data) -> void
		{
			for (const auto app{static_cast<Application *>(p_user_data)}; const auto &layer: app->layers)
			{
				if (p_event.isHandled())
					continue;

				if (layer.onEventFn)
					layer.onEventFn(*app, layer.userData, p_event);
			}
		});

		struct RenderTexture
		{
			gpu::TextureHandle      texture{nullptr};
			gpu::StateTrackerHandle stateTracker{nullptr};
			uint32                  shaderReadHeapSlot;
		};

		struct TestLayer
		{
			rnd::MeshManager *    meshMngr;
			rnd::MaterialManager *materialMngr;

			RenderTexture testTexture;

			gpu::ShaderHandle meshShader;
			gpu::ShaderHandle pixelShader;

			uint32 samplerHeapId;
		};

		TestLayer test_layer{};

		AppLayer &app_layer{app.layers[0u]};
		app_layer.userData = &test_layer;
		app_layer.initFn   = +[](Application &p_app, void *p_layer) mutable -> void
		{
			auto layer{static_cast<TestLayer *>(p_layer)};

			gpu::GPUContext *                       gpu_ctx{p_app.gpuCtx};
			gpu::UploadContext *                    upload_ctx{p_app.uploadCtx};
			const gpu::ResourceDescriptorHeapHandle resource_heap{p_app.resourceHeap};
			const gpu::SamplerDescriptorHeapHandle  sampler_heap{p_app.samplerHeap};

			layer->testTexture.texture            = gpu::texture_register_null(gpu_ctx);
			layer->testTexture.stateTracker       = gpu::state_tracker_register(upload_ctx, 1u);
			layer->testTexture.shaderReadHeapSlot = gpu::resource_descriptor_heap_alloc_texture_slot(gpu_ctx, resource_heap);

			{
				std::filesystem::path texture_path{R"(C:\dev\Orbos_Exile\resources\textures\olive_delights.ktx2)"};

				ktxTexture *texture{nullptr};

				KTX_error_code result = ktxTexture_CreateFromNamedFile(texture_path.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &texture);

				if (result != KTX_SUCCESS)
					TST_PERMA_ASSERT(false);

				if (texture->classId != ktxTexture2_c)
				{
					TST_PERMA_ASSERT(false);
					ktxTexture_Destroy(texture);
				}

				ktxTexture2 *kTexture2 = reinterpret_cast<ktxTexture2 *>(texture);

				if (ktxTexture2_NeedsTranscoding(kTexture2))
				{
					ktx_transcode_fmt_e targetFormat = KTX_TTF_BC7_RGBA;

					result = ktxTexture2_TranscodeBasis(kTexture2, targetFormat, 0);
					if (result != KTX_SUCCESS)
					{
						TST_PERMA_ASSERT(false);
						ktxTexture_Destroy(texture);
					}
				}
				uint32_t width        = kTexture2->baseWidth;
				uint32_t height       = kTexture2->baseHeight;
				uint32_t numMipLevels = kTexture2->numLevels;
				uint32_t vkFormat     = kTexture2->vkFormat;

				gpu::TextureDesc texture_desc{};
				texture_desc.extent      = {width, height, 1u};
				texture_desc.mipCount    = numMipLevels;
				texture_desc.layerCount  = 1u;
				texture_desc.type        = gpu::ETextureType::e2D;
				texture_desc.sampleCount = gpu::ESampleCount::e1;
				texture_desc.format      = gpu::format_from_vk_format(vkFormat);
				texture_desc.usage       = gpu::ETextureUsageFlagBits::eSampled | gpu::ETextureUsageFlagBits::eTransferSrc | gpu::ETextureUsageFlagBits::eTransferDst;

				gpu::texture_create_into(gpu_ctx, layer->testTexture.texture, texture_desc);
				gpu::resource_descriptor_heap_write_texture(gpu_ctx, resource_heap, layer->testTexture.shaderReadHeapSlot, layer->testTexture.texture, false);

				std::vector<gpu::BufferTextureCopyRegion> bufferCopyRegions(numMipLevels);
				for (uint32 level{0u}; level < numMipLevels; ++level)
				{
					ktx_size_t offset = 0;
					ktxTexture_GetImageOffset(texture, level, 0, 0, &offset);

					auto &copy_region{bufferCopyRegions[level]};

					copy_region.bufferOffset                  = offset;
					copy_region.textureSubresource.mipLevel   = level;
					copy_region.textureSubresource.baseLayer  = 0;
					copy_region.textureSubresource.layerCount = 1;
					copy_region.textureExtent.x               = std::max(1u, width >> level);
					copy_region.textureExtent.y               = std::max(1u, height >> level);
					copy_region.textureExtent.z               = 1;
				}

				gpu::TextureUploadDesc texture_upload_desc{};
				texture_upload_desc.data          = texture->pData;
				texture_upload_desc.size          = ktxTexture_GetDataSize(texture);
				texture_upload_desc.uploadRegions = bufferCopyRegions;

				gpu::upload_data_to_textures(upload_ctx, layer->testTexture.texture, texture_upload_desc, layer->testTexture.stateTracker);

				ktxTexture_Destroy(texture);

				gpu::state_tracker_wait_for(upload_ctx, layer->testTexture.stateTracker);
			}

			layer->samplerHeapId = gpu::sampler_descriptor_heap_alloc_sampler_slot(gpu_ctx, p_app.samplerHeap);

			gpu::SamplerDesc sampler_desc{};
			sampler_desc.minFilter     = gpu::EFilter::eLinear;
			sampler_desc.magFilter     = gpu::EFilter::eLinear;
			sampler_desc.mipmapMode    = gpu::ESamplerMipmapMode::eLinear;
			sampler_desc.addressModeU  = gpu::ESamplerAddressMode::eRepeat;
			sampler_desc.addressModeV  = gpu::ESamplerAddressMode::eRepeat;
			sampler_desc.addressModeW  = gpu::ESamplerAddressMode::eRepeat;
			sampler_desc.minLod        = 0.0f;
			sampler_desc.maxLod        = 1000.0f;
			sampler_desc.maxAnisotropy = -1.0f;
			gpu::sampler_descriptor_heap_write_sampler(gpu_ctx, sampler_heap, layer->samplerHeapId, sampler_desc);

			{
				Array<gpu::ShaderDesc, 2u> shader_descs;

				gpu::ShaderDesc &mesh_shader_desc{shader_descs[0u]};
				mesh_shader_desc.entryPointName = "main";
				mesh_shader_desc.code           = c_test_mesh_bytecode;
				mesh_shader_desc.codeSizeWords  = sizeof(c_test_mesh_bytecode) / sizeof(uint32);
				mesh_shader_desc.nextStage      = gpu::EShaderStageFlagBits::ePixel;
				mesh_shader_desc.stage          = gpu::EShaderStageFlagBits::eMesh;
				mesh_shader_desc.noTaskShader   = true;

				gpu::ShaderDesc &pixel_shader_desc{shader_descs[1u]};
				pixel_shader_desc.entryPointName = "main";
				pixel_shader_desc.code           = c_test_frag_bytecode;
				pixel_shader_desc.codeSizeWords  = sizeof(c_test_frag_bytecode) / sizeof(uint32);
				pixel_shader_desc.nextStage      = gpu::EShaderStageFlagBits::eNone;
				pixel_shader_desc.stage          = gpu::EShaderStageFlagBits::ePixel;
				pixel_shader_desc.noTaskShader   = true;

				Array<gpu::ShaderHandle, 2u> shaders;
				gpu::shader_create_n(gpu_ctx, shaders, shader_descs, true);
				layer->meshShader  = shaders[0u];
				layer->pixelShader = shaders[1u];
			}

			layer->meshMngr     = rnd::mesh_mngr_create({gpu_ctx, p_app.allocCtx, upload_ctx, p_app.frameCtx, resource_heap});
			layer->materialMngr = rnd::material_mngr_create({gpu_ctx, p_app.allocCtx, upload_ctx, p_app.frameCtx, resource_heap, c_max_frames_in_flight, 128u});

			#if 0
			{
				ast::RawSubmeshData raw_data{};
				raw_data.materialIndex = 0u;

				raw_data.vertices = {
					{{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {}, {}, {0.0f, 0.0f}},
					{{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {}, {}, {1.0f, 0.0f}},
					{{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {}, {}, {1.0f, 1.0f}},
					{{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {}, {}, {0.0f, 1.0f}},
					{{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {}, {}, {0.0f, 0.0f}},
					{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {}, {}, {1.0f, 0.0f}},
					{{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {}, {}, {1.0f, 1.0f}},
					{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {}, {}, {0.0f, 1.0f}},
					{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {}, {}, {0.0f, 0.0f}},
					{{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {}, {}, {1.0f, 0.0f}},
					{{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {}, {}, {1.0f, 1.0f}},
					{{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {}, {}, {0.0f, 1.0f}},
					{{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {}, {}, {0.0f, 0.0f}},
					{{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {}, {}, {1.0f, 0.0f}},
					{{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {}, {}, {1.0f, 1.0f}},
					{{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {}, {}, {0.0f, 1.0f}},
					{{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {}, {}, {0.0f, 0.0f}},
					{{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {}, {}, {1.0f, 0.0f}},
					{{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {}, {}, {1.0f, 1.0f}},
					{{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {}, {}, {0.0f, 1.0f}},
					{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {}, {}, {0.0f, 0.0f}},
					{{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {}, {}, {1.0f, 0.0f}},
					{{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {}, {}, {1.0f, 1.0f}},
					{{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {}, {}, {0.0f, 1.0f}}
				};

				raw_data.indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 8, 9, 10, 10, 11, 8, 12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20};

				Vector<rnd::StaticMeshVertex> final_vertices;
				Vector<rnd::Meshlet>          meshlets;
				Vector<uint32>                meshlet_vertices;
				Vector<uint8>                 meshlet_triangles;
				ast::generate_geometry_data(raw_data, final_vertices, meshlets, meshlet_vertices, meshlet_triangles);

				rnd::MaterialHandle material{rnd::material_create(layer->materialMngr)};

				rnd::StaticMeshGeometryData geometry_data{};
				geometry_data.vertices        = final_vertices;
				geometry_data.meshlets        = meshlets;
				geometry_data.meshletVertices = meshlet_vertices;
				geometry_data.meshletIndices  = meshlet_triangles;
				geometry_data.materials       = {&material, 1u};
				geometry_data.boundingSphere  = {0.0f, 0.0f, 0.0f, 1.0f};

				gpu::StateTrackerHandle static_mesh_state_tracker{gpu::state_tracker_register(upload_ctx, 1u)};
				rnd::StaticMeshHandle   static_mesh{rnd::static_mesh_create(layer->meshMngr, static_mesh_state_tracker, geometry_data)};
				gpu::state_tracker_wait_for(upload_ctx, static_mesh_state_tracker);
			}
			#endif
		};
		app_layer.destroyFn = +[](Application &p_app, void *p_layer) mutable -> void
		{
			auto layer{static_cast<TestLayer *>(p_layer)};

			gpu::GPUContext *                       gpu_ctx{p_app.gpuCtx};
			gpu::UploadContext *                    upload_ctx{p_app.uploadCtx};
			const gpu::ResourceDescriptorHeapHandle resource_heap{p_app.resourceHeap};
			const gpu::SamplerDescriptorHeapHandle  sampler_heap{p_app.samplerHeap};

			rnd::material_mngr_destroy(layer->materialMngr);
			rnd::mesh_mngr_destroy(layer->meshMngr);

			gpu::shader_destroy(gpu_ctx, layer->pixelShader);
			gpu::shader_destroy(gpu_ctx, layer->meshShader);

			gpu::sampler_descriptor_heap_free_sampler_slot(gpu_ctx, sampler_heap, layer->samplerHeapId);

			gpu::texture_destroy(gpu_ctx, layer->testTexture.texture);
			gpu::resource_descriptor_heap_free_texture_slot(gpu_ctx, resource_heap, layer->testTexture.shaderReadHeapSlot);

			// rnd::mesh_mngr_destroy(layer->meshMngr);
			// rnd::material_mngr_destroy(layer->materialMngr);
		};
		app_layer.onUpdateFn = +[](Application &p_app, void *p_layer, float32 p_dt) mutable -> void
		{
		};
		app_layer.onRenderFn = +[](Application &p_app, void *p_layer, gpu::CommandListHandle p_cmd) mutable -> void
		{
			auto layer{static_cast<TestLayer *>(p_layer)};

			const uint32             frame_index{p_app.frameIndex};
			const gpu::TextureHandle render_tex{p_app.window.currentTexture};

			gpu::GPUContext *                       gpu_ctx{p_app.gpuCtx};
			gpu::UploadContext *                    upload_ctx{p_app.uploadCtx};
			const gpu::ResourceDescriptorHeapHandle resource_heap{p_app.resourceHeap};
			const gpu::SamplerDescriptorHeapHandle  sampler_heap{p_app.samplerHeap};

			gpu::cmd_bind_resource_heap(gpu_ctx, p_cmd, resource_heap);
			gpu::cmd_bind_sampler_heap(gpu_ctx, p_cmd, sampler_heap);

			gpu::RenderingInfo rendering_info{};
			rendering_info.renderArea                 = knl::window_get_size(p_app.window);
			rendering_info.useSecondaryCommandBuffers = false;
			gpu::RenderingAttachmentInfo &colour_attachment{rendering_info.colourAttachments.emplace_back()};
			colour_attachment.clearValue   = gpu::ClearColourValue{1.0f, 0.0f, 1.0f, 1.0f};
			colour_attachment.renderTarget = render_tex;
			colour_attachment.usageOp      = gpu::EAttachmentUsageOP::eClearStore;

			gpu::cmd_begin_rendering(gpu_ctx, p_cmd, rendering_info);
			gpu::cmd_bind_shaders(gpu_ctx, p_cmd, {layer->meshShader, layer->pixelShader});

			gpu::cmd_set_viewport(gpu_ctx, p_cmd, math::Viewport{rendering_info.renderArea});
			gpu::cmd_set_scissor(gpu_ctx, p_cmd, rendering_info.renderArea);

			gpu::cmd_set_primitive_topology(gpu_ctx, p_cmd, gpu::EPrimitiveTopology::eTriangleList);
			gpu::cmd_set_primitive_restart(gpu_ctx, p_cmd, false);
			gpu::cmd_set_rasterizer_discard_enable(gpu_ctx, p_cmd, false);
			gpu::cmd_set_polygon_mode(gpu_ctx, p_cmd, gpu::EPolygonMode::eFill);
			gpu::cmd_set_cull_mode(gpu_ctx, p_cmd, gpu::ECullMode::eNone);
			gpu::cmd_set_front_face(gpu_ctx, p_cmd, gpu::EFrontFace::eCCW);
			gpu::cmd_set_depth_bias(gpu_ctx, p_cmd, false);
			gpu::cmd_set_line_width(gpu_ctx, p_cmd, 1.0f);

			gpu::cmd_set_colour_write_mask(gpu_ctx, p_cmd, {gpu::EColourComponentFlagBits::eAll});
			gpu::cmd_set_colour_blend_enable(gpu_ctx, p_cmd, {false});
			gpu::cmd_set_rasterization_samples(gpu_ctx, p_cmd, gpu::ESampleCount::e1);
			gpu::cmd_set_depth_state(gpu_ctx, p_cmd, false, false);
			gpu::cmd_set_stencil_state(gpu_ctx, p_cmd, false);
			gpu::cmd_set_colour_write_enable(gpu_ctx, p_cmd, {true});

			struct PushData
			{
				math::Float2 screenResolution;
				uint32       textureHeapId;
				uint32       samplerHeapId;
			};
			PushData push_data{};
			push_data.screenResolution = {static_cast<float32>(rendering_info.renderArea.size.x), static_cast<float32>(rendering_info.renderArea.size.y)};
			push_data.textureHeapId    = layer->testTexture.shaderReadHeapSlot;
			push_data.samplerHeapId    = layer->samplerHeapId;
			gpu::cmd_push_data(gpu_ctx, p_cmd, push_data);

			gpu::cmd_draw_mesh_tasks(gpu_ctx, p_cmd, 1u, 1u, 1u);

			gpu::cmd_end_rendering(gpu_ctx, p_cmd);
		};

		app.layers[0u].initFn(app, app.layers[0u].userData);

		Vector<gpu::CommandAllocatorHandle> command_allocators(app.maxFramesInFlight);
		Vector<gpu::CommandListHandle>      command_lists(app.maxFramesInFlight);
		for (uint32 i{0u}; i < app.maxFramesInFlight; ++i)
		{
			command_allocators[i] = gpu::cmd_allocator_create(app.gpuCtx, gpu::EQueueType::eGraphics);
			gpu::cmd_list_allocate_n(app.gpuCtx, command_allocators[i], std::span<gpu::CommandListHandle>{&command_lists[i], 1ull});
		}

		timeBeginPeriod(1u);

		auto last_time{std::chrono::steady_clock::now()};
		gpu::swapchain_set_max_framerate(app.gpuCtx, app.window.swapchain, gpu::c_uncappedFramerate); // vkSetLatencySleepModeNV
		while (true)
		{
			gpu::swapchain_wait_for_framerate(app.gpuCtx, app.window.swapchain); // vkLatencySleepNV + vkWaitSemaphores
			gpu::swapchain_set_frame_marker(app.gpuCtx, app.window.swapchain, gpu::EFrameMarker::eInputSample);

			if (!window_process_messages(app.window)) // glfwPollEvents. returns !glfwWindowShouldClose(...);
				break;

			if (!window_check_for_resize(app.window))
				continue;

			gpu::poll_uploads(app.uploadCtx);
			gpu::begin_frame(app.frameCtx, app.frameIndex);

			auto                           current_time{std::chrono::steady_clock::now()};
			std::chrono::duration<float32> elapsed{current_time - last_time};
			app.deltaTime = elapsed.count();
			last_time     = current_time;

			gpu::CommandListHandle &cmd{command_lists[app.frameIndex]};
			gpu::cmd_allocator_reset(app.gpuCtx, command_allocators[app.frameIndex]);
			gpu::cmd_list_open(app.gpuCtx, cmd);

			if (!window_begin_frame(app.window, cmd)) // Attempts to acquire the image
			{
				gpu::cmd_allocator_reset(app.gpuCtx, command_allocators[app.frameIndex]);
				continue;
			}

			if (!window_is_minimised(app.window))
			{
				gpu::swapchain_set_frame_marker(app.gpuCtx, app.window.swapchain, gpu::EFrameMarker::eSimulationStart);
				for (auto &layer: app.layers)
				{
					if (layer.onUpdateFn)
						layer.onUpdateFn(app, layer.userData, app.deltaTime);
				}
				gpu::swapchain_set_frame_marker(app.gpuCtx, app.window.swapchain, gpu::EFrameMarker::eSimulationEnd);
			}
			gpu::swapchain_set_frame_marker(app.gpuCtx, app.window.swapchain, gpu::EFrameMarker::eRendersubmitStart);
			if (!window_is_minimised(app.window))
			{
				for (auto &layer: app.layers)
				{
					if (layer.onRenderFn)
						layer.onRenderFn(app, layer.userData, cmd);
				}
			}

			gpu::swapchain_set_frame_marker(app.gpuCtx, app.window.swapchain, gpu::EFrameMarker::eRendersubmitEnd);
			gpu::swapchain_set_frame_marker(app.gpuCtx, app.window.swapchain, gpu::EFrameMarker::ePresentStart);
			window_submit_and_present(app.window, app.frameCtx, cmd); // also updates the input states
			gpu::swapchain_set_frame_marker(app.gpuCtx, app.window.swapchain, gpu::EFrameMarker::ePresentEnd);

			app.frameIndex = (app.frameIndex + 1u) % app.maxFramesInFlight;
		}

		timeEndPeriod(1u);

		gpu::wait_queue_idle(app.gpuCtx, gpu::EQueueType::eGraphics); // Wait for the commands lists to finish before destroying the allocators
		for (auto &alloc: command_allocators)
			gpu::cmd_allocator_destroy(app.gpuCtx, alloc);

		gpu::wait_idle(app.gpuCtx);
		gpu::poll_uploads(app.uploadCtx);

		for (const auto &layer: app.layers)
		{
			if (layer.destroyFn)
				layer.destroyFn(app, layer.userData);
		}
		knl::window_destroy(app.window);

		gpu::upload_ctx_destroy(app.uploadCtx);
		gpu::frame_ctx_destroy(app.frameCtx);
		gpu::allocation_ctx_destroy(app.allocCtx);
		gpu::gpu_ctx_destroy(app.gpuCtx);
	}

	knl::window_shutdown_api();

	#endif

	rpmalloc_finalize();

	#ifndef NDEBUG
	os::output_console_destroy();
	#endif

	return 0;
}
