#pragma once
#include "Material.hpp"

namespace tst
{
    using MaterialID = uint32_t;
    using MaterialInstanceID = uint64_t;

    constexpr MaterialID TST_INVALID_MATERIAL = 0;
    constexpr MaterialInstanceID TST_INVALID_MATERIAL_INSTANCE = 0;
    constexpr MaterialID TST_DEFAULT_MATERIAL = 1;

    class TST_API MaterialSystem
    {
    public:

        static void init();
        static void terminate();

        static MaterialID createMaterial(const std::string& material_name = "Default");
        static MaterialID loadMaterial(const std::string& filepath);

        static RefPtr<Material>& getMaterial(const std::string& material_name);
        static RefPtr<Material>& getMaterial(MaterialID id);
        static MaterialID getMaterialID(const std::string& material_name);
        static void releaseMaterial(MaterialID material_id);

        static MaterialInstanceID createMaterialInstance(MaterialID base_material_id, const std::string& instance_name);
        static RefPtr<MaterialInstance>& getMaterialInstance(MaterialInstanceID material_instance_id);
        static MaterialInstanceID getMaterialInstanceID(const std::string& instance_name);
        static void releaseMaterialInstance(MaterialInstanceID instance_id);

        static void saveAllMaterials(const std::string& directory);
        static void loadAllMaterials(const std::string& directory);

        static size_t getMaterialCount();
        static size_t getMaterialInstanceCount();
        static size_t getInstanceCountOfMaterial(MaterialID base_material_id);

        static std::vector<MaterialID> getAllMaterials();
        static std::vector<MaterialInstanceID> getAllMaterialInstances();
        static std::vector<MaterialInstanceID> getAllInstancesOfMaterial(MaterialID base_material_id);

    private:
        // Static member variables - these were missing the 'static' keyword in your original header
        static std::unordered_map<MaterialID, RefPtr<Material>> s_Materials;
        static std::unordered_map<std::string, MaterialID> s_MaterialNameMap;
        static std::unordered_map<MaterialInstanceID, RefPtr<MaterialInstance>> s_MaterialInstances;
        static std::unordered_map<std::string, MaterialInstanceID> s_MaterialInstanceNameMap;
        static bool s_initialized;
    };
}