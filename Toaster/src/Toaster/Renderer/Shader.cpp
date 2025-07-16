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

	std::shared_ptr<Shader> Shader::create(const std::string& vertexPath, const std::string& fragmentPath)
    {
		switch (Renderer::getApi())
		{
		case RendererAPI::API::None:
		{
			TST_ASSERT(false, "Render Api cannot be None!");
			return nullptr;
		}
		case RendererAPI::API::OpenGL:
		{
			return std::make_shared<OpenGLShader>(vertexPath, fragmentPath);
		}
		case RendererAPI::API::Vulkan:
		{
			TST_ASSERT(false, "Render Api [Vulkan] has not yet been implemented...");
			return nullptr;
		}
		}
    }

}
