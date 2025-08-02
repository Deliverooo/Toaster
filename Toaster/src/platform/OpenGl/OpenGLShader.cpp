#include "tstpch.h"
#include "OpenGLShader.hpp"

#include <filesystem>
#include <glad/glad.h>

#include <glm/vec3.hpp>

#include "glm/gtc/type_ptr.hpp"

namespace tst
{

	uint32_t OpenGLShader::compileShader(const std::string &shaderSrc, const unsigned int shaderType)
	{
		TST_PROFILE_FN();

		uint32_t shader = glCreateShader(shaderType);

		const char* source = shaderSrc.c_str();
		glShaderSource(shader, 1, &source, nullptr);

		glCompileShader(shader);

		int isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

		if (!isCompiled)
		{
			int maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<char> infoLog(maxLength);
			glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());

			glDeleteShader(shader);

			TST_CORE_ERROR("{0}", std::string(infoLog.begin(), infoLog.end()));
			return -1;
		}

		return shader;
	}

	OpenGLShader::OpenGLShader(const std::string &name, const std::string& vertexPath, const std::string& FragmentPath)
	{
		TST_PROFILE_FN();

		m_shaderName = name;
		auto vertexSource = readFile(vertexPath);
		auto fragmentSource = readFile(FragmentPath);

		TST_ASSERT(vertexSource.has_value() && fragmentSource.has_value(), "Failed to Read Shader Files!");

		uint32_t vertexShader	= compileShader(vertexSource.value(), GL_VERTEX_SHADER);
		uint32_t fragmentShader = compileShader(fragmentSource.value(), GL_FRAGMENT_SHADER);

		m_shaderId = glCreateProgram();

		glAttachShader(m_shaderId, vertexShader);
		glAttachShader(m_shaderId, fragmentShader);

		glLinkProgram(m_shaderId);

		int isLinked = 0;
		glGetProgramiv(m_shaderId, GL_LINK_STATUS, &isLinked);
		if (!isLinked)
		{
			int maxLength = 0;
			glGetProgramiv(m_shaderId, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<char> infoLog(maxLength);
			glGetProgramInfoLog(m_shaderId, maxLength, &maxLength, infoLog.data());

			glDeleteProgram(m_shaderId);

			glDeleteShader(vertexShader);
			glDeleteShader(fragmentShader);

			TST_CORE_ERROR("{0}", std::string(infoLog.begin(), infoLog.end()));

			return;
		}

		glDetachShader(m_shaderId, vertexShader);
		glDetachShader(m_shaderId, fragmentShader);
	}

	unsigned int OpenGLShader::shaderTypeLookup(std::string type)
	{
		if (type == "Vertex"	  || type == "vertex")		return GL_VERTEX_SHADER;
		if (type == "Fragment"	  || type == "fragment")	return GL_FRAGMENT_SHADER;
		if (type == "Geometry"	  || type == "geometry")	return GL_GEOMETRY_SHADER;
		if (type == "Compute"	  || type == "compute")		return GL_COMPUTE_SHADER;
		if (type == "Tesselation" || type == "tesselation") return GL_TESS_CONTROL_SHADER;
	}

	std::unordered_map<unsigned int, std::string> OpenGLShader::processShader(const std::string& shaderSrc)
	{
		TST_PROFILE_FN();

		std::unordered_map<unsigned int, std::string> shaderSources;


		int pos = shaderSrc.find("#type", 0);
		size_t typeSize = strlen("#type");

		while (pos != std::string::npos)
		{
			size_t endOfLine = shaderSrc.find_first_of("\r\n", pos);

			size_t begin = pos + 1 + typeSize;
			std::string shaderType = shaderSrc.substr(begin, endOfLine - begin);

			size_t nextLine = shaderSrc.find_first_not_of("\r\n", endOfLine);
			pos = shaderSrc.find("#type", nextLine);

			shaderSources[shaderTypeLookup(shaderType)] = shaderSrc.substr(nextLine, pos - (nextLine == std::string::npos ? shaderSources.size() - 1 : nextLine));
		}

		return shaderSources;
	}

	OpenGLShader::OpenGLShader(const std::string& shaderPath)
	{
		TST_PROFILE_FN();

		std::filesystem::path p(shaderPath);
		m_shaderName = p.stem().string();
		auto shaderSrc = readFile(shaderPath);
		TST_ASSERT(shaderSrc.has_value(), "Failed to Read Shader File!");

		auto shaderSources = processShader(shaderSrc.value());

		m_shaderId = glCreateProgram();

		std::vector<unsigned int> shaderIds;
		for (auto &keyVal : shaderSources)
		{

			const GLenum& type = keyVal.first;
			std::string &src = keyVal.second;

			uint32_t shader = compileShader(src, type);

			shaderIds.push_back(shader);
			glAttachShader(m_shaderId, shader);
		}

		glLinkProgram(m_shaderId);

		int isLinked = 0;
		glGetProgramiv(m_shaderId, GL_LINK_STATUS, &isLinked);
		if (!isLinked)
		{
			int maxLength = 0;
			glGetProgramiv(m_shaderId, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<char> infoLog(maxLength);
			glGetProgramInfoLog(m_shaderId, maxLength, &maxLength, infoLog.data());

			glDeleteProgram(m_shaderId);

			for (auto & shader : shaderIds)
			{
				glDeleteShader(shader);
			}


			TST_CORE_ERROR("{0}", std::string(infoLog.begin(), infoLog.end()));

			return;
		}

		for (auto& shader : shaderIds)
		{
			glDetachShader(m_shaderId, shader);
		}
	}

	OpenGLShader::OpenGLShader(const std::string& shaderName, const std::string& shaderPath)
	{
		TST_PROFILE_FN();

		auto shaderSrc = readFile(shaderPath);
		TST_ASSERT(shaderSrc.has_value(), "Failed to Read Shader File!");

		m_shaderName = shaderName;
		auto shaderSources = processShader(shaderSrc.value());

		m_shaderId = glCreateProgram();

		std::vector<unsigned int> shaderIds;
		for (auto& keyVal : shaderSources)
		{

			const GLenum& type = keyVal.first;
			std::string& src = keyVal.second;

			uint32_t shader = compileShader(src, type);

			shaderIds.push_back(shader);
			glAttachShader(m_shaderId, shader);
		}

		glLinkProgram(m_shaderId);

		int isLinked = 0;
		glGetProgramiv(m_shaderId, GL_LINK_STATUS, &isLinked);
		if (!isLinked)
		{
			int maxLength = 0;
			glGetProgramiv(m_shaderId, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<char> infoLog(maxLength);
			glGetProgramInfoLog(m_shaderId, maxLength, &maxLength, infoLog.data());

			glDeleteProgram(m_shaderId);

			for (auto& shader : shaderIds)
			{
				glDeleteShader(shader);
			}


			TST_CORE_ERROR("{0}", std::string(infoLog.begin(), infoLog.end()));

			return;
		}

		for (auto& shader : shaderIds)
		{
			glDetachShader(m_shaderId, shader);
		}
	}


	OpenGLShader::~OpenGLShader()
	{
		TST_PROFILE_FN();

		glDeleteProgram(m_shaderId);
	}


	void OpenGLShader::bind() const
	{
		TST_PROFILE_FN();

		glUseProgram(m_shaderId);
	}

	void OpenGLShader::unbind() const
	{
		TST_PROFILE_FN();

		glUseProgram(0);
	}

	std::string OpenGLShader::getName() const
	{
		return m_shaderName;
	}


	uint32_t OpenGLShader::getId() const
	{
		return m_shaderId;
	}

	void OpenGLShader::reload() const
	{
		
	}


	int OpenGLShader::getUniformLocation(const char* name)
	{
		TST_PROFILE_FN();

		if (m_uniformLocations.find(name) != m_uniformLocations.end())
		{
			return m_uniformLocations[name];
		}

		int location = glGetUniformLocation(m_shaderId, name);
		m_uniformLocations[name] = location;

		return location;
	}

	void OpenGLShader::uploadMatrix2f(const glm::mat2& mat, const char* name) { uploadUniformMatrix2f(mat, name); }
	void OpenGLShader::uploadMatrix3f(const glm::mat3& mat, const char* name) { uploadUniformMatrix3f(mat, name); }
	void OpenGLShader::uploadMatrix4f(const glm::mat4& mat, const char* name) { uploadUniformMatrix4f(mat, name); }


	void OpenGLShader::uploadVector1f(const glm::vec1& vec, const char* name) { uploadUniform1fv(vec, name); }
	void OpenGLShader::uploadVector2f(const glm::vec2& vec, const char* name) { uploadUniform2fv(vec, name); }
	void OpenGLShader::uploadVector3f(const glm::vec3& vec, const char* name) { uploadUniform3fv(vec, name); }
	void OpenGLShader::uploadVector4f(const glm::vec4& vec, const char* name) { uploadUniform4fv(vec, name); }

	void OpenGLShader::uploadBool(const bool b, const char* name) { uploadUniform1i(b, name); }

	void OpenGLShader::uploadIntArray(int* arr, const uint32_t count, const char* name)
	{
		glUniform1iv(glGetUniformLocation(m_shaderId, name), count, arr);
	}

	void OpenGLShader::uploadInt1(const int x, const char* name) { uploadUniform1i(x, name); }
	void OpenGLShader::uploadInt2(const int x, const int y, const char* name) { uploadUniform2i(x, y, name); }
	void OpenGLShader::uploadInt3(const int x, const int y, const int z, const char* name) { uploadUniform3i(x, y, z, name); }
	void OpenGLShader::uploadInt4(const int x, const int y, const int z, const int w, const char* name) { uploadUniform4i(x, y, z, w, name); }

	void OpenGLShader::uploadFloat1(const float x, const char* name) { uploadUniform1f(x, name); }
	void OpenGLShader::uploadFloat2(const float x, const float y, const char* name) { uploadUniform2f(x, y, name); }
	void OpenGLShader::uploadFloat3(const float x, const float y, const float z, const char* name) { uploadUniform3f(x, y, z, name); }
	void OpenGLShader::uploadFloat4(const float x, const float y, const float z, const float w, const char* name) { uploadUniform4f(x, y, z, w, name); }

	void OpenGLShader::uploadDouble1(const double x, const char* name) { uploadUniform1d(x, name); }
	void OpenGLShader::uploadDouble2(const double x, const double y, const char* name) { uploadUniform2d(x, y, name); }
	void OpenGLShader::uploadDouble3(const double x, const double y, const double z, const char* name) { uploadUniform3d(x, y, z, name); }
	void OpenGLShader::uploadDouble4(const double x, const double y, const double z, const double w, const char* name) { uploadUniform4d(x, y, z, w, name); }

	//---------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------

	void OpenGLShader::uploadUniform1i(const int x, const char* name) { glUniform1i(getUniformLocation(name), x); }
	void OpenGLShader::uploadUniform2i(const int x, const int y, const char* name) { glUniform2i(getUniformLocation(name), x, y); }
	void OpenGLShader::uploadUniform3i(const int x, const int y, const int z, const char* name) { glUniform3i(getUniformLocation(name), x, y, z); }
	void OpenGLShader::uploadUniform4i(const int x, const int y, const int z, const int w, const char* name) { glUniform4i(getUniformLocation(name), x, y, z, w); }

	void OpenGLShader::uploadUniform1f(const float x, const char* name) { glUniform1f(getUniformLocation(name), x); }
	void OpenGLShader::uploadUniform2f(const float x, const float y, const char* name) { glUniform2f(getUniformLocation(name), x, y); }
	void OpenGLShader::uploadUniform3f(const float x, const float y, const float z, const char* name) { glUniform3f(getUniformLocation(name), x, y, z);	}
	void OpenGLShader::uploadUniform4f(const float x, const float y, const float z, const float w, const char* name) { glUniform4f(getUniformLocation(name), x, y, z, w); }

	void OpenGLShader::uploadUniform1d(const double x, const char* name) { glUniform1d(getUniformLocation(name), x); }
	void OpenGLShader::uploadUniform2d(const double x, const double y, const char* name) { glUniform2d(getUniformLocation(name), x, y); }
	void OpenGLShader::uploadUniform3d(const double x, const double y, const double z, const char* name) { glUniform3d(getUniformLocation(name), x, y, z); }
	void OpenGLShader::uploadUniform4d(const double x, const double y, const double z, const double w, const char* name) { glUniform4d(getUniformLocation(name), x, y, z, w); }

	void OpenGLShader::uploadUniform1iv(const glm::vec<1, int>& vec, const char* name) { glUniform1iv(getUniformLocation(name), 1, &vec.x); }
	void OpenGLShader::uploadUniform2iv(const glm::vec<2, int>& vec, const char* name) { glUniform2iv(getUniformLocation(name), 1, glm::value_ptr(vec)); }
	void OpenGLShader::uploadUniform3iv(const glm::vec<3, int>& vec, const char* name) { glUniform3iv(getUniformLocation(name), 1, glm::value_ptr(vec)); }
	void OpenGLShader::uploadUniform4iv(const glm::vec<4, int>& vec, const char* name) { glUniform4iv(getUniformLocation(name), 1, glm::value_ptr(vec)); }

	void OpenGLShader::uploadUniform1fv(const glm::vec1& vec, const char* name) { glUniform1fv(getUniformLocation(name), 1, &vec.x); }
	void OpenGLShader::uploadUniform2fv(const glm::vec2& vec, const char* name) { glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(vec)); }
	void OpenGLShader::uploadUniform3fv(const glm::vec3& vec, const char* name) { glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(vec)); }
	void OpenGLShader::uploadUniform4fv(const glm::vec4& vec, const char* name) { glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(vec)); }

	void OpenGLShader::uploadUniformMatrix2f(const glm::mat2& mat, const char* name) { glUniformMatrix2fv(getUniformLocation(name), 1, GL_FALSE,  glm::value_ptr(mat)); }
	void OpenGLShader::uploadUniformMatrix3f(const glm::mat3& mat, const char* name) { glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE,  glm::value_ptr(mat)); }
	void OpenGLShader::uploadUniformMatrix4f(const glm::mat4& mat, const char* name) { glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE,  glm::value_ptr(mat)); }
}
