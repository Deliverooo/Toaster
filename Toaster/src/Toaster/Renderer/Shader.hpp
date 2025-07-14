#pragma once

namespace tst
{
	std::optional<std::string> readFile(const std::string& filePath);

	class TST_API Shader
	{
	public:
		Shader(const std::string &vertexSource, const std::string &fragmentSource);
		~Shader();

		void bind() const;
		void unbind() const;

		uint32_t getId() const {return m_shaderId;}

	private:

		uint32_t m_shaderId;
	};
}
