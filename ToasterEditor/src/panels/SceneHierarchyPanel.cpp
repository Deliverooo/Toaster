#include "SceneHierarchyPanel.hpp"


#include "Toaster/Scene/Components.hpp"
#include <filesystem>
#include <optional>


#include "imgui_internal.h"
#include "glm/gtc/type_ptr.hpp"

#include "Toaster/Renderer/Mesh.hpp"
#include "Toaster/Util/PlatformUtils.hpp"


namespace tst
{

	SceneHierarchyPanel::SceneHierarchyPanel(const RefPtr<Scene>& scene) : m_sceneContext(scene)
	{
		initComponentIcons();
	}

	void SceneHierarchyPanel::setSceneContext(const RefPtr<Scene>& sceneContext)
	{
		m_sceneContext = sceneContext;
		m_selectedEntity = {};
		clearSelection();
	}

	void SceneHierarchyPanel::initComponentIcons()
	{
		m_componentIcons["Transform"] = "↔️↕";
		m_componentIcons["Camera"] = "🎥";
		m_componentIcons["Light"] = "💡";
		m_componentIcons["Mesh Renderer"] = "🗿";
		m_componentIcons["Sprite Renderer"] = "🐸";
		m_componentIcons["Tag"] = "🏷️";
	}

	void SceneHierarchyPanel::pushStyleColours()
	{
		ImGui::PushStyleColor(ImGuiCol_WindowBg, { 0.1f, 0.1f, 0.1f, 0.95f });
		ImGui::PushStyleColor(ImGuiCol_Header, { 0.2f, 0.2f, 0.2f, 0.80f });
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, m_uiSettings.hoverColour);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, m_uiSettings.selectionColour);
		ImGui::PushStyleColor(ImGuiCol_TreeLines, { 0.4f, 0.4f, 0.4f, 1.0f });

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.2f, 0.2f, 0.60f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.3f, 0.3f, 0.3f, 0.80f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.2f, 0.2f, 0.2f, 1.00f });
	}

	void SceneHierarchyPanel::popStyleColours()
	{
		ImGui::PopStyleColor(8);
	}

	void SceneHierarchyPanel::drawToolbar()
	{
		ImGui::BeginGroup();

		if (ImGui::Button("+ Entity"))
		{
			auto entity = m_sceneContext->createEntity("New Entity");
			selectEntity(entity);
		}

		ImGui::SameLine();

		if (ImGui::Button("- Delete"))
		{
			if (m_selectedEntity)
			{
				m_sceneContext->removeEntity(m_selectedEntity);
				if (onEntityDeleted) { onEntityDeleted(m_selectedEntity); }
				m_selectedEntity = {};
			}
		}

		ImGui::SameLine();

		ImGui::Checkbox("Icons", &m_uiSettings.showIcons);
		ImGui::SameLine();
		ImGui::Checkbox("Compact", &m_uiSettings.compactMode);

		ImGui::EndGroup();
	}

	void SceneHierarchyPanel::handleEntityDragDrop(Entity entity)
	{
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("ENTITY_DRAG", &entity, sizeof(Entity));
			ImGui::Text("Move %s", entity.getComponent<TagComponent>().name.c_str());
			m_draggedEntity = entity;
			m_isDragging = true;
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG"))
			{
				Entity droppedEntity = *(Entity*)payload->Data;
				// TODO: Implement entity parenting logic

			}
			ImGui::EndDragDropTarget();
		}
	}


	void SceneHierarchyPanel::drawSearchBar()
	{
		if (m_uiSettings.showSearch)
		{
			ImGui::SetNextItemWidth(-1);

			char searchBuff[255];
			strcpy_s(searchBuff, sizeof(searchBuff), m_uiSettings.searchFilter.c_str());

			if (ImGui::InputTextWithHint("##Search", "Search for an entity", searchBuff, sizeof(searchBuff)))
			{
				m_uiSettings.searchFilter = searchBuff;
			}
		}
	}

	bool SceneHierarchyPanel::matchesSearchFilter(Entity entity) const
	{
		if (m_uiSettings.searchFilter.empty()) { return true; }

		if (!entity.hasComponent<TagComponent>()) { return false; }

		auto& tag = entity.getComponent<TagComponent>();
		std::string entityName = tag.name;
		std::string filter = m_uiSettings.searchFilter;


		std::transform(entityName.begin(), entityName.end(), entityName.begin(), ::tolower);
		std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

		return entityName.find(filter) != std::string::npos;
	}

	void SceneHierarchyPanel::drawEntityContextMenu(Entity entity)
	{
		if (ImGui::MenuItem("Duplicate"))
		{
			// TODO - Implement entity duplication
		}

		if (ImGui::MenuItem("Delete", "Del"))
		{
			m_sceneContext->removeEntity(entity);
			if (entity == m_selectedEntity) { m_selectedEntity = {}; }
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("Add Component"))
		{
			if (ImGui::MenuItem("Camera") && !entity.hasComponent<CameraComponent>())
			{
				entity.addComponent<CameraComponent>();
			}
			if (ImGui::MenuItem("Light") && !entity.hasComponent<LightComponent>())
			{
				entity.addComponent<LightComponent>();
			}
			if (ImGui::MenuItem("Mesh Renderer") && !entity.hasComponent<MeshRendererComponent>())
			{
				entity.addComponent<MeshRendererComponent>();
			}
			if (ImGui::MenuItem("Sprite Renderer") && !entity.hasComponent<SpriteRendererComponent>())
			{
				entity.addComponent<SpriteRendererComponent>();
			}

			ImGui::EndMenu();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Focus Camera"))
		{
			// TODO - Focus camera on selected entity
		}
	}

	void SceneHierarchyPanel::drawHierarchyContextMenu()
	{
		if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::BeginMenu("Create"))
			{
				if (ImGui::MenuItem("Empty Entity"))
				{
					auto entity = m_sceneContext->createEntity("Empty Entity");
					selectEntity(entity);
				}

				if (ImGui::MenuItem("Camera"))
				{
					auto entity = m_sceneContext->createEntity("Camera");
					entity.addComponent<CameraComponent>();
					selectEntity(entity);
				}

				if (ImGui::MenuItem("Light"))
				{
					auto entity = m_sceneContext->createEntity("Light");
					entity.addComponent<LightComponent>();
					selectEntity(entity);
				}

				ImGui::EndMenu();
			}
			ImGui::Separator();

			if (ImGui::MenuItem("Select All"))
			{
				// TODO: Implement select all
			}

			if (ImGui::MenuItem("Deselect All"))
			{
				clearSelection();
			}

			ImGui::EndPopup();
		}
	}

	void SceneHierarchyPanel::selectEntity(Entity entity, bool addToSelection)
	{
		if (!addToSelection) { clearSelection(); }

		m_selectedEntity = entity;
		m_selectedEntities.insert(entity);

		if (onEntitySelected) { onEntitySelected(entity); }
	}

	void SceneHierarchyPanel::clearSelection()
	{
		m_selectedEntities.clear();
		m_selectedEntity = { entt::null, nullptr };
	}

	void SceneHierarchyPanel::deselectEntity(Entity entity)
	{
		m_selectedEntities.erase(entity);

		if (m_selectedEntity == entity) { m_selectedEntity = { entt::null, nullptr }; }
	}





	void SceneHierarchyPanel::drawEntityNode(Entity entity, int depth = 0)
	{
		if (!matchesSearchFilter(entity)) { return; }


		auto& tag = entity.getComponent<TagComponent>();


		bool isSelected = (entity == m_selectedEntity);

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

		if (isSelected) { flags |= ImGuiTreeNodeFlags_Selected; }

		flags |= ImGuiTreeNodeFlags_Leaf;

		float indentSize = m_uiSettings.hierarchyIndentation * depth;
		if (depth > 0)
		{
			ImGui::Indent(indentSize);
		}

		const char* entityIcon = "📦";

		std::string displayName = m_uiSettings.showIcons ? (std::string(entityIcon) + " " + tag.name) : tag.name;

		bool opened = ImGui::TreeNodeEx((void*)(uint32_t)entity, flags, "%s", displayName.c_str());


		if (ImGui::IsItemClicked())
		{
			selectEntity(entity);
		}

		// Enhanced hover effects
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Entity: %s\nID: %u", tag.name.c_str(), (uint32_t)entity);
		}

		handleEntityDragDrop(entity);

		// Context menu
		if (ImGui::BeginPopupContextItem())
		{
			drawEntityContextMenu(entity);
			ImGui::EndPopup();
		}

		if (opened)
		{
			// Draw children here if you implement hierarchy
			ImGui::TreePop();
		}

		if (depth > 0)
		{
			ImGui::Unindent(indentSize);
		}
	}

	static void drawVec3Ctrl(const char* label, glm::vec3* vec, const glm::vec3& reset = glm::vec3(1.0f), float columnWidth = 100.0f, const char* v1 = "X", const char* v2 = "Y", const char* v3 = "Z")
	{
		ImGuiIO& io = ImGui::GetIO();
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
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 2, 0 });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.5f);

		static ImVec4 textColour = { 1.0f, 1.0f, 1.0f, 1.0f };

		float lineHeight = ImGui::GetFrameHeightWithSpacing();
		ImVec2 buttonSize = ImVec2{ lineHeight, lineHeight };


		ImGui::PushStyleColor(ImGuiCol_Text, textColour);
		ImGui::PushStyleColor(ImGuiCol_Button, { 0.7f, 0.1f, 0.1f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.7f, 0.1f, 0.1f, 1.0f });

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.5f);
		ImGui::PushFont(boldFnt);
		if (ImGui::Button(v1, buttonSize)) { vec->x = reset.x; }
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
		if (ImGui::Button(v3, buttonSize)) { vec->z = reset.z; }
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
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tagComp.name = std::string(buffer);
			}
		}



		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

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
			if (ImGui::MenuItem("Script"))
			{
				//m_selectedEntity.addComponent<NativeScriptComponent>();
				//ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		ComponentUiDrawInfo transformDrawInfo{};
		transformDrawInfo.removable = false;
		transformDrawInfo.displayName = "Transform";
		drawComponent<TransformComponent>(&entity, transformDrawInfo, [](TransformComponent* comp)
			{
				ImGui::Spacing();
				drawVec3Ctrl("Position", &comp->translation, { 0.0f, 0.0f, 0.0f });
				ImGui::Spacing();
				drawVec3Ctrl("Rotation", &comp->rotation, { 0.0f, 0.0f, 0.0f });
				ImGui::Spacing();
				drawVec3Ctrl("Scale", &comp->scale, { 1.0f, 1.0f, 1.0f });
				ImGui::Spacing();

			});

		ComponentUiDrawInfo cameraDrawInfo{};
		cameraDrawInfo.displayName = "Camera";
		drawComponent<CameraComponent>(&entity, cameraDrawInfo, [](CameraComponent* comp)
			{
				auto& camera = comp->camera;

				ImGui::Checkbox("Active Camera", &comp->active);

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

					if (ImGui::DragFloat("Ortho Size", &orthoSize, 0.1f, 0.1f, 10.0f)) { camera.setOrthoSize(orthoSize); }
					if (ImGui::DragFloat("Near Clip", &nearClip, 0.01f, -100.0f, 100.0f)) { camera.setOrthoNear(nearClip); }
					if (ImGui::DragFloat("Far Clip", &farClip, 0.01f, -100.0f, 100.0f)) { camera.setOrthoFar(farClip); }
				}
			});

		ComponentUiDrawInfo spriteRendererDrawInfo{};
		spriteRendererDrawInfo.displayName = "Sprite Renderer";
		drawComponent<SpriteRendererComponent>(&entity, spriteRendererDrawInfo, [](SpriteRendererComponent* comp)
			{
				if (ImGui::Button("Load Texture"))
				{
					std::string texturePath = FileDialog::openFile("Image Files (*.png; *.jpg; *.jpeg)\0*.png;*.jpg;*.jpeg\0");
					if (!texturePath.empty())
					{
						if (texturePath.front() == '"') {
							texturePath.erase(0, 1);
							texturePath.erase(texturePath.size() - 1);
						}
						comp->texture = Texture2D::create(texturePath);
						comp->colour = { 1.0f, 1.0f, 1.0f, 1.0f };
					}
				}

				if (comp->texture)
				{
					ImGuiIO& io = ImGui::GetIO();

					const char* filtering[] = { "Nearest", "Linear", "NearestMipmapNearest", "LinearMipmapNearest", "NearestMipmapLinear", "LinearMipmapLinear" };
					const char* currentMinFilteringMode = filtering[(int)comp->texture->getParams().minFilter];
					const char* currentMagFilteringMode = filtering[(int)comp->texture->getParams().magFilter];

					ImGui::ColorEdit4("Tint Colour", glm::value_ptr(comp->colour));

					ImGui::PushFont(io.Fonts->Fonts[2]);
					if (ImGui::TreeNode("Texture Properties"))
					{
						auto& properties = comp->texture->getParams();

						if (ImGui::BeginCombo("Min Filter", currentMinFilteringMode))
						{
							for (int i = 0; i < 6; i++)
							{
								bool selected = (currentMinFilteringMode == filtering[i]);
								if (ImGui::Selectable(filtering[i], selected))
								{
									currentMinFilteringMode = filtering[i];
									comp->texture->setFilterMode(static_cast<TextureFiltering>(i), properties.magFilter);
								}
								if (selected)
								{
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}

						if (ImGui::BeginCombo("Mag Filter", currentMagFilteringMode))
						{
							for (int i = 0; i < 6; i++)
							{
								bool selected = (currentMagFilteringMode == filtering[i]);
								if (ImGui::Selectable(filtering[i], selected))
								{
									currentMagFilteringMode = filtering[i];
									comp->texture->setFilterMode(properties.minFilter, static_cast<TextureFiltering>(i));
								}
								if (selected)
								{
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}

						ImGui::Spacing();

						const char* textureWrapping[] = { "Repeat", "ClampToEdge", "ClampToBorder", "MirroredRepeat" };
						const char* currentWrapS = textureWrapping[(int)comp->texture->getParams().wrapS];
						const char* currentWrapT = textureWrapping[(int)comp->texture->getParams().wrapT];


						if (ImGui::BeginCombo("Wrap S", currentWrapS))
						{
							for (int i = 0; i < 4; i++)
							{
								bool selected = (currentWrapS == textureWrapping[i]);
								if (ImGui::Selectable(textureWrapping[i], selected))
								{
									currentWrapS = textureWrapping[i];
									comp->texture->setWrapMode(static_cast<TextureWrapping>(i), comp->texture->getParams().wrapT);
								}
								if (selected)
								{
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}

						if (ImGui::BeginCombo("Wrap T", currentWrapT))
						{
							for (int i = 0; i < 4; i++)
							{
								bool selected = (currentWrapT == textureWrapping[i]);
								if (ImGui::Selectable(textureWrapping[i], selected))
								{
									currentWrapT = textureWrapping[i];
									comp->texture->setWrapMode(comp->texture->getParams().wrapS, static_cast<TextureWrapping>(i));
								}
								if (selected)
								{
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}

						ImGui::Spacing();


						bool flipX = comp->texture->getParams().flipX;
						bool flipY = comp->texture->getParams().flipY;
						if (ImGui::Checkbox("Flip X", &flipX))
						{
							comp->texture->setFlipX(flipX);
						}
						if (ImGui::Checkbox("Flip Y", &flipY))
						{
							comp->texture->setFlipY(flipY);
						}


						ImGui::TreePop();
					}
					ImGui::PopFont();
				}
				else
				{
					ImGui::ColorEdit4("Base Colour", glm::value_ptr(comp->colour));
				}
			});

		// Add to the drawComponents function
		ComponentUiDrawInfo meshRendererDrawInfo{};
		meshRendererDrawInfo.displayName = "Mesh Renderer";
		drawComponent<MeshRendererComponent>(&entity, meshRendererDrawInfo, [](MeshRendererComponent* comp)
			{

				if (ImGui::Button("Load Mesh"))
				{
					std::string meshPath = FileDialog::openFile("Mesh Files (*.fbx; *.obj; *.gltf; *.glb)\0*.fbx;*.obj;*.gltf;*.glb\0");
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

					if (ImGui::TreeNode("Mesh Info"))
					{
						ImGui::Text("Vertices: %d", comp->mesh->getVertexCount());
						ImGui::Text("Triangles: %d", comp->mesh->getIndexCount() / 3);
						ImGui::Text("Submeshes: %d", (int)comp->mesh->getSubMeshes().size());
						ImGui::Text("Materials: %d", comp->mesh->getMaterialIDs().size());
						ImGui::TreePop();
					}

					ImGui::Separator();

					if (ImGui::TreeNode("Viewport Display"))
					{

						ImGui::Checkbox("Show Bounding Box", &comp->showBoundingBox);

						ImGui::TreePop();
					}

					// Display material information
					ImGui::Separator();

					if (ImGui::TreeNode("Materials"))
					{
						for (uint32_t i = 0; i < comp->mesh->getMaterialIDs().size(); i++)
						{
							if (auto material = comp->mesh->getMaterial(i))
							{

								if (ImGui::TreeNode(material->getName().c_str()))
								{
									ImGui::PushID(material->getName().c_str());


									glm::vec3 diffuse = material->getProperty<glm::vec3>("diffuse");
									if (ImGui::ColorEdit3("Diffuse", glm::value_ptr(diffuse))) { material->setProperty("diffuse", diffuse); }

									float opacity = material->getProperty<float>("opacity");
									if (ImGui::SliderFloat("Opacity", &opacity, 0.0f, 1.0f)) { material->setProperty("opacity", opacity); }

									glm::vec3 specular = material->getProperty<glm::vec3>("specular");
									if (ImGui::ColorEdit3("Specular", glm::value_ptr(specular))) { material->setProperty("specular", specular); }

									float roughness = material->getProperty<float>("roughness");
									if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) { material->setProperty("roughness", roughness); }

									float metallic = material->getProperty<float>("metallic");
									if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) { material->setProperty("metallic", metallic); }

									bool backFaceCulling = material->getRenderState().backfaceCulling;
									if (ImGui::Checkbox("Backface Culling", &backFaceCulling)) { material->setBackfaceCulling(backFaceCulling); }

									ImGui::Text("Has Diffuse Map: %s", material->getDiffuseMap() ? "Yes" : "No");
									ImGui::Text("Has Specular Map: %s", material->getSpecularMap() ? "Yes" : "No");
									ImGui::Text("Has Normal Map: %s", material->getNormalMap() ? "Yes" : "No");


									ImGui::TreePop();

									ImGui::PopID();

								}
							}
						}
						ImGui::TreePop();
					}

				}
			});

		ComponentUiDrawInfo lightDrawInfo{};
		lightDrawInfo.displayName = "Light";
		drawComponent<LightComponent>(&entity, lightDrawInfo, [](LightComponent* comp)
			{
				const char* lightTypes[] = { "Directional", "Point", "Spot" };
				const char* currentLightType = lightTypes[(int)comp->light.type];

				if (ImGui::BeginCombo("Type", currentLightType))
				{
					for (int i = 0; i < 3; i++)
					{

						bool selected = currentLightType == lightTypes[i];

						if (ImGui::Selectable(lightTypes[i], selected))
						{
							currentLightType = lightTypes[i];
							comp->light.type = static_cast<Light::Type>(i);
						}

						if (selected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndCombo();
				}

				ImGui::ColorEdit3("Colour", glm::value_ptr(comp->light.colour));

				ImGui::DragFloat("Intensity", &comp->light.intensity, 0.001f, 0.0f, 100.0f);

				if (comp->light.type != Light::Type::Directional)
				{
					ImGui::Separator();

					ImGui::Text("Attenuation");
					ImGui::DragFloat("Constant", &comp->light.constant, 0.01f, 0.0f, 2.0f);
					ImGui::DragFloat("Linear", &comp->light.linear, 0.001f, 0.0f, 2.0f);
					ImGui::DragFloat("Quadratic", &comp->light.quadratic, 0.01f, 0.0f, 2.0f);
				}


				if (comp->light.type == Light::Type::Spot)
				{
					ImGui::Separator();

					ImGui::Text("Cone Properties");
					ImGui::DragFloat("Inner", &comp->light.innerCone, 0.25f, 0.0f, 180.0f);
					ImGui::DragFloat("Blending", &comp->light.outerOffset, 0.05f, 0.01f, 30.0f);

				}
			});
	}

	template<typename T>
	void SceneHierarchyPanel::drawComponent(Entity* entity, const ComponentUiDrawInfo& drawInfo, void(*uiFunc)(T*))
	{
		if (!entity->hasComponent<T>()) { return; }

		ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));

		float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImGui::Separator();

		ImGui::PushID(typeid(T).name());

		bool open = false;
		if (drawInfo.collapsible)
		{
			open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), drawInfo.treeNodeFlags, "%s %s", (drawInfo.icon) ? drawInfo.icon : "", drawInfo.displayName);
		}
		else
		{
			ImGui::AlignTextToFramePadding();
			if (drawInfo.icon)
			{
				ImGui::Text("%s %s", drawInfo.icon, drawInfo.displayName);
			}
			else
			{
				ImGui::Text("%s", drawInfo.displayName);
			}
			open = true;
		}

		ImGui::SameLine(contentRegionAvail.x - lineHeight * 2.0f);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.7f));

		if (ImGui::Button("*", ImVec2(lineHeight * 1.5f, lineHeight)))
		{
			ImGui::OpenPopup("ComponentOptions");
		}

		ImGui::PopStyleColor(3);

		bool removeComponent = false;
		if (ImGui::BeginPopup("ComponentOptions"))
		{
			if (ImGui::MenuItem("Reset"))
			{
				entity->removeComponent<T>();
				entity->addComponent<T>();
			}
			if (drawInfo.removable && ImGui::MenuItem("Remove Component"))
			{
				removeComponent = true;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Copy Component"))
			{
				// TODO Add component copying
			}
			if (ImGui::MenuItem("Paste Component"))
			{
				// TODO Add component pasting
			}

			ImGui::EndPopup();
		}

		if (open && drawInfo.collapsible)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 16.0f);
			ImGui::Indent();

			auto& comp = entity->getComponent<T>();
			if (uiFunc) { uiFunc(&comp); }

			ImGui::Unindent();
			ImGui::PopStyleVar();

			ImGui::TreePop();
		}
		else if (open && !drawInfo.collapsible)
		{
			auto& comp = entity->getComponent<T>();
			if (uiFunc) { uiFunc(&comp); }
		}

		if (removeComponent)
		{
			entity->removeComponent<T>();
		}

		ImGui::PopID();
		ImGui::PopStyleVar(2);

	}


	void SceneHierarchyPanel::onImGuiRender()
	{

		pushStyleColours();

		ImGui::Begin("Scene Hierarchy");

		drawToolbar();
		ImGui::Separator();

		drawSearchBar();

		if (m_uiSettings.showSearch) { ImGui::Separator(); }

		ImGui::BeginChild("Entity List", { 0, 0 }, false);

		if (m_sceneContext)
		{
			m_sceneContext->m_registry.view<entt::entity>().each([this](auto entityId)
				{
					Entity entity = { entityId, m_sceneContext.get() };
					drawEntityNode(entity);
				});
		}

		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
		{
			clearSelection();
		}

		drawHierarchyContextMenu();

		ImGui::EndChild();
		ImGui::End();


		ImGui::Begin("Inspector");

		if (m_selectedEntity)
		{
			drawComponents(m_selectedEntity);
		}
		else
		{
			ImGui::TextDisabled("No entity selected");

			if (ImGui::Button("Create New Entity"))
			{
				auto entity = m_sceneContext->createEntity("New Entity");
				selectEntity(entity);
			}
		}

		ImGui::End();

		popStyleColours();
	}

}