#include "SceneHierarchyPanel.hpp"

#include "Toaster/Scene/Components.hpp"
#include <filesystem>


#include "imgui_internal.h"
#include "glm/gtc/type_ptr.hpp"

#include "Toaster/Renderer/Mesh.hpp"


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

		bool deleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
			{
				deleted = true;
			}



			ImGui::EndPopup();
		}

		if (opened)
		{
			ImGui::TreePop();
		}

		if (deleted)
		{
			m_sceneContext->removeEntity(entity);

			if (m_selectedEntity == entity)
			{
				m_selectedEntity = {};
			}
		}
	}

	static void drawVec3Ctrl(const char* label, glm::vec3* vec, const glm::vec3& reset = glm::vec3(1.0f), float columnWidth = 100.0f, const char* v1 = "X", const char* v2 = "Y", const char* v3 = "Z")
	{
		ImGuiIO &io = ImGui::GetIO();
		auto boldFnt = io.Fonts->Fonts[2];

		static std::string v1StrId = std::string("##") + v1;
		static std::string v2StrId = std::string("##") + v2;
		static std::string v3StrId = std::string("##") + v3;

		ImGui::PushID(label);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label);
		ImGui::NextColumn();


		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{2, 0});
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.5f);

		static ImVec4 textColour = { 1.0f, 1.0f, 1.0f, 1.0f };

		float lineHeight = ImGui::GetFrameHeightWithSpacing();
		ImVec2 buttonSize = ImVec2{ lineHeight, lineHeight };


		ImGui::PushStyleColor(ImGuiCol_Text, textColour);
		ImGui::PushStyleColor(ImGuiCol_Button,		  { 0.7f, 0.1f, 0.1f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  { 0.7f, 0.1f, 0.1f, 1.0f });

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.5f);
		ImGui::PushFont(boldFnt);
		if (ImGui::Button(v1, buttonSize))	{ vec->x = reset.x;	}
		ImGui::PopFont();
		ImGui::PopStyleVar();
		ImGui::SameLine();
		ImGui::DragFloat(v1StrId.c_str(), &vec->x, 0.1f);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::PopStyleColor(4);

		

		ImGui::PushStyleColor(ImGuiCol_Text, textColour);
		ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f, 0.7f, 0.1f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.2f, 0.9f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f, 0.7f, 0.1f, 1.0f });

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.5f);
		ImGui::PushFont(boldFnt);
		if (ImGui::Button(v2, buttonSize)) { vec->y = reset.y; }
		ImGui::PopFont();
		ImGui::PopStyleVar();
		ImGui::SameLine();
		ImGui::DragFloat(v2StrId.c_str(), &vec->y, 0.1f);
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::PopStyleColor(4);


		ImGui::PushStyleColor(ImGuiCol_Text, textColour);
		ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f, 0.1f, 0.7f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.2f, 0.2f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f, 0.1f, 0.7f, 1.0f });

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.5f);
		ImGui::PushFont(boldFnt);
		if (ImGui::Button(v3, buttonSize)) { vec->z = reset.z;	}
		ImGui::PopFont();
		ImGui::PopStyleVar();
		ImGui::SameLine();
		ImGui::DragFloat(v3StrId.c_str(), &vec->z, 0.1f);
		ImGui::PopItemWidth();
		ImGui::PopStyleColor(4);

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

		ComponentUiDrawInfo transformDrawInfo{};
		transformDrawInfo.displayName = "Transform";
		drawComponent<TransformComponent>(&entity, transformDrawInfo, [](TransformComponent* comp)
		{
			ImGui::Spacing();
			drawVec3Ctrl("Position", &comp->translation, { 0.0f, 0.0f, 0.0f });
			ImGui::Spacing();
			drawVec3Ctrl("Rotation", &comp->rotation, { 0.0f, 0.0f, 0.0f });
			ImGui::Spacing();
			drawVec3Ctrl("Scale", &comp->scale, {1.0f, 1.0f, 1.0f});
			ImGui::Spacing();
				
		});

		ComponentUiDrawInfo cameraDrawInfo{};
		cameraDrawInfo.displayName = "Camera";
		drawComponent<CameraComponent>(&entity, cameraDrawInfo, [](CameraComponent* comp)
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

		ComponentUiDrawInfo spriteRendererDrawInfo{};
		spriteRendererDrawInfo.displayName = "Sprite Renderer";
		drawComponent<SpriteRendererComponent>(&entity, spriteRendererDrawInfo, [](SpriteRendererComponent *comp)
			{
				ImGui::ColorEdit4("Base Colour", glm::value_ptr(comp->colour));
			});

		// Add to the drawComponents function
		ComponentUiDrawInfo meshRendererDrawInfo{};
		meshRendererDrawInfo.displayName = "Mesh Renderer";
		drawComponent<MeshRendererComponent>(&entity, meshRendererDrawInfo, [](MeshRendererComponent* comp)
			{
				ImGui::ColorEdit4("Base Colour", glm::value_ptr(comp->colour));

				// Mesh file selector
				static char meshPathBuff[256] = "";
				ImGui::InputText("Mesh Path", meshPathBuff, sizeof(meshPathBuff));

				if (ImGui::Button("Load Mesh"))
				{
					std::string meshPath = meshPathBuff;
					if (!meshPath.empty())
					{
						if (meshPath.front() == '"') {
							meshPath.erase(0, 1);
							meshPath.erase(meshPath.size() - 1);
						}

						comp->mesh = Mesh::create(meshPath);
					}
				}

				if (comp->mesh)
				{
					ImGui::Text("Vertices: %d", comp->mesh->getVertexCount());
					ImGui::Text("Triangles: %d", comp->mesh->getIndexCount() / 3);
					ImGui::Text("Submeshes: %d", (int)comp->mesh->getSubMeshes().size());
					ImGui::Text("Materials: %d", comp->mesh->getMaterials().getMaterialCount());

					// Display material information
					ImGui::Separator();
					ImGui::Text("Materials:");

					for (uint32_t i = 0; i < comp->mesh->getMaterials().getMaterialCount(); i++)
					{
						if (auto material = comp->mesh->getMaterials().getMaterial(i))
						{

							if (ImGui::TreeNode(material->getName().c_str()))
							{
								ImGui::PushID(material->getName().c_str());

								auto& props = material->getMaterialProperties();


								glm::vec3 diffuse{ props.diffuse };
								if (ImGui::ColorEdit3("Diffuse", glm::value_ptr(diffuse)))
								{
									material->setDiffuse(diffuse);
								}
								float opacity = props.opacity;
								if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f)) { material->setOpacity(opacity); }

								glm::vec3 specular{ props.specular };
								if (ImGui::ColorEdit3("Specular", glm::value_ptr(specular)))
								{
									material->setSpecular(specular);
								}

								float shininess = props.shininess;
								if (ImGui::SliderFloat("Shininess", &shininess, 0.0f, 256.0f)) { material->setShininess(shininess); }

								bool backFaceCulling = props.backfaceCulling;
								if (ImGui::Checkbox("Backface Culling", &backFaceCulling))
								{
									material->setBackfaceCulling(backFaceCulling);
								}

								ImGui::Text("Has Diffuse Map: %s", material->getDiffuseMap() ? "Yes" : "No");
								ImGui::Text("Has Specular Map: %s", material->getSpecularMap() ? "Yes" : "No");
								ImGui::Text("Has Normal Map: %s", material->getNormalMap() ? "Yes" : "No");


								ImGui::TreePop();

								ImGui::PopID();

							}
						}
					}
				}
			});
	}

	template<typename T>
	void SceneHierarchyPanel::drawComponent(Entity* entity, const ComponentUiDrawInfo& drawInfo, void(*uiFunc)(T*))
	{
		if (entity->hasComponent<T>())
		{
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), drawInfo.treeNodeFlags, drawInfo.displayName);

			ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);
			if (ImGui::Button("...", ImVec2{30.0f, 20.0f}))
			{
				ImGui::OpenPopup("Component Properties");
			}

			bool removeComponent = false;

			if (ImGui::BeginPopup("Component Properties"))
			{
				if (ImGui::MenuItem("Remove Component"))
				{
					removeComponent = true;
				}
				ImGui::EndPopup();
			}

			if (open)
			{
				auto& comp = entity->getComponent<T>();

				uiFunc(&comp);

				ImGui::TreePop();
			}

			if (removeComponent)
			{
				entity->removeComponent<T>();
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

		ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 5.0f);
		if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::BeginMenu("New"))
			{
				if (ImGui::MenuItem("Entity")) { m_sceneContext->createEntity("entity"); }

				ImGui::EndMenu();
			}

			ImGui::MenuItem("Cut");
			ImGui::MenuItem("Copy");
			ImGui::MenuItem("Paste");


			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();


		ImGui::End();

		ImGui::Begin("Inspector");
		if (m_selectedEntity)
		{
			drawComponents(m_selectedEntity);

			if (ImGui::Button("Add Component"))
			{
				ImGui::OpenPopup("Add-Component");
			}

			if (ImGui::BeginPopup("Add-Component"))
			{
				if (ImGui::MenuItem("Camera"))
				{
					m_selectedEntity.addComponent<CameraComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Sprite Renderer"))
				{
					m_selectedEntity.addComponent<SpriteRendererComponent>();
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Mesh Renderer"))
				{
					m_selectedEntity.addComponent<MeshRendererComponent>();
					ImGui::CloseCurrentPopup();
				}


				ImGui::EndPopup();
			}
		}
		ImGui::End();
	}

}
