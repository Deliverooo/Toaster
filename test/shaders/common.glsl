// All the required extensions
#extension GL_EXT_nonuniform_qualifier: require
#extension GL_KHR_shader_subgroup_ballot: require
#extension GL_EXT_buffer_reference2: require
#extension GL_EXT_descriptor_heap: require
#extension GL_EXT_scalar_block_layout: require

#if defined(__MESH__) || defined(__TASK__)
    #extension GL_EXT_mesh_shader: require
#endif

#extension GL_EXT_shader_explicit_arithmetic_types_int64: require
#extension GL_EXT_shader_explicit_arithmetic_types_int32: require
#extension GL_EXT_shader_explicit_arithmetic_types_int16: require
#extension GL_EXT_shader_explicit_arithmetic_types_int8: require

#define PI 3.14159265358f

// For NVIDIA GPUs
#define WARP_SIZE 32u

// GLSL has no typedef, so I use this instead to match the tyes of toaster
#define int8 int8_t
#define int16 int16_t
#define int32 int32_t
#define int64 int64_t

#define uint8 uint8_t
#define uint16 uint16_t
#define uint32 uint32_t
#define uint64 uint64_t

#define uintptr uint64
#define intptr int64

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define int2 ivec2
#define int3 ivec3
#define int4 ivec4

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define float2x2 mat2
#define float3x3 mat3
#define float4x4 mat4

struct Vertex
{
    float3 position;
    float3 normal;
    float2 texCoord;
};

struct Meshlet
{
    uint32 materialIndex;

    uint32 vertexOffset;
    uint32 triangleOffset;
    uint32 vertexCount;
    uint32 triangleCount;

    float4 boundingSphere;
};

layout (descriptor_heap, scalar) readonly buffer VertexBuffer { Vertex vertices[]; } vertexPages[];
layout (descriptor_heap, scalar) readonly buffer MeshletBuffer { Meshlet meshlets[]; } meshletPages[];
layout (descriptor_heap, scalar) readonly buffer MeshletVertexBuffer { uint32 vertices[]; } meshletVertexPages[];
layout (descriptor_heap, scalar) readonly buffer MeshletTriangleBuffer { uint8 indices[]; } meshletTrianglePages[];
layout (descriptor_heap, scalar) readonly buffer MaterialIndirectionBuffer { uint32 materials[]; } materialIndirectionPages[];

struct MeshMetadata
{
    float4 boundingSphere;

    uint32 meshletCount;

    uint32 vertexBufferOffset;
    uint32 meshletBufferOffset;
    uint32 meshletVertexBufferOffset;
    uint32 meshletTriangleBufferOffset;
    uint32 materialIndirectionBufferOffset;

    uint8 vertexBufferPageId;
    uint8 meshletBufferPageId;
    uint8 meshletVertexBufferPageId;
    uint8 meshletTriangleBufferPageId;
    uint8 materialIndirectionBufferPageId;
};

layout (buffer_reference, scalar) readonly buffer MeshMetadataBuffer { MeshMetadata metadata[]; };

layout (buffer_reference, std140) readonly buffer CameraBuffer
{
    float4x4 view;
    float4x4 proj;
    float4x4 invProj;

    float4 position;
    float4 frustumPlanes[6u];
};

struct ObjectData
{
    uint32 meshId;
    uint32 transformId;
};

layout (buffer_reference, scalar) readonly buffer ObjectDataBuffer { ObjectData data[]; };
layout (buffer_reference, scalar) readonly buffer TransformDataBuffer { float4x4 data[]; };

layout (descriptor_heap) uniform texture2D texture2DHeap[];
layout (descriptor_heap) uniform textureCube textureCubeHeap[];
layout (descriptor_heap) uniform sampler samplerHeap[];

#define SAMPLE_TEXTURE(__textureId, __samplerId, __texCoord) texture(sampler2D(texture2DHeap[__textureId], samplerHeap[__samplerId]), __texCoord)
#define SAMPLE_CUBE(__textureId, __samplerId, __texCoord, __lod) textureLod(samplerCube(textureCubeHeap[__textureId], samplerHeap[__samplerId]), __texCoord, __lod)

struct Material
{
    float3 albedoColour;
    uint32 albedoMapHeapSlot;
    uint32 normalMapHeapSlot;
    uint32 backfaceCulling;
};
layout (buffer_reference, scalar) readonly buffer MaterialBuffer { Material materials[]; };