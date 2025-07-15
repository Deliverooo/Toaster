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

		virtual uint32_t getId() const override;

	private:
		uint32_t m_shaderId;
	};

}

