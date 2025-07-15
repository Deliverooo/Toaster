#pragma once

namespace tst
{
	std::optional<std::string> readFile(const std::string& filePath);

	class TST_API Shader
	{
	public:
		virtual ~Shader() {};

		static std::shared_ptr<Shader> create(const std::string& vertexPath, const std::string& fragmentPath);

		virtual void bind() const = 0;
		virtual void unbind() const = 0;

		virtual uint32_t getId() const = 0;
	};
}
