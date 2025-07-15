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

		m_imguiLayer = std::make_shared<ImguiLayer>();
		pushOverlay(m_imguiLayer);


		float vertices[] = {
			// Position           // Colour         // Normal                  // TexCoords
			-1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 0.0f,  -0.577f, -0.577f, -0.577f,  0.0f, 0.0f,  // 0
			 1.0f, -1.0f, -1.0f,   0.0f, 1.0f, 0.0f,   0.577f, -0.577f, -0.577f,  1.0f, 0.0f,  // 1
			 1.0f,  1.0f, -1.0f,   0.0f, 0.0f, 1.0f,   0.577f,  0.577f, -0.577f,  1.0f, 1.0f,  // 2
			-1.0f,  1.0f, -1.0f,   1.0f, 1.0f, 0.0f,  -0.577f,  0.577f, -0.577f,  0.0f, 1.0f,  // 3
			-1.0f, -1.0f,  1.0f,   0.0f, 1.0f, 1.0f,  -0.577f, -0.577f,  0.577f,  0.0f, 0.0f,  // 4
			 1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 1.0f,   0.577f, -0.577f,  0.577f,  1.0f, 0.0f,  // 5
			 1.0f,  1.0f,  1.0f,   0.5f, 0.5f, 0.5f,   0.577f,  0.577f,  0.577f,  1.0f, 1.0f,  // 6
			-1.0f,  1.0f,  1.0f,   1.0f, 1.0f, 1.0f,  -0.577f,  0.577f,  0.577f,  0.0f, 1.0f   // 7
		};

		uint32_t indices[] = {
			0, 1, 2, 2, 3, 0,  // -Z
			4, 5, 6, 6, 7, 4,  // +Z
			0, 3, 7, 7, 4, 0,  // -X
			1, 5, 6, 6, 2, 1,  // +X
			0, 4, 5, 5, 1, 0,  // -Y
			3, 2, 6, 6, 7, 3   // +Y
		};

		m_Vao = VertexArray::create();
		m_Vbo = VertexBuffer::create(vertices, sizeof(vertices));

		BufferLayout bufferLayout = {
			{"VertexPosition", ShaderDataType::Float3},
			{"VertexColour", ShaderDataType::Float3},
			{"VertexNormal", ShaderDataType::Float3},
			{"TextureCoords", ShaderDataType::Float2},
		};

		m_Vbo->setLayout(bufferLayout);
		m_Vao->addVertexBuffer(m_Vbo);

		m_Ebo = IndexBuffer::create(indices, 36);
		m_Vao->addIndexBuffer(m_Ebo);

		m_basicShader = Shader::create("C:/dev/Toaster/Toaster/res/shaders/BasicShader.vert",
			"C:/dev/Toaster/Toaster/res/shaders/BasicShader.frag");

		m_texture0 = Texture::create("C:/dev/Toaster/Toaster/res/images/Global_illumination1.png");

		m_basicShader->bind();
		glUniform1i(glGetUniformLocation(m_basicShader->getId(), "tex1"), 0);

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

#define TST_SHADER_UNIFORM_MAT4(name, value) glUniformMatrix4fv(glGetUniformLocation(m_basicShader->getId(), name), 1, GL_FALSE, glm::value_ptr(value));

	void Application::run() {

		m_running = true;

		while (m_running)
		{
			glViewport(0, 0, static_cast<int>(m_window->getWidth()), static_cast<int>(m_window->getHeight()));

			glClearColor(0.21f, 0.21f, 0.21f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			m_basicShader->bind();
			glm::mat4 projection = glm::perspective(glm::radians(90.0f), m_window->getAspect(), 0.1f, 100.0f);
			glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

			TST_SHADER_UNIFORM_MAT4("projection", projection);
			TST_SHADER_UNIFORM_MAT4("view", view);


			m_Vao->bind();
			m_texture0->bind();
			glm::mat4 trans = glm::mat4(1.0f);
			trans = glm::rotate(trans, glm::radians(static_cast<float>(glfwGetTime() * 20.0f)), glm::vec3(1.0, 0.0, 1.0));
			trans = glm::scale(trans, glm::vec3(0.5f));

			TST_SHADER_UNIFORM_MAT4("transform", trans);
			glDrawElements(GL_TRIANGLES, m_Ebo->count(), GL_UNSIGNED_INT, nullptr);




			for (std::shared_ptr<Layer> layer : m_layerStack)
			{
				layer->onUpdate();
			}

			m_imguiLayer->begin();
			for (std::shared_ptr<Layer> layer : m_layerStack)
			{
				layer->onImguiRender();
			}
			m_imguiLayer->end();

			m_window->update();
		}

	}

	void Application::pushLayer(std::shared_ptr<Layer> layer)
	{
		m_layerStack.pushLayer(layer);
		layer->onAttach();
	}
	void Application::pushOverlay(std::shared_ptr<Layer> overlay)
	{
		m_layerStack.pushOverlay(overlay);
		overlay->onAttach();
	}
	void Application::popLayer(std::shared_ptr<Layer> layer)
	{
		m_layerStack.popLayer(layer);
		layer->onDetach();
	}
	void Application::popOverlay(std::shared_ptr<Layer> overlay)
	{
		m_layerStack.popOverlay(overlay);
		overlay->onDetach();
	}


}
