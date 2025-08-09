#include "tstpch.h"
#include "Material.hpp"

#include "platform/OpenGl/OpenGLShader.hpp"

#include <filesystem>
#include "MaterialSystem.hpp"

namespace tst
{
    const std::string Material::s_baseUniformName = "u_Material.";

    Material::Material(const std::string& name)
        : m_Name(name)
    {

        setProperty_vec3("diffuse", glm::vec3{ 0.0f, 0.0f, 0.0f });
        setProperty_vec3("specular", glm::vec3{ 0.8f, 0.8f, 0.8f });
        setProperty_vec3("emissive", glm::vec3{ 1.0f, 1.0f, 1.0f });
        setProperty_vec3("ambient", glm::vec3{ 0.0f, 0.0f, 0.0f });

        setProperty_float("shininess", 64.0);
        setProperty_float("opacity", 1.0f);
        setProperty_float("metallic", 0.0f);
        setProperty_float("roughness", 0.6f);

        m_RenderState = {};

        //  -=[Legacy]=-
        //m_MaterialProperties = {};
        //  -=[Legacy]=-

        m_DiffuseMap = nullptr;
        m_SpecularMap = nullptr;
        m_NormalMap = nullptr;
        m_HeightMap = nullptr;
    }


    void Material::setProperty_int(const std::string& property_name, int value)
    {
        m_MaterialProperties[property_name] = value;
        makeDirty();
    }

    void Material::setProperty_float(const std::string& property_name, float value)
    {
        m_MaterialProperties[property_name] = value;
        makeDirty();
    }

    void Material::setProperty_bool(const std::string& property_name, bool value)
    {
        m_MaterialProperties[property_name] = value;
        makeDirty();
    }

    void Material::setProperty_vec3(const std::string& property_name, glm::vec3 value)
    {
        m_MaterialProperties[property_name] = value;
        makeDirty();
    }

    void Material::setProperty_vec4(const std::string& property_name, glm::vec4 value)
    {
        m_MaterialProperties[property_name] = value;
        makeDirty();
    }

    void Material::setProperty_mat3(const std::string& property_name, glm::mat3 value)
    {
        m_MaterialProperties[property_name] = value;
        makeDirty();
    }

    void Material::setProperty_mat4(const std::string& property_name, glm::mat4 value)
    {
        m_MaterialProperties[property_name] = value;
        makeDirty();
    }


    int Material::getProperty_int(const std::string& property_name) const
    {
        auto it = m_MaterialProperties.find(property_name);
        if (it != m_MaterialProperties.end())
        {
            if (auto value = std::get_if<int>(&it->second))
            {
                return *value;
            }
        }
        TST_CORE_ERROR("Failed to find material property {0} : [Int]", property_name);
        return -1;
    }

    float Material::getProperty_float(const std::string& property_name) const
    {
        auto it = m_MaterialProperties.find(property_name);
        if (it != m_MaterialProperties.end())
        {
            if (auto value = std::get_if<float>(&it->second))
            {
                return *value;
            }
        }
        TST_CORE_ERROR("Failed to find material property {0} : [Float]", property_name);
        return -1.0f;
    }

    bool Material::getProperty_bool(const std::string& property_name) const
    {
        auto it = m_MaterialProperties.find(property_name);
        if (it != m_MaterialProperties.end())
        {
            if (auto value = std::get_if<bool>(&it->second))
            {
                return *value;
            }
        }
        TST_CORE_ERROR("Failed to find material property {0} : [Bool]", property_name);
        return false;
    }

    glm::vec3 Material::getProperty_vec3(const std::string& property_name) const
    {
        auto it = m_MaterialProperties.find(property_name);
        if (it != m_MaterialProperties.end())
        {
            if (auto value = std::get_if<glm::vec3>(&it->second))
            {
                return *value;
            }
        }
        TST_CORE_ERROR("Failed to find material property {0} : [Vec3]", property_name);
        return {};
    }

    glm::vec4 Material::getProperty_vec4(const std::string& property_name) const
    {
        auto it = m_MaterialProperties.find(property_name);
        if (it != m_MaterialProperties.end())
        {
            if (auto value = std::get_if<glm::vec4>(&it->second))
            {
                return *value;
            }
        }
        TST_CORE_ERROR("Failed to find material property {0} : [Vec4]", property_name);
        return {};
    }

    glm::mat3 Material::getProperty_mat3(const std::string& property_name) const
    {
        auto it = m_MaterialProperties.find(property_name);
        if (it != m_MaterialProperties.end())
        {
            if (auto value = std::get_if<glm::mat3>(&it->second))
            {
                return *value;
            }
        }
        TST_CORE_ERROR("Failed to find material property {0} : [Mat3]", property_name);
        return {};
    }

