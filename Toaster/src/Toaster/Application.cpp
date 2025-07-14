#include "tstpch.h"

#include "Application.hpp"

#include "Events/ApplicationEvent.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtc/type_ptr.inl"
#include "glm/gtx/transform.hpp"

#ifdef TST_PLATFORM_WINDOWS
#include "platform/Windows/WindowsWindow.hpp"
#endif

namespace tst
{
	Application* Application::m_instance = nullptr;

	void Application::init()
	{
		m_window = std::unique_ptr<Window>(Window::Create({ 1280, 720, "Toaster", true}));
		m_window->setEventCallback([this](Event& e)
			{
				onEvent(e);
			});

		m_imguiLayer = new ImguiLayer();
		pushOverlay(m_imguiLayer);

		glGenVertexArrays(1, &m_Vao);
		glGenBuffers(1, &m_Vbo);
		glGenBuffers(1, &m_Ebo);
		 
		glBindVertexArray(m_Vao);

		float vertices[] = {
			// Position           // Colour          // Normal
			-1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 0.0f,  -0.577f, -0.577f, -0.577f,  // 0
			 1.0f, -1.0f, -1.0f,   0.0f, 1.0f, 0.0f,   0.577f, -0.577f, -0.577f,  // 1
			 1.0f,  1.0f, -1.0f,   0.0f, 0.0f, 1.0f,   0.577f,  0.577f, -0.577f,  // 2
			-1.0f,  1.0f, -1.0f,   1.0f, 1.0f, 0.0f,  -0.577f,  0.577f, -0.577f,  // 3
			-1.0f, -1.0f,  1.0f,   0.0f, 1.0f, 1.0f,  -0.577f, -0.577f,  0.577f,  // 4
			 1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 1.0f,   0.577f, -0.577f,  0.577f,  // 5
			 1.0f,  1.0f,  1.0f,   0.5f, 0.5f, 0.5f,   0.577f,  0.577f,  0.577f,  // 6
			-1.0f,  1.0f,  1.0f,   1.0f, 1.0f, 1.0f,  -0.577f,  0.577f,  0.577f   // 7
		};


		uint32_t indices[] = {
			0, 1, 2, 2, 3, 0,  // -Z
			4, 5, 6, 6, 7, 4,  // +Z
			0, 3, 7, 7, 4, 0,  // -X
			1, 5, 6, 6, 2, 1,  // +X
			0, 4, 5, 5, 1, 0,  // -Y
			3, 2, 6, 6, 7, 3   // +Y
		};

		glBindBuffer(GL_ARRAY_BUFFER, m_Vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

		// position
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);

		// colour
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// normal
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);


		m_basicShader = std::make_unique<Shader>("C:/dev/Toaster/Toaster/res/shaders/BasicShader.vert",
			"C:/dev/Toaster/Toaster/res/shaders/BasicShader.frag");

		glEnable(GL_DEPTH_TEST);
	}

	Application::Application() {

		m_instance = this;

		init();
	}

	Application::~Application() {
		// Destructor implementation
	}

	// Event handler for the application
	// This function is called whenever an event occurs
	// It logs the event and can be extended to handle specific events
	void Application::onEvent(Event& event)
	{
		EventDispatcher event_dispatcher(event);
		event_dispatcher.dispatch<WindowClosedEvent>([this](WindowClosedEvent &e){
			m_running = false;
			return true;
		});

		// Iterate through the layer stack in reverse order
		// This allows the topmost layer to handle the event first
		// If the event is handled, we stop further propagation
		// This is useful for layered applications where the top layer might handle input events
		// and prevent them from reaching lower layers
		for (auto it = m_layerStack.end(); it != m_layerStack.begin();)
		{
			(*--it)->onEvent(event);
			if (event.isHandled())
			{
				break;
			}
		}
	}

	void Application::run() {

		m_running = true;

		while (m_running)
		{
			glViewport(0, 0, static_cast<int>(m_window->getWidth()), static_cast<int>(m_window->getHeight()));

			glClearColor(0.21f, 0.21f, 0.21f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 trans = glm::mat4(1.0f);
			trans = glm::rotate(trans, glm::radians((float)glfwGetTime() * 20.0f), glm::vec3(1.0, 0.0, 1.0));
			trans = glm::scale(trans, glm::vec3(0.5f));

			glm::mat4 projection = glm::perspective(glm::radians(90.0f), (float)m_window->getWidth() / (float)m_window->getHeight(), 0.1f, 100.0f);

			glm::mat4 view = glm::mat4(1.0f);
			view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

			unsigned int transformLoc = glGetUniformLocation(m_basicShader->getId(), "transform");
			glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));

			unsigned int projectionLoc = glGetUniformLocation(m_basicShader->getId(), "projection");
			glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

			unsigned int viewLoc = glGetUniformLocation(m_basicShader->getId(), "view");
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

			m_basicShader->bind();
			glBindVertexArray(m_Vao);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

			for (Layer* layer : m_layerStack)
			{
				layer->onUpdate();
			}

			m_imguiLayer->begin();
			for (Layer* layer : m_layerStack)
			{
				layer->onImguiRender();
			}
			m_imguiLayer->end();

			m_window->update();
		}

	}

	void Application::pushLayer(Layer* layer)
	{
		m_layerStack.pushLayer(layer);
		layer->onAttach();
	}
	void Application::pushOverlay(Layer* overlay)
	{
		m_layerStack.pushOverlay(overlay);
		overlay->onAttach();
	}
	void Application::popLayer(Layer* layer)
	{
		m_layerStack.popLayer(layer);
		layer->onDetach();
	}
	void Application::popOverlay(Layer* overlay)
	{
		m_layerStack.popOverlay(overlay);
		overlay->onDetach();
	}


}
