#version 450 core


// The position of the current interpolated fragment from the vertex shader
in vec3 v_FragPos;

in vec3 v_Normal;
in vec2 v_TexCoord;

// Tangent and bitangent are used for normal mapping
in vec3 v_Tangent;
in vec3 v_Bitangent;

in flat int v_EntityId;

uniform vec3 u_ViewPos;

// While this not neccesaraly needed, having a skybox texture allows us to create reflections that mirror the environment textures around them
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
    float roughness;
    
    sampler2D diffuseMap;
    sampler2D specularMap;
    sampler2D normalMap;
    sampler2D heightMap;
    
    bool hasDiffuseMap;
    bool hasSpecularMap;
    bool hasNormalMap;
    bool hasHeightMap;
};

struct Light {

    // The type of light: 1 -> Directional | 2 -> Point | 3 -> Spot
    int type;

    // The position of the light in world space
    vec3 position;
    
    // The direction of the light, this is represented by the light's rotation. Note: this only applies to point and spot ligthts
    vec3 direction;

    // the radian colour emmited from the light source
    vec3 colour;

    // used as a multiplier for the colour, representing the power of the light
    float intensity;


    // Attenuation terms

    // The constant attenuation term, this is usually kept at 1
    float constant;
    // The linear attenuation term, this is used to model the linear decrease of the light radiance over an arbitry distance
    float linear;
    // The same as the linear term, but at a quadratic rate
    float quadratic;

    // The inner and outer cones for a spot light, outer is a sort of softener
    float innerCone;
    float outerCone;
};


// The max amount of lights allowed in one scene at a time (provided we are using this shader)
// Excceding this limit will not instigate a crash, but the extra added lights won't be contributing any illuminosity
#define MAX_LIGHTS 32


// The uniform telling the shader how many lights are currently in the scene
uniform int u_LightCount;
uniform Light u_Lights[MAX_LIGHTS];

// The currently bound material
uniform Material u_Material;


// The final output colour to be displayed
out vec4 FragColour;
out int EntityId;

// pie
#define PI 3.14159265359

// Trowbridge-Reitz GGX normal distribution function
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return a2 / max(denom, 0.0000001);
}

// Smith's method for geometric shadowing function
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / max(denom, 0.0000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// Fresnel-Schlick approximation
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Improved normal mapping
vec3 getNormalFromMap() {
    if (!u_Material.hasNormalMap) {
        return normalize(v_Normal);
    }
    
    vec3 tangentNormal = texture(u_Material.normalMap, v_TexCoord).xyz * 2.0 - 1.0;
    
    vec3 N = normalize(v_Normal);
    vec3 T = normalize(v_Tangent);
    vec3 B = normalize(v_Bitangent);
    mat3 TBN = mat3(T, B, N);
    
    return normalize(TBN * tangentNormal);
}

// Tone mapping function
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main() {
    vec3 albedo = u_Material.diffuse;
    float metallic = u_Material.metallic;
    float roughness = u_Material.roughness;
    float ao = 1.0;
    
    float transparency = u_Material.opacity;
    if (u_Material.hasDiffuseMap) {
        vec4 diffuseSample = texture(u_Material.diffuseMap, v_TexCoord);
        albedo *= diffuseSample.rgb;

        transparency *= diffuseSample.a;
    }
    
    if (u_Material.hasSpecularMap) {
        vec3 specularSample = texture(u_Material.specularMap, v_TexCoord).rgb;
        metallic = specularSample.r;
        roughness = specularSample.g;
        ao = specularSample.b;
    }
    
    vec3 N = getNormalFromMap();
    vec3 V = normalize(u_ViewPos - v_FragPos);
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Reflectance equation
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < u_LightCount; i++) {

    if (u_Lights[i].type == 0) {
            // Directional light
            vec3 L = normalize(-u_Lights[i].direction);
            vec3 H = normalize(V + L);
            float distance = 1.0; // Directional lights are at infinity
            float attenuation = 1.0; // No attenuation for directional lights
            vec3 radiance = u_Lights[i].colour * u_Lights[i].intensity * attenuation;
            
            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, L, roughness);
            vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
            
            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;
            
            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular = numerator / denominator;
            
            // Add to outgoing radiance Lo
            float NdotL = max(dot(N, L), 0.0);
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        }
        else if (u_Lights[i].type == 1) {
            // Point light
            vec3 L = normalize(u_Lights[i].position - v_FragPos);
            vec3 H = normalize(V + L);
            float distance = length(u_Lights[i].position - v_FragPos);
            float attenuation = 1.0 / (distance * distance);
            vec3 radiance = u_Lights[i].colour * u_Lights[i].intensity * attenuation;
            
            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, L, roughness);
            vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
            
            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;
            
            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular = numerator / denominator;
            
            // Add to outgoing radiance Lo
            float NdotL = max(dot(N, L), 0.0);
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        }
        else if (u_Lights[i].type == 2)
        {
            // Spot light
            vec3 L = normalize(u_Lights[i].position - v_FragPos);
            vec3 H = normalize(V + L);
            float distance = length(u_Lights[i].position - v_FragPos);
            float attenuation = 1.0 / (distance * distance);

            float coneAngle = acos(dot(L, normalize(u_Lights[i].direction)));
            float innerCone = radians(u_Lights[i].innerCone);
            float outerCone = radians(u_Lights[i].outerCone);

            if (outerCone - innerCone > 0.00001f) {
                // Linear interpolation between inner and outer cone
                float coneFactor = (outerCone - coneAngle) / (outerCone - innerCone);
                attenuation *= coneFactor;
            }

            attenuation *= 1.0 / (u_Lights[i].constant + u_Lights[i].linear * distance + u_Lights[i].quadratic * distance * distance);
            if (attenuation < 0.0) attenuation = 0.0; // Ensure non-negative attenuation
            if (attenuation > 1.0) attenuation = 1.0; // Clamp to 1.0
            // Radiance calculation
            vec3 radiance = u_Lights[i].colour * u_Lights[i].intensity * attenuation;
            
            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, L, roughness);
            vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
            
            vec3 kS = F;
            vec3 kD = vec3(1.0) - kS;
            kD *= 1.0 - metallic;
            
            vec3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
            vec3 specular = numerator / denominator;
            
            // Add to outgoing radiance Lo
            float NdotL = max(dot(N, L), 0.0);
            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        }


    }
    
    // Ambient lighting
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 colour = ambient + Lo;
    
    // HDR tonemapping
    colour = ACESFilm(colour);
    
    // Gamma correction
    const float gamma = 2.2;
    colour = pow(colour, vec3(1.0f / gamma));
    
    FragColour = vec4(colour, transparency);

    EntityId = v_EntityId;
}