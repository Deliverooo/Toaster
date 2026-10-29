#include "toast_kernel/application.hpp"
#include "toast_kernel/events/event.hpp"

#include "toast_gpu/api.hpp"
#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

#include <chrono>

namespace toaster::knl
{
	auto application_create(Application *p_out_app, const ApplicationDesc &p_desc) -> void
	{
		p_out_app->maxFramesInFlight = p_desc.maxFramesInFlight;

		rnd::render_context_create(&p_out_app->renderCtx, p_desc.renderCtxDesc);

		window_init_api();
		window_create(&p_out_app->window, &p_out_app->renderCtx, p_desc.windowDesc);

		window_set_event_callback_user_data(&p_out_app->window, p_out_app);
		window_set_event_callback(&p_out_app->window, +[](Event &p_event, void *p_user_data) -> void
		{
			const auto app{static_cast<Application *>(p_user_data)};
			for (auto &layer: app->layers)
			{
				if (p_event.isHandled())
					continue;

				if (layer.onEventFn)
					layer.onEventFn(app, layer.userData, p_event);
			}
		});
	}

	auto application_destroy(Application *p_app) -> void
	{
		gpu::wait_idle(p_app->renderCtx.gpuCtx);
		gpu::poll_uploads(p_app->renderCtx.uploadCtx);

		for (const auto &layer: p_app->layers)
		{
			if (layer.destroyFn)
				layer.destroyFn(p_app, layer.userData);
		}
		p_app->layers.clear();

		window_destroy(&p_app->window);

		rnd::render_context_destroy(&p_app->renderCtx);

		window_shutdown_api();
	}

	auto application_run(Application *p_app, uint32 p_target_fps) -> void
	{
		Vector<gpu::CommandListHandle> command_lists(p_app->maxFramesInFlight);
		for (auto &cmd: command_lists)
			cmd = gpu::cmd_list_get_or_create(p_app->renderCtx.gpuCtx, gpu::EQueueType::eGraphics);

		auto last_time{std::chrono::steady_clock::now()};
		gpu::swapchain_set_max_framerate(p_app->renderCtx.gpuCtx, p_app->window.swapchain, p_target_fps);
		while (p_app->running)
		{
			gpu::swapchain_wait_for_framerate(p_app->renderCtx.gpuCtx, p_app->window.swapchain);

			if (!window_process_messages(&p_app->window)) // glfwPollEvents. returns !glfwWindowShouldClose(...);
				break;

			if (!window_check_for_resize(&p_app->window))
				continue;

			gpu::poll_uploads(p_app->renderCtx.uploadCtx);
			gpu::begin_frame(p_app->renderCtx.frameCtx, p_app->frameIndex);

			auto                           current_time{std::chrono::steady_clock::now()};
			std::chrono::duration<float32> elapsed{current_time - last_time};
			p_app->deltaTime = elapsed.count();
			last_time        = current_time;

			gpu::CommandListHandle &cmd{command_lists[p_app->frameIndex]};
			gpu::cmd_list_reset(p_app->renderCtx.gpuCtx, cmd);
			gpu::cmd_list_open(p_app->renderCtx.gpuCtx, cmd);

			if (!window_begin_frame(&p_app->window, cmd)) // Attempts to acquire the image
			{
				gpu::cmd_list_reset(p_app->renderCtx.gpuCtx, cmd);
				continue;
			}

			if (!window_is_minimised(&p_app->window))
			{
				gpu::swapchain_set_frame_marker(p_app->renderCtx.gpuCtx, p_app->window.swapchain, gpu::EFrameMarker::eSimulationStart);
				for (auto &layer: p_app->layers)
				{
					if (layer.onUpdateFn)
						layer.onUpdateFn(p_app, layer.userData, p_app->deltaTime);
				}
				gpu::swapchain_set_frame_marker(p_app->renderCtx.gpuCtx, p_app->window.swapchain, gpu::EFrameMarker::eSimulationEnd);
			}
			gpu::swapchain_set_frame_marker(p_app->renderCtx.gpuCtx, p_app->window.swapchain, gpu::EFrameMarker::eRendersubmitStart);
			if (!window_is_minimised(&p_app->window))
			{
				for (auto &layer: p_app->layers)
				{
					if (layer.onRenderFn)
						layer.onRenderFn(p_app, layer.userData, cmd);
				}
			}

			window_submit_and_present(&p_app->window, cmd); // also updates the input states
			p_app->frameIndex = (p_app->frameIndex + 1u) % p_app->maxFramesInFlight;
		}

		for (auto &cmd: command_lists)
			gpu::cmd_list_free(p_app->renderCtx.gpuCtx, cmd);
	}

	auto application_close(Application *p_app) -> void
	{
		p_app->running = false;
	}

	auto application_add_layer(Application *p_app, const AppLayer *p_layer) -> void
	{
		p_app->layers.push_back(*p_layer);
		if (p_layer->initFn)
			p_layer->initFn(p_app, p_layer->userData);
	}

	auto application_get_input_ctx(Application *p_app) -> InputContext *
	{
		return &p_app->window.cbData.inputCtx;
	}

	auto application_get_input_ctx(const Application *p_app) -> const InputContext *
	{
		return &p_app->window.cbData.inputCtx;
	}
}
