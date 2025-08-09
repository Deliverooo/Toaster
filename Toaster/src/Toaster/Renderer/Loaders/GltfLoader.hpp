#pragma once

#include "Toaster/Renderer/Mesh.hpp"

#define TST_ENABLE_GLTF
#ifdef TST_ENABLE_GLTF

#include "assimp/matrix4x4.h"
// Forward declarations
struct aiNode;
struct aiMesh;
struct aiScene;
struct aiMaterial;

enum aiTextureType;

namespace tst
{
    class GltfLoader : public MeshLoader
    {
    public:
        virtual ~GltfLoader() override = default;

        virtual bool load(const std::string& filepath,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes,
            std::vector<MaterialID>& material_ids) override;

        virtual std::vector<std::string> getSupportedExtensions() const override;

    private:
        void processNode(aiNode* node, const aiScene* scene,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes);

        void processNode(aiNode* node, const aiScene* scene,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes,
            const aiMatrix4x4& parentTransform);

        void processMesh(aiMesh* mesh, const aiScene* scene,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes);

        void processMesh(aiMesh* mesh, const aiScene* scene,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes,
            const aiMatrix4x4& transform);

        void processMaterials(const aiScene* scene, std::vector<MaterialID>& material_ids, const std::string& directory);

        void loadMaterialTextures(aiMaterial* mat, aiTextureType type,
            RefPtr<Material> material, const std::string& directory);

        const aiScene* m_CurrentScene = nullptr;

        RefPtr<Texture2D> loadEmbeddedTexture(const std::string& texturePath, const aiScene* scene);

    };
}

#endif