#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec2 inUV;

layout(set = 0, binding = 0) uniform SceneData {

   mat4 View;
   mat4 Proj;
   mat4 ViewProj;
   mat4 InverseProj;
   mat4 InverseView;

   vec3 CameraPos;
   float AmbientIntensity;

} SceneDataBuffer;

struct Light {

   vec3 color;
   float intensity;

   vec3 position;
   float innerAngle;

   vec3 direction;
   float outerAngle;

   int type;
   float radius;

   int extra[2];
};

// GBuffer textures
layout(set = 1, binding = 0) readonly buffer Lights {
    Light lights[];
};

layout(set = 1, binding = 1) uniform sampler2D albedoTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D materialTexture;
layout(set = 1, binding = 4) uniform sampler2D depthTexture;
layout(set = 1, binding = 5) uniform samplerCube skyboxTexture;

#define PI        3.14159265359
#define TAU       6.28318530718

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIRECTIONAL 2

float DistributionGGX(vec3 N, vec3 H, float roughness) {

    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 DepthToWorld(vec2 screenPos, float depth) {

    vec4 clipSpacePos = vec4(screenPos*2.0 - 1.0, depth, 1.0);
    vec4 viewSpacePos = SceneDataBuffer.InverseProj * clipSpacePos;
    viewSpacePos /= viewSpacePos.w;
    vec4 worldSpacePos = SceneDataBuffer.InverseView * viewSpacePos;
    return worldSpacePos.xyz;
}

// Generates low-discrepancy sample points
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

// Generates Hammersley point for stratified sampling
vec2 Hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    
    float phi = 2.0 * 3.14159265359 * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    // Convert from tangent space to world space
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

vec2 ComputeBRDF(float NdotV, float roughness) {
    const int SAMPLE_COUNT = 1024; // More samples = more accuracy, but slower
    vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
    
    float A = 0.0;
    float B = 0.0;
    
    vec3 N = vec3(0.0, 0.0, 1.0);
    
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);
        
        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        
        if (NdotL > 0.0) {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);
            
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    
    return vec2(A, B) / float(SAMPLE_COUNT);
}

void main() {

    vec4 albedo = pow(texture(albedoTexture, inUV), vec4(2.2));
    vec3 N = texture(normalTexture, inUV).xyz;
    vec4 material = texture(materialTexture, inUV);
    float depth = texture(depthTexture, inUV).r;

    if ((length(albedo) + length(N) + length(material) + depth) == 0.0f) {
    
        discard;
    }

    vec3 ambientColor = vec3(0.6);

    if (length(N) == 0.0f) {
    
        outColor = vec4(ambientColor * SceneDataBuffer.AmbientIntensity, 1.0);
        return;
    }

    float occlusion = material.b;
    float roughness = material.r;
    float metallic = material.g;
    vec3 fragPos = DepthToWorld(inUV, depth);
    vec3 V = normalize(SceneDataBuffer.CameraPos - fragPos.xyz);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo.rgb, metallic);
    vec3 Lo = vec3(0.0);

    for (int i = 0; i < lights.length(); i++) {
    
        Light light = lights[i];
        vec3 L_ = light.position - fragPos.xyz;
        vec3 L = normalize(L_);
        float attenuation = 1;

        if (light.type == LIGHT_TYPE_DIRECTIONAL) {
        
            L = normalize(-light.direction);

        } else if (light.type == LIGHT_TYPE_SPOT) {
        
            float dist = length(light.position - fragPos.xyz);
            attenuation = 1.0 / (dist*dist);
            float theta = dot(L, normalize(-light.direction));
            float epsilon = light.innerAngle - light.outerAngle;
            attenuation *= clamp((theta - light.outerAngle)/epsilon, 0.0, 1.0);

        } else if (light.type == LIGHT_TYPE_POINT) {
        
            float dist = length(light.position - fragPos.xyz);
            attenuation = 1.0 / (dist*dist);
        }

        vec3 radiance = light.color * light.intensity * attenuation;

        vec3 H = normalize(V + L);
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        vec3 num = NDF * G * F;
        float denom = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 spec = num / denom;

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo.rgb / PI + spec)*radiance*NdotL;
    }

     vec3 ambient = vec3(0.0);
    {
        vec3 R = reflect(-V, N);
        vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;

        vec3 diffuse    = vec3(0.03) * albedo.rgb;

        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(skyboxTexture, R,  roughness * MAX_REFLECTION_LOD).rgb; 
        vec2 envBRDF = ComputeBRDF(max(dot(N, V), 0.0), roughness);

        vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

        ambient = (kD * diffuse + specular) * occlusion;
    }

    vec3 color = Lo + ambient;
    outColor = vec4(color, 1.0);
}