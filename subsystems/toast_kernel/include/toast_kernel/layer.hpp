#pragma once

#include "toast_kernel.hpp"
#include "events/event.hpp"
#include "toast_gpu/api.hpp"
#include "toast_math/math_vector.hpp"
#include "toast_render/render_context.hpp"

namespace toaster
{
	class Application;
	class InputContext;

	class TST_KERNEL_API IAppLayer
	{
	public:
		virtual ~IAppLayer() = default;

		virtual auto onInit() -> void = 0;

		virtual auto onDestroy() -> void
		{
		}

		virtual auto onUpdate([[maybe_unused]] float32 p_dt) -> void
		{
		}

		virtual auto onRender([[maybe_unused]] gpu::CommandListHandle p_cmd) -> void
		{
		}

		virtual auto onEvent([[maybe_unused]] Event &p_event) -> void
		{
		}

	protected:
		// Ts just makes things easier to access
		NonOwningPtr<Application>           m_app{nullptr};
		NonOwningPtr<InputContext>          m_inputCtx{nullptr};
		NonOwningPtr<render::RenderContext> m_renderCtx{nullptr};

	private:
		auto _register(Application *p_app) -> void;
		friend class Application;
	};
}
