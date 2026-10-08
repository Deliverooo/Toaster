#version 460
#define __FRAG__

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout (location = 0u) in float3 v_Position;
layout (location = 1u) in float3 v_WorldPos;
layout (location = 2u) in float2 v_TexCoord;
layout (location = 3u) flat in uint32 v_Material;

layout (location = 0u) out float4 o_Colour;

layout (push_constant) uniform PushData
{
    CameraBuffer camera;
    uint64 objectBuffer;
    MaterialBuffer materialBuffer;
    uint64 transformBuffer;

    uint32 samplerId;
    uint32 _padd[1u];
} pcs;

void main()
{
    Material material = pcs.materialBuffer.materials[v_Material];

    float3 tex_colour = SAMPLE_TEXTURE(material.albedoMapHeapSlot, pcs.samplerId, v_TexCoord).rgb;
    float3 final_colour = tex_colour * material.albedoColour;

    o_Colour = float4(tex_colour, 1.0f);
}