#pragma once


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

	private:

		RefPtr<Scene> m_sceneContext{nullptr};
		Entity m_selectedEntity{ entt::null, nullptr };
		
	};
}
