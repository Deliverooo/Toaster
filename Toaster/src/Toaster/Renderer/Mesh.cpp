#include "tstpch.h"
#include "Mesh.hpp"

#include <filesystem>

#include "Loaders/ObjLoader.hpp"
#include "Loaders/FbxLoader.hpp"
#include "Loaders/GltfLoader.hpp"
#include "Loaders/BlendLoader.hpp"


namespace tst
{
    std::unordered_map<MeshFormat, ScopedPtr<MeshLoader>> Mesh::s_loaders;
    bool Mesh::s_loadersInitialized = false;

    Mesh::Mesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices)
        : m_vertices(vertices), m_indices(indices)
    {

        SubMesh submesh;
        submesh.indexOffset = 0;
        submesh.indexCount = static_cast<uint32_t>(indices.size());
        submesh.materialId = TST_DEFAULT_MATERIAL;
        m_submeshes.push_back(submesh);
        m_materialIDs.push_back(TST_DEFAULT_MATERIAL);

        calculateTangents();
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
        TST_CORE_INFO("Initialized Gltf Loader");

#endif

        s_loadersInitialized = true;
    }

    Mesh::Mesh(const std::string& filepath) : m_Filepath(filepath)
    {
        TST_CORE_INFO("Filepath {0}", m_Filepath.value());
        if (!loadFromFile(filepath))
        {
            TST_CORE_ERROR("Failed to load mesh from file: {0}", filepath);

            SubMesh submesh;
            submesh.indexOffset = 0;
            submesh.indexCount = 0;
            submesh.materialId = TST_DEFAULT_MATERIAL;

            m_submeshes.push_back(submesh);
            m_materialIDs.push_back(TST_DEFAULT_MATERIAL);
        }

        calculateTangents();
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

    RefPtr<Material> Mesh::getMaterial(uint32_t submeshIndex) const
    {
        if (submeshIndex >= m_materialIDs.size())
        {
            TST_CORE_WARN("Submesh index {0} out of range, using default material", submeshIndex);
            return MaterialSystem::getMaterial(TST_DEFAULT_MATERIAL);
        }

        MaterialID matID = m_materialIDs[submeshIndex];
        if (matID == TST_INVALID_MATERIAL)
        {
            TST_CORE_WARN("Invalid material ID for submesh {0}, using default material", submeshIndex);
            return MaterialSystem::getMaterial(TST_DEFAULT_MATERIAL);
        }

        return MaterialSystem::getMaterial(matID);

    }

    MaterialID Mesh::getMaterialID(uint32_t submeshIndex) const
    {
        if (submeshIndex >= m_materialIDs.size())
        {
            TST_CORE_WARN("Submesh index {0} out of range, returning default material ID", submeshIndex);
            return TST_DEFAULT_MATERIAL;
        }

        return m_materialIDs[submeshIndex];

    }

    void Mesh::calculateTangents()
    {
        // Early return if no indices or vertices
        if (m_indices.empty() || m_vertices.empty()) {
            TST_CORE_WARN("Cannot calculate tangents: empty indices or vertices");
            return;
        }

        // Calculate tangents and bitangents for normal mapping
        for (size_t i = 0; i < m_indices.size(); i += 3)
        {
            if (i + 2 >= m_indices.size()) break;

            // Validate indices are within bounds
            uint32_t idx0 = m_indices[i + 0];
            uint32_t idx1 = m_indices[i + 1];
            uint32_t idx2 = m_indices[i + 2];

            if (idx0 >= m_vertices.size() || idx1 >= m_vertices.size() || idx2 >= m_vertices.size()) {
                TST_CORE_ERROR("Vertex index out of range in face {}: indices [{}, {}, {}], vertex count: {}",
                    i / 3, idx0, idx1, idx2, m_vertices.size());
                continue; // Skip this triangle
            }

            MeshVertex& v0 = m_vertices[idx0];
            MeshVertex& v1 = m_vertices[idx1];
            MeshVertex& v2 = m_vertices[idx2];

            glm::vec3 edge1 = v1.position - v0.position;
            glm::vec3 edge2 = v2.position - v0.position;

            glm::vec2 deltaUV1 = v1.textureCoords - v0.textureCoords;
            glm::vec2 deltaUV2 = v2.textureCoords - v0.textureCoords;

            float denominator = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            float f = (denominator != 0.0f) ? 1.0f / denominator : 0.0f;

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
        if (extension == ".gltf") return MeshFormat::GLTF;
        if (extension == ".glb") return MeshFormat::GLB;
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

        // Clear existing data
        m_vertices.clear();
        m_indices.clear();
        m_submeshes.clear();
        m_materialIDs.clear();

        if (it->second->load(filepath, m_vertices, m_indices, m_submeshes, m_materialIDs))
        {
            // Validate that we have consistent data
            if (m_submeshes.size() != m_materialIDs.size())
            {
                TST_CORE_ERROR("Submesh count ({}) doesn't match material count ({}). Padding with default materials.",
                    m_submeshes.size(), m_materialIDs.size());

                // Pad with default materials if needed
                while (m_materialIDs.size() < m_submeshes.size())
                {
                    m_materialIDs.push_back(TST_DEFAULT_MATERIAL);
                }
            }

            // Sync submesh materialId with materialIDs array
            for (size_t i = 0; i < m_submeshes.size() && i < m_materialIDs.size(); ++i)
            {
                m_submeshes[i].materialId = m_materialIDs[i];
            }

            // Validate submesh indices are within bounds
            for (size_t i = 0; i < m_submeshes.size(); ++i)
            {
                const auto& submesh = m_submeshes[i];
                if (submesh.indexOffset + submesh.indexCount > m_indices.size())
                {
                    TST_CORE_ERROR("Submesh {} has invalid index range: offset={}, count={}, total_indices={}",
                        i, submesh.indexOffset, submesh.indexCount, m_indices.size());

                    // Clamp to valid range
                    m_submeshes[i].indexCount = static_cast<uint32_t>(m_indices.size()) - submesh.indexOffset;
                    if (m_submeshes[i].indexCount == 0)
                    {
                        TST_CORE_ERROR("Submesh {} has no valid indices after clamping", i);
                    }
                }
            }

            TST_CORE_INFO("Successfully loaded mesh with new MaterialSystem: {0}", filepath);
            TST_CORE_INFO("  Vertices: {}, Indices: {}, Submeshes: {}, Materials: {}",
                m_vertices.size(), m_indices.size(), m_submeshes.size(), m_materialIDs.size());
            return true;
        }

        TST_CORE_ERROR("Failed to load mesh from loader: {0}", filepath);
        return false;
    }


}