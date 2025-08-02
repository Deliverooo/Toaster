#include "tstpch.h"
#include "Mesh.hpp"

#include <filesystem>

#include "Loaders/ObjLoader.hpp"
#include "Loaders/FbxLoader.hpp"
#include "Loaders/BlendLoader.hpp"


namespace tst
{
    std::unordered_map<MeshFormat, ScopedPtr<MeshLoader>> Mesh::s_loaders;
    bool Mesh::s_loadersInitialized = false;

    Mesh::Mesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices)
        : m_vertices(vertices), m_indices(indices)
    {

        m_materials.addMaterial(Material::createDefault());

        SubMesh submesh;
        submesh.indexOffset = 0;
        submesh.indexCount = static_cast<uint32_t>(indices.size());
        submesh.materialIndex = 0;
        m_submeshes.push_back(submesh);

        setupMesh();
    }

    void Mesh::initLoaders()
    {
        if (s_loadersInitialized) return;

        s_loaders[MeshFormat::OBJ] = make_scoped<ObjLoader>();
        TST_CORE_INFO("Initialized Obj Loader");

#ifdef TST_ENABLE_FBX
        s_loaders[MeshFormat::FBX] = make_scoped<FbxLoader>();
        TST_CORE_INFO("Initialized Fbx Loader");
#endif

#ifdef TST_ENABLE_BLEND
        s_loaders[MeshFormat::BLEND] = make_scoped<BlendLoader>();
        TST_CORE_INFO("Initialized Blend Loader");
#endif

#ifdef TST_ENABLE_GLTF  
        s_loaders[MeshFormat::GLTF] = make_scoped<GltfLoader>();
#endif

        s_loadersInitialized = true;
    }

    Mesh::Mesh(const std::string& filepath)
    {
        if (!loadFromFile(filepath))
        {
            TST_CORE_ERROR("Failed to load mesh from file: {0}", filepath);

            m_materials.addMaterial(Material::createDefault());
        }
        setupMesh();
    }

    void Mesh::setupMesh()
    {
        m_vertexArray = VertexArray::create();

        m_vertexBuffer = VertexBuffer::create(m_vertices.size() * sizeof(MeshVertex));
        m_vertexBuffer->setData(m_vertices.data(), m_vertices.size() * sizeof(MeshVertex));

        m_vertexBuffer->setLayout({
            {"a_Position",    ShaderDataType::Float3},
            {"a_Normal",      ShaderDataType::Float3},
            {"a_TexCoord",    ShaderDataType::Float2},
            {"a_Tangent",     ShaderDataType::Float3},
            {"a_Bitangent",   ShaderDataType::Float3}
            });

        m_vertexArray->addVertexBuffer(m_vertexBuffer);

        m_indexBuffer = IndexBuffer::create(m_indices.data(), static_cast<uint32_t>(m_indices.size()));
        m_vertexArray->addIndexBuffer(m_indexBuffer);
    }

    bool Mesh::loadFromFile(const std::string& filepath)
    {
        initLoaders();

        MeshFormat format = detectMeshFormat(filepath);
        if (format == MeshFormat::Unknown)
        {
            TST_CORE_ERROR("Unsupported mesh file format at: {0}", filepath);
            return false;
        }

        return loadWithFormat(filepath, format);
    }



    void Mesh::calculateTangents()
    {
        // Calculate tangents and bitangents for normal mapping
        for (size_t i = 0; i < m_indices.size(); i += 3)
        {
            if (i + 2 >= m_indices.size()) break;

            MeshVertex& v0 = m_vertices[m_indices[i + 0]];
            MeshVertex& v1 = m_vertices[m_indices[i + 1]];
            MeshVertex& v2 = m_vertices[m_indices[i + 2]];

            glm::vec3 edge1 = v1.position - v0.position;
            glm::vec3 edge2 = v2.position - v0.position;

            glm::vec2 deltaUV1 = v1.textureCoords - v0.textureCoords;
            glm::vec2 deltaUV2 = v2.textureCoords - v0.textureCoords;

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

            glm::vec3 tangent;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

            glm::vec3 bitangent;
            bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
            bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
            bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

            v0.tangent = tangent;
            v1.tangent = tangent;
            v2.tangent = tangent;

            v0.bitangent = bitangent;
            v1.bitangent = bitangent;
            v2.bitangent = bitangent;
        }
    }

    void Mesh::bind() const
    {
        m_vertexArray->bind();
    }

    void Mesh::unbind() const
    {
        m_vertexArray->unbind();
    }

    RefPtr<Mesh> Mesh::create(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices)
    {
        return make_reference<Mesh>(vertices, indices);
    }

    RefPtr<Mesh> Mesh::create(const std::string& filepath)
    {
        return make_reference<Mesh>(filepath);
    }

    MeshFormat Mesh::detectMeshFormat(const std::string& filepath)
    {
        std::filesystem::path path(filepath);
        std::string extension = path.extension().string();

        // Convert to lowercase
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        if (extension == ".obj") return MeshFormat::OBJ;
        if (extension == ".fbx") return MeshFormat::FBX;
        if (extension == ".gltf" || extension == ".glb") return MeshFormat::GLTF;
        if (extension == ".stl") return MeshFormat::STL;
        if (extension == ".blend") return MeshFormat::BLEND;

        return MeshFormat::Unknown;
    }


    bool Mesh::loadWithFormat(const std::string& filepath, MeshFormat format)
    {
        auto it = s_loaders.find(format);
        if (it == s_loaders.end())
        {
            TST_CORE_ERROR("Mesh Format Unsupported: {0}", static_cast<int>(format));
            return false;
        }

        bool success = it->second->load(filepath, m_vertices, m_indices, m_submeshes, m_materials);

        if (success)
        {
            calculateTangents();
            TST_CORE_INFO("Loaded mesh with {0} vertices, {1} indices, {2} submeshes, {3} materials",
                m_vertices.size(), m_indices.size(), m_submeshes.size(), m_materials.getMaterialCount());
        }

        return success;
    }


}
