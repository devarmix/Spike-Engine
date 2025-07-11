#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

struct SceneDrawData {

	mat4 View;
	mat4 Proj;
	mat4 ViewProj;
	mat4 InverseProj;
	mat4 InverseView;

	vec4 CameraPos;

	float AmbientIntensity;
	float NearProj;
	float FarProj;

	float pad0;
};

#define MAX_SCENE_DRAWS_PER_FRAME 8

layout(std140, set = 0, binding = 0) uniform DeclSceneDataBuffer {

	SceneDrawData SceneData[MAX_SCENE_DRAWS_PER_FRAME];
} SceneDataBuffer;

layout(set = 0, binding = 1) uniform sampler2D AlbedoMap;
layout(set = 0, binding = 2) uniform sampler2D NormalMap;
layout(set = 0, binding = 3) uniform sampler2D MaterialMap;  // Metallic, Roughness, AO
layout(set = 0, binding = 4) uniform sampler2D DepthMap;

layout(set = 0, binding = 5) uniform samplerCube EnvMap;
layout(set = 0, binding = 7) uniform samplerCube IrrMap;
layout(set = 0, binding = 8) uniform sampler2D BRDFMap;
layout(set = 0, binding = 9) uniform sampler2D SSAOMap;

struct SceneLight {

    vec4 Position;
    vec4 Direction; // for directional and spot
    vec4 Color;

    float Intensity;
    int Type; // 0 = directional, 1 = point, 2 = spot
    float InnerConeCos;
    float OuterConeCos;
};

layout(std430, set = 0, binding = 6) readonly buffer DeclLightsBuffer {

    SceneLight Lights[];
} LightsBuffer;

struct LightingData {

	uint SceneDataOffset;
	uint LightsOffset;
	uint LightsCount;
	uint EnvironmentMapNumMips;
};

layout(push_constant) uniform Constants {

    LightingData LightingConstants;
};

#define PI 3.1415926535897932384626433832795

vec3 DepthToWorld(vec2 screenPos, float depth) {

    vec4 clipSpacePos = vec4(screenPos*2.0 - 1.0, depth, 1.0);
    vec4 viewSpacePos = SceneDataBuffer.SceneData[LightingConstants.SceneDataOffset].InverseProj * clipSpacePos;
    viewSpacePos /= viewSpacePos.w;
    vec4 worldSpacePos = SceneDataBuffer.SceneData[LightingConstants.SceneDataOffset].InverseView * viewSpacePos;
    return worldSpacePos.xyz;
}

struct Material
{
    vec3 Albedo;
    float MetallicFactor;
    float RoughnessFactor;
    float AO;
};

struct PBRinfo
{
    // cos angle between normal and light direction.
	float NdotL;          
    // cos angle between normal and view direction.
	float NdotV;          
    // cos angle between normal and half vector.
	float NdotH;          
    // cos angle between view direction and half vector.
	float VdotH;
    // Roughness value, as authored by the model creator.
    float PerceptualRoughness;
    // Roughness mapped to a more linear value.
    float AlphaRoughness;
    // color contribution from diffuse lighting.
	vec3 DiffuseColor;    
    // color contribution from specular lighting.
	vec3 SpecularColor;
    // full reflectance color(normal incidence angle)
    vec3 Reflectance0;
    // reflectance color at grazing angle
    vec3 Reflectance90;
};

struct IBLinfo
{
    vec3 DiffuseLight;
    vec3 SpecularLight;
    vec3 Brdf;
};

float DistributionGGX(float nDotH, float rough) {

    float a = rough * rough;
    float a2 = a * a;

    float denominator = nDotH * nDotH * (a2 - 1.0f) + 1.0f;
    denominator = 1 / (PI * denominator * denominator);

    return a2 * denominator;
}

