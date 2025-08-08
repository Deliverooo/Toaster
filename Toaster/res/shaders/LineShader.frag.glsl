#version 330 core

in vec4 v_Colour;
out vec4 FragColour;

void main()
{
    FragColour = v_Colour;
}