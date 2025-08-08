#include "ToasterEditorLayer.hpp"

#include "imgui_internal.h"
#include "Imguizmo.h"
#include "Toaster/Renderer/MeshRenderer.hpp"
#include "Toaster/Renderer/SkyBoxRenderer.hpp"
#include "util/Random.hpp"
#include "Toaster/Scene/Scene.hpp"
#include "Toaster/Scene/SceneSerializer.hpp"
#include "Toaster/Util/PlatformUtils.hpp"


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
		FramebufferCreateInfo framebufferCreateInfo;
		framebufferCreateInfo.attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::D32F_S8 };

		framebufferCreateInfo.width = 1280;
		framebufferCreateInfo.height = 720;
		m_Framebuffer = Framebuffer::create(framebufferCreateInfo);

		loadAssets();

		m_Scene = make_reference<Scene>();

		m_SceneHierarchyPanel.setSceneContext(m_Scene);

		m_EditorCamera = EditorCamera(45.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
	}

	void ToasterEditorLayer::onUpdate(DeltaTime dt)
	{
		static float clock = 0.0f;
		clock += dt;

		// Ensure minimum viewport size to prevent aspect ratio issues
		float safeViewportX = std::max(m_ViewportSize.x, 1.0f);
		float safeViewportY = std::max(m_ViewportSize.y, 1.0f);

		if (safeViewportX > 0.0f && safeViewportY > 0.0f &&
			(safeViewportX != m_Framebuffer->getInfo().width || safeViewportY != m_Framebuffer->getInfo().height))
		{
			m_Framebuffer->resize(static_cast<uint32_t>(safeViewportX), static_cast<uint32_t>(safeViewportY));
			m_EditorCamera.setViewportSize(static_cast<uint32_t>(safeViewportX), static_cast<uint32_t>(safeViewportY));
			m_Scene->onViewportResize(static_cast<uint32_t>(safeViewportX), static_cast<uint32_t>(safeViewportY));
		}


		if (m_ViewportFocused)
		{
		}
		m_EditorCamera.onUpdate(dt);


		MeshRenderer::resetStats();
		m_Framebuffer->bind();
		RenderCommand::setClearColour(m_clearColour);
		RenderCommand::clear();

		m_Framebuffer->clearAttachment(1, -1);

		m_Scene->onEditorUpdate(m_EditorCamera, dt);

		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		glm::vec2 viewportSize = m_ViewportBounds[1] - m_ViewportBounds[0];
		my = viewportSize.y - my;
		int mouseX = (int)mx;
		int mouseY = (int)my;

		if (mouseX >= 0 && mouseY >= 0 && mouseX < static_cast<int>(viewportSize.x) && mouseY < static_cast<int>(viewportSize.y))
		{
			int pixelData = m_Framebuffer->readPixel(1, mouseX, mouseY);
		}


		// Temporary fix
		SkyBoxRenderer::render(m_EditorCamera);

	    m_Framebuffer->unbind();
	}
	void ToasterEditorLayer::onEvent(Event& e)
	{

		m_EditorCamera.onEvent(e);

		EventDispatcher eventDispatcher(e);
		eventDispatcher.dispatch<KeyPressedEvent>([this](KeyPressedEvent& e)
			{
				return onKeyPressedEvent(e);
			});
		eventDispatcher.dispatch<MouseButtonPressEvent>([this](MouseButtonPressEvent& e)
			{
				return onMouseButtonPressed(e);
			});
	}

	bool ToasterEditorLayer::onKeyPressedEvent(KeyPressedEvent& e)
	{

		bool controlPressed = Input::isKeyPressed(Key::LeftControl) || Input::isKeyPressed(Key::RightControl);
		bool shiftPressed = Input::isKeyPressed(Key::LeftShift) || Input::isKeyPressed(Key::RightShift);
		bool altPressed = Input::isKeyPressed(Key::LeftAlt) || Input::isKeyPressed(Key::RightAlt);
		switch (e.getKeycode())
		{
			case Key::S:
			{
				if (controlPressed)
				{
					saveScene();
				}

				break;
			}
			case Key::O:
			{
				if (controlPressed)
				{
					openScene();
				}

				break;
			}
			case Key::N:
			{
				if (controlPressed)
				{
					newScene();
				}

				break;
			}

			case Key::T:
			{
				if (m_GizmoType != ImGuizmo::OPERATION::TRANSLATE)
				{
					m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
				}
				break;
			}
			case Key::R:
			{
				if (m_GizmoType != ImGuizmo::OPERATION::ROTATE)
				{
					m_GizmoType = ImGuizmo::OPERATION::ROTATE;
				}
				break;
			}
			case Key::E:
			{
				if (m_GizmoType != ImGuizmo::OPERATION::SCALE)
				{
					m_GizmoType = ImGuizmo::OPERATION::SCALE;
				}
				break;
			}


		case Key::L:

			if (altPressed)
			{
				if (m_IsLocalTransform)  { m_IsLocalTransform = false; break; }
				if (!m_IsLocalTransform) { m_IsLocalTransform = true;  break; }
			}
		}

		return false;
	}

	bool ToasterEditorLayer::onMouseButtonPressed(MouseButtonPressEvent& e)
	{
		if (e.getMouseButton() == TST_MOUSE_BUTTON_RIGHT)
		{
			if (m_GizmoType != -1 && m_SceneHierarchyPanel.getSelectedEntity())
			{
				m_GizmoType = -1;
			}
		}
		return false;
	}

	void ToasterEditorLayer::saveSceneAs()
	{
		std::string filepath = FileDialog::saveFile("Toaster Scene (*.toast)\0*.toast\0");
		if (!filepath.empty())
		{
			SceneSerializer serializer(m_Scene);
			serializer.serialize(filepath);
		}
	}

	void ToasterEditorLayer::newScene()
	{
		m_Scene = make_reference<Scene>();
		m_Scene->onViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
		m_SceneHierarchyPanel.setSceneContext(m_Scene);
	}

	void ToasterEditorLayer::openScene()
	{
		std::string filepath = FileDialog::openFile("Toaster Scene (*.toast)\0*.toast\0");
		if (!filepath.empty())
		{
			m_Scene = make_reference<Scene>();
			m_Scene->onViewportResize(static_cast<uint32_t>(m_ViewportSize.x), static_cast<uint32_t>(m_ViewportSize.y));
			m_SceneHierarchyPanel.setSceneContext(m_Scene);

			SceneSerializer serializer(m_Scene);
			serializer.deserialize(filepath);
		}
	}

	void ToasterEditorLayer::saveScene()
	{
		std::string filepath = FileDialog::saveFile("Toaster Scene (*.toast)\0*.toast\0");
		if (!filepath.empty())
		{
			SceneSerializer serializer(m_Scene);
			serializer.serialize(filepath);
		}
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
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl+N"))
				{
					newScene();
				}
				if (ImGui::MenuItem("Open", "Ctrl+O"))
				{
					openScene();
				}
				if (ImGui::MenuItem("Save", "Ctrl+S"))
				{
					saveScene();
				}
				if (ImGui::MenuItem("Save as", "Ctrl+Shift+S"))
				{
					saveSceneAs();
				}
				if (ImGui::MenuItem("Quit", "Ctrl+Q")) { Application::getInstance().close(); }

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
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%d", MeshRenderer::getStats().triangleCount);
		ImGui::PopItemWidth();

		ImGui::Text("Texture Bindings:");
		ImGui::SameLine();
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%d", MeshRenderer::getStats().textureBindings);
		ImGui::PopItemWidth();

		ImGui::Text("Vertices Submitted");
		ImGui::SameLine();
		ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "%d", MeshRenderer::getStats().verticesSubmitted);
		ImGui::PopItemWidth();

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });

		ImGui::Begin("Viewport");

		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();

		m_ViewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewportHovered = ImGui::IsWindowHovered();

		bool shouldBlockEvents = (!m_ViewportFocused || !m_ViewportHovered);
		Application::getInstance().getImguiLayer()->setBlockEvents(shouldBlockEvents);

		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportSize.x, viewportSize.y };




		ImGui::Image(m_Framebuffer->getColourAttachmentId(), { m_ViewportSize.x, m_ViewportSize.y }, ImVec2(0, 1), ImVec2(1, 0)); 


		Entity selectedEntity = m_SceneHierarchyPanel.getSelectedEntity();
		if (selectedEntity && m_GizmoType != -1)
		{
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();

			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);

			auto &transformComp = selectedEntity.getComponent<TransformComponent>();
			glm::mat4 transformMatrix = transformComp.matrix();

			ImGuizmo::Manipulate(glm::value_ptr(m_EditorCamera.getViewMatrix()), glm::value_ptr(m_EditorCamera.getProjectionMatrix()), static_cast<ImGuizmo::OPERATION>(m_GizmoType), ((m_IsLocalTransform) ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD), glm::value_ptr(transformMatrix));
			if (ImGuizmo::IsUsing())
			{
				glm::vec3 translation, rotation, scale;
				decomposeTransformationMat(transformMatrix, translation, rotation, scale);

				transformComp.translation = translation;

				glm::vec3 deltaRotation = rotation - transformComp.rotation;
				transformComp.rotation += deltaRotation;

				transformComp.scale = scale;
			}
		}

		ImGui::End();

		ImGui::PopStyleVar();

		ImGui::End();
	}
}
