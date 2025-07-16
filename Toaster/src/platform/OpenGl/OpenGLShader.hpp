#pragma once
#include "Toaster/Renderer/Shader.hpp"

namespace tst
{
	class TST_API OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& vertexPath, const std::string& fragmentPath);
		virtual ~OpenGLShader() override;

		virtual void bind() const override;
		virtual void unbind() const override;

		virtual void uploadUniform1i(const int uni, const char* name) const override;
		virtual void uploadUniform1f(const float uni, const char* name) const override;
		virtual void uploadUniform1d(const double uni, const char* name) const override;

		virtual void uploadUniform3fv(const glm::vec3 &uni, const char* name) const override;
		virtual void uploadUniformMatrix4f(const glm::mat4 &uni, const char* name) const override;

		virtual uint32_t getId() const override;

	private:
		uint32_t m_shaderId;
	};

}

