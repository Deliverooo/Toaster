#include "tstpch.h"
#include "Material.hpp"

#include "platform/OpenGl/OpenGLShader.hpp"

namespace tst
{
	Material::Material(RefPtr<Shader> &shader) : m_shader(shader)
	{

	}

	void Material::set(const std::string& uniformName, glm::vec3& value)
	{
		((OpenGLShader*)m_shader.get())->uploadUniform3fv(value, uniformName.c_str());


	}


}
