#include <random>

#include "Toaster.h"
#include "imgui.h"
#include "Toaster/CameraController.hpp"

struct Transform2D
{

};

struct Particle3D
{
	glm::vec3 position{ 0.0f };
	glm::vec3 velocity{ 0.0f };
};

struct Transform3D
{
	glm::vec3 position{ 0.0f };
	glm::vec3 rotation{ 0.0f, 1.0f, 0.0f };
	glm::vec3 scale{ 1.0f };

	const glm::mat4 matrix() const
	{
		glm::mat4 sca = glm::scale(glm::mat4(1.0f), scale);
		glm::mat4 rot = (glm::length(rotation) <= 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::length(rotation), glm::normalize(rotation));
		glm::mat4 trans = glm::translate(glm::mat4(1.0f), position);
		return trans * rot * sca;
	}

	static const glm::mat4 constructMatrix(const glm::vec3 &p, const glm::vec3& r, const glm::vec3& s)
	{
		glm::mat4 sca = glm::scale(glm::mat4(1.0f), s);
		glm::mat4 rot = (glm::length(r) <= 0.0f) ? glm::mat4(1.0f) : glm::rotate(glm::mat4(1.0f), glm::length(r), glm::normalize(r));
		glm::mat4 trans = glm::translate(glm::mat4(1.0f), p);
		return trans * rot * sca;
	}
};


class TestLayer : public tst::Layer
{
public:
	TestLayer()
	{

		m_PerspectiveCameraController = std::make_shared<tst::PerspectiveCameraController>(90.0f, 1.77f);
		//m_PerspectiveCameraController = std::make_shared<tst::PerspectiveCameraController>(90.0f, 1.77f);

		float vertices[] = {

			/*POSITION				COLOUR				NORMAL		 TEXTURE COORDS*/
		 1.0f, -1.0f, -1.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
		 1.0f,  1.0f, -1.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,

		-1.0f, -1.0f,  1.0f,   0.0f, 1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
		-1.0f, -1.0f, -1.0f,   0.0f, 1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
		-1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
		-1.0f,  1.0f,  1.0f,   0.0f, 1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,

		-1.0f,  1.0f, -1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
		 1.0f,  1.0f, -1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
		 1.0f,  1.0f,  1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
		-1.0f,  1.0f,  1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

		-1.0f, -1.0f,  1.0f,   1.0f, 1.0f, 0.0f,   0.0f,-1.0f, 0.0f,   0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f,   1.0f, 1.0f, 0.0f,   0.0f,-1.0f, 0.0f,   1.0f, 0.0f,
		 1.0f, -1.0f, -1.0f,   1.0f, 1.0f, 0.0f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,   1.0f, 1.0f, 0.0f,   0.0f,-1.0f, 0.0f,   0.0f, 1.0f,

		 1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
		-1.0f, -1.0f,  1.0f,   1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
		-1.0f,  1.0f,  1.0f,   1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
		 1.0f,  1.0f,  1.0f,   1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,

		-1.0f, -1.0f, -1.0f,   0.0f, 1.0f, 1.0f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,
		 1.0f, -1.0f, -1.0f,   0.0f, 1.0f, 1.0f,   0.0f, 0.0f,-1.0f,   1.0f, 0.0f,
		 1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 1.0f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f,
		-1.0f,  1.0f, -1.0f,   0.0f, 1.0f, 1.0f,   0.0f, 0.0f,-1.0f,   0.0f, 1.0f,
		};

		uint32_t indices[] = {
			0,  1,  2,    0,  2,  3,
			4,  5,  6,    4,  6,  7,
			8,  9, 10,    8, 10, 11,
			12, 13, 14,   12, 14, 15,
			16, 17, 18,   16, 18, 19,
			20, 21, 22,   20, 22, 23
		};


		float planeVertices[] = {
			-1.0f,  0.0f,  1.0f,   0.0f, 1.0f,
			 1.0f,  0.0f,  1.0f,   1.0f, 1.0f,
			 1.0f,  0.0f, -1.0f,   1.0f, 0.0f,
			-1.0f,  0.0f, -1.0f,   0.0f, 0.0f,
		};

		uint32_t planeIndices[] = {
			0,  1,  2,    0,  2,  3
		};



		m_Vao = tst::VertexArray::create();
		tst::RefPtr<tst::VertexBuffer> vertexBuffer = tst::VertexBuffer::create(vertices, sizeof(vertices));
		tst::BufferLayout bufferLayout = {
			{"VertexPosition",	 tst::ShaderDataType::Float3},
			{"VertexColour",	 tst::ShaderDataType::Float3},
			{"VertexNormal",	 tst::ShaderDataType::Float3},
			{"TextureCoords",	 tst::ShaderDataType::Float2},
		};
		vertexBuffer->setLayout(bufferLayout);
		m_Vao->addVertexBuffer(vertexBuffer);
		auto indexBuffer = tst::IndexBuffer::create(indices, 36);
		m_Vao->addIndexBuffer(indexBuffer);



		m_PlaneVao = tst::VertexArray::create();
		tst::RefPtr<tst::VertexBuffer> planeVertexBuffer = tst::VertexBuffer::create(planeVertices, sizeof(planeVertices));
		tst::BufferLayout planeBufferLayout = {
			{"VertexPosition",	 tst::ShaderDataType::Float3},
			{"TextureCoords",	 tst::ShaderDataType::Float2},
		};
		planeVertexBuffer->setLayout(planeBufferLayout);
		m_PlaneVao->addVertexBuffer(planeVertexBuffer);
		auto planeIndexBuffer = tst::IndexBuffer::create(planeIndices, 6);
		m_PlaneVao->addIndexBuffer(planeIndexBuffer);


		for (int i = 0; i < 5; i++)
		{
			m_particles.push_back(std::make_shared<Particle3D>());
		}

		auto basicShader = m_shaderLib.loadShader("BasicShader", "assets/shaders/TestShader.glsl");
		auto flatTextureShader = m_shaderLib.loadShader("FlatTextureShader", "assets/shaders/FlatTextureShader.glsl");

		m_PlaneTexture = tst::Texture2D::create("assets/textures/Global_illumination1.png");
		m_texture0 = tst::Texture2D::create("assets/textures/orbo0.png");
	}

