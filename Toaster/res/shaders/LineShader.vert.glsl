#version 330 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec4 a_Colour;

uniform mat4 u_ViewProj;

out vec4 v_Colour;

void main()
{
    v_Colour = a_Colour;
    gl_Position = u_ViewProj * vec4(a_Position, 1.0);
}