#version 460
in vec3 VertexColour;
in vec3 VertexNormal;

out vec4 FragColour;

vec3 lightPos = vec3(0.1f, 0.5f, 0.7f);

void main(){
	
	float diff = max(dot(VertexNormal, lightPos), 0.05f);
	vec3 diffuse = VertexColour * diff;
	FragColour = vec4(diffuse, 1.0f);
}