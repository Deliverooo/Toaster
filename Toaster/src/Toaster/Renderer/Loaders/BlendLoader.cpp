#include "tstpch.h"
#include "BlendLoader.hpp"

#ifdef TST_ENABLE_BLEND

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "assimp/version.h"

namespace tst
{
    bool BlendLoader::load(const std::string& filepath,
        std::vector<MeshVertex>& vertices,
        std::vector<uint32_t>& indices,
        std::vector<SubMesh>& submeshes,
        MaterialLibrary& materials)
    {

        TST_CORE_WARN("Blender Support is deprecated :(");

        Assimp::Importer importer;


        aiString extensions;
        importer.GetExtensionList(extensions);
        TST_CORE_INFO("Toaster supported extensions: {0}", extensions.C_Str());

        bool blendSupported = importer.IsExtensionSupported(".blend");
        TST_CORE_INFO("Blend support: {0}", blendSupported ? "YES" : "NO");

        if (!blendSupported) {
            TST_CORE_ERROR("Toaster's Assimp installation does not support .blend files");
            return false;
        }

        unsigned int flags = aiProcess_Triangulate;

        TST_CORE_INFO("Attempting to load Blend file: {0}", filepath);

        const aiScene* scene = importer.ReadFile(filepath, flags);

        if (!scene) {
            std::string error = importer.GetErrorString();
            TST_CORE_ERROR("Failed to load Blend file: {0}", error);

            if (error.find("BlenderDNA") != std::string::npos) {
                TST_CORE_ERROR("This appears to be a Blender DNA parsing error.");
                TST_CORE_ERROR("The Cause:");
                TST_CORE_ERROR("1. Direct export from blender to .blend files is not supported :)\n");
                TST_CORE_ERROR("Possible fixes:");
                TST_CORE_ERROR("1. Use an old version of Blender if you really want to use .blend files");
                TST_CORE_ERROR("2. In Blender, goto [ File -> Export -> .xxx] and choose the appropriate format");

            }
            return false;
        }

        if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
            TST_CORE_WARN("Scene loaded but marked as incomplete");
        }

        if (!scene->mRootNode) {
            TST_CORE_ERROR("Scene has no root node");
            return false;
        }

        TST_CORE_INFO("Successfully loaded Blend file structure");
        TST_CORE_INFO("Meshes: {0}, Materials: {1}, Textures: {2}",
            scene->mNumMeshes, scene->mNumMaterials, scene->mNumTextures);

        processMaterials(scene, materials, filepath);

        processNode(scene->mRootNode, scene, vertices, indices, submeshes);

        TST_CORE_INFO("Successfully processed Blend: {0} vertices, {1} indices, {2} submeshes",
            vertices.size(), indices.size(), submeshes.size());

        return true;
    }

    void BlendLoader::processNode(aiNode* node, const aiScene* scene,
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

    void BlendLoader::processMesh(aiMesh* mesh, const aiScene* scene,
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

    void BlendLoader::processMaterials(const aiScene* scene, MaterialLibrary& materials, const std::string& directory)
    {
        std::string dir = directory.substr(0, directory.find_last_of('/'));

        for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
            aiMaterial* mat = scene->mMaterials[i];

            // Create material
            auto material = Material::create();

            // Get material name
            aiString name;
            mat->Get(AI_MATKEY_NAME, name);
            material->setName(name.C_Str());

            // Get diffuse colour
            aiColor3D colour(1.0f, 1.0f, 1.0f);
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, colour);
            material->setDiffuse(glm::vec3(colour.r, colour.g, colour.b));

            // Get specular colour
            mat->Get(AI_MATKEY_COLOR_SPECULAR, colour);
            material->setSpecular(glm::vec3(colour.r, colour.g, colour.b));

            // Get shininess
            float shininess = 32.0f;
            mat->Get(AI_MATKEY_SHININESS, shininess);
            material->setShininess(shininess);

            // Load textures
            loadMaterialTextures(mat, aiTextureType_DIFFUSE, material, dir);
            loadMaterialTextures(mat, aiTextureType_SPECULAR, material, dir);
            loadMaterialTextures(mat, aiTextureType_NORMALS, material, dir);

            materials.addMaterial(material);
        }
    }

    void BlendLoader::loadMaterialTextures(aiMaterial* mat, aiTextureType type,
        RefPtr<Material> material, const std::string& directory)
    {
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);

            std::string texturePath = directory + "/" + str.C_Str();

            try {
                auto texture = Texture2D::create(texturePath);

                switch (type) {
                case aiTextureType_DIFFUSE:
                    material->setDiffuseMap(texture);
                    break;
                case aiTextureType_SPECULAR:
                    material->setSpecularMap(texture);
                    break;
                case aiTextureType_NORMALS:
                    material->setNormalMap(texture);
                    break;
                case aiTextureType_HEIGHT:
                    material->setHeightMap(texture);
                }
            }
            catch (const std::exception& e) {
                TST_CORE_WARN("Failed to load texture: {0} - {1}", texturePath, e.what());
            }
        }
    }

    std::vector<std::string> BlendLoader::getSupportedExtensions() const
    {
        return { ".blend", ".BLEND" };
    }

}

#endif