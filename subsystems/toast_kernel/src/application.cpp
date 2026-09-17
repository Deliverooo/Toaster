#include "toast_kernel/application.hpp"

#include <chrono>

#include "toast_gpu/frame.hpp"
#include "toast_gpu/upload.hpp"

namespace toaster
{
	Application::Application()
	{
		Window::initWindowingAPI();

		m_renderCtx = makeUnique<render::RenderContext>();

		WindowDesc window_desc{};
		window_desc.startMaximized = true;
		window_desc.title          = "Toaster-3.0";
		m_window                   = makeUnique<Window>(window_desc);

		m_window->setEventCallback([this](Event &p_event)
		{
			for (auto layer: m_layers)
			{
				if (p_event.isHandled())
					continue;
				layer->onEvent(p_event);
			}
		});
	}

	Application::~Application()
	{
		gpu::waitIdle();

		for (auto layer: m_layers)
		{
			layer->onDestroy();
			delete layer;
		}
		m_layers.clear();

		m_window.reset();
		m_renderCtx.reset();

		Window::shutdownWindowingAPI();
	}

	auto Application::run() -> void
	{
		std::array<gpu::CommandListHandle, maxFramesInFlight> command_lists;
		for (auto &cmd: command_lists)
			cmd = gpu::getOrCreateCommandList(gpu::EQueueType::eGraphics);

		auto last_time{std::chrono::high_resolution_clock::now()};
		while (m_running) // TODO: Events / ts
		{
			if (!m_window->processMessages())
				break;

			if (!m_window->checkForResize())
				continue;

			auto                           current_time{std::chrono::high_resolution_clock::now()};
			std::chrono::duration<float32> elapsed{current_time - last_time};
			m_deltaTime = elapsed.count();
			last_time   = current_time;

			gpu::upload::flushUploads();
			gpu::frame::beginFrame(m_frameIndex);

			gpu::CommandListHandle &cmd{command_lists[m_frameIndex]};
			gpu::resetCommandList(cmd);

			gpu::openCommandList(cmd);
			if (!m_window->beginFrame(cmd))
			{
				gpu::resetCommandList(cmd);
				continue;
			}

			if (!m_window->isMinimised())
			{
				for (auto &layer: m_layers)
				{
					layer->onUpdate(m_deltaTime);
					layer->onRender(cmd);
				}
			}
			m_window->submitAndPresent(cmd);

			m_frameIndex = (m_frameIndex + 1u) % maxFramesInFlight;
		}

		for (auto &cmd: command_lists)
			gpu::freeCommandList(cmd);
		command_lists.fill(nullptr);
	}

	auto Application::close() -> void
	{
		m_running = false;
		// TODO: Close stuff
	}

