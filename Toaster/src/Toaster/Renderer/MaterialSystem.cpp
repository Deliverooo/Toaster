#include "tstpch.h"
#include "MaterialSystem.hpp"

namespace tst
{
    // Static member definitions
    std::unordered_map<MaterialID, RefPtr<Material>> MaterialSystem::s_Materials;
    std::unordered_map<std::string, MaterialID> MaterialSystem::s_MaterialNameMap;
    std::unordered_map<MaterialInstanceID, RefPtr<MaterialInstance>> MaterialSystem::s_MaterialInstances;
    std::unordered_map<std::string, MaterialInstanceID> MaterialSystem::s_MaterialInstanceNameMap;
    bool MaterialSystem::s_initialized = false;

    static MaterialID s_NextMaterialID = 1;
    static MaterialInstanceID s_NextMaterialInstanceID = 1;

    void MaterialSystem::init()
    {
        if (s_initialized)
        {
            TST_CORE_WARN("MaterialSystem already initialized");
            return;
        }

        TST_CORE_INFO("Initializing MaterialSystem");

        // Create default material
        auto defaultMaterial = Material::createDefault();
        s_Materials[TST_DEFAULT_MATERIAL] = defaultMaterial;
        s_MaterialNameMap["Default"] = TST_DEFAULT_MATERIAL;

        s_NextMaterialID = TST_DEFAULT_MATERIAL + 1;
        s_initialized = true;

        TST_CORE_INFO("MaterialSystem initialized with {} materials", s_Materials.size());
    }

    void MaterialSystem::terminate()
    {
        if (!s_initialized)
        {
            TST_CORE_WARN("MaterialSystem not initialized");
            return;
        }

        TST_CORE_INFO("Terminating MaterialSystem");

        s_Materials.clear();
        s_MaterialNameMap.clear();
        s_MaterialInstances.clear();
        s_MaterialInstanceNameMap.clear();
        s_initialized = false;

        TST_CORE_INFO("MaterialSystem terminated");
    }

    MaterialID MaterialSystem::createMaterial(const std::string& material_name)
    {
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return TST_INVALID_MATERIAL;
        }

        // Check if material already exists
        auto it = s_MaterialNameMap.find(material_name);
        if (it != s_MaterialNameMap.end())
        {
            TST_CORE_WARN("Material '{0}' already exists, returning existing ID", material_name);
            return it->second;
        }

        MaterialID newID = s_NextMaterialID++;
        auto material = Material::create(material_name);

        s_Materials[newID] = material;
        s_MaterialNameMap[material_name] = newID;

