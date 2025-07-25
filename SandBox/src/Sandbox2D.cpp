#include "Sandbox2D.hpp"


#include "Utils/Random.hpp"


SandBox2DLayer::SandBox2DLayer() : m_OrthoCameraCtrl(1.77f)
{
	
}

void SandBox2DLayer::onAttach()
{

	tst::FramebufferCreateInfo framebufferCreateInfo{};
	framebufferCreateInfo.width = static_cast<uint32_t>(m_WindowWidth);
	framebufferCreateInfo.height = static_cast<uint32_t>(m_WindowHeight);
	m_Framebuffer = tst::Framebuffer::create(framebufferCreateInfo);

	tst::TextureParams sheetParams{};
	sheetParams.minFilter = tst::TextureFiltering::NearestMipmapNearest;
	sheetParams.magFilter = tst::TextureFiltering::Nearest;

	tst::RefPtr<tst::Texture2D> dirtSheet = tst::Texture2D::create(TST_REL_PATH"textures/sprite sheets/Tiles_0.png", sheetParams);

	m_Texture0 = tst::Texture2D::create(TST_REL_PATH"textures/orbo0.png");
	m_Texture1 = tst::Texture2D::create(TST_REL_PATH"textures/One_Leg_Bird.png");
	m_Texture2 = tst::Texture2D::create(TST_REL_PATH"textures/rocky_terrain_02_diff_2k.png");

	m_Texture3 = tst::SubTexture2D::createPixelPerfect(dirtSheet, 0, 2, 16, 16);

}
void SandBox2DLayer::onDetach()
{

}

void SandBox2DLayer::onUpdate(tst::DeltaTime dt)
{

	static float particleDissapearThreashhold = 0.5f;
	static float clock = 0.0f;
	clock += dt * 5.0f;

	tst::Renderer2D::resetStats();

	m_Framebuffer->bind();

	m_OrthoCameraCtrl.onUpdate(dt);

	tst::RenderCommand::setClearColour(m_clearColour);
	tst::RenderCommand::clear();

	tst::Renderer2D::begin(m_OrthoCameraCtrl.getCamera());

	tst::Renderer2D::drawQuad({ -1.0f, 0.0f, 0.0f}, 0.0f, { 0.5f, 0.5f }, {1.0f, 0.2f, 0.0f, 0.1f});
	tst::Renderer2D::drawQuad({ 0.6f, 2.5f, 0.0f }, 0.0f, { 1.0f, 1.0f }, m_Texture3, 1.0f);
	tst::Renderer2D::drawQuad({ 0.0f, 2.0f, 0.0f}, clock * std::exp(sin(clock)), {1.0f, 1.0f}, m_Texture0, 1.0f);
	tst::Renderer2D::drawQuad({ 1.0f, 1.0f, 0.0f}, 0.0f, { 0.25f, 1.0f }, m_Texture1, 1.0f);
	tst::Renderer2D::drawQuad({ 0.0f, 0.0f, -0.9f}, 0.0f, { 10.0f, 10.0f }, m_Texture2, 1.0f);

	tst::Renderer2D::end();


	if (m_OrthoCameraCtrl.getVelocity().x > particleDissapearThreashhold || m_OrthoCameraCtrl.getVelocity().x < -particleDissapearThreashhold
			|| m_OrthoCameraCtrl.getVelocity().y > particleDissapearThreashhold || m_OrthoCameraCtrl.getVelocity().y < -particleDissapearThreashhold)
		{
			ParticleCreateInfo particle_create_info{};
			particle_create_info.Position = m_OrthoCameraCtrl.getPosition();
			particle_create_info.Velocity = { Random::Float(), Random::Float() };
			particle_create_info.VelocityVariation = { 0.3f, 0.3f };

			particle_create_info.SizeBegin = { 0.2f, 0.2f };
			particle_create_info.SizeEnd = { 0.025f, 0.025f };
			particle_create_info.SizeVariation = { 0.1f, 0.1f };

			particle_create_info.ColourBegin = { 1.0f, 0.2f, 0.2f, 1.0f };
			particle_create_info.ColourEnd = { 0.2f, 0.0f, 0.0f, 0.0f };
			particle_create_info.ColourVariation = { 0.35f, 0.325f, 0.025f, 0.0f };

			particle_create_info.Lifetime = 0.25f;

			m_ParticleSystem.emit(particle_create_info);
	}

	m_ParticleSystem.onUpdate(dt);

	tst::Renderer2D::begin(m_OrthoCameraCtrl.getCamera());
	m_ParticleSystem.onRender();
	tst::Renderer2D::end();

	m_Framebuffer->unbind();

}
void SandBox2DLayer::onEvent(tst::Event& e)
{
	m_OrthoCameraCtrl.onEvent(e);

	tst::EventDispatcher eventDispatcher(e);

	eventDispatcher.dispatch<tst::KeyPressedEvent>([this](tst::KeyPressedEvent& e)
		{
			return onKeyPressedEvent(e);
		});
	eventDispatcher.dispatch<tst::KeyHeldEvent>([this](tst::KeyHeldEvent& e)
		{
			return onKeyHeldEvent(e);
		});

}

