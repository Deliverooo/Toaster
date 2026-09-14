#pragma once

#include "input.hpp"

#include "toast_gpu/api.hpp"

#include "toast_math/math_vector.hpp"

struct GLFWwindow;

namespace toaster
{
	class InputContext;

	struct TST_KERNEL_API WindowDesc
	{
		tsm::uint2 size{1920u, 1080u};
		CString    title{"Toaster-3.0"};

		bool startMaximized{false};
	};

	class TST_KERNEL_API Window
	{
	public:
		// Initialises GLFW
		static auto initWindowingAPI() -> void;
		// Take a guess
		static auto shutdownWindowingAPI() -> void;

		Window(const WindowDesc &p_desc);
		~Window();

		// Returns true if the window should close
		auto processMessages() const -> bool;

		auto beginFrame(gpu::CommandListHandle p_cmd) -> bool; // Skip a frame if this returns false.
		auto submitAndPresent(gpu::CommandListHandle p_cmd) -> void;

		// Returns true if the window was either resized successfully or there was no resizing to do.
		// If false, it means that the window is either an invalid size or the swapchain could not be correctly recreated. In this case use this to skip a frame.
		auto checkForResize() -> bool;

		auto isResized() const -> bool { return m_cbData.resized; }
		auto isMaximised() const -> bool { return m_cbData.maximised; }
		auto isMinimised() const -> bool { return m_cbData.minimised; }
		auto isFullscreen() const -> bool;

		auto setResized(bool p_resized) -> void { m_cbData.resized = p_resized; } // Safe to modify because the wnd proc is on the main thread
		auto maximiseWindow() -> void;
		auto minimiseWindow() -> void;
		auto restoreWindow() -> void;
		auto setFullscreen() -> void;
		auto setWindowed() -> void;

		auto getWidth() const -> uint32 { return m_cbData.width; }
		auto getHeight() const -> uint32 { return m_cbData.height; }
		auto getSize() const -> tsm::uint2 { return {m_cbData.width, m_cbData.height}; }
		auto getAspectRatio() const -> float32 { return static_cast<float32>(m_cbData.width) / static_cast<float32>(m_cbData.height); }
		auto isInvalidSize() const -> bool { return m_cbData.width == 0u || m_cbData.height == 0u; }

		auto getInputCtx() -> InputContext & { return m_cbData.inputCtx; }
		auto getInputCtx() const -> const InputContext & { return m_cbData.inputCtx; }

		auto getGLFWWindow() const -> GLFWwindow * { return m_window; }

		auto getSwapchain() const -> gpu::SwapchainHandle { return m_swapchain; }

		auto getCurrentTexture() const -> gpu::TextureHandle { return m_currentTexture; }

	private:
		GLFWwindow *m_window{nullptr};

		// The exact data that is passed as a user data pointer to the wnd proc
		struct CallbackData
		{
			uint32 width{0u};
			uint32 height{0u};

			bool minimised{false};
			bool maximised{false};
			bool resized{false};

			InputContext inputCtx;
		};

		CallbackData m_cbData{};

		gpu::SurfaceHandle   m_windowSurface{nullptr};
		gpu::SwapchainHandle m_swapchain{nullptr};

		gpu::TextureHandle m_currentTexture{nullptr};
	};
}
