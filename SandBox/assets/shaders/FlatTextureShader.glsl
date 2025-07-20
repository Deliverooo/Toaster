#type Vertex


#version 460
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inTexCoords;


uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_Transform;

out vec2 v_TextureCoords;

void main(){

	v_TextureCoords = inTexCoords;

	gl_Position = u_Projection * u_View * u_Transform * vec4(inPos, 1.0f);
}

#type Fragment
#version 460

in vec2 v_TextureCoords;
out vec4 FragColour;
uniform sampler2D u_Tex0;

void main(){

	FragColour = texture(u_Tex0, v_TextureCoords);
}
