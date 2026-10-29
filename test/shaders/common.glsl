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

#define TST_BIT(__shift) 1u << __shift

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
    float3 tangent;
    float3 bitangent;
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
layout (buffer_reference, scalar) readonly buffer MaterialIndirectionBuffer { uint32 materials[]; };

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

#define TST_MATERIAL_FLAGS_BACKFACE_CULLING_BIT TST_BIT(0u)
#define TST_MATERIAL_FLAGS_BACKFACE_TRANSPARENT_BIT TST_BIT(1u)
#define TST_MATERIAL_FLAGS_NO_NORMAL_MAP_BIT TST_BIT(2u)
#define TST_MATERIAL_FLAGS_NO_ROUGHNESS_MAP_BIT TST_BIT(3u)
#define TST_MATERIAL_FLAGS_NO_METALNESS_MAP_BIT TST_BIT(4u)

struct Material
{
    float4 albedoColour;
    uint32 albedoMapHeapSlot;
    uint32 normalMapHeapSlot;
    uint32 roughnessMapHeapSlot;
    uint32 metalnessMapHeapSlot;
    float roughness;
    float metalness;
    uint8 flags;
};
layout (buffer_reference, scalar) readonly buffer MaterialBuffer { Material materials[]; };

struct PointLight
{
    float3 position;
    float3 colour;
};
layout (buffer_reference, scalar) readonly buffer PointLightBuffer { PointLight pointLights[]; };

#define OEX_OIT_LAYER_COUNT 2u

struct OITFragment
{
    uint32 colour;
    float depth;
};

struct OITPixel
{
    uint32 count;
    OITFragment fragments[OEX_OIT_LAYER_COUNT];
};

layout (buffer_reference, scalar) coherent buffer OITStorage { OITPixel pixels[]; };

struct PBRGlobals
{
    vec3 normal;
    vec3 view;
    float nDotV;

    float3 f0;
    float4 albedo;
    float roughness;
    float metalness;
};

PBRGlobals glob;

float distributionGGX(vec3 p_normal, vec3 p_halfway, float p_roughness)
{
    float a = p_roughness * p_roughness;
    float a_squared = a * a;
    float n_dot_h = max(dot(p_normal, p_halfway), 0.0);
    float n_dot_h_squared = n_dot_h * n_dot_h;

    float numerator = a_squared;
    float denominator = (n_dot_h_squared * (a_squared - 1.0) + 1.0);
    denominator = PI * denominator * denominator;

    return numerator / denominator;
}

float geometrySchlickGGX(float p_n_dot_v, float p_roughness)
{
    float r = (p_roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float numerator = p_n_dot_v;
    float denominator = p_n_dot_v * (1.0f - k) + k;

    return numerator / denominator;
}

float geometrySmith(float p_n_dot_v, float p_n_dot_l, float p_roughness)
{
    float ggx2 = geometrySchlickGGX(p_n_dot_v, p_roughness);
    float ggx1 = geometrySchlickGGX(p_n_dot_l, p_roughness);

    return ggx1 * ggx2;
}

float3 fresnelSchlick(float3 p_f0, float p_cos_theta)
{
    return p_f0 + (1.0f - p_f0) * pow(clamp(1.0f - p_cos_theta, 0.0f, 1.0f), 5.0f);
}

float3 fresnelSchlickRoughness(float p_cos_theta, float3 p_f0, float p_roughness)
{
    return p_f0 + (max(float3(1.0f - p_roughness), p_f0) - p_f0) * pow(clamp(1.0f - p_cos_theta, 0.0f, 1.0f), 5.0f);
}
