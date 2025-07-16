#include "Toaster.h"
#include "imgui.h"
#include "../../dependencies/GLFW/include/GLFW/glfw3.h"

struct Transform2D
{

};

struct Transform3D
{
	glm::vec3 position{ 0.0f };
	glm::vec3 rotation{ 0.0f, 1.0f, 0.0f };
	glm::vec3 scale{ 1.0f };

	const glm::mat4 &matrix() const
	{
		return glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), scale), glm::length(rotation), glm::normalize(rotation)), position);
	}
};

class TestLayer : public tst::Layer
{
public:
	TestLayer()
	{
		m_Camera = std::make_shared<tst::PerspectiveCamera>(glm::radians(90.0f), 16.0f / 9.0f, 0.1f, 100.0f);
		m_Camera->setPosition({ 0.0f, 0.0f, 2.5f });
		


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

		m_Vao = tst::VertexArray::create();
		std::shared_ptr<tst::VertexBuffer> vertexBuffer = tst::VertexBuffer::create(vertices, sizeof(vertices));

		tst::BufferLayout bufferLayout = {
			{"VertexPosition", tst::ShaderDataType::Float3},
			{"VertexColour", tst::ShaderDataType::Float3},
			{"VertexNormal", tst::ShaderDataType::Float3},
			{"TextureCoords", tst::ShaderDataType::Float2},
		};

		vertexBuffer->setLayout(bufferLayout);
		m_Vao->addVertexBuffer(vertexBuffer);

		auto indexBuffer = tst::IndexBuffer::create(indices, 36);
		m_Vao->addIndexBuffer(indexBuffer);

		m_basicShader = tst::Shader::create("C:/dev/Toaster/Toaster/res/shaders/BasicShader.vert",
		                                    "C:/dev/Toaster/Toaster/res/shaders/BasicShader.frag");

		m_texture0 = tst::Texture::create("C:/dev/Toaster/Toaster/res/images/Global_illumination1.png");
	}

	void onUpdate() override
	{
		static float dt = 0.02f;

		tst::RenderCommand::setClearColour(m_clearColour);
		tst::RenderCommand::clear();

		auto& activeCamera = m_Camera;

		if (tst::Input::isKeyPressed(TST_KEY_W))			{ cameraTransform.position += dt * activeCamera->front(); }
		if (tst::Input::isKeyPressed(TST_KEY_A))			{ cameraTransform.position -= dt * activeCamera->right(); }
		if (tst::Input::isKeyPressed(TST_KEY_S))			{ cameraTransform.position -= dt * activeCamera->front(); }
		if (tst::Input::isKeyPressed(TST_KEY_D))			{ cameraTransform.position += dt * activeCamera->right(); }

		if (tst::Input::isKeyPressed(TST_KEY_SPACE))		{ cameraTransform.position += dt * glm::vec3(0.0f, 1.0f, 0.0f); }
		if (tst::Input::isKeyPressed(TST_KEY_LEFT_SHIFT))	{ cameraTransform.position -= dt * glm::vec3(0.0f, 1.0f, 0.0f); }

		if (tst::Input::isKeyPressed(TST_KEY_LEFT))			{ cameraTransform.rotation.y -= dt * m_cameraSpeed; }
		if (tst::Input::isKeyPressed(TST_KEY_RIGHT))		{ cameraTransform.rotation.y += dt * m_cameraSpeed; }
		if (tst::Input::isKeyPressed(TST_KEY_UP))			{ cameraTransform.rotation.x += dt * m_cameraSpeed; }
		if (tst::Input::isKeyPressed(TST_KEY_DOWN))			{ cameraTransform.rotation.x -= dt * m_cameraSpeed; }


		activeCamera->setPosition(cameraTransform.position);
		activeCamera->setRotation(cameraTransform.rotation);

		tst::Renderer::begin(activeCamera);

		Transform3D modelTransform = {
			glm::vec3{0.0f, 0.0f, 0.0f}, // pos
			glm::vec3{0.0f, 1.0f, 0.0f}, // rot
			glm::vec3{1.0f, 1.0f, 1.0f}};// sca

		m_texture0->bind();
		tst::Renderer::submit(m_Vao, modelTransform.matrix(), m_basicShader);

		tst::Renderer::end();
	}

#define TST_BIND_EVENT(func) std::bind(&func, this, std::placeholders::_1)

	void onEvent(tst::Event& e) override
	{
		tst::EventDispatcher event_dispatcher(e);
		event_dispatcher.dispatch<tst::KeyPressedEvent>(TST_BIND_EVENT(TestLayer::onKeyPressedEvent));
	}

	bool onKeyPressedEvent(tst::KeyPressedEvent &e)
	{
		TST_TRACE("{0}", e);

		if (e.getKeycode() == TST_KEY_P)
		{
			m_Camera = std::make_shared<tst::PerspectiveCamera>(90.0f, 16.0f / 9.0f, 0.1f, 100.0f);
			m_Camera->setPosition(cameraTransform.position);
			m_Camera->setRotation(cameraTransform.rotation);
		} else if (e.getKeycode() == TST_KEY_O)
		{
			m_Camera = std::make_shared<tst::OrthoCamera>(-1.0f, 1.0f, -1.0f, 1.0f);
			m_Camera->setPosition(cameraTransform.position);
			m_Camera->setRotation(cameraTransform.rotation);
		}
		return false;
	}

	void onImguiRender() override
	{
		
	}
private:

	std::shared_ptr<tst::VertexArray> m_Vao;

	std::shared_ptr<tst::Texture> m_texture0;
	std::shared_ptr<tst::Shader> m_basicShader;

	std::shared_ptr<tst::Camera> m_Camera;

	Transform3D cameraTransform{};

	float m_cameraSpeed = 0.5f;

	glm::vec4 m_clearColour{ 1.0f };
};

class SandboxApp : public tst::Application
{
public:
	SandboxApp() {
			
		pushLayer(std::make_shared<TestLayer>());
	}	
	~SandboxApp() {

	}
};

// the definition of the CreateApplication function
tst::Application* tst::CreateApplication() {
	std::cout << "Created app" << "\n";

	return new SandboxApp();
}