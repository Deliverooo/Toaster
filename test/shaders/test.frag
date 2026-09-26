#version 460
#extension GL_EXT_nonuniform_qualifier: require
#extension GL_EXT_buffer_reference2: require
#extension GL_EXT_descriptor_heap: enable
#extension GL_EXT_scalar_block_layout: enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64: enable

layout (location = 0) in vec3 v_Position;
layout (location = 1) in vec3 v_WorldPos;
layout (location = 2) in vec2 v_TexCoord;
layout (location = 3) flat in uint v_Material;

layout (location = 0) out vec4 o_Colour;

layout (descriptor_heap) uniform texture2D texture2DHeap[];
layout (descriptor_heap) uniform sampler samplerHeap[];

struct Material
{
    vec3 albedoColour;
    uint _padd;
    uint albedoMapHeapSlot;
    uint normalMapHeapSlot;
    uint _padd2[2];
};
layout (buffer_reference, scalar) readonly buffer MaterialBuffer { Material materials[]; };

layout (buffer_reference, std140) readonly buffer CameraBuffer
{
    mat4 view;
    mat4 proj;
    vec4 position;
};

layout (push_constant) uniform PushData
{
    CameraBuffer camera;
    uint64_t objectBuffer;
    MaterialBuffer materialBuffer;

    uint _padd[1];
    uint samplerId;
} pcs;

#define SAMPLE_TEXTURE(__textureId, __samplerId, __texCoord) texture(sampler2D(texture2DHeap[__textureId], samplerHeap[__samplerId]), __texCoord)

void main()
{
    Material material = pcs.materialBuffer.materials[v_Material];

    vec3 tex_colour = SAMPLE_TEXTURE(material.albedoMapHeapSlot, pcs.samplerId, v_TexCoord).rgb;

    vec3 final_colour = tex_colour * material.albedoColour;

    o_Colour = vec4(tex_colour, 1.0f);
}