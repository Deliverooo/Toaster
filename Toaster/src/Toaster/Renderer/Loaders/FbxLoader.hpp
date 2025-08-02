#pragma once

#include "Toaster/Renderer/Mesh.hpp"

#ifdef TST_ENABLE_FBX

// Forward declarations
struct aiNode;
struct aiMesh;
struct aiScene;
struct aiMaterial;
enum aiTextureType;

namespace tst
{
    class FbxLoader : public MeshLoader
    {
    public:
        virtual ~FbxLoader() override = default;

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

        const aiScene* m_CurrentScene = nullptr;

        RefPtr<Texture2D> loadEmbeddedTexture(const std::string& texturePath, const aiScene* scene);
        
    };
}

#endif