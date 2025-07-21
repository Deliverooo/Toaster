#include "Sandbox3D.hpp"

void SandBox3DLayer::onAttach()
{
	TST_PROFILE_FN();

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


	m_PerspectiveCameraCtrl = std::make_shared<tst::PerspectiveCameraController>(90.0f, 1.77f);

	m_Texture0 = tst::Texture2D::create("assets/textures/orbo0.png");


}
void SandBox3DLayer::onDetach()
{
	
}
void SandBox3DLayer::onUpdate(tst::DeltaTime dt)
{
	TST_PROFILE_FN();

	tst::RenderCommand::setClearColour(m_clearColour);
	tst::RenderCommand::clear();

	m_PerspectiveCameraCtrl->onUpdate(dt);


	{
		TST_PROFILE_SCP("Renderer3D::begin");
		tst::Renderer3D::begin(m_PerspectiveCameraCtrl->getCamera());
	}

	{
		TST_PROFILE_SCP("Renderer3D::drawQuad");
		tst::Renderer3D::drawQuad({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.2f, 1.0f });
		tst::Renderer3D::drawQuad({ 1.0f, 1.0f, 0.1f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, m_Texture0);
	}


	tst::Renderer3D::end();
}
void SandBox3DLayer::onEvent(tst::Event& e)
{
	m_PerspectiveCameraCtrl->onEvent(e);
}
void SandBox3DLayer::onImguiRender()
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
			m_CubeColour = { 1.0f, 1.0f, 1.0f, 1.0f };
		}
		ImGui::Spacing();

		ImGui::Text("Colour");
		ImGui::Spacing();
		ImGui::ColorPicker3("Colour Picker", &m_CubeColour[0]);
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
}