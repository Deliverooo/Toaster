#include "tstpch.h"
#include "Shader.hpp"

#include "Glad/glad.h"

namespace tst
{
	std::optional<std::string> readFile(const std::string& filePath)
	{
        std::ifstream fileObj(filePath, std::ios::ate | std::ios::binary);

        if (!fileObj.is_open()) {
            TST_CORE_ERROR("Failed to Read Shader File: {0}", filePath);
            return {};
        }

        const size_t file_size = static_cast<size_t>(fileObj.tellg());
        std::vector<char> buff(file_size);
        fileObj.seekg(0);

        fileObj.read(buff.data(), static_cast<std::streamsize>(file_size));

        fileObj.close();

        TST_CORE_INFO("Successfully Read Shader File: {0}", filePath);

        return std::string(buff.begin(), buff.end());
	}
		
    Shader::Shader(const std::string& vertexPath, const std::string& FragmentPath)
    {
        auto vertexSource = readFile(vertexPath);
        auto fragmentSource = readFile(FragmentPath);
        TST_ASSERT(vertexSource.has_value() && fragmentSource.has_value(), "Failed to Read Shader Files!");
		

		uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);

		// Send the vertex shader source code to GL
		// Note that std::string's .c_str is NULL character terminated.
		const char* source = vertexSource->c_str();
		glShaderSource(vertexShader, 1, &source, nullptr);

		// Compile the vertex shader
		glCompileShader(vertexShader);

		int isCompiled = 0;
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &isCompiled);
		
		if (!isCompiled)
		{
			int maxLength = 0;
			glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<char> infoLog(maxLength);
			glGetShaderInfoLog(vertexShader, maxLength, &maxLength, infoLog.data());

			// We don't need the shader anymore.
			glDeleteShader(vertexShader);

			TST_CORE_ERROR("{0}", std::string(infoLog.begin(), infoLog.end()));
			return;
		}

		// Create an empty fragment shader handle
		uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

		// Send the fragment shader source code to GL
		// Note that std::string's .c_str is NULL character terminated.
		source = fragmentSource->c_str();
		glShaderSource(fragmentShader, 1, &source, nullptr);

		// Compile the fragment shader
		glCompileShader(fragmentShader);

		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &isCompiled);
		if (!isCompiled)
		{
			int maxLength = 0;
			glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<char> infoLog(maxLength);
			glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, infoLog.data());

			// We don't need the shader anymore.
			glDeleteShader(fragmentShader);
			// Either of them. Don't leak shaders.
			glDeleteShader(vertexShader);

			TST_CORE_ERROR("{0}", std::string(infoLog.begin(), infoLog.end()));

			return;
		}

		// Vertex and fragment shaders are successfully compiled.
		// Now time to link them together into a program.
		// Get a program object.
		m_shaderId =  glCreateProgram();

		// Attach our shaders to our program
		glAttachShader(m_shaderId, vertexShader);
		glAttachShader(m_shaderId, fragmentShader);

		// Link our program
		glLinkProgram(m_shaderId);

		// Note the different functions here: glGetProgram* instead of glGetShader*.
		int isLinked = 0;
		glGetProgramiv(m_shaderId, GL_LINK_STATUS, &isLinked);
		if (!isLinked)
		{
			int maxLength = 0;
			glGetProgramiv(m_shaderId, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<char> infoLog(maxLength);
			glGetProgramInfoLog(m_shaderId, maxLength, &maxLength, infoLog.data());

			// We don't need the program anymore.
			glDeleteProgram(m_shaderId);
			// Don't leak shaders either.
			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			TST_CORE_ERROR("{0}", std::string(infoLog.begin(), infoLog.end()));

			return;
		}

		// Always detach shaders after a successful link.
		glDetachShader(m_shaderId, vertexShader);
		glDetachShader(m_shaderId, fragmentShader);
    }

	Shader::~Shader()
	{
		glDeleteProgram(m_shaderId);
	}


	void Shader::bind() const
	{
		glUseProgram(m_shaderId);
	} 

	void Shader::unbind() const
	{
		glUseProgram(0);
	}


}
