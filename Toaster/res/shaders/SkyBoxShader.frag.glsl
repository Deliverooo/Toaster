#version 450 core

in vec3 v_TexCoord;

uniform samplerCube u_Skybox;

out vec4 FragColour;

void main() {
	vec3 normCoord = normalize(v_TexCoord);
	FragColour = texture(u_Skybox, normCoord);
}