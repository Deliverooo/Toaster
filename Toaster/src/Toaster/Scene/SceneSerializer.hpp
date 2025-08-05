#pragma once
#include "Scene.hpp"


namespace tst
{

	class SceneSerializer
	{
	public:
		SceneSerializer(const RefPtr<Scene>& scene);

		void serialize(const std::string& filepath);
		void serializeRuntime(const std::string& filepath);

		bool deserialize(const std::string& filepath);
		void deserializeRuntime(const std::string& filepath);

	private:
		RefPtr<Scene> m_Scene;
	};
}
