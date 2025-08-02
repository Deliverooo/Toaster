#include "Sandbox3D.hpp"

#include "Utils/Random.hpp"

void SandBox3DLayer::onAttach()
{
	TST_PROFILE_FN();

	m_PerspectiveCameraCtrl = std::make_shared<tst::PerspectiveCameraController>(90.0f, 1.77f);

	m_Texture0 = tst::Texture2D::create("assets/textures/orbo0.png");


}
void SandBox3DLayer::onDetach()
{

}
void SandBox3DLayer::onUpdate(tst::DeltaTime dt)
{

	//tst::Renderer3D::resetStats();
	//m_PerspectiveCameraCtrl->onUpdate(dt);

	//tst::RenderCommand::setClearColour(m_clearColour);
	//tst::RenderCommand::clear();

	//tst::Renderer3D::begin(m_PerspectiveCameraCtrl->getCamera());

	//tst::Renderer3D::drawCube({ 0.0f, 0.0f, 0.0f }, glm::vec3(1.0f), glm::vec3(1.0f), {0.5f, 1.0f, 0.6f, 1.0f});
	//tst::Renderer3D::drawCube({ 1.0f, 1.0f, 1.5f }, glm::vec3(0.0f), glm::vec3(0.8f), m_Texture0);

	//tst::Renderer3D::end();

}
void SandBox3DLayer::onEvent(tst::Event& e)
{
	m_PerspectiveCameraCtrl->onEvent(e);

	tst::EventDispatcher eventDispatcher(e);

	eventDispatcher.dispatch<tst::KeyPressedEvent>([this](tst::KeyPressedEvent& e)
		{
			return onKeyPressedEvent(e);
		});
}

bool SandBox3DLayer::onKeyPressedEvent(tst::KeyPressedEvent& e)
{


	return false;
}

bool SandBox3DLayer::onKeyHeldEvent(tst::KeyHeldEvent& e)
{

	return false;
}


void SandBox3DLayer::onImguiRender()
{
	ImGui::Begin("Sandbox 3D");

	ImGui::Text("Yes, I am!");

	ImGui::End();
}