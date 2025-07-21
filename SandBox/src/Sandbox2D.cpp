#include "Sandbox2D.hpp"



void SandBox2DLayer::onAttach()
{
	TST_PROFILE_FN();

	m_OrthoCameraCtrl = std::make_shared<tst::OrthoCamera2DController>(1.77f);

	m_Texture0 = tst::Texture2D::create("assets/textures/orbo0.png");
	m_Texture1 = tst::Texture2D::create("assets/textures/One_Leg_Bird.png");

}
void SandBox2DLayer::onDetach()
{
	TST_PROFILE_FN();

}

void SandBox2DLayer::onUpdate(tst::DeltaTime dt)
{
	TST_PROFILE_FN();

	tst::RenderCommand::setClearColour(m_clearColour);
	tst::RenderCommand::clear();

	m_OrthoCameraCtrl->onUpdate(dt);


	{
		TST_PROFILE_SCP("Renderer2D::begin");
		tst::Renderer2D::begin(m_OrthoCameraCtrl->getCamera());
	}

	{
		TST_PROFILE_SCP("Renderer2D::drawQuad");
		tst::Renderer2D::drawQuad({ 0.0f, 1.0f, 0.1f}, { 1.0f, 1.0f }, { 90.0f }, m_Texture0);
		tst::Renderer2D::drawQuad(m_modelTransform.position, m_modelTransform.scale, m_modelTransform.rotation, m_Texture1);
	}


	tst::Renderer2D::end();

}
void SandBox2DLayer::onEvent(tst::Event& e)
{
	m_OrthoCameraCtrl->onEvent(e);
}
void SandBox2DLayer::onImguiRender()
{
	static bool PreferencesOpen = false;

	TST_PROFILE_FN();

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

			m_modelTransform.rotation = 0.0f;

			m_modelTransform.scale.x = 1.0f;
			m_modelTransform.scale.y = 1.0f;
		}

		ImGui::Text("Position");
		ImGui::Spacing();

		ImGui::SliderFloat("X", &m_modelTransform.position.x, -10.0f, 10.0f);
		ImGui::SliderFloat("Y", &m_modelTransform.position.y, -10.0f, 10.0f);

		ImGui::Text("Rotation");
		ImGui::Spacing();

		ImGui::SliderFloat("X##1", &m_modelTransform.rotation, -360.0f, 360.0f);

		ImGui::Text("Scale");
		ImGui::Spacing();

		ImGui::SliderFloat("X##4", &m_modelTransform.scale.x, -10.0f, 10.0f);
		ImGui::SliderFloat("Y##5", &m_modelTransform.scale.y, -10.0f, 10.0f);
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
			m_quadColour = { 1.0f, 1.0f, 1.0f, 1.0f };
		}
		ImGui::Spacing();

		ImGui::Text("Colour");
		ImGui::Spacing();
		ImGui::ColorPicker3("Colour Picker", &m_quadColour[0]);
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