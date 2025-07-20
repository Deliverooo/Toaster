#version 460
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inCol;
layout(location = 2) in vec3 inNorm;
layout(location = 3) in vec2 inTexCoords;


uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_Transform;

out vec3 v_Colour;
out vec3 v_Normal;
out vec2 v_TextureCoords;

void main(){

	v_Colour = inCol;
	v_Normal = inNorm;
	v_TextureCoords = inTexCoords;

	gl_Position = u_Projection * u_View * u_Transform * vec4(inPos, 1.0f);
}