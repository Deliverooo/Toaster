#version 450 core

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_Tangent;
in vec3 v_Bitangent;

uniform vec3 u_ViewPos;
uniform samplerCube u_Skybox;

// Material properties
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emissive;
    float shininess;
    float opacity;
    float metallic;
    
    sampler2D diffuseMap;
    sampler2D specularMap;
    sampler2D normalMap;
    
    bool hasDiffuseMap;
    bool hasSpecularMap;
    bool hasNormalMap;
};

struct Light {
    vec3 position;
    vec3 direction;
    vec3 colour;
    float intensity;
};

#define MAX_LIGHTS 32

// Ensure these are declared BEFORE any functions that use them
uniform int u_LightCount;
uniform Material u_Material;
uniform Light u_Lights[MAX_LIGHTS];

out vec4 FragColour;

vec3 calculateNormal()
{
    vec3 normal = normalize(v_Normal);
    
    if (u_Material.hasNormalMap)
    {
        vec3 normalMap = texture(u_Material.normalMap, v_TexCoord).rgb * 2.0 - 1.0;
        vec3 T = normalize(v_Tangent);
        vec3 B = normalize(v_Bitangent);
        vec3 N = normalize(v_Normal);
        mat3 TBN = mat3(T, B, N);
        normal = normalize(TBN * normalMap);
    }
    
    return normal;
}

vec3 calculateLighting(vec3 normal, vec3 viewDir, vec3 albedo, vec3 specularColour)
{
    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);
    
    // Clamp light count to prevent shader errors
    int lightCount = min(u_LightCount, MAX_LIGHTS);
    
    for (int i = 0; i < lightCount; i++)
    {        
        vec3 lightDir = normalize(u_Lights[i].position - v_WorldPos);
        
        float distance = length(u_Lights[i].position - v_WorldPos);
        float attenuation = 1.0 / (1.0 + 0.014 * distance + 0.0007 * distance * distance);
        
        float NdotL = max(dot(normal, lightDir), 0.0);

        vec3 diffuse = u_Lights[i].colour * u_Lights[i].intensity * NdotL * attenuation;
        totalDiffuse += diffuse;
        
        // Specular lighting (Blinn-Phong)
        if (NdotL > 0.0 && u_Material.shininess > 0.0)
        {
            vec3 halfwayDir = normalize(lightDir + viewDir);
            float NdotH = max(dot(normal, halfwayDir), 0.0);
            float spec = pow(NdotH, max(u_Material.shininess, 1.0));

            vec3 specular = u_Lights[i].colour * u_Lights[i].intensity * spec * attenuation;
            totalSpecular += specular;
        }
    }
    
    return totalDiffuse * albedo + totalSpecular * specularColour;
}

void main()
{  
    vec3 normal = calculateNormal();
    vec3 viewDir = normalize(u_ViewPos - v_WorldPos);
    
    // Material colours
    vec3 albedo = u_Material.diffuse;
    float transparency = u_Material.opacity;
   
    if (u_Material.hasDiffuseMap)
    {
        vec4 texColour = texture(u_Material.diffuseMap, v_TexCoord);
        albedo *= texColour.rgb;
        transparency *= texColour.a;
    }

    vec3 specularColour = u_Material.specular;
    if (u_Material.hasSpecularMap)
    {
        specularColour *= texture(u_Material.specularMap, v_TexCoord).rgb;
    }

    // Metallic reflections
    vec3 metallicColour = vec3(0.0);
    if (u_Material.metallic > 0.0)
    {
        vec3 I = normalize(v_WorldPos - u_ViewPos);
        vec3 R = reflect(I, normal);
        vec3 skyboxColour = texture(u_Skybox, R).rgb;
        metallicColour = skyboxColour * u_Material.metallic;
    }

    // Ambient lighting
    vec3 ambient = u_Material.ambient * albedo * 0.1;
    
    // Calculate lighting
    vec3 lighting = calculateLighting(normal, viewDir, albedo, specularColour);
    
    // Emissive
    vec3 emissive = u_Material.emissive;
    
    // Final colour
    vec3 result = ambient + lighting + metallicColour + emissive;
    
    FragColour = vec4(result, transparency);
}