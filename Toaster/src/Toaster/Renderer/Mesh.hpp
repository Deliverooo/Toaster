#pragma once

#include "Toaster/Renderer/VertexArray.hpp"
#include "Toaster/Renderer/Buffer.hpp"
#include "Toaster/Renderer/Texture.hpp"

#include <vector>
#include <string>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

// Include GLM comparison operators
#include <glm/gtc/matrix_transform.hpp>

#include "Material.hpp"
#include "MaterialSystem.hpp"


namespace tst
{
    enum class MeshFormat
    {
        Unknown = 0,
        OBJ,
        FBX,
        GLTF,
        GLB,
        STL,
        BLEND,

    };
    struct MeshVertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 textureCoords;
        glm::vec3 tangent;
        glm::vec3 bitangent;


        // Add explicit comparison operators for the struct
        bool operator==(const MeshVertex& other) const {
            return position == other.position &&
                normal == other.normal &&
                textureCoords == other.textureCoords;
        }

        bool operator!=(const MeshVertex& other) const {
            return !(*this == other);
        }
    };

    struct SubMesh
    {
        uint32_t indexOffset;
        uint32_t indexCount;
        MaterialID materialId;

        SubMesh() : indexOffset(0), indexCount(0), materialId(TST_DEFAULT_MATERIAL) {}
    };

    class MeshLoader;
    class ObjLoader;
    class FbxLoader;
    class BlendLoader;
    class GltfLoader;
    class StlLoader;
    class UsdLoader;

    class Mesh
    {
    public:
        Mesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices);
        Mesh(const std::string& filepath);
        ~Mesh() = default;

        void bind() const;
        void unbind() const;

        const std::vector<MeshVertex>& getVertices() const { return m_vertices; }
        const std::vector<uint32_t>& getIndices() const { return m_indices; }
        const std::vector<SubMesh>& getSubMeshes() const { return m_submeshes; }

        // New methods for MaterialSystem integration
        const std::vector<MaterialID>& getMaterialIDs() const { return m_materialIDs; }
        RefPtr<Material> getMaterial(uint32_t submeshIndex) const;
        MaterialID getMaterialID(uint32_t submeshIndex) const;

        std::string getFilepath() const { return ((m_Filepath.has_value()) ? m_Filepath.value() : "Null"); }

        uint32_t getVertexCount() const { return static_cast<uint32_t>(m_vertices.size()); }
        uint32_t getIndexCount() const { return static_cast<uint32_t>(m_indices.size()); }

        static RefPtr<Mesh> create(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices);
        static RefPtr<Mesh> create(const std::string& filepath);

        const RefPtr<VertexArray>& getVertexArray() const { return m_vertexArray; }

    private:
        void setupMesh();
        bool loadFromFile(const std::string& filepath);

        void calculateTangents();

        MeshFormat detectMeshFormat(const std::string& filepath);

        bool loadWithFormat(const std::string& filepath, MeshFormat format);

        static void initLoaders();

        std::vector<MeshVertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<SubMesh> m_submeshes;

        std::vector<MaterialID> m_materialIDs;

        RefPtr<VertexArray> m_vertexArray;
        RefPtr<VertexBuffer> m_vertexBuffer;
        RefPtr<IndexBuffer> m_indexBuffer;

        std::optional<std::string> m_Filepath;

        static std::unordered_map<MeshFormat, ScopedPtr<MeshLoader>> s_loaders;
        static bool s_loadersInitialized;
    };


    class MeshLoader
    {
    public:
        virtual ~MeshLoader() = default;

        // -=[LEGACY]=-
        virtual bool load(const std::string& filepath,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes,
            MaterialLibrary& materials) {return false;};
        // -=[LEGACY]=-

        virtual bool load(const std::string& filepath,
            std::vector<MeshVertex>& vertices,
            std::vector<uint32_t>& indices,
            std::vector<SubMesh>& submeshes,
            std::vector<MaterialID>& materials) = 0;

        virtual std::vector<std::string> getSupportedExtensions() const = 0;
    };
}

namespace std {
    template<>
    struct hash<tst::MeshVertex> {
        size_t operator()(const tst::MeshVertex& vertex) const noexcept {
            size_t h1 = 0;
            size_t h2 = 0;
            size_t h3 = 0;

            // Hash position manually to avoid GLM conflicts
            h1 = std::hash<float>{}(vertex.position.x) ^
                (std::hash<float>{}(vertex.position.y) << 1) ^
                (std::hash<float>{}(vertex.position.z) << 2);

            // Hash normal manually
            h2 = std::hash<float>{}(vertex.normal.x) ^
                (std::hash<float>{}(vertex.normal.y) << 1) ^
                (std::hash<float>{}(vertex.normal.z) << 2);

            // Hash texture coordinates manually
            h3 = std::hash<float>{}(vertex.textureCoords.x) ^
                (std::hash<float>{}(vertex.textureCoords.y) << 1);

            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}