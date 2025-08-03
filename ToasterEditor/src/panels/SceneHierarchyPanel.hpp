#pragma once


#include "imgui.h"
#include "Toaster/Core/Log.hpp"
#include "Toaster/Scene/Scene.hpp"
#include "Toaster/Scene/Entity.hpp"



namespace tst 
{

	class TST_API SceneHierarchyPanel
	{
	public:

		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const RefPtr<Scene>& scene);

		void setSceneContext(const RefPtr<Scene>& sceneContext);

		void onImGuiRender();

		void drawEntityNode(Entity entity);
		void drawComponents(Entity entity);


		struct ComponentUiDrawInfo
		{
			const char* displayName;
			ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed;

		};

		template<typename T>
		void drawComponent(Entity* entity, const ComponentUiDrawInfo &drawInfo, void(*uiFunc)(T*));

	private:

		RefPtr<Scene> m_sceneContext{nullptr};
		Entity m_selectedEntity{ entt::null, nullptr };
		
	};
}