float GeometricOcclusion(PBRinfo pbrInfo) {

    float alphaRoughness2 = pbrInfo.AlphaRoughness * pbrInfo.AlphaRoughness;
    float NdotL2 = pbrInfo.NdotL * pbrInfo.NdotL;
    float NdotV2 = pbrInfo.NdotV * pbrInfo.NdotV;

    float attenuationL = (2.0f * pbrInfo.NdotL / (pbrInfo.NdotL + sqrt(alphaRoughness2 + (1.0f - alphaRoughness2) * (NdotL2))));
    float attenuationV = (2.0f * pbrInfo.NdotV / (pbrInfo.NdotV + sqrt(alphaRoughness2 + (1.0f - alphaRoughness2) * (NdotV2))));

    return attenuationL * attenuationV;
}

vec3 FresnelSchlick(PBRinfo pbrInfo) {

    return (pbrInfo.Reflectance0 + (pbrInfo.Reflectance90 - pbrInfo.Reflectance0) * pow(1.0f - pbrInfo.VdotH, 5.0f));
}

vec3 CalculateDirLight(uint i, vec3 normal, vec3 view, Material material, PBRinfo pbrInfo) {

    vec3 lightDir = normalize(-vec3(LightsBuffer.Lights[i].Direction));
    vec3 halfway = normalize(view + lightDir);

    {
        pbrInfo.NdotL = max(dot(normal, lightDir), 0.0);
        pbrInfo.NdotH = max(dot(normal, halfway), 0.0);
        pbrInfo.VdotH = max(dot(halfway, view), 0.0);
    }

    vec3 inRadiance = LightsBuffer.Lights[i].Intensity * LightsBuffer.Lights[i].Color.rbg;

    // Cook-torrance brdf
    vec3 F = FresnelSchlick(pbrInfo);
    float D = DistributionGGX(pbrInfo.NdotH, material.RoughnessFactor);
    float G = GeometricOcclusion(pbrInfo);

    // Energy conservation
    // Specular and Diffuse
    vec3 kS = F;
    vec3 kD = vec3(1.0f) - kS;
    kD *= 1.0f - material.MetallicFactor;

    vec3 numerator = D * G * F;
    float denominator = 4.0f * pbrInfo.NdotV * pbrInfo.NdotL;

    vec3 diffuse = kD * (pbrInfo.DiffuseColor / PI);
    vec3 specular = numerator / max(denominator, 0.0001f);

    return ((diffuse + specular) * inRadiance * pbrInfo.NdotL);
}

vec3 CalculatePointLight(uint i, vec3 normal, vec3 view, Material material, PBRinfo pbrInfo, vec3 position) {

    vec3 lightDir = normalize(vec3(LightsBuffer.Lights[i].Position) - position);
    vec3 halfway = normalize(view + lightDir);

    {
        pbrInfo.NdotL = max(dot(normal, lightDir), 0.0);
        pbrInfo.NdotH = max(dot(normal, halfway), 0.0);
        pbrInfo.VdotH = max(dot(halfway, view), 0.0);
    }

    vec3 inRadiance = LightsBuffer.Lights[i].Intensity * LightsBuffer.Lights[i].Color.rgb;

    // Cook-torrance brdf
    vec3 F = FresnelSchlick(pbrInfo);
    float D = DistributionGGX(pbrInfo.NdotH, material.RoughnessFactor);
    float G = GeometricOcclusion(pbrInfo);

    // Energy conservation
    // Specular and Diffuse
    vec3 kS = F;
    vec3 kD = vec3(1.0f) - kS;
    kD *= 1.0f - material.MetallicFactor;

    vec3 numerator = D * G * F;
    float denominator = 4.0f * pbrInfo.NdotV * pbrInfo.NdotL;

    vec3 diffuse = kD * (pbrInfo.DiffuseColor / PI);
    vec3 specular = numerator / max(denominator, 0.0001f);

    // TODO: Make these const. adjustable by the GUI.
    // Distance of 50:
    float lightConst = 1.0f;
    float lightLinear = 0.09f;
    float lightQuadratic = 0.032f;
   
    float distance = length(vec3(LightsBuffer.Lights[i].Position) - position);
    float attenuation = (1.0f / (lightConst + lightLinear * distance + lightQuadratic * (distance * distance)));

    return (attenuation * (diffuse + specular) * inRadiance * pbrInfo.NdotL);
}

