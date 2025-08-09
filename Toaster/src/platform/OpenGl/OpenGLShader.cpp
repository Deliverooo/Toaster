#include "tstpch.h"
#include "OpenGLShader.hpp"

#include <filesystem>
#include <glad/glad.h>

#include <glm/vec3.hpp>

#include "glm/gtc/type_ptr.hpp"
#include "Toaster/Renderer/RenderCommand.hpp"

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
		GLint currentProgram;
		glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

		if (currentProgram == static_cast<GLint>(m_shaderId)) {
			return; // Already bound, avoid state change
		}

		// Validate program before binding
		GLint isLinked = GL_FALSE;
		glGetProgramiv(m_shaderId, GL_LINK_STATUS, &isLinked);

		if (isLinked == GL_FALSE) {
			TST_CORE_ERROR("Attempting to bind unlinked shader program '{}'", m_shaderName);
			return;
		}

		glUseProgram(m_shaderId);

		// Clear any potential GL errors after binding
		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			TST_CORE_WARN("GL error {} after binding shader '{}'", error, m_shaderName);
		}
	}

	void OpenGLShader::unbind() const
	{
		glUseProgram(0);
	}

	std::string OpenGLShader::getName() const
	{
		return m_shaderName;
	}

	void OpenGLShader::reload() const
	{
		
	}

	void OpenGLShader::shaderDebugInfo() const
	{
		TST_CORE_INFO("Shader '{}' Debug Info:", m_shaderName);
		TST_CORE_INFO("  Shader ID: {}", m_shaderId);

		// Check if shader program is valid
		GLint linked;
		glGetProgramiv(m_shaderId, GL_LINK_STATUS, &linked);
		TST_CORE_INFO("  Link Status: {}", linked ? "SUCCESS" : "FAILED");

		if (!linked) {
			GLint infoLength;
			glGetProgramiv(m_shaderId, GL_INFO_LOG_LENGTH, &infoLength);
			if (infoLength > 0) {
				std::vector<char> infoLog(infoLength);
				glGetProgramInfoLog(m_shaderId, infoLength, nullptr, infoLog.data());
				TST_CORE_ERROR("  Link Error: {}", std::string(infoLog.data()));
			}
		}

		// List all active uniforms
		GLint numUniforms;
		glGetProgramiv(m_shaderId, GL_ACTIVE_UNIFORMS, &numUniforms);
		TST_CORE_INFO("  Active Uniforms: {}", numUniforms);

		for (int i = 0; i < numUniforms; i++) {
			char name[256];
			GLsizei length;
			GLint size;
			GLenum type;
			glGetActiveUniform(m_shaderId, i, sizeof(name), &length, &size, &type, name);
			TST_CORE_INFO("    [{}] {} (type: {}, size: {})", i, name, type, size);
		}
	}

	int OpenGLShader::getUniformLocation(const std::string& name)
	{
		// Use std::string as key to avoid pointer lifetime issues

		auto it = m_uniformLocations.find(name);
		if (it != m_uniformLocations.end())
		{
			return it->second;
		}

		int location = glGetUniformLocation(m_shaderId, name.c_str());

		// Always cache the result, even if it's -1
		m_uniformLocations[name] = location;

		if (location == -1) {
			static std::unordered_set<std::string> warned_uniforms;
			if (warned_uniforms.find(name) == warned_uniforms.end()) {
				TST_CORE_WARN("Uniform '{}' not found in shader '{}'", name, m_shaderName);
				warned_uniforms.insert(name);
			}
		}

		return location;
	}


	bool OpenGLShader::hasUniform(const std::string &name)
	{
	   return getUniformLocation(name) != -1;
	}

	void OpenGLShader::uploadMatrix2f(const glm::mat2& mat, const char* name) { uploadUniformMatrix2f(mat, name); }
	void OpenGLShader::uploadMatrix3f(const glm::mat3& mat, const char* name) { uploadUniformMatrix3f(mat, name); }
	void OpenGLShader::uploadMatrix4f(const glm::mat4& mat, const char* name) { uploadUniformMatrix4f(mat, name); }

	void OpenGLShader::uploadVector1f(const glm::vec1& vec, const char* name) { if (hasUniform(name)) uploadUniform1fv(vec, name); }
	void OpenGLShader::uploadVector2f(const glm::vec2& vec, const char* name) { if (hasUniform(name)) uploadUniform2fv(vec, name); }
	void OpenGLShader::uploadVector3f(const glm::vec3& vec, const char* name) { if (hasUniform(name)) uploadUniform3fv(vec, name); }
	void OpenGLShader::uploadVector4f(const glm::vec4& vec, const char* name) { if (hasUniform(name)) uploadUniform4fv(vec, name); }

	void OpenGLShader::uploadBool(const bool b, const char* name) { if (hasUniform(name)) uploadUniform1i(b, name); }

	void OpenGLShader::uploadIntArray(int* arr, const uint32_t count, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform1iv(location, count, arr);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadInt1(const int x, const char* name) { if (hasUniform(name)) uploadUniform1i(x, name); }
	void OpenGLShader::uploadInt2(const int x, const int y, const char* name) { if (hasUniform(name)) uploadUniform2i(x, y, name); }
	void OpenGLShader::uploadInt3(const int x, const int y, const int z, const char* name) { if (hasUniform(name)) uploadUniform3i(x, y, z, name); }
	void OpenGLShader::uploadInt4(const int x, const int y, const int z, const int w, const char* name) { if (hasUniform(name)) uploadUniform4i(x, y, z, w, name); }

	void OpenGLShader::uploadFloat1(const float x, const char* name) { if (hasUniform(name)) uploadUniform1f(x, name); }
	void OpenGLShader::uploadFloat2(const float x, const float y, const char* name) { if (hasUniform(name)) uploadUniform2f(x, y, name); }
	void OpenGLShader::uploadFloat3(const float x, const float y, const float z, const char* name) { if (hasUniform(name)) uploadUniform3f(x, y, z, name); }
	void OpenGLShader::uploadFloat4(const float x, const float y, const float z, const float w, const char* name) { if (hasUniform(name)) uploadUniform4f(x, y, z, w, name); }

	void OpenGLShader::uploadDouble1(const double x, const char* name) { if (hasUniform(name)) uploadUniform1d(x, name); }
	void OpenGLShader::uploadDouble2(const double x, const double y, const char* name) { if (hasUniform(name)) uploadUniform2d(x, y, name); }
	void OpenGLShader::uploadDouble3(const double x, const double y, const double z, const char* name) { if (hasUniform(name)) uploadUniform3d(x, y, z, name); }
	void OpenGLShader::uploadDouble4(const double x, const double y, const double z, const double w, const char* name) { if (hasUniform(name)) uploadUniform4d(x, y, z, w, name); }

	//---------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------------------------------------------------------------------------

	void OpenGLShader::uploadUniform1i(const int x, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform1i(location, x);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}
	void OpenGLShader::uploadUniform2i(const int x, const int y, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform2i(location, x, y);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform3i(const int x, const int y, const int z, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform3i(location, x, y, z);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform4i(const int x, const int y, const int z, const int w, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform4i(location, x, y, z, w);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}
	void OpenGLShader::uploadUniform1f(const float x, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform1f(location, x);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform2f(const float x, const float y, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform2f(location, x, y);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform3f(const float x, const float y, const float z, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform3f(location, x, y, z);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform4f(const float x, const float y, const float z, const float w, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform4f(location, x, y, z, w);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}
	void OpenGLShader::uploadUniform1d(const double x, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform1d(location, x);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform2d(const double x, const double y, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform2d(location, x, y);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform3d(const double x, const double y, const double z, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform3d(location, x, y, z);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform4d(const double x, const double y, const double z, const double w, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform4d(location, x, y, z, w);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform1iv(const glm::vec<1, int>& vec, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform1iv(location, 1, &vec.x);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform2iv(const glm::vec<2, int>& vec, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform2iv(location, 1, glm::value_ptr(vec));
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform3iv(const glm::vec<3, int>& vec, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform3iv(location, 1, glm::value_ptr(vec));
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform4iv(const glm::vec<4, int>& vec, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform4iv(location, 1, glm::value_ptr(vec));
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform1fv(const glm::vec1& vec, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform1fv(location, 1, &vec.x);
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform2fv(const glm::vec2& vec, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform2fv(location, 1, glm::value_ptr(vec));
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform3fv(const glm::vec3& vec, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform3fv(location, 1, glm::value_ptr(vec));
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniform4fv(const glm::vec4& vec, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniform4fv(location, 1, glm::value_ptr(vec));
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniformMatrix2f(const glm::mat2& mat, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(mat));
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniformMatrix3f(const glm::mat3& mat, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(mat));
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}

	void OpenGLShader::uploadUniformMatrix4f(const glm::mat4& mat, const char* name)
	{
		int location = getUniformLocation(name);
		if (location != -1) {
			glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
			GraphicsAPI::checkError("Uploading uniform '" + std::string(name) + "' to shader '" + m_shaderName + "'");
		}
	}
}
