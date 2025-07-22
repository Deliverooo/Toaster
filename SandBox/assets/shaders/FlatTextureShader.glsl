#type Vertex

#version 460
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColour;
layout(location = 2) in vec2 inTexCoords;


uniform mat4 u_Projection;
uniform mat4 u_View;

out vec2 v_TextureCoords;
out vec4 v_Colour;

void main(){

	v_TextureCoords = inTexCoords;
	v_Colour = inColour;

	gl_Position = u_Projection * u_View * vec4(inPos, 1.0f);
}




#type Fragment
#version 460

in vec2 v_TextureCoords;
in vec4 v_Colour;
out vec4 FragColour;

uniform vec4 u_Colour;
uniform sampler2D u_Tex0;

uniform vec4 u_TintColour;
uniform float u_TilingScale;

void main(){

	//FragColour = texture(u_Tex0, v_TextureCoords * u_TilingScale) * u_Colour * u_TintColour;
	FragColour = v_Colour;
}
