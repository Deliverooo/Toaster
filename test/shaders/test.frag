#version 460
#define __FRAG__

#extension GL_GOOGLE_include_directive: require
#include "common.glsl"

layout (location = 0u) in PSIn
{
    float2 position;
    float2 texCoord;
} m_In;

layout (location = 0u) out float4 o_Colour;

layout (push_constant) uniform PushData
{
    float2 screenResolution;
    uint32 textureHeapId;
    uint32 samplerHeapId;
} pcs;

void main()
{
    float4 tex_colour = SAMPLE_TEXTURE(pcs.textureHeapId, pcs.samplerHeapId, m_In.texCoord);
    o_Colour = float4(tex_colour.rgb, 1.0f);
}