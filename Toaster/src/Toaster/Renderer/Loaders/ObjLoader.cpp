#include "tstpch.h"
#include "ObjLoader.hpp"


#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <filesystem>

namespace tst
{

    bool ObjLoader::load(const std::string& filepath, std::vector<MeshVertex>& vertices, std::vector<uint32_t>& indices, std::vector<SubMesh>& submeshes, std::vector<MaterialID> &material_ids)
    {

        tinyobj::ObjReaderConfig reader_config;

        std::filesystem::path parentDirectory = std::filesystem::path(filepath).remove_filename();
        reader_config.mtl_search_path = parentDirectory.string();

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(filepath, reader_config)) {
            if (!reader.Error().empty()) {
                TST_CORE_ERROR("TinyObjReader: {0}", reader.Error());
            }
            return false;
        }

        if (!reader.Warning().empty()) {
            TST_CORE_WARN("TinyObjReader: {0}", reader.Warning());
        }

        auto& attrib = reader.GetAttrib();
        auto& shapes = reader.GetShapes();
        auto& materials = reader.GetMaterials();

        std::vector<MaterialID> createdMaterials;
        createdMaterials.reserve(materials.size());


        for (const auto& mat : materials)
        {

            MaterialID matId = MaterialSystem::createMaterial(mat.name);

            auto material = MaterialSystem::getMaterial(matId);

            // Better default values if material properties are missing
            material->setProperty<glm::vec3>("ambient", {
                mat.ambient[0] > 0 ? mat.ambient[0] : 0.1f,
                mat.ambient[1] > 0 ? mat.ambient[1] : 0.1f,
                mat.ambient[2] > 0 ? mat.ambient[2] : 0.1f
                });
            material->setProperty<glm::vec3>("diffuse",{
                mat.diffuse[0] > 0 ? mat.diffuse[0] : 0.8f,
                mat.diffuse[1] > 0 ? mat.diffuse[1] : 0.8f,
                mat.diffuse[2] > 0 ? mat.diffuse[2] : 0.8f
                });
            material->setProperty<glm::vec3>("specular", {
                mat.specular[0] >= 0 ? mat.specular[0] : 0.2f,
                mat.specular[1] >= 0 ? mat.specular[1] : 0.2f,
                mat.specular[2] >= 0 ? mat.specular[2] : 0.2f
                });
            material->setProperty<glm::vec3>("emission", { mat.emission[0], mat.emission[1], mat.emission[2] });
            material->setProperty<float>("shininess", mat.shininess > 0 ? mat.shininess : 32.0f);
            material->setProperty<float>("opacity", mat.dissolve > 0 ? mat.dissolve : 0.5f);


            static TextureParams defaultParams = {
                .wrapS = TextureWrapping::Repeat,
                .wrapT = TextureWrapping::Repeat,
                .minFilter = TextureFiltering::LinearMipmapLinear,
                .magFilter = TextureFiltering::Linear
            };

            if (!mat.diffuse_texname.empty())
            {
                std::filesystem::path texturePath = parentDirectory / mat.diffuse_texname; // Use / operator instead of append

                try
                {
                    auto texture = Texture2D::create(texturePath.string(), defaultParams);
                    material->setDiffuseMap(texture);
                    TST_CORE_INFO("Loaded Diffuse Map: {0}", texturePath.string());
                }
                catch (...)
                {
                    TST_CORE_ERROR("Failed to load Diffuse Map: {0}", texturePath.string());
                }
            }

            if (!mat.specular_texname.empty())
            {
                std::filesystem::path texturePath = parentDirectory / mat.specular_texname; // Use / operator

                try
                {
                    auto texture = Texture2D::create(texturePath.string(), defaultParams);
                    material->setSpecularMap(texture);
                    TST_CORE_INFO("Loaded Specular Map: {0}", texturePath.string());
                }
                catch (...)
                {
                    TST_CORE_ERROR("Failed to load Specular Map: {0}", texturePath.string());
                }
            }

            if (!mat.normal_texname.empty())
            {
                std::filesystem::path texturePath = parentDirectory / mat.normal_texname; // Use / operator

                try
                {
                    auto texture = Texture2D::create(texturePath.string(), defaultParams);
                    material->setNormalMap(texture);
                    TST_CORE_INFO("Loaded Normal Map: {0}", texturePath.string());
                }
                catch (...)
                {
                    TST_CORE_ERROR("Failed to load Normal Map: {0}", texturePath.string());
                }
            }

            if (!mat.bump_texname.empty())
            {
                std::filesystem::path texturePath = parentDirectory / mat.bump_texname; // Use / operator

                try
                {
                    auto texture = Texture2D::create(texturePath.string(), defaultParams);
                    material->setHeightMap(texture);
                    TST_CORE_INFO("Loaded Height Map: {0}", texturePath.string());
                }
                catch (...)
                {
                    TST_CORE_ERROR("Failed to load Height Map: {0}", texturePath.string());
                }
            }

            createdMaterials.push_back(matId);

            TST_CORE_INFO("Material '{}' loaded:", mat.name);
            TST_CORE_INFO("  Diffuse: ({}, {}, {})", mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
            TST_CORE_INFO("  Diffuse texture: '{}'", mat.diffuse_texname);
            TST_CORE_INFO("  Has diffuse texture: {}", !mat.diffuse_texname.empty());
        }

        if (createdMaterials.empty())
        {
            createdMaterials.push_back(TST_DEFAULT_MATERIAL);
        }

        std::unordered_map<MeshVertex, uint32_t> uniqueVertices{};

        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++) {
            SubMesh submesh;
            submesh.indexOffset = static_cast<uint32_t>(indices.size());

            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++) {
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                    MeshVertex vertex{};

                    // vertex position
                    vertex.position = {
                        attrib.vertices[3 * (size_t)(idx.vertex_index) + 0],
                        attrib.vertices[3 * (size_t)(idx.vertex_index) + 1],
                        attrib.vertices[3 * (size_t)(idx.vertex_index) + 2]
                    };

                    // vertex normal
                    if (idx.normal_index >= 0) {
                        vertex.normal = {
                            attrib.normals[3 * (size_t)(idx.normal_index) + 0],
                            attrib.normals[3 * (size_t)(idx.normal_index) + 1],
                            attrib.normals[3 * (size_t)(idx.normal_index) + 2]
                        };
                    }
                    // vertex texture coordinate
                    if (idx.texcoord_index >= 0) {
                        vertex.textureCoords = {
                            attrib.texcoords[2 * (size_t)(idx.texcoord_index) + 0],
                            1.0f - attrib.texcoords[2 * (size_t)(idx.texcoord_index) + 1] // Flip Y coordinate
                        };
                    }

                    // Check if this vertex already exists
                    if (uniqueVertices.count(vertex) == 0) {
                        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                    }

                    indices.push_back(uniqueVertices[vertex]);
                }
                index_offset += fv;
            }

            submesh.indexCount = static_cast<uint32_t>(indices.size()) - submesh.indexOffset;

            MaterialID materialId;
            if (!shapes[s].mesh.material_ids.empty() && shapes[s].mesh.material_ids[0] >= 0 &&
                shapes[s].mesh.material_ids[0] < static_cast<int>(createdMaterials.size()))
            {
                // Fix: Use the MaterialID from your created materials array directly
                materialId = createdMaterials[shapes[s].mesh.material_ids[0]];
            }
            else
            {
                materialId = TST_DEFAULT_MATERIAL; // Use 0, not TST_DEFAULT_MATERIAL
            }

            submesh.materialId = materialId;
            material_ids.push_back(materialId);
            submeshes.push_back(submesh);
        }

        TST_CORE_INFO("Loaded mesh with {0} vertices, {1} indices, {2} submeshes, {3} materials", vertices.size(), indices.size(), submeshes.size(), createdMaterials.size());

        return true;
    }

}