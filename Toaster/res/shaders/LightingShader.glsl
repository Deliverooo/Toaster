#type vertex
#version 330 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColour;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in float inTexIndex;
layout(location = 5) in float inTilingFactor;
//layout(location = 6) in float inMaterialId;

uniform mat4 u_Projection;
uniform mat4 u_View;

out vec3 v_FragPos;
out vec3 v_Normal;
out vec4 v_Colour;
out vec2 v_TexCoord;
out float v_TexIndex;
out float v_TilingFactor;
//out float v_MaterialId;

void main()
{
    v_FragPos = inPosition;
    v_Normal = inNormal;
    v_Colour = inColour;
    v_TexCoord = inTexCoord;
    v_TexIndex = inTexIndex;
    v_TilingFactor = inTilingFactor;
    //v_MaterialId = inMaterialId;

    gl_Position = u_Projection * u_View * vec4(inPosition, 1.0);
}

#type fragment
#version 330 core

struct Light {
    vec3 position;
    vec3 colour;
    float intensity;
    float radius;
    int type; // 0 = directional, 1 = point, 2 = spot
};

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

#define MAX_LIGHTS 16
#define MAX_MATERIALS 32

//uniform Light u_Lights[MAX_LIGHTS];
//uniform Material u_Materials[MAX_MATERIALS];
//uniform int u_LightCount;
//uniform vec3 u_CameraPos;
uniform sampler2D u_Textures[32];

in vec3 v_FragPos;
in vec3 v_Normal;
in vec4 v_Colour;
in vec2 v_TexCoord;
in float v_TexIndex;
in float v_TilingFactor;
//in float v_MaterialId;

out vec4 FragColour;

vec3 calculateDirectionalLight(Light light, Material material, vec3 normal, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(-light.position); // For directional lights, position is direction
    
    // Ambient
    vec3 ambient = light.colour * material.ambient * light.intensity;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.colour * (diff * material.diffuse) * light.intensity;
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = light.colour * (spec * material.specular) * light.intensity;
    
    return (ambient + diffuse + specular) * albedo;
}

vec3 calculatePointLight(Light light, Material material, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distance = length(light.position - fragPos);
    
    // Attenuation
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    if (light.radius > 0.0) {
        attenuation *= max(0.0, 1.0 - (distance / light.radius));
    }
    
    // Ambient
    vec3 ambient = light.colour * material.ambient * light.intensity * attenuation;
    
    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.colour * (diff * material.diffuse) * light.intensity * attenuation;
    
    // Specular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    vec3 specular = light.colour * (spec * material.specular) * light.intensity * attenuation;
    
    return (ambient + diffuse + specular) * albedo;
}

Light light0 = Light(vec3(0.0, 10.0, 0.0), vec3(1.0, 1.0, 1.0), 1.0, 50.0, 1);

Material defaultMaterial = Material(vec3(0.1, 0.1, 0.1), vec3(0.8, 0.8, 0.8), vec3(1.0, 1.0, 1.0), 32.0);


void main()
{
    // Sample texture
    vec4 texColour = v_Colour;
    int texIndex = int(v_TexIndex);
    if (texIndex > 0) {
        // Texture sampling (simplified - you'd need proper texture array handling)
        texColour *= texture(u_Textures[texIndex], v_TexCoord * v_TilingFactor);
    }
    
    vec3 albedo = texColour.rgb;
    float alpha = texColour.a;
    
    // Get material
    //int matId = int(v_MaterialId);
    //Material material = u_Materials[matId];
    
    vec3 normal = normalize(v_Normal);
    vec3 viewDir = normalize(u_CameraPos - v_FragPos);
    
    vec3 result = vec3(0.0);
    
    result += calculatePointLight(light0, defaultMaterial, normal, v_FragPos, viewDir, albedo);


    
    
    FragColour = vec4(result, alpha);
}