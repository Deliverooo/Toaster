#include "ToasterEditorLayer.hpp"

#include "util/Random.hpp"

#define TST_EDITOR_REL_PATH "C:/dev/Toaster/ToasterEditor/res/"

namespace tst
{
	ParticleSystem3D::ParticleSystem3D()
	{
		m_Particles.resize(500);
	}

	void ParticleSystem3D::emit(const Particle3DCreateInfo& createInfo)
	{
		Particle3D& particle = m_Particles[m_ParticleIndex];
		particle.Active = true;
		particle.Position = createInfo.Position;


		particle.Rotation = glm::vec3(Random::Float() * 2.0f * glm::pi<float>());

		// Velocity
		particle.Velocity = createInfo.Velocity;
		particle.Velocity.x += createInfo.VelocityVariation.x * (Random::Float() - 0.5f);
		particle.Velocity.y += createInfo.VelocityVariation.y * (Random::Float() - 0.5f);
		particle.Velocity.z += createInfo.VelocityVariation.z * (Random::Float() - 0.5f);

		// Colour
		//particle.ColourBegin = createInfo.ColourBegin;
		particle.ColourBegin.r = createInfo.ColourBegin.r + createInfo.ColourVariation.r * (Random::Float() - 0.5f);
		particle.ColourBegin.g = createInfo.ColourBegin.g + createInfo.ColourVariation.g * (Random::Float() - 0.5f);
		particle.ColourBegin.b = createInfo.ColourBegin.b + createInfo.ColourVariation.b * (Random::Float() - 0.5f);
		particle.ColourBegin.a = createInfo.ColourBegin.a + createInfo.ColourVariation.a * (Random::Float() - 0.5f);

		particle.ColourEnd = createInfo.ColourEnd;

		// Size
		particle.SizeBegin.x = createInfo.SizeBegin.x + createInfo.SizeVariation.x * (Random::Float() - 0.5f);
		particle.SizeBegin.y = createInfo.SizeBegin.y + createInfo.SizeVariation.y * (Random::Float() - 0.5f);
		particle.SizeBegin.z = createInfo.SizeBegin.z + createInfo.SizeVariation.z * (Random::Float() - 0.5f);

		particle.SizeEnd = createInfo.SizeEnd;

		// Life
		particle.Lifetime = createInfo.Lifetime;
		particle.LifeRemaining = createInfo.Lifetime;

		m_ParticleIndex = --m_ParticleIndex % m_Particles.size();
	}

	void ParticleSystem3D::onUpdate(tst::DeltaTime dt)
	{
		for (auto& particle : m_Particles)
		{
			if (!particle.Active)
				continue;

			if (particle.LifeRemaining <= 0.0f)
			{
				particle.Active = false;
				continue;
			}

			particle.LifeRemaining -= dt;
			particle.Position += particle.Velocity * static_cast<float>(dt);
			particle.Rotation += glm::vec3(0.01f * dt);
		}
	}

	void ParticleSystem3D::onRender()
	{
		for (auto& particle : m_Particles)
		{
			if (!particle.Active)
				continue;

			float lifePercent = particle.LifeRemaining / particle.Lifetime;
			glm::vec4 Colour = glm::mix(particle.ColourEnd, particle.ColourBegin, lifePercent);

			// Fades the particle based on its life remaining
			Colour.a *= lifePercent;

			glm::vec3 size = glm::mix(particle.SizeEnd, particle.SizeBegin, lifePercent);
			Renderer3D::drawCube(particle.Position, particle.Rotation, size, Colour);
		}
	}


	glm::vec4 ToasterEditorLayer::screenSpaceToWorldSpace(glm::vec2 screenCoords, float depth)
	{
		float vX = static_cast<float>(m_Framebuffer->getInfo().width) * 0.5f;
		float vY = static_cast<float>(m_Framebuffer->getInfo().height) * 0.5f;

		float mX = screenCoords.x - vX;
		float mY = screenCoords.y - vY;

		glm::vec4 sreenSpaceCoords = glm::vec4(mX / vX, -mY / vY, depth, 1.0f);

		glm::mat4 inverseProjView = glm::inverse(m_PerspectiveCameraCtrl.getCamera()->getProjectionMatrix() * m_PerspectiveCameraCtrl.getCamera()->getViewMatrix());
		glm::vec4 worldSpaceCoords = inverseProjView * sreenSpaceCoords;

		worldSpaceCoords.x /= worldSpaceCoords.w;
		worldSpaceCoords.y /= worldSpaceCoords.w;
		worldSpaceCoords.z /= worldSpaceCoords.w;

		return worldSpaceCoords;
	}

