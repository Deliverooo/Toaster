#include <iostream>
#include <toast_os/console.hpp>
#include <toast_os/entry_points.hpp>

#include <print>
#include <toast_kernel/input.hpp>
#include <toast_kernel/window.hpp>

#include <toast_gpu/api.hpp>
#include <toast_render/render_context.hpp>

#include "toast_kernel/application.hpp"

using namespace toaster;

TST_OS_WINMAIN()
{
	os::output_console_create("OEX Debug Output");

	{
		knl::ApplicationDesc app_desc{};
		app_desc.maxFramesInFlight = 3u;

		knl::WindowDesc &window_desc{app_desc.windowDesc};
		window_desc.title          = "Orbo's Exile";
		window_desc.size           = {1920u, 1080u};
		window_desc.startMaximized = false;

		rnd::RenderContextDesc &render_ctx_desc{app_desc.renderCtxDesc};
		render_ctx_desc.maxFramesInFlight = 3u;
		render_ctx_desc.enableDebugInfo = false;

		knl::Application application{};
		knl::application_create(&application, app_desc);

		knl::AppLayer app_layer{};
		app_layer.initFn = +[](knl::Application *p_app, void *) mutable -> void
		{
		};
		app_layer.destroyFn = +[](knl::Application *p_app, void *) mutable -> void
		{
		};
		app_layer.onUpdateFn = +[](knl::Application *p_app, void *p_layer, float32 p_dt) mutable -> void
		{
		};
		app_layer.onRenderFn = +[](knl::Application *p_app, void *p_layer, gpu::CommandListHandle p_cmd) mutable -> void
		{
			const uint32             frame_index{p_app->frameIndex};
			const gpu::TextureHandle render_tex{p_app->window.currentTexture};

			rnd::RenderContext *render_ctx{&p_app->renderCtx};
			gpu::GPUContext *   gpu_ctx{render_ctx->gpuCtx};

			gpu::cmd_bind_resource_heap(gpu_ctx, p_cmd, render_ctx->resourceHeap);
			gpu::cmd_bind_sampler_heap(gpu_ctx, p_cmd, render_ctx->samplerHeap);
		};
		// app_layer.onEventFn = +[](knl::Application *p_app, void *p_layer, knl::Event &p_event) mutable -> void
		// {
		// 	oex::oex_onEvent(p_app, static_cast<oex::OEXLayer *>(p_layer), p_event);
		// };

		knl::application_add_layer(&application, &app_layer);

		knl::application_run(&application);
		knl::application_destroy(&application);
	}

	os::output_console_destroy();
	return 0;
}
