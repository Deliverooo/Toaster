#include <iostream>
#include <toast_os/console.hpp>
#include <toast_os/entry_points.hpp>

#include <toast_kernel/application.hpp>

#include <toast_gpu/api.hpp>
#include <toast_gpu/upload.hpp>

#include "toast_lib/pool.hpp"
#include "toast_render/mesh.hpp"

#include <print>

#include "toast_asset/mesh_importer.hpp"
#include "toast_render/material.hpp"

namespace toaster::rnd
{
	struct MeshManager;
}

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

		struct RenderTexture
		{
			gpu::TextureHandle      texture{nullptr};
			gpu::StateTrackerHandle stateTracker{nullptr};
			uint32                  shaderReadHeapSlot;
		};

		struct TestLayer
		{
			// rnd::MaterialManager *materialMngr;
			// rnd::MeshManager *    meshMngr;

			Vector<gpu::TextureHandle> pendingMipmapGenerations;

			RenderTexture testTexture;

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

			// layer->materialMngr = rnd::material_mngr_create(render_ctx, 128u);
			// layer->meshMngr     = rnd::mesh_mngr_create(render_ctx);

			layer->testTexture.texture      = gpu::texture_register_null(gpu_ctx);
			layer->testTexture.stateTracker = gpu::state_tracker_register(upload_ctx, 1u);

			gpu::state_tracker_register_ready_callback(upload_ctx, layer->testTexture.stateTracker, [layer]() -> void
			{
				layer->pendingMipmapGenerations.push_back(layer->testTexture.texture);
			});
			layer->testTexture.shaderReadHeapSlot = gpu::resource_descriptor_heap_alloc_texture_slot(gpu_ctx, render_ctx->resourceHeap);

			ast::texture_load_from_file(render_ctx, layer->testTexture.texture, layer->testTexture.shaderReadHeapSlot, layer->testTexture.stateTracker,
										R"(C:\dev\Toaster-3.0\test\resources\textures\doorbell_pig.jpg)");

			gpu::state_tracker_wait_for(upload_ctx, layer->testTexture.stateTracker);

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

			// rpmalloc_dump_statistics(stdout);
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

			gpu::texture_destroy(gpu_ctx, layer->testTexture.texture);
			gpu::resource_descriptor_heap_free_texture_slot(gpu_ctx, render_ctx->resourceHeap, layer->testTexture.shaderReadHeapSlot);

			// rnd::mesh_mngr_destroy(layer->meshMngr);
			// rnd::material_mngr_destroy(layer->materialMngr);
		};
		app_layer.onUpdateFn = +[](knl::Application *p_app, void *p_layer, float32 p_dt) mutable -> void
		{
		};
		app_layer.onRenderFn = +[](knl::Application *p_app, void *p_layer, gpu::CommandListHandle p_cmd) mutable -> void
		{
			auto layer{static_cast<TestLayer *>(p_layer)};

			const uint32             frame_index{p_app->frameIndex};
			const gpu::TextureHandle render_tex{p_app->window.currentTexture};

			rnd::RenderContext *render_ctx{&p_app->renderCtx};
			gpu::GPUContext *   gpu_ctx{render_ctx->gpuCtx};

			if (!layer->pendingMipmapGenerations.empty())
			{
				for (const gpu::TextureHandle texture: layer->pendingMipmapGenerations)
					gpu::cmd_generate_mipmaps(gpu_ctx, p_cmd, texture);
				layer->pendingMipmapGenerations.clear();
			}

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
			push_data.textureHeapId    = layer->testTexture.shaderReadHeapSlot;
			push_data.samplerHeapId    = layer->samplerHeapId;
			gpu::cmd_push_data(gpu_ctx, p_cmd, push_data);

			gpu::cmd_draw_mesh_tasks(gpu_ctx, p_cmd, 1u, 1u, 1u);

			gpu::cmd_end_rendering(gpu_ctx, p_cmd);
		};

		knl::application_add_layer(&application, &app_layer);

		knl::application_run(&application, 360u);
		knl::application_destroy(&application);
	}

	#endif

	rpmalloc_finalize();

	// std::cin.get();
	os::output_console_destroy();

	return 0;
}