	// Application::Application(const ApplicationSpecInfo &p_spec_info, const CommandLineArgs *p_command_line_args) : m_specInfo(p_spec_info),
	// 																											   m_commandLineArgs(p_command_line_args)
	// {
	// 	Window::initWindowingAPI();
	//
	// 	render::RenderContextSpecInfo render_context_spec_info{};
	// 	render_context_spec_info.sdkDir             = m_specInfo.sdkDir / "bin";
	// 	render_context_spec_info.instanceExtensions = Window::getRequiredInstanceExtensions();
	// 	render_context_spec_info.printDebugInfo     = m_specInfo.printGPUDebugInfo;
	// 	m_renderContext                             = new render::RenderContext{render_context_spec_info};
	//
	// 	TST_PERMA_ASSERT(m_renderContext);
	//
	// 	#pragma region create window
	// 	m_window = new Window(m_renderContext, m_specInfo.windowSpecInfo);
	// 	m_window->setEventCallback([this](Event &e)
	// 	{
	// 		EventDispatcher dispatcher{e};
	// 		dispatcher.dispatch<WindowCloseEvent>(TST_BIND_EVENT_FN(Application::onWindowCloseEvent));
	// 		dispatcher.dispatch<WindowResizeEvent>(TST_BIND_EVENT_FN(Application::onWindowResizeEvent));
	//
	// 		for (const auto &layer: m_layers)
	// 		{
	// 			if (e.isHandled())
	// 				continue;
	// 			layer->onEvent(e);
	// 		}
	// 	});
	//
	// 	m_window->getSwapchain()->setResizeUserDataPointer(this);
	// 	m_window->getSwapchain()->setResizeCallback([](void *p_user_data, tsm::uint2 p_size) -> void
	// 	{
	// 		for (const auto app{static_cast<Application *>(p_user_data)}; const auto &layer: app->m_layers)
	// 		{
	// 			layer->onResize(p_size);
	// 		}
	// 	});
	//
	// 	#pragma endregion
	// }
	//
	// Application::~Application() noexcept
	// {
	// 	for (auto &layer: m_layers)
	// 	{
	// 		layer->onDestroy();
	// 		m_renderContext->gpuWaitIdle(); // Wait until the GPU is finished using all the layers' resources
	// 		delete layer;
	// 	}
	//
	// 	m_layers.clear();
	//
	// 	delete m_window;
	// 	delete m_renderContext;
	// 	Window::shutdownWindowingAPI();
	// }
	//
	// auto Application::run() -> int32
	// {
	// 	for (auto &layer: m_layers)
	// 		layer->onUIInit(m_onUIInitUserData);
	//
	// 	while (m_isRunning)
	// 	{
	// 		const auto startTime{static_cast<float32>(glfwGetTime())};
	// 		m_deltaTime     = startTime - m_lastFrameTime;
	// 		m_lastFrameTime = startTime;
	//
	// 		m_window->processEvents();
	// 		m_window->beginFrame();
	//
	// 		m_renderContext->setCurrentCommandBuffer(&m_window->getSwapchain()->getCurrentCommandBuffer());
	// 		m_renderContext->getGPUContext()->bindDescriptorHeap();
	//
	// 		if (!m_minimized)
	// 		{
	// 			for (auto &layer: m_layers)
	// 				layer->onUpdate(m_deltaTime);
	//
	// 			if (m_cbBeginUIRender)
	// 				m_cbBeginUIRender();
	//
	// 			for (auto &layer: m_layers)
	// 				layer->onUIRender();
	//
	// 			if (m_cbEndUIRender)
	// 				m_cbEndUIRender();
	// 		}
	// 		m_window->endFrame();
	// 	}
	//
	// 	return 0;
	// }
	//
	// auto Application::close() noexcept -> void
	// {
	// 	m_isRunning = false;
	// }
	//
	// auto Application::createScene(const String &p_name) const -> UniquePtr<scene::Scene>
	// {
	// 	return toaster::makeUnique<scene::Scene>(m_renderContext, nullptr, p_name);
	// }
	//
	// auto Application::onWindowCloseEvent([[maybe_unused]] WindowCloseEvent &p_event) -> bool
	// {
	// 	m_isRunning = false;
	// 	return true;
	// }
	//
	// auto Application::onWindowResizeEvent(WindowResizeEvent &p_event) -> bool
	// {
	// 	const uint32 width{p_event.getWidth()};
	// 	const uint32 height{p_event.getHeight()};
	//
	// 	if (width == 0 || height == 0)
	// 	{
	// 		m_minimized = true;
	// 		return false;
	// 	}
	//
	// 	m_minimized = false;
	// 	return false;
	// }
	//
	// auto Application::setOnUIInitUserData(void *p_user_data) -> void
	// {
	// 	m_onUIInitUserData = p_user_data;
	// }
	//
	// auto Application::setBeginUIRenderCallback(const std::function<void()> &p_cb_begin_ui_render) -> void
	// {
	// 	m_cbBeginUIRender = p_cb_begin_ui_render;
	// }
	//
	// auto Application::setEndUIRenderCallback(const std::function<void()> &p_cb_end_ui_render) -> void
	// {
	// 	m_cbEndUIRender = p_cb_end_ui_render;
	// }
}
