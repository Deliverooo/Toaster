#pragma once


#include "imgui.h"
#include "Toaster/Core/Log.hpp"
#include "Toaster/Scene/Scene.hpp"
#include "Toaster/Scene/Entity.hpp"



namespace tst 
{


	
	struct HierarchyNode
	{
		Entity entity;
		std::vector<RefPtr<HierarchyNode>> children;
		RefPtr<HierarchyNode> parent;
		bool expanded{ true };
		bool visible{ true };
		int depth{ 0 };
	};

	struct PanelUiSettings
	{
		bool showIcons{ true };
		bool showSearch{ true };
		bool compactMode{ false };

		float hierarchyIndentation{ 10.0f };
		ImVec4 selectionColour{ 0.3f, 0.5f, 0.9f, 0.5f };
		ImVec4 hoverColour{ 0.2f, 0.4f, 0.7f, 0.3f };

		std::string searchFilter;
	};

	class TST_API SceneHierarchyPanel
	{
	public:

		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const RefPtr<Scene>& scene);

		void setSceneContext(const RefPtr<Scene>& sceneContext);

		void onImGuiRender();

		void drawEntityNode(Entity entity, int depth);
		void drawComponents(Entity entity);

		void drawEntityContextMenu(Entity entity);
		void drawHierarchyContextMenu();

		Entity getSelectedEntity() const { return m_selectedEntity; }
		const std::unordered_set<entt::entity>& getSelectedEntities() const { return m_selectedEntities; }


		void setSearchFilter(const std::string& filter) { m_uiSettings.searchFilter = filter; }
		bool matchesSearchFilter(Entity entity) const;

		void (*onEntitySelected)(Entity entity) = nullptr;
		void (*onEntityDeleted)	(Entity entity) = nullptr;
		void (*onEntityCopied)	(Entity entity) = nullptr;

		struct ComponentUiDrawInfo
		{
			const char* displayName;
			const char* icon{ nullptr };

			ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen |
				ImGuiTreeNodeFlags_SpanAvailWidth |
				ImGuiTreeNodeFlags_AllowItemOverlap |
				ImGuiTreeNodeFlags_Framed;

			// e.g. you don't want to be able to remove the transform or tag component
			bool removable{ true };
			bool collapsible{ true };
		};

		template<typename T>
		void drawComponent(Entity* entity, const ComponentUiDrawInfo &drawInfo, void(*uiFunc)(T*));

		void pushStyleColours();
		void popStyleColours();
		void drawToolbar();
		void drawSearchBar();

	private:

		void initComponentIcons();
		void handleEntityDragDrop(Entity entity);
		bool isEntityVisible(Entity entity) const;

		void selectEntity(Entity entity, bool addToSelection = false);
		void deselectEntity(Entity entity);
		void clearSelection();


		RefPtr<Scene> m_sceneContext{nullptr};
		Entity m_selectedEntity{ entt::null, nullptr };
		std::unordered_set<entt::entity> m_selectedEntities;

		PanelUiSettings m_uiSettings;
		Entity m_draggedEntity{ entt::null, nullptr };
		bool m_isDragging{ false };

		// TODO - change from utf-8 icons to image ones
		std::unordered_map<std::string, const char*> m_componentIcons;

	};
}
