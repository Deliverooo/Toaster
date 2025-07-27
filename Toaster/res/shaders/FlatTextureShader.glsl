#type Vertex

#version 450 core
layout(location = 0) in vec3  inPos;
layout(location = 1) in vec4  inColour;
layout(location = 2) in vec2  inTexCoords;
layout(location = 3) in float inTexIndex;
layout(location = 4) in float inTilingFactor;


uniform mat4 u_Projection;
uniform mat4 u_View;

out vec4  v_Colour;
out vec2  v_TextureCoords;
out float v_TextureIndex;
out float v_TilingFactor;

void main(){

	v_Colour		 = inColour;
	v_TextureCoords	 = inTexCoords;
	v_TextureIndex	 = inTexIndex;
	v_TilingFactor	 = inTilingFactor;

	gl_Position = u_Projection * u_View * vec4(inPos, 1.0f);
}

// ------------ Fragment Shader ------------

#type Fragment
#version 450 core

in vec4  v_Colour;
in vec2  v_TextureCoords;
flat in float v_TextureIndex;
in float v_TilingFactor;

out vec4 FragColour;

uniform sampler2D	u_Textures[32];



void main(){



	FragColour = texture(u_Textures[int(v_TextureIndex)], v_TextureCoords * v_TilingFactor) * v_Colour;

}