#version 330 core
out vec4 FragColour;

in vec3 TexCoords;

uniform samplerCube skybox;

void main()
{    
    FragColour = texture(skybox, TexCoords);
}