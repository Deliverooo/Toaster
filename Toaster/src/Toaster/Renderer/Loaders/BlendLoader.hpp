#pragma once

#include "Toaster/Renderer/Mesh.hpp"

#define TST_ENABLE_BLEND

#ifdef TST_ENABLE_BLEND

// Forward declarations
struct aiNode;
struct aiMesh;
struct aiScene;
struct aiMaterial;
enum aiTextureType;

namespace tst
{
    class BlendLoader : public MeshLoader
    {
    public:
        virtual ~BlendLoader() override = default;

        virtual bool load(const std::string& filepath,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes,
            MaterialLibrary& materials) override;

        virtual std::vector<std::string> getSupportedExtensions() const override;

    private:
        void processNode(aiNode* node, const aiScene* scene,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes);

        void processMesh(aiMesh* mesh, const aiScene* scene,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes);

        void processMaterials(const aiScene* scene, MaterialLibrary& materials, const std::string& directory);

        void loadMaterialTextures(aiMaterial* mat, aiTextureType type,
            RefPtr<Material> material, const std::string& directory);
    };
}

#endif