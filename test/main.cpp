#include <iostream>
#include <toast_os/console.hpp>
#include <toast_os/entry_points.hpp>

#include <toast_kernel/application.hpp>

#include <toast_gpu/api.hpp>
#include <toast_gpu/upload.hpp>

#include <toast_render/texture.hpp>

using namespace toaster;

#include <test.mesh.h>
#include <test.frag.h>

TST_OS_WINMAIN()
{
	os::output_console_create("Toaster Test Debug Output");

	rpmalloc_interface_t malloc_interface{};
	malloc_interface.error_callback = +[](CString p_message) -> void
	{
		printf("rpmalloc Error: %s\n", p_message);
	};
	rpmalloc_initialize(&malloc_interface);

	{
		Vector<int32> vector;
		vector.emplace_back(0);
		vector.emplace_back(1);
		vector.emplace_back(2);
		vector.emplace_back(3);
		vector.emplace_back(4);
	}

	#if 1
	{
		knl::ApplicationDesc app_desc{};
		app_desc.maxFramesInFlight = 3u;

		knl::WindowDesc &window_desc{app_desc.windowDesc};
		window_desc.title          = "Toaster Test";
		window_desc.size           = {1920u, 1080u};
		window_desc.startMaximized = false;

		rnd::RenderContextDesc &render_ctx_desc{app_desc.renderCtxDesc};
		render_ctx_desc.maxFramesInFlight = 3u;
		render_ctx_desc.enableDebugInfo   = false;

		knl::Application application{};
		knl::application_create(&application, app_desc);

		struct TestLayer
		{
			rnd::TextureManager *textureMngr;
			rnd::TextureHandle   testTexture;

			gpu::ShaderHandle meshShader;
			gpu::ShaderHandle pixelShader;

			uint32 samplerHeapId;
		};

		TestLayer test_layer{};

		knl::AppLayer app_layer{};
		app_layer.userData = &test_layer;
		app_layer.initFn   = +[](knl::Application *p_app, void *p_layer) mutable -> void
		{
			auto layer{static_cast<TestLayer *>(p_layer)};

			rnd::RenderContext *render_ctx{&p_app->renderCtx};
			gpu::GPUContext *   gpu_ctx{render_ctx->gpuCtx};
			gpu::UploadContext *upload_ctx{render_ctx->uploadCtx};

			layer->textureMngr = rnd::texture_mngr_create(render_ctx);

			gpu::TextureDesc texture_desc{};
			texture_desc.extent      = {10u, 10u, 1u};
			texture_desc.format      = gpu::EFormat::eR8G8B8A8Srgb;
			texture_desc.layerCount  = 1u;
			texture_desc.sampleCount = gpu::ESampleCount::e1;

			gpu::TextureUploadDesc upload_desc{};
			texture_desc.mipCount = 4u;
			texture_desc.type     = gpu::ETextureType::e2D;
			texture_desc.usage    = gpu::ETextureUsageFlagBits::eSampled | gpu::ETextureUsageFlagBits::eTransferDst | gpu::ETextureUsageFlagBits::eTransferSrc;
			layer->testTexture    = rnd::texture_create(layer->textureMngr, texture_desc, 1u);

			rnd::Texture *texture{rnd::texture_get(layer->textureMngr, layer->testTexture)};

			Array<uint32, 10u * 10u> data;
			for (uint32 i{0u}; i < 10u; ++i)
			{
				for (uint32 j{0u}; j < 10u; ++j)
				{
					data[i * 10u + j] = (uint32) rand();
				}
			}

			upload_desc.dstTexture = texture->texture;
			upload_desc.data       = data.data();
			upload_desc.size       = data.size_bytes();
			upload_desc.extent     = {0u, 0u, 0u};
			upload_desc.offset     = {0u, 0u, 0u};
			upload_desc.mipLevel   = 0u;
			upload_desc.baseLayer  = 0u;
			upload_desc.layerCount = 1u;
			gpu::upload_data_to_texture(upload_ctx, upload_desc, texture->stateTracker);
			gpu::state_tracker_wait_for(upload_ctx, texture->stateTracker);

			layer->samplerHeapId = gpu::sampler_descriptor_heap_alloc_sampler_slot(gpu_ctx, render_ctx->samplerHeap);

			gpu::SamplerDesc sampler_desc{};
			sampler_desc.minFilter     = gpu::EFilter::eNearest;
			sampler_desc.magFilter     = gpu::EFilter::eNearest;
			sampler_desc.mipmapMode    = gpu::ESamplerMipmapMode::eNearest;
			sampler_desc.addressModeU  = gpu::ESamplerAddressMode::eRepeat;
			sampler_desc.addressModeV  = gpu::ESamplerAddressMode::eRepeat;
			sampler_desc.addressModeW  = gpu::ESamplerAddressMode::eRepeat;
			sampler_desc.minLod        = 0.0f;
			sampler_desc.maxLod        = 1000.0f;
			sampler_desc.maxAnisotropy = -1.0f;
			gpu::sampler_descriptor_heap_write_sampler(gpu_ctx, render_ctx->samplerHeap, layer->samplerHeapId, sampler_desc);

			{
				gpu::ShaderDesc mesh_shader_desc{};
				mesh_shader_desc.entryPointName = "main";
				mesh_shader_desc.code           = c_test_mesh_bytecode;
				mesh_shader_desc.codeSizeWords  = sizeof(c_test_mesh_bytecode) / sizeof(uint32);
				mesh_shader_desc.nextStage      = gpu::EShaderStageFlagBits::ePixel;
				mesh_shader_desc.stage          = gpu::EShaderStageFlagBits::eMesh;
				mesh_shader_desc.noTaskShader   = true;
				layer->meshShader               = gpu::shader_create(gpu_ctx, mesh_shader_desc);
			}
			{
				gpu::ShaderDesc pixel_shader_desc{};
				pixel_shader_desc.entryPointName = "main";
				pixel_shader_desc.code           = c_test_frag_bytecode;
				pixel_shader_desc.codeSizeWords  = sizeof(c_test_frag_bytecode) / sizeof(uint32);
				pixel_shader_desc.nextStage      = gpu::EShaderStageFlagBits::eNone;
				pixel_shader_desc.stage          = gpu::EShaderStageFlagBits::ePixel;
				pixel_shader_desc.noTaskShader   = true;
				layer->pixelShader               = gpu::shader_create(gpu_ctx, pixel_shader_desc);
			}

			rpmalloc_dump_statistics(stdout);
		};
		app_layer.destroyFn = +[](knl::Application *p_app, void *p_layer) mutable -> void
		{
			auto layer{static_cast<TestLayer *>(p_layer)};

			rnd::RenderContext *render_ctx{&p_app->renderCtx};
			gpu::GPUContext *   gpu_ctx{render_ctx->gpuCtx};
			gpu::UploadContext *upload_ctx{render_ctx->uploadCtx};

			gpu::shader_destroy(gpu_ctx, layer->pixelShader);
			gpu::shader_destroy(gpu_ctx, layer->meshShader);

			gpu::sampler_descriptor_heap_free_sampler_slot(gpu_ctx, render_ctx->samplerHeap, layer->samplerHeapId);

			rnd::texture_destroy(layer->textureMngr, layer->testTexture);
			rnd::texture_mngr_destroy(layer->textureMngr);
		};
		app_layer.onUpdateFn = +[](knl::Application *p_app, void *p_layer, float32 p_dt) mutable -> void
		{
		};
		app_layer.onRenderFn = +[](knl::Application *p_app, void *p_layer, gpu::CommandListHandle p_cmd) mutable -> void
		{
			auto layer{static_cast<TestLayer *>(p_layer)};

			rnd::texture_mngr_poll_uploads(layer->textureMngr, p_cmd);

			const uint32             frame_index{p_app->frameIndex};
			const gpu::TextureHandle render_tex{p_app->window.currentTexture};

			rnd::RenderContext *render_ctx{&p_app->renderCtx};
			gpu::GPUContext *   gpu_ctx{render_ctx->gpuCtx};

			gpu::cmd_bind_resource_heap(gpu_ctx, p_cmd, render_ctx->resourceHeap);
			gpu::cmd_bind_sampler_heap(gpu_ctx, p_cmd, render_ctx->samplerHeap);

			gpu::RenderingInfo rendering_info{};
			rendering_info.renderArea                 = window_get_size(&p_app->window);
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
			push_data.textureHeapId    = rnd::texture_get(layer->textureMngr, layer->testTexture)->shaderReadHeapSlot;
			push_data.samplerHeapId    = layer->samplerHeapId;
			gpu::cmd_push_data(gpu_ctx, p_cmd, push_data);

			gpu::cmd_draw_mesh_tasks(gpu_ctx, p_cmd, 1u, 1u, 1u);

			gpu::cmd_end_rendering(gpu_ctx, p_cmd);
		};

		knl::application_add_layer(&application, &app_layer);

		knl::application_run(&application, UINT32_MAX);
		knl::application_destroy(&application);
	}

	#endif

	rpmalloc_finalize();

	os::output_console_destroy();

	return 0;
}