        TST_CORE_INFO("Created material '{0}' with ID {1}", material_name, newID);
        return newID;
    }

    MaterialID MaterialSystem::loadMaterial(const std::string& filepath)
    {
        // TODO: Implement material loading from file
        TST_CORE_WARN("MaterialSystem::loadMaterial not implemented yet");
        return createMaterial("LoadedMaterial_" + filepath);
    }

    RefPtr<Material>& MaterialSystem::getMaterial(const std::string& material_name)
    {
        static RefPtr<Material> nullMaterial;

        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return nullMaterial;
        }

        auto it = s_MaterialNameMap.find(material_name);
        if (it == s_MaterialNameMap.end())
        {
            TST_CORE_ERROR("Material '{0}' not found", material_name);
            return nullMaterial;
        }

        return s_Materials[it->second];
    }

    RefPtr<Material>& MaterialSystem::getMaterial(MaterialID id)
    {
        static RefPtr<Material> nullMaterial;

        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return nullMaterial;
        }

        auto it = s_Materials.find(id);
        if (it == s_Materials.end())
        {
            TST_CORE_ERROR("Material with ID {0} not found", id);
            return nullMaterial;
        }

        return it->second;
    }

    MaterialID MaterialSystem::getMaterialID(const std::string& material_name)
    {
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return TST_INVALID_MATERIAL;
        }

        auto it = s_MaterialNameMap.find(material_name);
        if (it == s_MaterialNameMap.end())
        {
            TST_CORE_ERROR("Material '{0}' not found", material_name);
            return TST_INVALID_MATERIAL;
        }

        return it->second;
    }

    void MaterialSystem::releaseMaterial(MaterialID material_id)
    {
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return;
        }

        if (material_id == TST_DEFAULT_MATERIAL)
        {
            TST_CORE_WARN("Cannot release default material");
            return;
        }

        auto it = s_Materials.find(material_id);
        if (it == s_Materials.end())
        {
            TST_CORE_WARN("Material with ID {0} not found for release", material_id);
            return;
        }

        // Remove from name map
        std::string materialName = it->second->getName();
        s_MaterialNameMap.erase(materialName);

        // Remove from materials map
        s_Materials.erase(it);

        TST_CORE_INFO("Released material '{0}' with ID {1}", materialName, material_id);
    }

    MaterialInstanceID MaterialSystem::createMaterialInstance(MaterialID base_material_id, const std::string& instance_name)
    {
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return TST_INVALID_MATERIAL_INSTANCE;
        }

        // Check if base material exists
        auto baseMaterialIt = s_Materials.find(base_material_id);
        if (baseMaterialIt == s_Materials.end())
        {
            TST_CORE_ERROR("Base material with ID {0} not found", base_material_id);
            return TST_INVALID_MATERIAL_INSTANCE;
        }

        // Check if instance name already exists
        auto instanceIt = s_MaterialInstanceNameMap.find(instance_name);
        if (instanceIt != s_MaterialInstanceNameMap.end())
        {
            TST_CORE_WARN("Material instance '{0}' already exists, returning existing ID", instance_name);
            return instanceIt->second;
        }

        MaterialInstanceID newInstanceID = s_NextMaterialInstanceID++;
        auto materialInstance = MaterialInstance::create(base_material_id, instance_name);

        s_MaterialInstances[newInstanceID] = materialInstance;
        s_MaterialInstanceNameMap[instance_name] = newInstanceID;

        TST_CORE_INFO("Created material instance '{0}' with ID {1} based on material ID {2}",
            instance_name, newInstanceID, base_material_id);
        return newInstanceID;
    }

    RefPtr<MaterialInstance>& MaterialSystem::getMaterialInstance(MaterialInstanceID material_instance_id)
    {
        static RefPtr<MaterialInstance> nullMaterialInstance;

        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return nullMaterialInstance;
        }

        auto it = s_MaterialInstances.find(material_instance_id);
        if (it == s_MaterialInstances.end())
        {
            TST_CORE_ERROR("Material instance with ID {0} not found", material_instance_id);
            return nullMaterialInstance;
        }

        return it->second;
    }

    MaterialInstanceID MaterialSystem::getMaterialInstanceID(const std::string& instance_name)
    {
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return TST_INVALID_MATERIAL_INSTANCE;
        }

        auto it = s_MaterialInstanceNameMap.find(instance_name);
        if (it == s_MaterialInstanceNameMap.end())
        {
            TST_CORE_ERROR("Material instance '{0}' not found", instance_name);
            return TST_INVALID_MATERIAL_INSTANCE;
        }

        return it->second;
    }

    void MaterialSystem::releaseMaterialInstance(MaterialInstanceID instance_id)
    {
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return;
        }

        auto it = s_MaterialInstances.find(instance_id);
        if (it == s_MaterialInstances.end())
        {
            TST_CORE_WARN("Material instance with ID {0} not found for release", instance_id);
            return;
        }

        // Remove from name map
        std::string instanceName = it->second->getName();
        s_MaterialInstanceNameMap.erase(instanceName);

        // Remove from instances map
        s_MaterialInstances.erase(it);

        TST_CORE_INFO("Released material instance '{0}' with ID {1}", instanceName, instance_id);
    }

    void MaterialSystem::saveAllMaterials(const std::string& directory)
    {
        // TODO: Implement material saving
        TST_CORE_WARN("MaterialSystem::saveAllMaterials not implemented yet");
    }

    void MaterialSystem::loadAllMaterials(const std::string& directory)
    {
        // TODO: Implement material loading
        TST_CORE_WARN("MaterialSystem::loadAllMaterials not implemented yet");
    }

    size_t MaterialSystem::getMaterialCount()
    {
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return 0;
        }
        return s_Materials.size();
    }

    size_t MaterialSystem::getMaterialInstanceCount()
    {
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return 0;
        }
        return s_MaterialInstances.size();
    }

    size_t MaterialSystem::getInstanceCountOfMaterial(MaterialID base_material_id)
    {
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return 0;
        }

        size_t count = 0;
        for (const auto& [id, instance] : s_MaterialInstances)
        {
            if (instance->getBaseMaterialID() == base_material_id)
            {
                count++;
            }
        }
        return count;
    }

    std::vector<MaterialID> MaterialSystem::getAllMaterials()
    {
        std::vector<MaterialID> materials;
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return materials;
        }

        materials.reserve(s_Materials.size());
        for (const auto& [id, material] : s_Materials)
        {
            materials.push_back(id);
        }
        return materials;
    }

    std::vector<MaterialInstanceID> MaterialSystem::getAllMaterialInstances()
    {
        std::vector<MaterialInstanceID> instances;
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return instances;
        }

        instances.reserve(s_MaterialInstances.size());
        for (const auto& [id, instance] : s_MaterialInstances)
        {
            instances.push_back(id);
        }
        return instances;
    }

    std::vector<MaterialInstanceID> MaterialSystem::getAllInstancesOfMaterial(MaterialID base_material_id)
    {
        std::vector<MaterialInstanceID> instances;
        if (!s_initialized)
        {
            TST_CORE_ERROR("MaterialSystem not initialized");
            return instances;
        }

        for (const auto& [id, instance] : s_MaterialInstances)
        {
            if (instance->getBaseMaterialID() == base_material_id)
            {
                instances.push_back(id);
            }
        }
        return instances;
    }
}