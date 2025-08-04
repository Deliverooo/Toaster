#pragma once
#include "Toaster/Core/Log.hpp"


namespace tst
{
	std::optional<std::string> readFile(const std::string& filePath);

	class TST_API Shader
	{
	public:
		virtual ~Shader() {};

		static RefPtr<Shader> create(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
		static RefPtr<Shader> create(const std::string& shaderPath);
		static RefPtr<Shader> create(const std::string& shaderName, const std::string& shaderPath);

		virtual void reload() const = 0;

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual std::string getName() const = 0;
		virtual uint32_t getId() const = 0;

		virtual bool hasUniform(const char* name) = 0;

		virtual void shaderDebugInfo() const = 0;
		virtual void uploadMatrix2f(const glm::mat2& mat, const char* name) = 0;
		virtual void uploadMatrix3f(const glm::mat3& mat, const char* name) = 0;
		virtual void uploadMatrix4f(const glm::mat4& mat, const char* name) = 0;

		virtual void uploadVector1f(const glm::vec1& vec, const char* name) = 0;
		virtual void uploadVector2f(const glm::vec2& vec, const char* name) = 0;
		virtual void uploadVector3f(const glm::vec3& vec, const char* name) = 0;
		virtual void uploadVector4f(const glm::vec4& vec, const char* name) = 0;

		virtual void uploadBool(const bool b, const char* name) = 0;

		virtual void uploadIntArray(int* arr, const uint32_t count, const char* name) = 0;
		virtual void uploadInt1(const int x, const char* name) = 0;
		virtual void uploadInt2(const int x, const int y, const char* name) = 0;
		virtual void uploadInt3(const int x, const int y, const int z, const char* name) = 0;
		virtual void uploadInt4(const int x, const int y, const int z, const int w, const char* name) = 0;

		virtual void uploadFloat1(const float x, const char* name) = 0;
		virtual void uploadFloat2(const float x, const float y, const char* name) = 0;
		virtual void uploadFloat3(const float x, const float y, const float z, const char* name) = 0;
		virtual void uploadFloat4(const float x, const float y, const float z, const float w, const char* name) = 0;

		virtual void uploadDouble1(const double x, const char* name) = 0;
		virtual void uploadDouble2(const double x, const double y, const char* name) = 0;
		virtual void uploadDouble3(const double x, const double y, const double z, const char* name) = 0;
		virtual void uploadDouble4(const double x, const double y, const double z, const double w, const char* name) = 0;
	};

	class TST_API ShaderLib
	{
	public:

		void addShader(const RefPtr<Shader> shader);
		void removeShader(const std::string& shaderName);
		RefPtr<Shader> getShader(const std::string& shaderName);

		RefPtr<Shader> loadShader(const std::string& shaderPath);
		RefPtr<Shader> loadShader(const std::string& shaderName, const std::string& shaderPath);
		RefPtr<Shader> loadShader(const std::string& shaderName, const std::string& vertexPath, const std::string& FragmentPath);

	private:

		std::unordered_map<std::string, RefPtr<Shader>> m_shaders;
	};
}
