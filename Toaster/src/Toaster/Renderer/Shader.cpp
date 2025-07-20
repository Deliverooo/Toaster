#include "tstpch.h"

#include "Shader.hpp"

#include "Renderer.hpp"
#include "platform/OpenGl/OpenGLShader.hpp"

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

	RefPtr<Shader> Shader::create(const std::string& name,const std::string& vertexPath, const std::string& fragmentPath)
    {
        switch (Renderer::getApi())
        {
        case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
        case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLShader>(name, vertexPath, fragmentPath); }
        case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
        }

        TST_ASSERT(false, "Unknown Render API specified");
        return nullptr;
    }

    RefPtr<Shader> Shader::create(const std::string& shaderPath)
    {
        switch (Renderer::getApi())
        {
        case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
        case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLShader>(shaderPath); }
        case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
        }

        TST_ASSERT(false, "Unknown Render API specified");
        return nullptr;
    }

    RefPtr<Shader> Shader::create(const std::string& shaderName, const std::string& shaderPath)
    {
        switch (Renderer::getApi())
        {
        case RendererAPI::API::None: { TST_ASSERT(false, "Render Api cannot be None!"); return nullptr; }
        case RendererAPI::API::OpenGL: { return std::make_shared<OpenGLShader>(shaderName, shaderPath); }
        case RendererAPI::API::Vulkan: { TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented..."); return nullptr; }
        }

        TST_ASSERT(false, "Unknown Render API specified");
        return nullptr;
    }


    void ShaderLib::addShader(const RefPtr<Shader> shader)
    {
        std::string shaderName = shader->getName();

        if (m_shaders.find(shaderName) != m_shaders.end())
        {
            TST_CORE_ERROR("Shader {0} already exists", shaderName);
        }
        m_shaders[shaderName] = shader;
    }

    RefPtr<Shader> ShaderLib::getShader(const std::string& shaderName)
	{
        if (m_shaders.find(shaderName) == m_shaders.end())
        {
            TST_CORE_ERROR("Shader {0} Does Not exist", shaderName);

            return nullptr;
        }
        return m_shaders[shaderName];
	}

    void ShaderLib::removeShader(const std::string& shaderName)
    {
        if (m_shaders.find(shaderName) == m_shaders.end())
        {
            TST_CORE_ERROR("Shader {0} Does Not exist", shaderName);
            return;
        }
        m_shaders.erase(shaderName);
		TST_CORE_INFO("Shader {0} removed successfully", shaderName);
    }

    RefPtr<Shader> ShaderLib::loadShader(const std::string& shaderPath)
    {
        auto shader = Shader::create(shaderPath);
        addShader(shader);
        return shader;
    }

    RefPtr<Shader> ShaderLib::loadShader(const std::string& shaderName, const std::string& shaderPath)
    {
        auto shader = Shader::create(shaderName, shaderPath);
        addShader(shader);
        return shader;
    }

    RefPtr<Shader> ShaderLib::loadShader(const std::string &shaderName, const std::string& vertexPath, const std::string &FragmentPath)
    {
        auto shader = Shader::create(shaderName, vertexPath, FragmentPath);
        addShader(shader);
        return shader;
    }
}
