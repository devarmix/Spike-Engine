#define USE_SCENE_RENDERING_DATA
#include "ShaderCommon.hlsli"

BEGIN_DECL_SHADER_RESOURCES(Resources)
    DECL_SHADER_TEXTURE_SRV(AlbedoMap)
    DECL_SHADER_TEXTURE_SRV(NormalMap)
    DECL_SHADER_TEXTURE_SRV(MaterialMap)
    DECL_SHADER_TEXTURE_SRV(DepthMap)
    DECL_SHADER_TEXTURE_SRV(EnvMap)
    DECL_SHADER_TEXTURE_SRV(IrrMap)
    DECL_SHADER_TEXTURE_SRV(BRDFMap)
    DECL_SHADER_BUFFER_SRV(LightsBuffer)
    DECL_SHADER_UINT(EnvMapNumMips)
END_DECL_SHADER_RESOURCES(Resources)

#define PI 3.1415926535897932384626433832795

float3 DepthToWorld(float2 screenPos, float depth) {

    float4 clipSpacePos = float4(screenPos * 2.0f - 1.0f, depth, 1.0f);
    float4 viewSpacePos = mul(SceneDataBuffer.InverseProj, clipSpacePos);
    viewSpacePos /= viewSpacePos.w;
    float4 worldSpacePos = mul(SceneDataBuffer.InverseView, viewSpacePos);
    return worldSpacePos.xyz;
}

struct Material
{
    float3 Albedo;
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
	float3 DiffuseColor;    
    // color contribution from specular lighting.
	float3 SpecularColor;
    // full reflectance color(normal incidence angle)
    float3 Reflectance0;
    // reflectance color at grazing angle
    float3 Reflectance90;
};

struct IBLinfo
{
    float3 DiffuseLight;
    float3 SpecularLight;
    float3 Brdf;
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

float3 FresnelSchlick(PBRinfo pbrInfo) {

    return (pbrInfo.Reflectance0 + (pbrInfo.Reflectance90 - pbrInfo.Reflectance0) * pow(1.0f - pbrInfo.VdotH, 5.0f));
}

float3 CalculateDirLight(SceneLightGPUData light, float3 normal, float3 view, Material material, PBRinfo pbrInfo) {

    float3 lightDir = normalize(-light.Direction.xyz);
    float3 halfway = normalize(view + lightDir);

    {
        pbrInfo.NdotL = max(dot(normal, lightDir), 0.0);
        pbrInfo.NdotH = max(dot(normal, halfway), 0.0);
        pbrInfo.VdotH = max(dot(halfway, view), 0.0);
    }

    float3 inRadiance = light.Intensity * light.Color.rbg;

    // Cook-torrance brdf
    float3 F = FresnelSchlick(pbrInfo);
    float D = DistributionGGX(pbrInfo.NdotH, material.RoughnessFactor);
    float G = GeometricOcclusion(pbrInfo);

    // Energy conservation
    // Specular and Diffuse
    float3 kS = F;
    float3 kD = (float3)1.0f - kS;
    kD *= 1.0f - material.MetallicFactor;

    float3 numerator = D * G * F;
    float denominator = 4.0f * pbrInfo.NdotV * pbrInfo.NdotL;

    float3 diffuse = kD * (pbrInfo.DiffuseColor / PI);
    float3 specular = numerator / max(denominator, 0.0001f);

    return ((diffuse + specular) * inRadiance * pbrInfo.NdotL);
}

float3 CalculatePointLight(SceneLightGPUData light, float3 normal, float3 view, Material material, PBRinfo pbrInfo, float3 position) {

    float3 lightDir = normalize(light.Position.xyz - position);
    float3 halfway = normalize(view + lightDir);

    {
        pbrInfo.NdotL = max(dot(normal, lightDir), 0.0);
        pbrInfo.NdotH = max(dot(normal, halfway), 0.0);
        pbrInfo.VdotH = max(dot(halfway, view), 0.0);
    }

    float3 inRadiance = light.Intensity * light.Color.rgb;

    // Cook-torrance brdf
    float3 F = FresnelSchlick(pbrInfo);
    float D = DistributionGGX(pbrInfo.NdotH, material.RoughnessFactor);
    float G = GeometricOcclusion(pbrInfo);

    // Energy conservation
    // Specular and Diffuse
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= 1.0f - material.MetallicFactor;

    float3 numerator = D * G * F;
    float denominator = 4.0f * pbrInfo.NdotV * pbrInfo.NdotL;

    float3 diffuse = kD * (pbrInfo.DiffuseColor / PI);
    float3 specular = numerator / max(denominator, 0.0001f);

    // TODO: Make these const. adjustable by the GUI.
    // Distance of 50:
    float lightConst = 1.0f;
    float lightLinear = 0.09f;
    float lightQuadratic = 0.032f;
   
    float distance = length(light.Position.xyz - position);
    float attenuation = (1.0f / (lightConst + lightLinear * distance + lightQuadratic * (distance * distance)));

    return (attenuation * (diffuse + specular) * inRadiance * pbrInfo.NdotL);
}

float3 GetIBLcontribution(PBRinfo pbrInfo, IBLinfo iblInfo, Material material) {

    float3 diffuse = iblInfo.DiffuseLight * pbrInfo.DiffuseColor;
    float3 specular = (iblInfo.SpecularLight * (pbrInfo.SpecularColor * iblInfo.Brdf.x + iblInfo.Brdf.y));

    return diffuse + specular;
}

struct VSOutput {

