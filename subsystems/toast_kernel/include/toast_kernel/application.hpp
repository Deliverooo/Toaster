#pragma once

#include "layer.hpp"
#include "window.hpp"
#include "toast_render/render_context.hpp"

namespace toaster::knl
{
	struct TST_KERNEL_API Application
	{
		Vector<AppLayer> layers;
		rnd::RenderContext    renderCtx{};
		Window                window{};

		float32 deltaTime{0.0f};
		uint32  frameIndex{0u};
		uint32  maxFramesInFlight{3u};
		bool    running{true};
	};

	struct TST_KERNEL_API ApplicationDesc
	{
		WindowDesc             windowDesc;
		rnd::RenderContextDesc renderCtxDesc;
		uint32                 maxFramesInFlight;
	};

	auto TST_KERNEL_API application_create(Application *p_out_app, const ApplicationDesc &p_desc) -> void;
	auto TST_KERNEL_API application_destroy(Application *p_app) -> void;
	auto TST_KERNEL_API application_run(Application *p_app, uint32 p_target_fps) -> void;

	auto TST_KERNEL_API application_close(Application *p_app) -> void;

	auto TST_KERNEL_API application_add_layer(Application *p_app, const AppLayer *p_layer) -> void;

	// Essentially just a helper instead of going to &app->window.cbData.inputCtx
	auto TST_KERNEL_API application_get_input_ctx(Application *p_app) -> InputContext *;
	auto TST_KERNEL_API application_get_input_ctx(const Application *p_app) -> const InputContext *;
}
