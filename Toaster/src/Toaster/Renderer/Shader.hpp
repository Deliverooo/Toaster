#pragma once
#include "Toaster/Core.hpp"

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