    float2 TexCoord : TEXCOORD;
    float4 VertexPos : SV_POSITION;
};

VSOutput VSMain(uint vertexID : SV_VertexID) {

    VSOutput output;
    output.TexCoord = float2((vertexID << 1) & 2, (vertexID & 2));
    output.VertexPos = float4(output.TexCoord * 2.0f + -1.0f, 0.0f, 1.0f);

    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET0 {

    TEXTURE_2D_SRV(albedoTex, Resources.AlbedoMap)
    TEXTURE_2D_SRV(normalTex, Resources.NormalMap)
    TEXTURE_2D_SRV(materialTex, Resources.MaterialMap)
    TEXTURE_2D_SRV(depthTex, Resources.DepthMap)
    CUBE_TEXTURE_SRV(envTex, Resources.EnvMap)
    CUBE_TEXTURE_SRV(irrTex, Resources.IrrMap)
    TEXTURE_2D_SRV(brdfTex, Resources.BRDFMap)

    float depth = depthTex.Sample(input.TexCoord).r;
    if (depth == 0.0f) {
        discard;
    }

    float3 normal = normalTex.Sample(input.TexCoord).rgb;
    float3 position = DepthToWorld(input.TexCoord, depth);
    float3 view = normalize(SceneDataBuffer.CameraPos.xyz - position);
    float3 reflection = -normalize(reflect(view, normal));

    Material material;
    {
        material.Albedo = albedoTex.Sample(input.TexCoord).rgb;
        material.MetallicFactor = materialTex.Sample(input.TexCoord).g;
        material.RoughnessFactor = materialTex.Sample(input.TexCoord).r;
        material.AO = materialTex.Sample(input.TexCoord).b;
    }

    PBRinfo pbrInfo;
    {
        float F0 = 0.04f;

        pbrInfo.NdotV = max(dot(normal, view), 0.001f);
        pbrInfo.DiffuseColor = material.Albedo.rgb * ((float3)1.0f - (float3)F0);
        pbrInfo.DiffuseColor *= 1.0f - material.MetallicFactor;
        pbrInfo.SpecularColor = lerp((float3)F0, material.Albedo, material.MetallicFactor);

        pbrInfo.PerceptualRoughness = clamp(material.RoughnessFactor, 0.04f, 1.0f);
        pbrInfo.AlphaRoughness = (pbrInfo.PerceptualRoughness * pbrInfo.PerceptualRoughness);

        // Reflectance
        float reflectance = max(max(pbrInfo.SpecularColor.r, pbrInfo.SpecularColor.g), pbrInfo.SpecularColor.b);
      
        pbrInfo.Reflectance0 = pbrInfo.SpecularColor.rgb;
        pbrInfo.Reflectance90 = (float3)clamp(reflectance * 25.0f, 0.0f, 1.0f);
    }

    IBLinfo iblInfo;
    {
        iblInfo.DiffuseLight = irrTex.Sample(normal).rgb;

        float2 brdfSamplePoint = clamp(float2(pbrInfo.NdotV, 1.0f - pbrInfo.PerceptualRoughness), (float2)0.0f, (float2)1.0f);

        float mipCount = Resources.EnvMapNumMips;
        float lod = pbrInfo.PerceptualRoughness * mipCount;
        iblInfo.Brdf = brdfTex.Sample(brdfSamplePoint).rgb;
      
        iblInfo.SpecularLight = envTex.SampleLevel(reflection.xyz, lod).rgb;
    }

    float3 color = GetIBLcontribution(pbrInfo, iblInfo, material);

    BUFFER_SRV(lightsBuffer, Resources.LightsBuffer)
    for (uint i = 0u; i < SceneDataBuffer.LightsCount; i++) {

        SceneLightGPUData light = lightsBuffer.LoadAtIndex<SceneLightGPUData>(i);

        if (light.Type == 0) {

            color += CalculateDirLight(light, normal, view, material, pbrInfo);
        } else {

            color += CalculatePointLight(light, normal, view, material, pbrInfo, position);
        }
    }

    color = material.AO * color;
    return float4(color, 1.0f);
}