    glm::mat4 Material::getProperty_mat4(const std::string& property_name) const
    {
        auto it = m_MaterialProperties.find(property_name);
        if (it != m_MaterialProperties.end())
        {
            if (auto value = std::get_if<glm::mat4>(&it->second))
            {
                return *value;
            }
        }
        TST_CORE_ERROR("Failed to find material property {0} : [Mat4]", property_name);
        return {};
    }

    bool Material::hasProperty(const std::string& property_name) const
    {
        // Note: C++ 23 Has the contains method
        return m_MaterialProperties.find(property_name) != m_MaterialProperties.end();
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

        for (const auto& [name, value] : m_MaterialProperties)
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

        material->setProperty_vec3("ambient", glm::vec3{ 0.2f, 0.2f, 0.2f });
        material->setProperty_vec3("diffuse", glm::vec3{ 0.8f, 0.8f, 0.8f });
        material->setProperty_vec3("specular", glm::vec3{ 0.2f, 0.2f, 0.2f });
        material->setProperty_vec3("emissive", glm::vec3{ 0.0f, 0.0f, 0.0f });
        material->setProperty_float("shininess", 32.0f);
        material->setProperty_float("opacity", 1.0f);
        material->setProperty_float("metallic", 0.0f);
        material->setProperty_float("roughness", 1.0f);

        return material;
    }


    MaterialInstance::MaterialInstance(MaterialID base_material_id, const std::string& instance_name)
        : Material(instance_name), m_BaseMaterialID(base_material_id)
    {
        TST_CORE_INFO("Created MaterialInstance '{0}' based on MaterialID {1}", instance_name, base_material_id);
    }

    void MaterialInstance::setProperty_int(const std::string& property_name, int value)
    {
        m_PropertyOverrides[property_name] = value;
        makeDirty();
    }

    void MaterialInstance::setProperty_float(const std::string& property_name, float value)
    {
        m_PropertyOverrides[property_name] = value;
        makeDirty();
    }

    void MaterialInstance::setProperty_bool(const std::string& property_name, bool value)
    {
        m_PropertyOverrides[property_name] = value;
        makeDirty();
    }

    void MaterialInstance::setProperty_vec3(const std::string& property_name, glm::vec3 value)
    {
        m_PropertyOverrides[property_name] = value;
        makeDirty();
    }

    void MaterialInstance::setProperty_vec4(const std::string& property_name, glm::vec4 value)
    {
        m_PropertyOverrides[property_name] = value;
        makeDirty();
    }

    void MaterialInstance::setProperty_mat3(const std::string& property_name, glm::mat3 value)
    {
        m_PropertyOverrides[property_name] = value;
        makeDirty();
    }

    void MaterialInstance::setProperty_mat4(const std::string& property_name, glm::mat4 value)
    {
        m_PropertyOverrides[property_name] = value;
        makeDirty();
    }


    int MaterialInstance::getProperty_int(const std::string& property_name) const
    {
        auto it = m_PropertyOverrides.find(property_name);
        if (it != m_PropertyOverrides.end())
        {
            if (auto value = std::get_if<int>(&it->second))
            {
                return *value;
            }
        }

        if (auto baseMaterial = getBaseMaterial())
        {
            return baseMaterial->getProperty_int(property_name);
        }

        TST_CORE_ERROR("Material property {0} [Int] not found in instance or base material", property_name);
        return -1;
    }

    float MaterialInstance::getProperty_float(const std::string& property_name) const
    {
        auto it = m_PropertyOverrides.find(property_name);
        if (it != m_PropertyOverrides.end())
        {
            if (auto value = std::get_if<float>(&it->second))
            {
                return *value;
            }
        }

        if (auto baseMaterial = getBaseMaterial())
        {
            return baseMaterial->getProperty_float(property_name);
        }

        TST_CORE_ERROR("Material property {0} [Float] not found in instance or base material", property_name);
        return -1.0f;
    }

    bool MaterialInstance::getProperty_bool(const std::string& property_name) const
    {
        auto it = m_PropertyOverrides.find(property_name);
        if (it != m_PropertyOverrides.end())
        {
            if (auto value = std::get_if<bool>(&it->second))
            {
                return *value;
            }
        }

        if (auto baseMaterial = getBaseMaterial())
        {
            return baseMaterial->getProperty_bool(property_name); // Fixed: was calling getProperty_int
        }

        TST_CORE_ERROR("Material property {0} [Bool] not found in instance or base material", property_name);
        return false;
    }

