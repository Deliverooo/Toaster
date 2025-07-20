#version 460

in vec3 v_Colour;

out vec4 FragColour;


void main(){
	
	FragColour = vec4(v_Colour, 1.0f);
}