	void onUpdate(tst::DeltaTime dt) override
	{
		float deltaTime = dt * 3.0f;


		tst::RenderCommand::setClearColour(m_clearColour);
		tst::RenderCommand::clear();

		m_PerspectiveCameraController->onUpdate(dt);

		tst::Renderer::begin(m_PerspectiveCameraController->getCamera());

		auto basicShader		= m_shaderLib.getShader("BasicShader");
		auto flatTextureShader  = m_shaderLib.getShader("FlatTextureShader");


		m_PlaneTexture->bind();

		basicShader->bind();
		tst::Renderer::submit(m_Vao, basicShader, m_modelTransform.matrix());

		//for (auto& particle : m_particles)
		//{
		//	if (particle->position.y < -3.0f)
		//	{
		//		particle->velocity.y *= -1.0f;
		//	}
		//	particle->velocity += glm::vec3(0.0f, -1.0f, 0.0f) * 1.0f * deltaTime;
		//	particle->position += particle->velocity * deltaTime;
		//	tst::Renderer::submit(m_Vao, basicShader, glm::translate(glm::mat4(1.0f), particle->position));
		//}

		m_texture0->bind();
		flatTextureShader->bind();
		tst::Renderer::submit(m_PlaneVao, flatTextureShader, m_planeTransform.matrix());

		tst::Renderer::end();
	}

	void onEvent(tst::Event& e) override
	{
		m_PerspectiveCameraController->onEvent(e);

		tst::EventDispatcher event_dispatcher(e);
		event_dispatcher.dispatch<tst::KeyPressedEvent>([this](tst::KeyPressedEvent& e)
			{
				return onKeyPressedEvent(e);
			});
		event_dispatcher.dispatch<tst::MouseMoveEvent>([this](tst::MouseMoveEvent& e)
			{
				return onMouseMoveEvent(e);
			});
		event_dispatcher.dispatch<tst::MouseScrollEvent>([this](tst::MouseScrollEvent& e)
			{
				return onMouseScrollEvent(e);
			});
		event_dispatcher.dispatch<tst::WindowResizedEvent>([this](tst::WindowResizedEvent& e)
			{
				return onWindowResizedEvent(e);
			});
	}

	bool onKeyPressedEvent(tst::KeyPressedEvent &e)
	{

		return false;
	}

	bool onMouseMoveEvent(tst::MouseMoveEvent& e)
	{

		return false;
	}

	bool onMouseScrollEvent(tst::MouseScrollEvent &e)
	{

		return false;
	}