bool SandBox2DLayer::onKeyPressedEvent(tst::KeyPressedEvent& e)
{
	if (e.getKeycode() == TST_KEY_T)
	{
		
	}

	if (e.getKeycode() == TST_KEY_Y)
	{
		tst::Renderer2D::flush();
	}
	return false;
}

bool SandBox2DLayer::onKeyHeldEvent(tst::KeyHeldEvent& e)
{


	return false;
}


void SandBox2DLayer::onImguiRender()
{
	static bool enableDocking = true;

	if (enableDocking)
	{
		static bool p_open = true;
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}


		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &p_open, window_flags);
		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		// Submit the DockSpace
		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}


		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Options"))
			{

				if (ImGui::MenuItem("Quit", "Ctrl+Q")) { tst::Application::getInstance().close(); }


				ImGui::Separator();

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}


		ImGui::Begin("2D Renderer Stats");

		ImGui::Text("Draw Calls:			%d", tst::Renderer2D::getStats().drawCallCount);
		ImGui::Text("Quad Count:			%d", tst::Renderer2D::getStats().quadCount);
		ImGui::Text("Batches Per Frame:		%d", tst::Renderer2D::getStats().batchesPerFrame);
		ImGui::Text("Texture Bindings:		%d", tst::Renderer2D::getStats().textureBindings);
		ImGui::Text("Vertices Submitted:	%d", tst::Renderer2D::getStats().verticesSubmitted);
		ImGui::Text("Vertex Count:			%d", tst::Renderer2D::getStats().totalVertexCount());
		ImGui::Text("Index Count:			%d", tst::Renderer2D::getStats().totalIndexCount());
		ImGui::Text("Batch Efficiency:		%f", tst::Renderer2D::getStats().batchEfficiency());

		ImGui::End();

		ImGui::Begin("Viewport");
		ImGui::Image(m_Framebuffer->getColourAttachmentId(), ImVec2(m_WindowWidth, m_WindowHeight), ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();


		ImGui::End();
	} else
	{
		ImGui::Begin("2D Renderer Stats");

		ImGui::Text("Draw Calls:			%d", tst::Renderer2D::getStats().drawCallCount);
		ImGui::Text("Quad Count:			%d", tst::Renderer2D::getStats().quadCount);
		ImGui::Text("Batches Per Frame:		%d", tst::Renderer2D::getStats().batchesPerFrame);
		ImGui::Text("Texture Bindings:		%d", tst::Renderer2D::getStats().textureBindings);
		ImGui::Text("Vertices Submitted:	%d", tst::Renderer2D::getStats().verticesSubmitted);
		ImGui::Text("Vertex Count:			%d", tst::Renderer2D::getStats().totalVertexCount());
		ImGui::Text("Index Count:			%d", tst::Renderer2D::getStats().totalIndexCount());
		ImGui::Text("Batch Efficiency:		%f", tst::Renderer2D::getStats().batchEfficiency());


		ImGui::Image(m_Texture0->getId(), ImVec2(static_cast<float>(m_Texture0->getWidth()), static_cast<float>(m_Texture0->getHeight())));

		ImGui::End();
		ImGui::End();
	}
    
}




