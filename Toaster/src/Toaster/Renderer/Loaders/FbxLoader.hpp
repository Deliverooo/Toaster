#pragma once

#include "Toaster/Renderer/Mesh.hpp"

#define TST_ENABLE_FBX
#ifdef TST_ENABLE_FBX

#include "assimp/matrix4x4.h"
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

        // New MaterialSystem method
        virtual bool load(const std::string& filepath,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes,
            std::vector<MaterialID>& materialIDs) override;


        virtual std::vector<std::string> getSupportedExtensions() const override;

    private:
        void processNode(aiNode* node, const aiScene* scene,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes,
            const std::vector<MaterialID>& materialIDs);

        void processNode(aiNode* node, const aiScene* scene,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes,
            const aiMatrix4x4& parentTransform,
            const std::vector<MaterialID>& materialIDs);

        void processMesh(aiMesh* mesh, const aiScene* scene,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes,
            const std::vector<MaterialID>& materialIDs);

        void processMesh(aiMesh* mesh, const aiScene* scene,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes,
            const aiMatrix4x4& transform,
            const std::vector<MaterialID>& materialIDs);

        // New MaterialSystem methods
        void processMaterials(const aiScene* scene, std::vector<MaterialID>& materialIDs, const std::string& directory);
        void loadMaterialTextures(aiMaterial* mat, aiTextureType type,
            RefPtr<Material> material, const std::string& directory);
        RefPtr<Texture2D> loadEmbeddedTexture(const std::string& texturePath, const aiScene* scene);



        const aiScene* m_CurrentScene = nullptr;
    };
}

#endif