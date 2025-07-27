#version 460

in vec3 v_Colour;
in vec3 v_Normal;
in vec2 v_TextureCoords;

out vec4 FragColour;

uniform sampler2D u_Tex0;

void main(){
	
	vec3 s_Tex0 = vec3(texture(u_Tex0, v_TextureCoords));


	FragColour = texture(u_Tex0, v_TextureCoords);

}