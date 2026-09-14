#version 460
#extension GL_EXT_nonuniform_qualifier: require
#extension GL_EXT_buffer_reference2: require
#extension GL_EXT_descriptor_heap: enable
#extension GL_EXT_scalar_block_layout: enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64: enable
#extension GL_EXT_shader_explicit_arithmetic_types_int32: enable

layout (location = 0) out vec2 o_TexCoord;

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    vec2 texCoord;
};

layout (buffer_reference, scalar) readonly buffer VertexBuffer { Vertex vertices[]; };
layout (buffer_reference, scalar) readonly buffer IndexBuffer { uint32_t indices[]; };

layout (buffer_reference, std140) readonly buffer CameraBuffer
{
    mat4 view;
    mat4 proj;
};

layout (push_constant) uniform PushData
{
    CameraBuffer camera;

    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;

    uint vertexBufferOffset;
    uint indexBufferOffset;

    uint texture;
    uint textureSampler;
} pcs;

void main()
{
    uint32_t index = pcs.indexBuffer.indices[pcs.vertexBufferOffset + gl_VertexIndex];
    Vertex vertex = pcs.vertexBuffer.vertices[pcs.indexBufferOffset + index];

    vec4 world_pos = vec4(vertex.position.xyz, 1.0f);

    vec4 view_pos = pcs.camera.view * world_pos;
    gl_Position = pcs.camera.proj * view_pos;

    o_TexCoord = vertex.texCoord;
}