    glm::vec3 MaterialInstance::getProperty_vec3(const std::string& property_name) const
    {
        auto it = m_PropertyOverrides.find(property_name);
        if (it != m_PropertyOverrides.end())
        {
            if (auto value = std::get_if<glm::vec3>(&it->second))
            {
                return *value;
            }
        }

        if (auto baseMaterial = getBaseMaterial())
        {
            return baseMaterial->getProperty_vec3(property_name);
        }

        TST_CORE_ERROR("Material property {0} [Vec3] not found in instance or base material", property_name);
        return {};
    }

    glm::vec4 MaterialInstance::getProperty_vec4(const std::string& property_name) const
    {
        auto it = m_PropertyOverrides.find(property_name);
        if (it != m_PropertyOverrides.end())
        {
            if (auto value = std::get_if<glm::vec4>(&it->second))
            {
                return *value;
            }
        }

        if (auto baseMaterial = getBaseMaterial())
        {
            return baseMaterial->getProperty_vec4(property_name);
        }

        TST_CORE_ERROR("Material property {0} [Vec4] not found in instance or base material", property_name);
        return {};
    }

    glm::mat3 MaterialInstance::getProperty_mat3(const std::string& property_name) const
    {
        auto it = m_PropertyOverrides.find(property_name);
        if (it != m_PropertyOverrides.end())
        {
            if (auto value = std::get_if<glm::mat3>(&it->second))
            {
                return *value;
            }
        }

        if (auto baseMaterial = getBaseMaterial())
        {
            return baseMaterial->getProperty_mat3(property_name);
        }

        TST_CORE_ERROR("Material property {0} [Mat3] not found in instance or base material", property_name);
        return {};
    }

    glm::mat4 MaterialInstance::getProperty_mat4(const std::string& property_name) const
    {
        auto it = m_PropertyOverrides.find(property_name);
        if (it != m_PropertyOverrides.end())
        {
            if (auto value = std::get_if<glm::mat4>(&it->second))
            {
                return *value;
            }
        }

        if (auto baseMaterial = getBaseMaterial())
        {
            return baseMaterial->getProperty_mat4(property_name);
        }

        TST_CORE_ERROR("Material property {0} [Mat4] not found in instance or base material", property_name);
        return {};
    }

    bool MaterialInstance::hasProperty(const std::string& property_name) const
    {
        // Check if we have an override
        if (m_PropertyOverrides.find(property_name) != m_PropertyOverrides.end())
        {
            return true;
        }

        // Check base material
        if (auto baseMaterial = getBaseMaterial())
        {
            return baseMaterial->hasProperty(property_name);
        }

        return false;
    }

    const RefPtr<Texture2D>& MaterialInstance::getDiffuseMap() const
    {
        return m_DiffuseMapOverride ? m_DiffuseMapOverride : Material::getDiffuseMap();
    }

    const RefPtr<Texture2D>& MaterialInstance::getSpecularMap() const
    {
        return m_SpecularMapOverride ? m_SpecularMapOverride : Material::getSpecularMap();
    }

    const RefPtr<Texture2D>& MaterialInstance::getNormalMap() const
    {
        return m_NormalMapOverride ? m_NormalMapOverride : Material::getNormalMap();
    }

    const RefPtr<Texture2D>& MaterialInstance::getHeightMap() const
    {
        return m_HeightMapOverride ? m_HeightMapOverride : Material::getHeightMap();
    }

    bool MaterialInstance::hasTexture() const
    {
        return m_DiffuseMapOverride || m_SpecularMapOverride ||
            m_NormalMapOverride || m_HeightMapOverride ||
            Material::hasTexture();
    }

    void MaterialInstance::bind(const RefPtr<Shader>& shader) const
    {
        if (!shader)
        {
            TST_CORE_ERROR("Shader is null, cannot bind material instance");
            return;
        }

        m_lastBoundShader = shader;
        uploadUniforms(shader);
        bindTextures();
    }

    void MaterialInstance::unbind() const
    {
        if (m_DiffuseMapOverride)  m_DiffuseMapOverride->unbind();
        if (m_SpecularMapOverride) m_SpecularMapOverride->unbind();
        if (m_NormalMapOverride)   m_NormalMapOverride->unbind();
        if (m_HeightMapOverride)   m_HeightMapOverride->unbind();

        Material::unbind();
    }

    bool MaterialInstance::hasPropertyOverride(const std::string& property_name) const
    {
        return m_PropertyOverrides.find(property_name) != m_PropertyOverrides.end();
    }

    void MaterialInstance::resetProperty(const std::string& property_name)
    {
        auto it = m_PropertyOverrides.find(property_name);
        if (it != m_PropertyOverrides.end())
        {
            m_PropertyOverrides.erase(it);
            makeDirty();
        }
    }

