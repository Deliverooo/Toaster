#type Vertex
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform mat3 u_NormalMatrix;

out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out vec3 v_Tangent;
out vec3 v_Bitangent;

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    v_Normal = u_NormalMatrix * a_Normal;
    v_TexCoord = a_TexCoord;
    v_Tangent = u_NormalMatrix * a_Tangent;
    v_Bitangent = u_NormalMatrix * a_Bitangent;
    
    gl_Position = u_ViewProjection * worldPos;
}


#type Fragment
#version 450 core

in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in vec3 v_Tangent;
in vec3 v_Bitangent;

uniform vec3 u_ViewPos;
uniform vec3 u_LightPos;
uniform vec3 u_LightColour;

// Material properties
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    vec3 emissive;
    float shininess;
    float opacity;
    
    sampler2D diffuseMap;
    sampler2D specularMap;
    sampler2D normalMap;
    
    bool hasDiffuseMap;
    bool hasSpecularMap;
    bool hasNormalMap;
};

uniform Material u_Material;

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

void main()
{  

    vec3 normal = calculateNormal();
    vec3 viewDir = normalize(u_ViewPos - v_WorldPos);
    vec3 lightDir = normalize(u_LightPos - v_WorldPos);
    
    // Material colours
    vec3 albedo = u_Material.diffuse;
    
    float transparency = u_Material.opacity;

    if (u_Material.hasDiffuseMap)
    {
        vec4 texColour = vec4(texture(u_Material.diffuseMap, v_TexCoord));

        albedo *= texColour.rgb;

        transparency = texColour.a * u_Material.opacity;
    }

    
    vec3 specularColour = u_Material.specular;
    if (u_Material.hasSpecularMap)
    {
        specularColour *= texture(u_Material.specularMap, v_TexCoord).rgb;
    }
    
    // Simple lighting calculation :)
    vec3 ambient = (u_Material.ambient * albedo + 0.1 * albedo);
    float diff = max(dot(normal, lightDir), 0.0);
    diff = smoothstep(0.0, 0.1, diff) * diff;
    vec3 diffuse = diff * u_LightColour * albedo;
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), max(u_Material.shininess, 16.0f));
    vec3 specular = spec * u_LightColour * specularColour;

    vec3 emissive = u_Material.emissive;
    vec3 result = ambient + diffuse + specular + emissive;
    result.r = clamp(result.r, 0.0, 1.0);
    result.g = clamp(result.g, 0.0, 1.0);
    result.b = clamp(result.b, 0.0, 1.0);
    result = mix(vec3(0.0), result, transparency);
    FragColour = vec4(albedo, transparency);
}