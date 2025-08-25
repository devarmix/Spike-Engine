#define USE_SCENE_RENDERING_DATA
#include "ShaderCommon.hlsli"

BEGIN_DECL_SHADER_RESOURCES(Resources)
    DECL_SHADER_TEXTURE_SRV(SampledTextureA)
    DECL_SHADER_TEXTURE_SRV(SampledTextureB)
    DECL_SHADER_TEXTURE_SRV(NoiseMap)
    DECL_SHADER_TEXTURE_UAV(OutTexture)
    DECL_SHADER_SCALAR(Radius)
    DECL_SHADER_SCALAR(Bias)
    DECL_SHADER_UINT(NumSamples)
    DECL_SHADER_SCALAR(Intensity)
    DECL_SHADER_UINT(SSAOStage)
    DECL_SHADER_FLOAT2(TexSize)

    DECL_SHADER_RESOURCES_STRUCT_PADDING(2)
END_DECL_SHADER_RESOURCES(Resources)

#define SSAO_STAGE_GEN 0
#define SSAO_STAGE_BLUR 1

float3 GetViewPosFromDepth(float2 texCoord, float depth) {

    const float4 ndc = float4(texCoord * 2.f - 1.f, depth, 1.f);

    float4 posVS = mul(SceneDataBuffer.InverseProj, ndc);
    return posVS.xyz / posVS.w; 
}

static const float3 KernelData[32] = {
	float3(-0.668154f, -0.084296f, 0.219458f),
	float3(-0.092521f,  0.141327f, 0.505343f),
	float3(-0.041960f,  0.700333f, 0.365754f),
	float3( 0.722389f, -0.015338f, 0.084357f),
	float3(-0.815016f,  0.253065f, 0.465702f),
	float3( 0.018993f, -0.397084f, 0.136878f),
	float3( 0.617953f, -0.234334f, 0.513754f),
	float3(-0.281008f, -0.697906f, 0.240010f),
	float3( 0.303332f, -0.443484f, 0.588136f),
	float3(-0.477513f,  0.559972f, 0.310942f),
	float3( 0.307240f,  0.076276f, 0.324207f),
	float3(-0.404343f, -0.615461f, 0.098425f),
	float3( 0.152483f, -0.326314f, 0.399277f),
	float3( 0.435708f,  0.630501f, 0.169620f),
	float3( 0.878907f,  0.179609f, 0.266964f),
	float3(-0.049752f, -0.232228f, 0.264012f),
	float3( 0.537254f, -0.047783f, 0.693834f),
	float3( 0.001000f,  0.177300f, 0.096643f),
	float3( 0.626400f,  0.524401f, 0.492467f),
	float3(-0.708714f, -0.223893f, 0.182458f),
	float3(-0.106760f,  0.020965f, 0.451976f),
	float3(-0.285181f, -0.388014f, 0.241756f),
	float3( 0.241154f, -0.174978f, 0.574671f),
	float3(-0.405747f,  0.080275f, 0.055816f),
	float3( 0.079375f,  0.289697f, 0.348373f),
	float3( 0.298047f, -0.309351f, 0.114787f),
	float3(-0.616434f, -0.117369f, 0.475924f),
	float3(-0.035249f,  0.134591f, 0.840251f),
	float3( 0.175849f,  0.971033f, 0.211778f),
	float3( 0.024805f,  0.348056f, 0.240006f),
	float3(-0.267123f,  0.204885f, 0.688595f),
	float3(-0.077639f, -0.753205f, 0.070938f)
    };

float GenSSAO(float2 texCoord) {

    TEXTURE_2D_SRV(noiseTex, Resources.NoiseMap)
    TEXTURE_2D_SRV(depthTex, Resources.SampledTextureA)
    TEXTURE_2D_SRV(normalTex, Resources.SampledTextureB)

    const float2 noiseScale = Resources.TexSize / 4.0f;
    const float3 randomVec = normalize(noiseTex.SampleLevel(texCoord * noiseScale, 0.0).xyz);
    const float3 viewPos = GetViewPosFromDepth(texCoord, depthTex.SampleLevel(texCoord, 0.0).r);

    float3 viewNormal = mul((float3x3)SceneDataBuffer.View, normalTex.SampleLevel(texCoord, 0.0).xyz);

    const float3 T = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    const float3 B = cross(viewNormal, T);
    const float3x3 TBN = float3x3(T, B, viewNormal);

    float occlusion = 0.0f;
    for (int i = 0; i < Resources.NumSamples; i++) {

        float3 samplePos = mul(TBN, KernelData[i].xyz);
        samplePos = viewPos + samplePos * Resources.Radius;

        float4 offset = float4(samplePos, 1.0);
        offset = mul(SceneDataBuffer.Proj, offset);
        offset.xy /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        float3 reconPos = GetViewPosFromDepth(offset.xy, depthTex.SampleLevel(offset.xy, 0.0).r);
        float rangeCheck = smoothstep(0.0f, 1.f, Resources.Radius / abs(viewPos.z - reconPos.z));
        if (reconPos.z <= samplePos.z - Resources.Bias) occlusion += 1.0 * rangeCheck;
    }
    
    occlusion = 1.0 - (occlusion / float(Resources.NumSamples));
    return pow(abs(occlusion), Resources.Intensity);
}

float BlurSSAO(float2 texCoord) {

    TEXTURE_2D_SRV(sampledTex, Resources.SampledTextureA)

    float2 texelSize = 1.0f / Resources.TexSize;
    float result = 0.0f;
    for (int x = -2; x < 2; x++) {

        for (int y = -2; y < 2; y++) {

            float2 offset = float2(float(x), float(y)) * texelSize;
            result += sampledTex.SampleLevel(texCoord + offset, 0.0).r;
        }
    }

    return result / (4.0f * 4.0f);
}

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.TexSize.x && threadID.y < Resources.TexSize.y) {

        float2 texCoord = float2(float(threadID.x) / Resources.TexSize.x, float(threadID.y) / Resources.TexSize.y);
        texCoord += (1.0f / Resources.TexSize) * 0.5f;

        if (Resources.SSAOStage == SSAO_STAGE_GEN) {

            float ssao = GenSSAO(texCoord);
            TEXTURE_2D_UAV_FLOAT(output, Resources.OutTexture)
            output.Store(threadID.xy, ssao);

        } else if (Resources.SSAOStage == SSAO_STAGE_BLUR) {

            float ssao = BlurSSAO(texCoord);

            TEXTURE_2D_SRV(sampledTex, Resources.SampledTextureB)
            TEXTURE_2D_UAV_FLOAT4(output, Resources.OutTexture)

            float4 sampledColor = sampledTex.SampleLevel(texCoord, 0.0);
            output.Store(threadID.xy, sampledColor * ssao);
        }
    }
}