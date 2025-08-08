#include "tstpch.h"
#include "Material.hpp"

#include "platform/OpenGl/OpenGLShader.hpp"

#include <filesystem>

namespace tst
{
	const std::string Material::s_baseUniformName = "u_Material.";

    Material::Material(const std::string& name)
        : m_Name(name)
    {

        m_CustomProperties["diffuse"]   = glm::vec3{ 0.0f, 0.0f, 0.0f };
        m_CustomProperties["specular"]  = glm::vec3{ 0.8f, 0.8f, 0.8f };
        m_CustomProperties["emissive"]  = glm::vec3{ 1.0f, 1.0f, 1.0f };
        m_CustomProperties["ambient"]   = glm::vec3{ 0.0f, 0.0f, 0.0f };

        m_CustomProperties["shininess"]         = 64.0f;
        m_CustomProperties["opacity"]           = 1.0f;
        m_CustomProperties["metallic"]          = 0.0f;
        m_CustomProperties["roughness"]         = 0.6f;

        m_RenderState = {};

        m_MaterialProperties = {};

        m_DiffuseMap  = nullptr;
        m_SpecularMap = nullptr;
        m_NormalMap   = nullptr;
        m_HeightMap   = nullptr;
    }

    bool Material::hasTexture() const
    {
        return m_DiffuseMap || m_SpecularMap || m_NormalMap || m_HeightMap;
    }

    void Material::bind(const RefPtr<Shader>& shader) const
    {
        if (!shader)
        {
            TST_CORE_ERROR("Shader is null, cannot bind material");
            return;
		}

		m_lastBoundShader = shader;
        uploadUniforms(shader);
        bindTextures();


    }

    void Material::bindTextures() const
    {
        static RefPtr<Texture2D> whiteTexture = Texture2D::create(0xffffffff);
        int textureSlot = 1;


        RefPtr<Texture2D> diffuseTex = (m_DiffuseMap == nullptr) ? whiteTexture : m_DiffuseMap;
        diffuseTex->bind(textureSlot);
        if (auto shader = m_lastBoundShader.lock())
        {
            shader->uploadInt1(textureSlot, "u_Material.diffuseMap");
            shader->uploadBool(m_DiffuseMap != nullptr, "u_Material.hasDiffuseMap");
        }
        textureSlot++;

        RefPtr<Texture2D> specularTex = (m_SpecularMap == nullptr) ? whiteTexture : m_SpecularMap;
        specularTex->bind(textureSlot);
        if (auto shader = m_lastBoundShader.lock())
        {
            shader->uploadInt1(textureSlot, "u_Material.specularMap");
            shader->uploadBool(m_SpecularMap != nullptr, "u_Material.hasSpecularMap");
        }
        textureSlot++;

        RefPtr<Texture2D> normalTex = (m_NormalMap == nullptr) ? whiteTexture : m_NormalMap;
        normalTex->bind(textureSlot);
        if (auto shader = m_lastBoundShader.lock())
        {
            shader->uploadInt1(textureSlot, "u_Material.normalMap");
            shader->uploadBool(m_NormalMap != nullptr, "u_Material.hasNormalMap");
        }
        textureSlot++;

        RefPtr<Texture2D> heightTex = (m_HeightMap == nullptr) ? whiteTexture : m_HeightMap;
        heightTex->bind(textureSlot);
        if (auto shader = m_lastBoundShader.lock())
        {
            shader->uploadInt1(textureSlot, "u_Material.heightMap");
            shader->uploadBool(m_HeightMap != nullptr, "u_Material.hasHeightMap");
        }
        textureSlot++;

        
    }

    struct Material::MaterialUniformUploader
    {
        const RefPtr<Shader>& shader;
        const std::string& uniformName;

        void operator()(float value) const {
            shader->uploadFloat1(value, (Material::s_baseUniformName + uniformName).c_str());
        }

        void operator()(int value) const {
            shader->uploadInt1(value, (Material::s_baseUniformName + uniformName).c_str());
        }

        void operator()(bool value) const {
            shader->uploadBool(value, (Material::s_baseUniformName + uniformName).c_str());
        }

        void operator()(const glm::vec3& value) const {
            shader->uploadVector3f(value, (Material::s_baseUniformName + uniformName).c_str());
        }

        void operator()(const glm::vec4& value) const {
            shader->uploadVector4f(value, (Material::s_baseUniformName + uniformName).c_str());
        }

        void operator()(const glm::mat3& value) const {
            shader->uploadMatrix3f(value, (Material::s_baseUniformName + uniformName).c_str());
        }

        void operator()(const glm::mat4& value) const {
            shader->uploadMatrix4f(value, (Material::s_baseUniformName + uniformName).c_str());
        }
    };

    void Material::uploadUniforms(const RefPtr<Shader>& shader) const
    {

        for (const auto& [name, value] : m_CustomProperties)
        {
			const std::string& fullName = Material::s_baseUniformName + name;
            if (!shader->hasUniform(fullName)) { continue; }

            // Basically a way to provide a function that executes based on the type of the variant
            std::visit(MaterialUniformUploader{ shader, name }, value);
        }
    }



    void Material::unbind() const
    {
        if (m_DiffuseMap)  m_DiffuseMap->unbind();
        if (m_SpecularMap) m_SpecularMap->unbind();
        if (m_NormalMap)   m_NormalMap->unbind();
		if (m_HeightMap)   m_HeightMap->unbind();
    }


    RefPtr<Material> Material::create(const std::string& name)
    {
        return make_reference<Material>(name);
    }

    RefPtr<Material> Material::createDefault()
    {
        auto material = create("DefaultMaterial");

        material->setProperty("u_Material.ambient", glm::vec3{ 0.2f, 0.2f, 0.2f });
        material->setProperty("u_Material.diffuse", glm::vec3{ 0.8f, 0.8f, 0.8f });
        material->setProperty("u_Material.specular", glm::vec3{ 0.2f, 0.2f, 0.2f });
        material->setProperty("u_Material.emissive", glm::vec3{ 0.0f, 0.0f, 0.0f });
        material->setProperty("u_Material.shininess", 32.0f);
        material->setProperty("u_Material.opacity", 1.0f);
		material->setProperty("u_Material.metallic", 0.0f);
		material->setProperty("u_Material.roughness", 1.0f);

        return material;
    }

    void MaterialLibrary::addMaterial(const RefPtr<Material>& material)
    {
        if (!material) { return; }

        m_materialIndices[material->getName()] = static_cast<uint32_t>(m_materials.size());
        m_materials.push_back(material);
    }

    RefPtr<Material> MaterialLibrary::getMaterial(const std::string& name) const
    {
        auto it = m_materialIndices.find(name);
        if (it != m_materialIndices.end())
        {
            return m_materials[it->second];
        }

        return nullptr;
    }

    RefPtr<Material> MaterialLibrary::getMaterial(uint32_t index) const
    {
        return (index >= m_materials.size()) ? nullptr : m_materials[index];
    }

    bool MaterialLibrary::hasMaterial(const std::string& name) const
    {
        return m_materialIndices.find(name) != m_materialIndices.end();
    }

}