    void MaterialInstance::resetTextureOverride(const std::string& texture_type)
    {
        if (texture_type == "diffuse" || texture_type == "Diffuse")
        {
            m_DiffuseMapOverride = nullptr;
        }
        else if (texture_type == "specular" || texture_type == "Specular")
        {
            m_SpecularMapOverride = nullptr;
        }
        else if (texture_type == "normal" || texture_type == "Normal")
        {
            m_NormalMapOverride = nullptr;
        }
        else if (texture_type == "height" || texture_type == "Height")
        {
            m_HeightMapOverride = nullptr;
        }
        makeDirty();
    }

    void MaterialInstance::setDiffuseMapOverride(const RefPtr<Texture2D>& diffuse_map)
    {
        m_DiffuseMapOverride = diffuse_map;
        makeDirty();
    }

    void MaterialInstance::setSpecularMapOverride(const RefPtr<Texture2D>& specular_map)
    {
        m_SpecularMapOverride = specular_map;
        makeDirty();
    }

    void MaterialInstance::setNormalMapOverride(const RefPtr<Texture2D>& normal_map)
    {
        m_NormalMapOverride = normal_map;
        makeDirty();
    }

    void MaterialInstance::setHeightMapOverride(const RefPtr<Texture2D>& height_map)
    {
        m_HeightMapOverride = height_map;
        makeDirty();
    }

    RefPtr<MaterialInstance> MaterialInstance::create(MaterialID base_material_id, const std::string& instance_name)
    {
        return make_reference<MaterialInstance>(base_material_id, instance_name);
    }

    void MaterialInstance::uploadUniforms(const RefPtr<Shader>& shader) const
    {
        // First upload base material properties
        auto baseMaterial = getBaseMaterial();
        if (baseMaterial)
        {
            baseMaterial->uploadUniforms(shader);
        }

        // Then upload overrides
        for (const auto& [name, value] : m_PropertyOverrides)
        {
            const std::string& fullName = Material::s_baseUniformName + name;
            if (!shader->hasUniform(fullName)) { continue; }

            std::visit(MaterialUniformUploader{ shader, name }, value);
        }
    }

    void MaterialInstance::bindTextures() const
    {
        static RefPtr<Texture2D> whiteTexture = Texture2D::create(0xffffffff);
        int textureSlot = 1;

        // Bind diffuse texture (override or base)
        RefPtr<Texture2D> diffuseTex = m_DiffuseMapOverride ? m_DiffuseMapOverride :
            (Material::getDiffuseMap() ? Material::getDiffuseMap() : whiteTexture);
        diffuseTex->bind(textureSlot);
        if (auto shader = m_lastBoundShader.lock())
        {
            shader->uploadInt1(textureSlot, "u_Material.diffuseMap");
            shader->uploadBool(m_DiffuseMapOverride != nullptr || Material::getDiffuseMap() != nullptr, "u_Material.hasDiffuseMap");
        }
        textureSlot++;

        // Bind specular texture (override or base)
        RefPtr<Texture2D> specularTex = m_SpecularMapOverride ? m_SpecularMapOverride :
            (Material::getSpecularMap() ? Material::getSpecularMap() : whiteTexture);
        specularTex->bind(textureSlot);
        if (auto shader = m_lastBoundShader.lock())
        {
            shader->uploadInt1(textureSlot, "u_Material.specularMap");
            shader->uploadBool(m_SpecularMapOverride != nullptr || Material::getSpecularMap() != nullptr, "u_Material.hasSpecularMap");
        }
        textureSlot++;

        // Bind normal texture (override or base)
        RefPtr<Texture2D> normalTex = m_NormalMapOverride ? m_NormalMapOverride :
            (Material::getNormalMap() ? Material::getNormalMap() : whiteTexture);
        normalTex->bind(textureSlot);
        if (auto shader = m_lastBoundShader.lock())
        {
            shader->uploadInt1(textureSlot, "u_Material.normalMap");
            shader->uploadBool(m_NormalMapOverride != nullptr || Material::getNormalMap() != nullptr, "u_Material.hasNormalMap");
        }
        textureSlot++;

        // Bind height texture (override or base)
        RefPtr<Texture2D> heightTex = m_HeightMapOverride ? m_HeightMapOverride :
            (Material::getHeightMap() ? Material::getHeightMap() : whiteTexture);
        heightTex->bind(textureSlot);
        if (auto shader = m_lastBoundShader.lock())
        {
            shader->uploadInt1(textureSlot, "u_Material.heightMap");
            shader->uploadBool(m_HeightMapOverride != nullptr || Material::getHeightMap() != nullptr, "u_Material.hasHeightMap");
        }
        textureSlot++;
    }

    RefPtr<Material> MaterialInstance::getBaseMaterial() const
    {
        return MaterialSystem::getMaterial(m_BaseMaterialID);
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