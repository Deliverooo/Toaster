#pragma once
#include "Toaster/Renderer/Shader.hpp"
#include "Toaster/Core/Log.hpp"

namespace tst
{
	class TST_API OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
		OpenGLShader(const std::string& shaderPath);
		OpenGLShader(const std::string& shaderName, const std::string& shaderPath);

		virtual ~OpenGLShader() override;

		virtual void bind() const override;
		virtual void unbind() const override;
		virtual std::string getName() const override;

		virtual void reload() const override;

		virtual void uploadMatrix2f(const glm::mat2& mat, const char* name) override;
		virtual void uploadMatrix3f(const glm::mat3& mat, const char* name) override;
		virtual void uploadMatrix4f(const glm::mat4& mat, const char* name) override;

		virtual void uploadVector1f(const glm::vec1& vec, const char* name) override;
		virtual void uploadVector2f(const glm::vec2& vec, const char* name) override;
		virtual void uploadVector3f(const glm::vec3& vec, const char* name) override;
		virtual void uploadVector4f(const glm::vec4& vec, const char* name) override;

		virtual void uploadBool(const bool b, const char* name) override;

		virtual void uploadInt1(const int x, const char* name) override;
		virtual void uploadInt2(const int x, const int y, const char* name) override;
		virtual void uploadInt3(const int x, const int y, const int z, const char* name) override;
		virtual void uploadInt4(const int x, const int y, const int z, const int w, const char* name) override;

		virtual void uploadFloat1(const float x, const char* name) override;
		virtual void uploadFloat2(const float x, const float y, const char* name) override;
		virtual void uploadFloat3(const float x, const float y, const float z, const char* name) override;
		virtual void uploadFloat4(const float x, const float y, const float z, const float w, const char* name) override;

		virtual void uploadDouble1(const double x, const char* name) override;
		virtual void uploadDouble2(const double x, const double y, const char* name) override;
		virtual void uploadDouble3(const double x, const double y, const double z, const char* name) override;
		virtual void uploadDouble4(const double x, const double y, const double z, const double w, const char* name) override;

		void uploadUniform1i(const int x, const char* name);
		void uploadUniform2i(const int x, const int y, const char* name);
		void uploadUniform3i(const int x, const int y, const int z, const char* name);
		void uploadUniform4i(const int x, const int y, const int z, const int w, const char* name);

		void uploadUniform1f(const float x, const char* name);
		void uploadUniform2f(const float x, const float y, const char* name);
		void uploadUniform3f(const float x, const float y, const float z, const char* name);
		void uploadUniform4f(const float x, const float y, const float z, const float w, const char* name);

		void uploadUniform1d(const double x, const char* name);
		void uploadUniform2d(const double x, const double y, const char* name);
		void uploadUniform3d(const double x, const double y, const double z, const char* name);
		void uploadUniform4d(const double x, const double y, const double z, const double w, const char* name);

		void uploadUniform1iv(const glm::vec<1, int>& vec, const char* name);
		void uploadUniform2iv(const glm::vec<2, int>& vec, const char* name);
		void uploadUniform3iv(const glm::vec<3, int>& vec, const char* name);
		void uploadUniform4iv(const glm::vec<4, int>& vec, const char* name);

		void uploadUniform1fv(const glm::vec1 &vec, const char* name);
		void uploadUniform2fv(const glm::vec2 &vec, const char* name);
		void uploadUniform3fv(const glm::vec3 &vec, const char* name);
		void uploadUniform4fv(const glm::vec4 &vec, const char* name);

		void uploadUniformMatrix2f(const glm::mat2 &mat, const char* name);
		void uploadUniformMatrix3f(const glm::mat3 &mat, const char* name);
		void uploadUniformMatrix4f(const glm::mat4 &mat, const char* name);



		virtual uint32_t getId() const;

	private:
		uint32_t m_shaderId;
		std::unordered_map<const char*, int> m_uniformLocations;

		std::string m_shaderName;

		int getUniformLocation(const char* name);
		unsigned int shaderTypeLookup(std::string type);
		uint32_t compileShader(const std::string& shaderSrc, const unsigned int shaderType);
		std::unordered_map<unsigned int, std::string> processShader(const std::string& shaderSrc);
	};

}

