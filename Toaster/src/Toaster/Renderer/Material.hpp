#pragma once

#include "Shader.hpp"


namespace tst
{

	class TST_API Material
	{
	public:

		Material(RefPtr<Shader> &shader);

		//template<typename T>
		void set(const std::string& uniformName, glm::vec3 &value);


	private:
		RefPtr<Shader> &m_shader;
	};
}
