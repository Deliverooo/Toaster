#include "ToasterEditorLayer.hpp"

#include "imgui_internal.h"
#include "Toaster/Renderer/MeshRenderer.hpp"
#include "util/Random.hpp"
#include "Toaster/Scene/Scene.hpp"
#include "Toaster/Scene/SceneSerializer.hpp"

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


		particle.Rotation = glm::vec3(Random::Gen<float>() * 2.0f * glm::pi<float>());

		// Velocity
		particle.Velocity = createInfo.Velocity;
		particle.Velocity.x += createInfo.VelocityVariation.x * (Random::Gen<float>() - 0.5f);
		particle.Velocity.y += createInfo.VelocityVariation.y * (Random::Gen<float>() - 0.5f);
		particle.Velocity.z += createInfo.VelocityVariation.z * (Random::Gen<float>() - 0.5f);

		// Colour
		//particle.ColourBegin = createInfo.ColourBegin;
		particle.ColourBegin.r = createInfo.ColourBegin.r + createInfo.ColourVariation.r * (Random::Gen<float>() - 0.5f);
		particle.ColourBegin.g = createInfo.ColourBegin.g + createInfo.ColourVariation.g * (Random::Gen<float>() - 0.5f);
		particle.ColourBegin.b = createInfo.ColourBegin.b + createInfo.ColourVariation.b * (Random::Gen<float>() - 0.5f);
		particle.ColourBegin.a = createInfo.ColourBegin.a + createInfo.ColourVariation.a * (Random::Gen<float>() - 0.5f);

		particle.ColourEnd = createInfo.ColourEnd;

		// Size
		particle.SizeBegin.x = createInfo.SizeBegin.x + createInfo.SizeVariation.x * (Random::Gen<float>() - 0.5f);
		particle.SizeBegin.y = createInfo.SizeBegin.y + createInfo.SizeVariation.y * (Random::Gen<float>() - 0.5f);
		particle.SizeBegin.z = createInfo.SizeBegin.z + createInfo.SizeVariation.z * (Random::Gen<float>() - 0.5f);

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

			particle.Velocity.y -= dt * 5.0f;

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
			//Renderer3D::drawCube(particle.Position, particle.Rotation, size, Colour);
		}
	}


	glm::vec4 ToasterEditorLayer::screenSpaceToWorldSpace(const glm::mat4 &projection, const glm::mat4 &view, glm::vec2 screenCoords, float depth)
	{
		float vX = static_cast<float>(m_Framebuffer->getInfo().width) * 0.5f;
		float vY = static_cast<float>(m_Framebuffer->getInfo().height) * 0.5f;

		float mX = screenCoords.x - vX;
		float mY = screenCoords.y - vY;

		glm::vec4 sreenSpaceCoords = glm::vec4(mX / vX, -mY / vY, depth, 1.0f);

		glm::mat4 inverseProjView = glm::inverse(projection * view);
		glm::vec4 worldSpaceCoords = inverseProjView * sreenSpaceCoords;

		worldSpaceCoords.x /= worldSpaceCoords.w;
		worldSpaceCoords.y /= worldSpaceCoords.w;
		worldSpaceCoords.z /= worldSpaceCoords.w;

		return worldSpaceCoords;
	}

	ToasterEditorLayer::ToasterEditorLayer()
	{

	}

	void ToasterEditorLayer::loadAssets()
	{
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


		m_OrboTexture		  = Texture2D::create(TST_EDITOR_RESOURCE_DIR"/textures/orbo0.png",		  pixelTextureParams);
		m_GrassTexture		  = Texture2D::create(TST_EDITOR_RESOURCE_DIR"/textures/Grass2K.png",	  defaultParams);
		m_BirdTexture		  = Texture2D::create(TST_EDITOR_RESOURCE_DIR"/textures/One_Leg_Bird.png", defaultParams);
		m_RayTraceRoomTexture = Texture2D::create(TST_EDITOR_RESOURCE_DIR"/textures/RayTraceRoom.png", defaultParams);
	}

	void ToasterEditorLayer::onAttach()
	{
		FramebufferCreateInfo framebufferCreateInfo{ 1280, 720 };
		m_Framebuffer = Framebuffer::create(framebufferCreateInfo);

		loadAssets();

		m_Scene = make_reference<Scene>();



		m_CameraEntity = m_Scene->createEntity("camera");
		m_CameraEntity.addComponent<CameraComponent>();


		class CameraController : public ScriptableEntity
		{
		public:

			void onCreate()
			{
				
			}
			void onDestroy()
			{
				
			}
			void onUpdate(DeltaTime dt)
			{
				static float clock = 0.0f;
				clock += dt;

				static float cameraSpeed = 5.0f;

				if (m_entity.getComponent<CameraComponent>().active)
				{
					auto& trans = m_entity.getComponent<TransformComponent>().translation;

					glm::vec3 cameraDirection{ 0.0f };

					if (Input::isKeyPressed(TST_KEY_W)) { cameraDirection -= glm::vec3(0.0f, 0.0f, 1.0f) * dt.getTime_s() * cameraSpeed; }
					if (Input::isKeyPressed(TST_KEY_S)) { cameraDirection += glm::vec3(0.0f, 0.0f, 1.0f) * dt.getTime_s() * cameraSpeed; }
					if (Input::isKeyPressed(TST_KEY_A)) { cameraDirection -= glm::vec3(1.0f, 0.0f, 0.0f) * dt.getTime_s() * cameraSpeed; }
					if (Input::isKeyPressed(TST_KEY_D)) { cameraDirection += glm::vec3(1.0f, 0.0f, 0.0f) * dt.getTime_s() * cameraSpeed; }

					if (Input::isKeyPressed(TST_KEY_SPACE)) { cameraDirection += glm::vec3(0.0f, 1.0f, 0.0f) * dt.getTime_s() * cameraSpeed; }
					if (Input::isKeyPressed(TST_KEY_LEFT_SHIFT)) { cameraDirection -= glm::vec3(0.0f, 1.0f, 0.0f) * dt.getTime_s() * cameraSpeed; }

					trans += cameraDirection;
				}

			} 
			
		};

		m_CameraEntity.addComponent<NativeScriptComponent>().bind<CameraController>();

		m_SceneHierarchyPanel.setSceneContext(m_Scene);

		SceneSerializer serializer(m_Scene);
		serializer.serialize("res/scenes/Test.toast");
	}

	void ToasterEditorLayer::onUpdate(DeltaTime dt)
	{
		// Ensure minimum viewport size to prevent aspect ratio issues
		float safeViewportX = std::max(m_ViewportSize.x, 1.0f);
		float safeViewportY = std::max(m_ViewportSize.y, 1.0f);

		if (safeViewportX > 0.0f && safeViewportY > 0.0f &&
			(safeViewportX != m_Framebuffer->getInfo().width || safeViewportY != m_Framebuffer->getInfo().height))
		{
			m_Framebuffer->resize(static_cast<uint32_t>(safeViewportX), static_cast<uint32_t>(safeViewportY));

			m_Scene->onViewportResize(static_cast<uint32_t>(safeViewportX), static_cast<uint32_t>(safeViewportY));
		}

		static float clock = 0.0f;
		clock += dt;

		MeshRenderer::resetStats();
		m_Framebuffer->bind();
		RenderCommand::setClearColour(m_clearColour);
		RenderCommand::clear();

		m_Scene->onUpdate(dt);


	    m_Framebuffer->unbind();
	}
	void ToasterEditorLayer::onEvent(Event& e)
	{

		EventDispatcher eventDispatcher(e);

		eventDispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& e)
			{
				return onKeyPressedEvent(e);
			});
	}

	bool ToasterEditorLayer::onKeyPressedEvent(KeyPressedEvent& e)
	{


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
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowMinSize.x = 300.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		style.WindowMinSize.x = 30.0f;

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Options"))
			{
				if (ImGui::MenuItem("Quit", "Ctrl+Q")) { Application::getInstance().close(); }
				if (ImGui::MenuItem("Save", "Ctrl+S")) { TST_INFO("Saved!"); }
				if (ImGui::MenuItem("Save as", "Ctrl+Shift+S")) { TST_INFO("Saved as!"); }

				ImGui::Separator();

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		m_SceneHierarchyPanel.onImGuiRender();

		ImGui::Begin("Settings");




		ImGui::Text("3D Renderer Stats");
		ImGui::Spacing();

		ImGui::PushMultiItemsWidths(8, ImGui::GetContentRegionAvail().x - ImGui::CalcItemWidth());

		ImGui::Text("Draw Calls:");
		ImGui::SameLine();
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%d", MeshRenderer::getStats().drawCallCount);
		ImGui::PopItemWidth();

		ImGui::Text("Quad Count:");
		ImGui::SameLine();
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%d", MeshRenderer::getStats().quadCount);
		ImGui::PopItemWidth();

		ImGui::Text("Texture Bindings:");
		ImGui::SameLine();
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%d", MeshRenderer::getStats().textureBindings);
		ImGui::PopItemWidth();

		ImGui::Text("Vertices Submitted");
		ImGui::SameLine();
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%d", MeshRenderer::getStats().verticesSubmitted);
		ImGui::PopItemWidth();

		ImGui::Text("Vertex Count:");
		ImGui::SameLine();
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%d", MeshRenderer::getStats().totalVertexCount());
		ImGui::PopItemWidth();

		ImGui::Text("Index Count:");
		ImGui::SameLine();
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%d", MeshRenderer::getStats().totalIndexCount());
		ImGui::PopItemWidth();

		ImGui::Text("Batch Efficiency:");
		ImGui::SameLine();
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%f", MeshRenderer::getStats().batchEfficiency());
		ImGui::PopItemWidth();

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });

		ImGui::Begin("Viewport");

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();
		bool shouldBlockEvents = (!m_ViewportFocused || !m_ViewportHovered);
		Application::getInstance().getImguiLayer()->setBlockEvents(shouldBlockEvents);


		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportSize.x, viewportSize.y };
		m_ViewportSize.x = std::max(m_ViewportSize.x, 1.0f);
		m_ViewportSize.y = std::max(m_ViewportSize.y, 1.0f);

		static bool flatShading = true;
		static bool depthPass = false;

		ImGui::Image(m_Framebuffer->getColourAttachmentId(), { m_ViewportSize.x, m_ViewportSize.y }, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();

		/*ImGui::Begin("Depth Pass");
		float framebufferWidth = static_cast<float>(m_Framebuffer->getInfo().width);
		float framebufferHeight = static_cast<float>(m_Framebuffer->getInfo().height);
		ImGui::Image(m_Framebuffer->getDepthAttachmentId(), { framebufferWidth, framebufferHeight }, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();*/

		ImGui::PopStyleVar();

		ImGui::End();
	}
}