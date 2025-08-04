#include "tstpch.h"
#include "Material.hpp"

#include "platform/OpenGl/OpenGLShader.hpp"

#include <filesystem>

namespace tst
{
    Material::Material(const std::string& name)
        : m_Name(name)
    {
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
        if (!shader) { return; }

        shader->uploadVector3f(m_MaterialProperties.ambient, "u_Material.ambient");
        shader->uploadVector3f(m_MaterialProperties.diffuse, "u_Material.diffuse");
        shader->uploadVector3f(m_MaterialProperties.specular, "u_Material.specular");
        shader->uploadVector3f(m_MaterialProperties.emissive, "u_Material.emissive");
        shader->uploadFloat1(m_MaterialProperties.shininess, "u_Material.shininess");
        shader->uploadFloat1(m_MaterialProperties.opacity, "u_Material.opacity");
        shader->uploadFloat1(m_MaterialProperties.metallic, "u_Material.metallic");

        int textureSlot = 1;

        if (m_DiffuseMap)
        {

            m_DiffuseMap->bind(textureSlot);
            shader->uploadInt1(textureSlot, "u_Material.diffuseMap");
            shader->uploadBool(true, "u_Material.hasDiffuseMap");
            textureSlot++;
        }
        else
        {
            shader->uploadBool(false, "u_Material.hasDiffuseMap");
        }

        if (m_SpecularMap)
        {
            m_SpecularMap->bind(textureSlot);
            shader->uploadInt1(textureSlot, "u_Material.specularMap");
            shader->uploadBool(true, "u_Material.hasSpecularMap");
            textureSlot++;
        }
        else
        {
            shader->uploadBool(false, "u_Material.hasSpecularMap");
        }

        if (m_NormalMap)
        {
            m_NormalMap->bind(textureSlot);
            shader->uploadInt1(textureSlot, "u_Material.normalMap");
            shader->uploadBool(true, "u_Material.hasNormalMap");
            textureSlot++;
        }
        else
        {
            shader->uploadBool(false, "u_Material.hasNormalMap");
        }

        if (m_HeightMap)
        {
            m_HeightMap->bind(textureSlot);
            shader->uploadInt1(textureSlot, "u_Material.heightMap");
            shader->uploadBool(true, "u_Material.hasHeightMap");
            textureSlot++;
        }
        else
        {
            shader->uploadBool(false, "u_Material.hasHeightMap");
        }
    }

    RefPtr<Material> Material::create(const std::string& name)
    {
        return make_reference<Material>(name);
    }

    RefPtr<Material> Material::createDefault()
    {
        auto material = create("DefaultMaterial");

        material->setAmbient({ 0.2f, 0.2f, 0.2f });
        material->setDiffuse({ 0.8f, 0.8f, 0.8f });
        material->setSpecular({ 0.2f, 0.2f, 0.2f });
        material->setEmissive({ 0.0f, 0.0f, 0.0f });
        material->setShininess(32.0f);
        material->setOpacity(1.0f);
		material->setBackfaceCulling(false);
		material->setMetallic(0.0f);

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
