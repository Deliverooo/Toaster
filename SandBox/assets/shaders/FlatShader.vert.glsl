#version 460
layout(location = 0) in vec3 inPos;

uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_Transform;

void main(){

	gl_Position = u_Projection * u_View * u_Transform * vec4(inPos, 1.0f);
}