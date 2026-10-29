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

vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float perlinNoise(vec2 st) {
    vec2 p = floor(st);
    vec2 f = fract(st);

    // Smooth interpolation curve (quintic or cubic)
    vec2 u = f * f * (3.0 - 2.0 * f);

    // Mix dot products from the 4 corners of the cell
    float n00 = dot(hash(p + vec2(0.0, 0.0)), f - vec2(0.0, 0.0));
    float n10 = dot(hash(p + vec2(1.0, 0.0)), f - vec2(1.0, 0.0));
    float n01 = dot(hash(p + vec2(0.0, 1.0)), f - vec2(0.0, 1.0));
    float n11 = dot(hash(p + vec2(1.0, 1.0)), f - vec2(1.0, 1.0));

    return mix(mix(n00, n10, u.x), mix(n01, n11, u.x), u.y);
}

void main()
{
    float4 tex_colour = SAMPLE_TEXTURE(pcs.textureHeapId, pcs.samplerHeapId, m_In.texCoord);
    o_Colour = float4(float3(perlinNoise(m_In.texCoord * 100.0f)), 1.0f);
}