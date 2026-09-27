#version 460
#extension GL_EXT_nonuniform_qualifier: require
#extension GL_EXT_buffer_reference2: require
#extension GL_EXT_descriptor_heap: enable
#extension GL_EXT_scalar_block_layout: enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64: enable
#extension GL_EXT_shader_explicit_arithmetic_types_int32: enable
#extension GL_EXT_shader_explicit_arithmetic_types_int16: enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8: enable

layout (location = 0) out vec3 o_Position;
layout (location = 1) out vec3 o_WorldPos;
layout (location = 2) out vec2 o_TexCoord;
layout (location = 3) flat out uint o_Material;

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 texCoord;
};

layout (descriptor_heap, scalar) readonly buffer VertexBuffer { Vertex vertices[]; } vertexPages[];
layout (descriptor_heap, scalar) readonly buffer IndexBuffer { uint32_t indices[]; } indexPages[];

layout (buffer_reference, std140) readonly buffer CameraBuffer
{
    mat4 view;
    mat4 proj;
    vec4 position;
};

struct ObjectData
{
    uint32_t material;
    uint32_t vertexBufferOffset;
    uint32_t indexBufferOffset;
    uint32_t transformId;

    vec3 aabbMin;
    vec3 aabbMax;

    uint8_t vertexPageId;
    uint8_t indexPageId;

    uint8_t _padd[6u];
};

layout (buffer_reference, scalar) readonly buffer ObjectDataBuffer { ObjectData data[]; };
layout (buffer_reference, scalar) readonly buffer TransformDataBuffer { mat4 data[]; };

layout (push_constant) uniform PushData
{
    CameraBuffer camera;
    ObjectDataBuffer objectBuffer;
    uint64_t materialBuffer;
    TransformDataBuffer transformBuffer;

    uint samplerId;
    uint _padd[1];
} pcs;

void main()
{
    ObjectData data = pcs.objectBuffer.data[gl_BaseInstance];

    uint32_t index = indexPages[data.indexPageId].indices[data.indexBufferOffset + gl_VertexIndex];
    Vertex vertex = vertexPages[data.vertexPageId].vertices[data.vertexBufferOffset + index];

    mat4 transform = pcs.transformBuffer.data[data.transformId];

    vec4 world_pos = transform * vec4(vertex.position.xyz, 1.0f);
    o_Position = vertex.position.xyz;
    o_WorldPos = world_pos.xyz;

    vec4 view_pos = pcs.camera.view * world_pos;
    gl_Position = pcs.camera.proj * view_pos;

    o_TexCoord = vertex.texCoord;
    o_Material = data.material;
}