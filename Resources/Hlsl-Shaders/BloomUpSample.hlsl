#include "ShaderCommon.hlsli"

[[vk::binding(0, 0)]] Texture2D<float4> SampledTextureA;
[[vk::binding(1, 0)]] Texture2D<float4> SampledTextureB;
[[vk::binding(2, 0)]] SamplerState TexSampler;
[[vk::binding(3, 0)]] RWTexture2D<float4> OutTexture;

struct BloomConstants {

    float2 OutSize;
    uint BloomStage;
    float Intensity;
    float FilterRadius;

    float Padding0[3];
}; [[vk::push_constant]] BloomConstants Resources;

#define BLOOM_STAGE_UP_SAMPLE 0
#define BLOOM_STAGE_COMPOSITE 1

float3 UpSample9(float2 texCoord) {

    float x = Resources.FilterRadius;
    float y = Resources.FilterRadius;

    float3 a = SampledTextureA.SampleLevel(TexSampler, float2(texCoord.x - x, texCoord.y + y), 0.0).rgb;
    float3 b = SampledTextureA.SampleLevel(TexSampler, float2(texCoord.x,     texCoord.y + y), 0.0).rgb;
    float3 c = SampledTextureA.SampleLevel(TexSampler, float2(texCoord.x + x, texCoord.y + y), 0.0).rgb;

    float3 d = SampledTextureA.SampleLevel(TexSampler, float2(texCoord.x - x, texCoord.y), 0.0).rgb;
    float3 e = SampledTextureA.SampleLevel(TexSampler, float2(texCoord.x,     texCoord.y), 0.0).rgb;
    float3 f = SampledTextureA.SampleLevel(TexSampler, float2(texCoord.x + x, texCoord.y), 0.0).rgb;

    float3 g = SampledTextureA.SampleLevel(TexSampler, float2(texCoord.x - x, texCoord.y - y), 0.0).rgb;
    float3 h = SampledTextureA.SampleLevel(TexSampler, float2(texCoord.x,     texCoord.y - y), 0.0).rgb;
    float3 i = SampledTextureA.SampleLevel(TexSampler, float2(texCoord.x + x, texCoord.y - y), 0.0).rgb;

    float3 upsample = e*4.0f;
	upsample += (b+d+f+h)*2.0f;
	upsample += (a+c+g+i);
	upsample *= 1.0f / 16.0f;

    return upsample;
}

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < uint(Resources.OutSize.x) && threadID.y < uint(Resources.OutSize.y)) {

        float2 imgSize = Resources.OutSize;
        float2 texCoord = float2(float(threadID.x) / imgSize.x, float(threadID.y) / imgSize.y);
        texCoord += (1.0f / imgSize) * 0.5f;

        float3 color;
        if (Resources.BloomStage == BLOOM_STAGE_UP_SAMPLE) {

            color = UpSample9(texCoord);

            float3 colorB = SampledTextureB.SampleLevel(TexSampler, texCoord, 0.0).rgb;
            color += colorB;
        } else if (Resources.BloomStage == BLOOM_STAGE_COMPOSITE) {

            color = UpSample9(texCoord);

            float3 colorB = SampledTextureB.SampleLevel(TexSampler, texCoord, 0.0).rgb;
            color = lerp(colorB, color, Resources.Intensity);
        }

        OutTexture[threadID.xy] =  float4(color, 1.0f);
    }
}