	bool onWindowResizedEvent(tst::WindowResizedEvent& e)
	{

		return false;
	}
	void onImguiRender() override
	{
		static bool PreferencesOpen = false;

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{

				}
				if (ImGui::MenuItem("Import"))
				{

				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Preferences", "Ctrl+,"))
				{
					PreferencesOpen = true;
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{


				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		ImGui::Begin("Properties");
		if (ImGui::CollapsingHeader("Transform"))
		{
			if (ImGui::Button("Reset"))
			{
				m_modelTransform.position.x = 0.0f;
				m_modelTransform.position.y = 0.0f;
				m_modelTransform.position.z = 0.0f;

				m_modelTransform.rotation.x = 0.0f;
				m_modelTransform.rotation.y = 0.0f;
				m_modelTransform.rotation.z = 0.0f;

				m_modelTransform.scale.x = 1.0f;
				m_modelTransform.scale.y = 1.0f;
				m_modelTransform.scale.z = 1.0f;
			}

			ImGui::Text("Position");
			ImGui::Spacing();

			ImGui::SliderFloat("X", &m_modelTransform.position.x, -10.0f, 10.0f);
			ImGui::SliderFloat("Y", &m_modelTransform.position.y, -10.0f, 10.0f);
			ImGui::SliderFloat("Z", &m_modelTransform.position.z, -10.0f, 10.0f);


			ImGui::Text("Rotation");
			ImGui::Spacing();


			ImGui::SliderFloat("X##1", &m_modelTransform.rotation.x, -6.29f, 6.29f);
			ImGui::SliderFloat("Y##2", &m_modelTransform.rotation.y, -6.29f, 6.29f);
			ImGui::SliderFloat("Z##3", &m_modelTransform.rotation.z, -6.29f, 6.29f);


			ImGui::Text("Scale");
			ImGui::Spacing();

			ImGui::SliderFloat("X##4", &m_modelTransform.scale.x, -10.0f, 10.0f);
			ImGui::SliderFloat("Y##5", &m_modelTransform.scale.y, -10.0f, 10.0f);
			ImGui::SliderFloat("Z##6", &m_modelTransform.scale.z, -10.0f, 10.0f);
		}

		if (ImGui::CollapsingHeader("Environment"))
		{
			if (ImGui::Button("Reset##1"))
			{
				m_clearColour.r = 0.15f;
				m_clearColour.g = 0.15f;
				m_clearColour.b = 0.15f;
			}

			ImGui::Text("Colour");
			ImGui::Spacing();

			ImGui::SliderFloat("R", &m_clearColour.r, 0.0f, 1.0f);
			ImGui::SliderFloat("G", &m_clearColour.g, 0.0f, 1.0f);
			ImGui::SliderFloat("B", &m_clearColour.b, 0.0f, 1.0f);
		}

		if (ImGui::CollapsingHeader("Material"))
		{
			if (ImGui::Button("Reset##2"))
			{
				cubeColour = { 1.0f, 1.0f, 1.0f };
			}
			ImGui::Spacing();

			ImGui::Text("Colour");
			ImGui::Spacing();
			ImGui::ColorPicker3("Colour Picker", &cubeColour[0]);
		}
		ImGui::End();

		if (PreferencesOpen)
		{
			ImGui::Begin("Preferences", &PreferencesOpen);

			if (ImGui::Button("CameraMode"))
			{

			}

			ImGui::End();
		}



		m_viewportWidth =	static_cast<uint32_t>(ImGui::GetContentRegionAvail().x);
		m_viewportHeight =	static_cast<uint32_t>(ImGui::GetContentRegionAvail().y);



	}

	void preferencesWindow()
	{

	}

	void render()
	{
		
	}

private:
	glm::vec4 m_clearColour{ 0.15f };

	tst::RefPtr<tst::PerspectiveCameraController> m_PerspectiveCameraController;

	std::vector<tst::RefPtr<Particle3D>> m_particles;
	tst::RefPtr<tst::VertexArray> m_Vao;
	tst::RefPtr<tst::VertexArray> m_PlaneVao;

	Transform3D m_modelTransform = {
	glm::vec3{0.0f, 0.0f, 0.0f}, // position
	glm::vec3{0.0f, 0.0f, 0.0f}, // rotation
	glm::vec3{0.15f, 0.15f, 0.15f}  // scale
	};

	Transform3D m_planeTransform = {
	glm::vec3{0.0f, 0.0f, 3.0f}, // position
	glm::vec3{0.0f, 0.0f, 6.28f}, // rotation
	glm::vec3{1.0f, 1.0f, 1.0f}  // scale
	};


	tst::ShaderLib m_shaderLib;


	tst::RefPtr<tst::Texture2D> m_texture0;
	tst::RefPtr<tst::Texture2D> m_PlaneTexture;

	glm::vec3 cubeColour{1.0f, 1.0f, 1.0f};

	uint32_t m_viewportWidth = 0;
	uint32_t m_viewportHeight = 0;
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