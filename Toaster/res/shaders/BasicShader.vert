#version 460
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inCol;
layout(location = 2) in vec3 inNorm;
layout(location = 3) in vec2 inTexCoords;

out vec3 VertexColour;
out vec3 VertexNormal;
out vec2 TextureCoords;

uniform mat4 transform;
uniform mat4 view;
uniform mat4 projection;

void main(){

	VertexColour = inCol;
	VertexNormal = normalize(inNorm);
	TextureCoords = inTexCoords;
	gl_Position = projection * view * transform * vec4(inPos, 1.0f);
}