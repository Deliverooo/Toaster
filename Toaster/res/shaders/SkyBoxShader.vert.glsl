#version 450 core

layout(location = 0) in vec3 inPos;

uniform mat4 u_ViewProjection;

out vec3 v_TexCoord;

void main() {
	v_TexCoord = inPos;
	vec4 pos = u_ViewProjection * vec4(inPos, 1.0f);
	gl_Position = pos.xyww;
}