#include "SceneHierarchyPanel.hpp"

#include "Toaster/Scene/Components.hpp"

#include <imgui.h>

#include "glm/gtc/type_ptr.hpp"

namespace tst
{

	SceneHierarchyPanel::SceneHierarchyPanel(const RefPtr<Scene>& scene)
	{
		m_sceneContext = scene;
	}

	void SceneHierarchyPanel::setSceneContext(const RefPtr<Scene>& sceneContext)
	{
		m_sceneContext = sceneContext;
	}

	void SceneHierarchyPanel::drawEntityNode(Entity entity)
	{
		auto& tag = entity.getComponent<TagComponent>();

		ImGuiTreeNodeFlags flags = ((entity == m_selectedEntity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		bool opened = ImGui::TreeNodeEx((void*)(uint32_t)entity, flags, "%s", tag.name.c_str());

		if (ImGui::IsItemClicked())
		{
			m_selectedEntity = entity;
		}

		if (opened)
		{
			ImGui::TreePop();
		}
	}


	void SceneHierarchyPanel::drawComponents(Entity entity)
	{

		if (entity.hasComponent<TagComponent>())
		{
			auto& tagComp = entity.getComponent<TagComponent>();

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, sizeof(buffer), tagComp.name.c_str());
			if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
			{
				tagComp.name = std::string(buffer);
			}
		}
		if (entity.hasComponent<TransformComponent>())
		{
			if (ImGui::TreeNodeEx((void*)typeid(TransformComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Transform"))
			{
				auto& transformComp = entity.getComponent<TransformComponent>();

				ImGui::DragFloat3("Position", glm::value_ptr(transformComp.transform[3]), 0.25f);

				ImGui::TreePop();
			}
		}
		if (entity.hasComponent<CameraComponent>())
		{
			if (ImGui::TreeNodeEx((void*)typeid(CameraComponent).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, "Camera"))
			{
				auto& cameraComp = entity.getComponent<CameraComponent>();

				ImGui::Checkbox("Main Camera", &cameraComp.main);

				const char* projectionTypes[] = { "Perspective", "Orthographic" };
				const char* currentProjectionType = projectionTypes[(int)cameraComp.camera.getProjectionType()];

				if (ImGui::BeginCombo("Projection", currentProjectionType))
				{
					for (int i = 0; i < 2; i++)
					{
						bool selected = (currentProjectionType == projectionTypes[i]);

						if (ImGui::Selectable(projectionTypes[i], selected))
						{
							currentProjectionType = projectionTypes[i];
							cameraComp.camera.setProjectionType(static_cast<SceneCamera::ProjectionType>(i));
						}

						if (selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				if (cameraComp.camera.getProjectionType() == SceneCamera::ProjectionType::Perspective)
				{
					float fov		= cameraComp.camera.getPerspectiveFov();
					float nearClip  = cameraComp.camera.getPerspectiveNear();
					float farClip	= cameraComp.camera.getPerspectiveFar();

					if (ImGui::DragFloat("Fov", &fov, 0.1f, 1.0f, 180.0f))				{ cameraComp.camera.setPerspectiveFov(fov); }
					if (ImGui::DragFloat("Near Clip", &nearClip, 0.01f, 0.01f, 100.0f))	{ cameraComp.camera.setPerspectiveNear(nearClip); }
					if (ImGui::DragFloat("Far Clip", &farClip, 0.01f, 0.01f, 1000.0f))  { cameraComp.camera.setPerspectiveFar(farClip); }

				} else if (cameraComp.camera.getProjectionType() == SceneCamera::ProjectionType::Orthographic)
				{
					float orthoSize = cameraComp.camera.getOrthoSize();
					float nearClip	= cameraComp.camera.getOrthoNear();
					float farClip	= cameraComp.camera.getOrthoFar();

					if (ImGui::DragFloat("Ortho Size", &orthoSize, 0.1f, 0.1f, 10.0f))	  { cameraComp.camera.setOrthoSize(orthoSize);	}
					if (ImGui::DragFloat("Near Clip", &nearClip, 0.01f, -100.0f, 100.0f)) { cameraComp.camera.setOrthoNear(nearClip); }
					if (ImGui::DragFloat("Far Clip", &farClip, 0.01f, -100.0f, 100.0f))	  { cameraComp.camera.setOrthoFar(farClip); }
				}

				ImGui::TreePop();
			}
		}
	}



	void SceneHierarchyPanel::onImGuiRender()
	{
		ImGui::Begin("Scene Hierarchy");

		m_sceneContext->m_registry.view<entt::entity>().each([this](auto entityId)
		{
			Entity entity = { entityId, m_sceneContext.get()};
			drawEntityNode(entity);
		});

		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
		{
			m_selectedEntity = {};
		}

		ImGui::End();

		ImGui::Begin("Inspector");
		if (m_selectedEntity)
		{
			drawComponents(m_selectedEntity);
		}
		ImGui::End();
	}

}
