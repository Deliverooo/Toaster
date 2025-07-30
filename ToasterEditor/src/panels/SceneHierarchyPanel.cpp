#include "SceneHierarchyPanel.hpp"

#include "Toaster/Scene/Components.hpp"

#include <imgui.h>

#include "imgui_internal.h"
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

	static void drawVec3Ctrl(const char* label, glm::vec3* vec, const glm::vec3& reset = glm::vec3(1.0f), float columnWidth = 100.0f)
	{

		ImGui::PushID(label);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label);
		ImGui::NextColumn();


		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.5f);

		float lineHeight = ImGui::GetFrameHeightWithSpacing();
		ImVec2 buttonSize = ImVec2{ lineHeight, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, { 1.0f, 0.0f, 0.0f, 1.0f });
		if (ImGui::Button("x", buttonSize))	{ vec->x = reset.x;	}
		ImGui::SameLine();
		ImGui::DragFloat("##x", &vec->x, 0.1f);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 1.0f, 0.0f, 1.0f });
		if (ImGui::Button("y", buttonSize)) { vec->y = reset.y; }
		ImGui::SameLine();
		ImGui::DragFloat("##y", &vec->y, 0.1f);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::PopStyleColor();

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.0f, 0.0f, 1.0f, 1.0f });
		if (ImGui::Button("z", buttonSize)) { vec->z = reset.z;	}
		ImGui::SameLine();
		ImGui::DragFloat("##z", &vec->z, 0.1f);
		ImGui::PopItemWidth();
		ImGui::PopStyleColor();

		ImGui::PopStyleVar(2);
		ImGui::Columns(1);

		ImGui::PopID();
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


		drawComponent<TransformComponent>(&entity, "Transform", [](TransformComponent* comp)
		{
			ImGui::Spacing();
			drawVec3Ctrl("Position", &comp->translation, {0.0f, 0.0f, 0.0f});
			ImGui::Spacing();
			drawVec3Ctrl("Rotation", &comp->rotation, {0.0f, 0.0f, 0.0f});
			ImGui::Spacing();
			drawVec3Ctrl("Scale", &comp->scale, {1.0f, 1.0f, 1.0f});
			ImGui::Spacing();
				
		});

		drawComponent<CameraComponent>(&entity, "Camera", [](CameraComponent* comp)
			{
				auto& camera = comp->camera;

				ImGui::Checkbox("Main Camera", &comp->main);

				const char* projectionTypes[] = { "Perspective", "Orthographic" };
				const char* currentProjectionType = projectionTypes[(int)camera.getProjectionType()];

				if (ImGui::BeginCombo("Projection", currentProjectionType))
				{
					for (int i = 0; i < 2; i++)
					{
						bool selected = (currentProjectionType == projectionTypes[i]);

						if (ImGui::Selectable(projectionTypes[i], selected))
						{
							currentProjectionType = projectionTypes[i];
							comp->camera.setProjectionType(static_cast<SceneCamera::ProjectionType>(i));
						}

						if (selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				if (comp->camera.getProjectionType() == SceneCamera::ProjectionType::Perspective)
				{
					float fov = camera.getPerspectiveFov();
					float nearClip = camera.getPerspectiveNear();
					float farClip = camera.getPerspectiveFar();

					if (ImGui::DragFloat("Fov", &fov, 0.1f, 1.0f, 180.0f)) { camera.setPerspectiveFov(fov); }
					if (ImGui::DragFloat("Near Clip", &nearClip, 0.01f, 0.01f, 100.0f)) { camera.setPerspectiveNear(nearClip); }
					if (ImGui::DragFloat("Far Clip", &farClip, 0.01f, 0.01f, 1000.0f)) { camera.setPerspectiveFar(farClip); }

				}
				else if (camera.getProjectionType() == SceneCamera::ProjectionType::Orthographic)
				{
					float orthoSize = camera.getOrthoSize();
					float nearClip = camera.getOrthoNear();
					float farClip = camera.getOrthoFar();

					if (ImGui::DragFloat("Ortho Size", &orthoSize, 0.1f, 0.1f, 10.0f))	  { camera.setOrthoSize(orthoSize); }
					if (ImGui::DragFloat("Near Clip", &nearClip, 0.01f, -100.0f, 100.0f)) { camera.setOrthoNear(nearClip); }
					if (ImGui::DragFloat("Far Clip", &farClip, 0.01f, -100.0f, 100.0f))	  { camera.setOrthoFar(farClip); }
				}
			});


		drawComponent<SpriteRendererComponent>(&entity, "Sprite Renderer", [](SpriteRendererComponent *comp)
			{
				ImGui::ColorEdit4("Base Colour", glm::value_ptr(comp->colour));
			});
	}

	template<typename T>
	void SceneHierarchyPanel::drawComponent(Entity *entity, const char* displayName, void(*uiFunc)(T*))
	{
		if (entity->hasComponent<T>())
		{
			if (ImGui::TreeNodeEx((void*)typeid(T).hash_code(), ImGuiTreeNodeFlags_DefaultOpen, displayName))
			{
				auto& comp = entity->getComponent<T>();

				uiFunc(&comp);

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
