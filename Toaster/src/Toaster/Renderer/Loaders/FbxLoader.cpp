#include "tstpch.h"

#include "FbxLoader.hpp"

#ifdef TST_ENABLE_FBX

#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace tst
{

    bool FbxLoader::load(const std::string& filepath,
        std::vector<MeshVertex>& vertices,
        std::vector<uint32_t>& indices,
        std::vector<SubMesh>& submeshes,
        MaterialLibrary& materials)
    {
        Assimp::Importer importer;

        // Set processing flags
        unsigned int flags = aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_OptimizeMeshes |
            aiProcess_JoinIdenticalVertices;

        const aiScene* scene = importer.ReadFile(filepath, flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            TST_CORE_ERROR("Failed to load FBX file: {0}", importer.GetErrorString());
            return false;
        }


        // Process materials first
        processMaterials(scene, materials, filepath);

        // Process all meshes
        processNode(scene->mRootNode, scene, vertices, indices, submeshes);

        TST_CORE_INFO("Successfully loaded FBX: {0} vertices, {1} indices, {2} submeshes",
            vertices.size(), indices.size(), submeshes.size());

        return true;
    }

    void FbxLoader::processNode(aiNode* node, const aiScene* scene,
        std::vector<MeshVertex>& vertices,
        std::vector<uint32_t>& indices,
        std::vector<SubMesh>& submeshes)
    {
        // Process all meshes in this node
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene, vertices, indices, submeshes);
        }

        // Process child nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, vertices, indices, submeshes);
        }
    }

    void FbxLoader::processMesh(aiMesh* mesh, const aiScene* scene,
        std::vector<MeshVertex>& vertices,
        std::vector<uint32_t>& indices,
        std::vector<SubMesh>& submeshes)
    {
        SubMesh submesh;
        submesh.indexOffset = static_cast<uint32_t>(indices.size());
        submesh.materialIndex = mesh->mMaterialIndex;

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            MeshVertex vertex;

            // Position
            vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            // Normal
            if (mesh->HasNormals()) {
                vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            }

            // Texture coordinates
            if (mesh->mTextureCoords[0]) {
                vertex.textureCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }
            else {
                vertex.textureCoords = glm::vec2(0.0f, 0.0f);
            }

            // Tangent
            if (mesh->HasTangentsAndBitangents()) {
                vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
            }

            vertices.push_back(vertex);
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j] + static_cast<uint32_t>(vertices.size()) - mesh->mNumVertices);
            }
        }

        submesh.indexCount = static_cast<uint32_t>(indices.size()) - submesh.indexOffset;
        submeshes.push_back(submesh);
    }

    void FbxLoader::processMaterials(const aiScene* scene, MaterialLibrary& materials, const std::string& directory)
    {
        // Store scene reference for embedded texture access
        m_CurrentScene = scene;

        std::string dir = std::filesystem::path(directory).replace_extension("").remove_filename().string();

        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* mat = scene->mMaterials[i];

            // Create material
            auto material = Material::create();

            // Get material name
            aiString name;
            mat->Get(AI_MATKEY_NAME, name);
            material->setName(name.C_Str());

            // Get material properties
            aiColor3D colour(1.0f, 1.0f, 1.0f);
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, colour);
            material->setDiffuse(glm::vec3(colour.r, colour.g, colour.b));

            mat->Get(AI_MATKEY_COLOR_SPECULAR, colour);
            material->setSpecular(glm::vec3(colour.r, colour.g, colour.b));

            float shininess = 32.0f;
            mat->Get(AI_MATKEY_SHININESS, shininess);
            material->setShininess(shininess);

            // Load textures (now supports embedded textures)
            loadMaterialTextures(mat, aiTextureType_DIFFUSE, material, dir);
            loadMaterialTextures(mat, aiTextureType_SPECULAR, material, dir);
            loadMaterialTextures(mat, aiTextureType_NORMALS, material, dir);

            materials.addMaterial(material);
        }
    }

    void FbxLoader::loadMaterialTextures(aiMaterial* mat, aiTextureType type,
        RefPtr<Material> material, const std::string& directory)
    {
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            aiReturn result = mat->GetTexture(type, i, &str);

            if (result != AI_SUCCESS) {
                TST_CORE_ERROR("Failed to get texture path for texture type {0}, index {1}",
                    static_cast<int>(type), i);
                continue;
            }

            std::string texturePath = str.C_Str();
            TST_CORE_INFO("Processing texture: '{0}'", texturePath);

            RefPtr<Texture2D> texture = nullptr;

            // Check if this is an embedded texture (starts with '*')
            if (!texturePath.empty() && texturePath[0] == '*') {
                TST_CORE_INFO("Loading Embedded texture00");

                texture = loadEmbeddedTexture(texturePath, m_CurrentScene);
            }
            else {
                std::filesystem::path path = std::filesystem::path(directory).parent_path() / std::filesystem::path(texturePath);
                TST_CORE_INFO("Loading relative path: {0}", path.string());
                texture = Texture2D::create(path.string());
            }

            if (texture) {
	            switch (type)
	            {
	            case aiTextureType_DIFFUSE:
                    material->setDiffuseMap(texture); break;
	            case aiTextureType_SPECULAR:
                    material->setSpecularMap(texture); break;
	            case aiTextureType_NORMALS:
                    material->setNormalMap(texture); break;
	            case aiTextureType_HEIGHT:
                    material->setHeightMap(texture); break;
	            }
            }
        }
    }

    RefPtr<Texture2D> FbxLoader::loadEmbeddedTexture(const std::string& texturePath, const aiScene* scene)
    {
        // Parse embedded texture index from path (format: "*0", "*1", etc.)
        if (texturePath.length() < 2) {
            TST_CORE_ERROR("Invalid embedded texture path: {0}", texturePath);
            return nullptr;
        }

        try {
            unsigned int textureIndex = std::stoul(texturePath.substr(1));

            if (textureIndex >= scene->mNumTextures) {
                TST_CORE_ERROR("Embedded texture index {0} out of range (max: {1})",
                    textureIndex, scene->mNumTextures - 1);
                return nullptr;
            }

            const aiTexture* aiTex = scene->mTextures[textureIndex];

            TST_CORE_INFO("Loading embedded texture {0}", textureIndex);

            if (aiTex->mHeight == 0) {
                // Compressed texture - would need separate handling
                TST_CORE_WARN("Compressed embedded textures not yet supported");
                return nullptr;
            }
            else {

                auto texture = Texture2D::create(aiTex->mWidth, aiTex->mHeight);

                // Convert aiTexel array to raw RGBA data
                std::vector<unsigned char> pixelData;
                pixelData.reserve(aiTex->mWidth * aiTex->mHeight * 4);

                for (unsigned int i = 0; i < aiTex->mWidth * aiTex->mHeight; i++) {
                    const aiTexel& texel = aiTex->pcData[i];
                    pixelData.push_back(texel.r);
                    pixelData.push_back(texel.g);
                    pixelData.push_back(texel.b);
                    pixelData.push_back(texel.a);
                }

                // Use setData to upload the pixel data
                texture->setData(pixelData.data());

                return texture;
            }
        }
        catch (const std::exception& e) {
            TST_CORE_ERROR("Failed to parse embedded texture index from '{0}': {1}", texturePath, e.what());
            return nullptr;
        }
    }

    std::vector<std::string> FbxLoader::getSupportedExtensions() const
    {
        return { ".fbx", ".FBX" };
    }
}

#endif