#pragma once

#include "layer.hpp"
#include "window.hpp"
#include "toast_render/render_context.hpp"

namespace toaster
{
	class TST_KERNEL_API Application
	{
	public:
		static constexpr uint32 maxFramesInFlight{3u};

		Application();
		~Application();

		auto run() -> void;
		auto close() -> void;

		[[nodiscard]] auto getRenderCtx() const -> render::RenderContext & { return *m_renderCtx; }
		[[nodiscard]] auto getWindow() const -> Window & { return *m_window; }

		auto getFrameIndex() const -> uint32 { return m_frameIndex; }

		template<typename TLayer, typename... TArgs> requires std::derived_from<TLayer, IAppLayer>
		auto addLayer(TArgs &&... p_args) -> void
		{
			auto &layer{m_layers.emplace_back(new TLayer(std::forward<TArgs>(p_args)...))};
			layer->_register(this);
			layer->onInit();
		}

	private:
		UniquePtr<render::RenderContext> m_renderCtx{nullptr};
		UniquePtr<Window>                m_window{nullptr};

		std::vector<IAppLayer *> m_layers;

		float32 m_deltaTime{0.0f};
		uint32  m_frameIndex{0u}; // TODO: Move to render ctx

		bool m_running{true};

		friend class IAppLayer;
	};
}