vec3 GetIBLcontribution(PBRinfo pbrInfo, IBLinfo iblInfo, Material material) {

    vec3 diffuse = iblInfo.DiffuseLight * pbrInfo.DiffuseColor;
    vec3 specular = (iblInfo.SpecularLight * (pbrInfo.SpecularColor * iblInfo.Brdf.x + iblInfo.Brdf.y));

    return diffuse + specular;
}

void main() {

    float depth = texture(DepthMap, fragUV).r;
    if (depth == 0.0f) {
        discard;
    }

    vec3 normal = texture(NormalMap, fragUV).rgb;
    vec3 position = DepthToWorld(fragUV, depth);
    vec3 view = normalize(vec3(SceneDataBuffer.SceneData[LightingConstants.SceneDataOffset].CameraPos) - position);
    vec3 reflection = -normalize(reflect(view, normal));

    Material material;
    {
        material.Albedo = texture(AlbedoMap, fragUV).rgb;
        material.MetallicFactor = texture(MaterialMap, fragUV).g;
        material.RoughnessFactor = texture(MaterialMap, fragUV).r;
        material.AO = texture(MaterialMap, fragUV).b;
    }

    PBRinfo pbrInfo;
    {
        float F0 = 0.04f;

        pbrInfo.NdotV = max(dot(normal, view), 0.001f);
        pbrInfo.DiffuseColor = material.Albedo.rgb * (vec3(1.0f) - vec3(F0));
        pbrInfo.DiffuseColor *= 1.0f - material.MetallicFactor;
        pbrInfo.SpecularColor = mix(vec3(F0), material.Albedo, material.MetallicFactor);

        pbrInfo.PerceptualRoughness = clamp(material.RoughnessFactor, 0.04f, 1.0f);
        pbrInfo.AlphaRoughness = (pbrInfo.PerceptualRoughness * pbrInfo.PerceptualRoughness);

        // Reflectance
        float reflectance = max(max(pbrInfo.SpecularColor.r, pbrInfo.SpecularColor.g), pbrInfo.SpecularColor.b);
      
        pbrInfo.Reflectance0 = pbrInfo.SpecularColor.rgb;
        pbrInfo.Reflectance90 = vec3(clamp(reflectance * 25.0f, 0.0f, 1.0f));
    }

    IBLinfo iblInfo;
    {
        iblInfo.DiffuseLight = texture(IrrMap, normal).rgb;

        vec2 brdfSamplePoint = clamp(vec2(pbrInfo.NdotV, 1.0f - pbrInfo.PerceptualRoughness), vec2(0.0f), vec2(1.0f));

        float mipCount = LightingConstants.EnvironmentMapNumMips;
        float lod = pbrInfo.PerceptualRoughness * mipCount;
        iblInfo.Brdf = textureLod(BRDFMap, brdfSamplePoint, 0.0f).rgb;
      
        iblInfo.SpecularLight = textureLod(EnvMap, reflection.xyz, lod).rgb;
    }

    vec3 color = GetIBLcontribution(pbrInfo, iblInfo, material);
    for (uint i = 0u; i < LightingConstants.LightsCount; i++) {

        if (LightsBuffer.Lights[i].Type == 0) {

            color += CalculateDirLight(i, normal, view, material, pbrInfo);
        } else {

            color += CalculatePointLight(i, normal, view, material, pbrInfo, position);
        }
    }

    float ssao = texture(SSAOMap, fragUV).r;
    color = material.AO * ssao * color;
    outColor = vec4(color, 1.0f);
}