	ToasterEditorLayer::ToasterEditorLayer() : m_OrthoCameraCtrl(1.77f), m_PerspectiveCameraCtrl(90.0f, 1.77f)
	{

	}

	void ToasterEditorLayer::onAttach()
	{

		FramebufferCreateInfo framebufferCreateInfo{ 1280, 720 };
		m_Framebuffer = Framebuffer::create(framebufferCreateInfo);

		//m_RenderOutputFramebuffer = Framebuffer::create(FramebufferCreateInfo{ 300, 600 });

		TextureParams defaultParams{};
		defaultParams.minFilter = TextureFiltering::LinearMipmapLinear;
		defaultParams.magFilter = TextureFiltering::Linear;
		defaultParams.wrapS = TextureWrapping::Repeat;
		defaultParams.wrapT = TextureWrapping::Repeat;
		defaultParams.generateMipmaps = true;

		TextureParams pixelTextureParams{};
		pixelTextureParams.minFilter = TextureFiltering::LinearMipmapNearest;
		pixelTextureParams.magFilter = TextureFiltering::Nearest;
		pixelTextureParams.wrapS = TextureWrapping::Repeat;
		pixelTextureParams.wrapT = TextureWrapping::Repeat;
		pixelTextureParams.generateMipmaps = true;

		RefPtr<Texture2D> dirtSheet = Texture2D::create(TST_EDITOR_REL_PATH"textures/sprite sheets/Tiles_0.png", pixelTextureParams);

		m_OrboTexture = Texture2D::create(TST_EDITOR_REL_PATH"textures/orbo0.png", pixelTextureParams);
		m_GrassTexture = Texture2D::create(TST_EDITOR_REL_PATH"textures/Grass2K.png", defaultParams);
		m_BirdTexture = Texture2D::create(TST_EDITOR_REL_PATH"textures/One_Leg_Bird.png", defaultParams);
		m_RayTraceRoomTexture = Texture2D::create(TST_EDITOR_REL_PATH"textures/RayTraceRoom.png", defaultParams);

		m_Texture0 = SubTexture2D::createPixelPerfect(dirtSheet, 0, 2, 16, 16);

		//m_renderTexture = Texture2D::create(300, 600);

		//unsigned char* data = new unsigned char[300 * 600 * 4];
		//for (int i = 0; i < 300 * 600 * 4; i++)
		//{
		//	data[i] = static_cast<unsigned char>(0xff);
		//}
		//m_renderTexture->setData(data, 300 * 600 * 4);
		//delete[] data;


		m_Scene = std::make_shared<Scene>();

		Entity entity = m_Scene->createEntity();

		glm::mat4 mat{4.0f};
		entity.addComponent<TransformComponent>(mat);
		entity.addComponent<SpriteRendererComponent>();


		TST_TRACE("{0}", entity.hasComponent<TransformComponent>());

		auto& colour = entity.getComponent<SpriteRendererComponent>();
		colour.colour = { 1.0f, 0.9f, 1.0f, 1.0f };
	}

