#version 460


out vec4 FragColour;

uniform vec4 u_Colour;

void main(){
	
	FragColour = u_Colour;
}