	void ToasterEditorLayer::onUpdate(DeltaTime dt)
	{

		if (m_ViewportFocused)
		{

			m_PerspectiveCameraCtrl.onUpdate(dt);

			if (Input::isMouseButtonPressed(TST_MOUSE_BUTTON_1) && m_particleDrawMode)
			{
				Particle3DCreateInfo particleCreateInfo{};

				auto [mX, mY] = Input::getMousePos();

				particleCreateInfo.Position = screenSpaceToWorldSpace({ mX, mY }, 0.5f);
				particleCreateInfo.Velocity = m_PerspectiveCameraCtrl.getFrontVector();
				particleCreateInfo.VelocityVariation = { 0.7f, 0.0f, 0.7f };
				particleCreateInfo.ColourBegin = { 0.9f, 0.08f, 0.08f, 0.7f };
				particleCreateInfo.ColourEnd = { 0.05f, 0.02f, 0.02f, 0.5f };
				particleCreateInfo.ColourVariation = { 0.9f, 0.05f, 0.05f, 0.0f };
				particleCreateInfo.SizeBegin = { 0.07f, 0.07f, 0.07f };
				particleCreateInfo.SizeEnd = { 0.025f, 0.025f, 0.025f };
				particleCreateInfo.SizeVariation = { 0.001f, 0.001f, 0.001f };
				particleCreateInfo.Lifetime = { 0.8f };

				m_particleSystem.emit(particleCreateInfo);
			}
		}


		static float clock = 0.0f;
		clock += dt;


		m_particleSystem.onUpdate(dt);

		if (Input::isKeyPressed(TST_KEY_F) && m_ViewportFocused)
		{
			Particle3DCreateInfo particleCreateInfo{};
			//particleCreateInfo.Position = {1.0f, 3.0f, -5.0f};
			particleCreateInfo.Position = { Random::Float(-5.0f, 5.0f), 0.0f, Random::Float(-5.0f, 5.0f) };
			particleCreateInfo.Velocity = {0.1f, 1.0f, 0.1f};
			particleCreateInfo.VelocityVariation = { 0.7f, 0.0f, 0.7f };
			particleCreateInfo.ColourBegin = { 0.9f, 0.08f, 0.08f, 0.7f };
			particleCreateInfo.ColourEnd = { 0.2f, 0.02f, 0.02f, 0.2f };
			particleCreateInfo.ColourVariation = { 0.9f, 0.05f, 0.05f, 0.0f };
			particleCreateInfo.SizeBegin = { 0.07f, 0.07f, 0.07f };
			particleCreateInfo.SizeEnd = { 0.025f, 0.025f, 0.025f };
			particleCreateInfo.SizeVariation = { 0.001f, 0.001f, 0.001f };
			particleCreateInfo.Lifetime = { 2.0f };

			m_particleSystem.emit(particleCreateInfo);
		}

		Renderer3D::resetStats();

		m_Framebuffer->bind();
		RenderCommand::setClearColour(m_clearColour);
		RenderCommand::clear();


		Renderer3D::begin(m_PerspectiveCameraCtrl.getCamera());

		Renderer3D::drawCube({ 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 0.0f }, { 10.0f, 10.0f, 10.0f }, m_OrboTexture);

		m_Scene->onUpdate(dt);

		m_particleSystem.onRender();

		Renderer3D::end();

		m_Framebuffer->unbind();
	}
	void ToasterEditorLayer::onEvent(Event& e)
	{
		m_PerspectiveCameraCtrl.onEvent(e);

		EventDispatcher eventDispatcher(e);

		eventDispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& e)
			{
				return onKeyPressedEvent(e);
			});
	}

	bool ToasterEditorLayer::onKeyPressedEvent(KeyPressedEvent& e)
	{
		if (e.getKeycode() == TST_KEY_E)
		{
			Particle3DCreateInfo particleCreateInfo{};
			particleCreateInfo.Position = (m_PerspectiveCameraCtrl.getPosition() + (m_PerspectiveCameraCtrl.getFrontVector() * 0.25f));
			particleCreateInfo.Velocity			 = m_PerspectiveCameraCtrl.getFrontVector();
			particleCreateInfo.VelocityVariation = { 0.2f, 0.2f, 0.2f };
			particleCreateInfo.ColourBegin		 = { 1.0f, 0.05f, 0.05f, 1.0f };
			particleCreateInfo.ColourEnd		 = { 0.2f, 0.0f, 0.0f, 0.2f };
			particleCreateInfo.ColourVariation	 = { 0.8f, 0.025f, 0.025f, 0.0f };
			particleCreateInfo.SizeBegin		 = { 0.25f, 0.25f, 0.25f };
			particleCreateInfo.SizeEnd			 = { 0.05f, 0.05f, 0.05f };
			particleCreateInfo.SizeVariation	 = { 0.07f, 0.07f, 0.07f };
			particleCreateInfo.Lifetime			 = { 1.0f };
			m_particleSystem.emit(particleCreateInfo);
		}

		if (e.getKeycode() == TST_KEY_G && m_ViewportFocused)
		{
			Input::focusMouseCursor();
		}

		return false;
	}

	void ToasterEditorLayer::onImguiRender()
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
				if (ImGui::MenuItem("Quit",		"Ctrl+Q"))			{ Application::getInstance().close(); }
				if (ImGui::MenuItem("Save",		"Ctrl+S"))			{ TST_INFO("Saved!"); }
				if (ImGui::MenuItem("Save as",  "Ctrl+Shift+S"))	{ TST_INFO("Saved as!"); }

				ImGui::Separator();

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Begin("Settings");

		ImGui::Checkbox("Particle Draw Mode", &m_particleDrawMode);

		ImGui::End();

		//ImGui::Begin("Render Output");

		//ImVec2 imageSize = ImGui::GetContentRegionAvail();
		//ImGui::Image(m_RenderOutputFramebuffer->getColourAttachmentId(), imageSize);

		//static glm::vec4 v1{ 1.0f, 0.0f, 0.0f, 0.0f };
		//static glm::vec4 v2{ 0.0f, 1.0f, 0.0f, 0.0f };
		//static glm::vec4 v3{ 0.0f, 0.0f, 1.0f, 0.0f };
		//static glm::vec4 v4{ 0.0f, 0.0f, 0.0f, 1.0f };

		//ImGui::SliderFloat("Matrix column 0##0", &v1.x, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 1##0", &v2.x, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 2##0", &v3.x, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 3##0", &v4.x, 0.0f, 2.0f);

		//ImGui::SliderFloat("Matrix column 0##1", &v1.y, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 1##1", &v2.y, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 2##1", &v3.y, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 3##1", &v4.y, 0.0f, 2.0f);

		//ImGui::SliderFloat("Matrix column 0##2", &v1.z, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 1##2", &v2.z, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 2##2", &v3.z, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 3##2", &v4.z, 0.0f, 2.0f);

		//ImGui::SliderFloat("Matrix column 0##3", &v1.w, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 1##3", &v2.w, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 2##3", &v3.w, 0.0f, 2.0f);
		//ImGui::SliderFloat("Matrix column 3##3", &v4.w, 0.0f, 2.0f);

		//glm::mat4 matrix = glm::mat4(v1, v2, v3, v4);

		//if (ImGui::Button("Render"))
		//{

		//}

		//m_RenderOutputFramebuffer->bind();
		//RenderCommand::setClearColour({ 0.15f, 0.15f, 0.15f, 1.0f });
		//RenderCommand::clear();
		//Renderer3D::begin(m_PerspectiveCameraCtrl.getCamera());

		//Renderer3D::drawCube(matrix, m_BirdTexture);
		//Renderer3D::drawQuad(glm::mat4(1.0f), m_BirdTexture);
		//Renderer3D::end();
		//m_RenderOutputFramebuffer->unbind();

		//ImGui::End();

		ImGui::Begin("3D Renderer Stats");

		ImGui::Text("Draw Calls:			%d", Renderer3D::getStats().drawCallCount);
		ImGui::Text("Quad Count:			%d", Renderer3D::getStats().quadCount);
		ImGui::Text("Batches Per Frame:		%d", Renderer3D::getStats().batchesPerFrame);
		ImGui::Text("Texture Bindings:		%d", Renderer3D::getStats().textureBindings);
		ImGui::Text("Vertices Submitted:	%d", Renderer3D::getStats().verticesSubmitted);
		ImGui::Text("Vertex Count:			%d", Renderer3D::getStats().totalVertexCount());
		ImGui::Text("Index Count:			%d", Renderer3D::getStats().totalIndexCount());
		ImGui::Text("Batch Efficiency:		%f", Renderer3D::getStats().batchEfficiency());

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });

		ImGui::Begin("Viewport");

		m_ViewportFocused = ImGui::IsWindowFocused();
		Application::getInstance().getImguiLayer()->setBlockEvents(!m_ViewportFocused || !m_ViewportHovered);

		m_ViewportHovered = ImGui::IsWindowHovered();

		ImVec2 viewportSize = ImGui::GetContentRegionAvail();

		if (m_ViewportSize.x != viewportSize.x || m_ViewportSize.y != viewportSize.y)
		{
			m_Framebuffer->resize(static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y));
			//m_RenderOutputFramebuffer->resize(static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y));
			m_ViewportSize = { viewportSize.x, viewportSize.y };

			m_PerspectiveCameraCtrl.resize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));

			TST_INFO("Resized Viewport [{0}, {1}]", viewportSize.x, viewportSize.y);
		}

		static bool flatShading = true;
		static bool depthPass = false;

		ImGui::Image(m_Framebuffer->getColourAttachmentId(), { m_ViewportSize.x, m_ViewportSize.y }, ImVec2(0, 1), ImVec2(1, 0));


		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::End();
	}
}