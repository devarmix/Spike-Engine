#include "ShaderCommon.hlsli"

[[vk::binding(0, 0)]] Texture2D<float> SSAOMainTex;
[[vk::binding(1, 0)]] Texture2D<float4> SceneColorTex;
[[vk::binding(2, 0)]] SamplerState TexSampler;
[[vk::binding(3, 0)]] RWTexture2D<float4> OutTexture;

struct SSAOConstants {

    float2 TexSize;
    float Padding0[2];
}; [[vk::push_constant]] SSAOConstants Resources;

float BlurSSAO(float2 texCoord) {

    float2 texelSize = 1.0f / Resources.TexSize;
    float result = 0.0f;

    for (int x = -2; x < 2; x++) {

        for (int y = -2; y < 2; y++) {

            float2 offset = float2(float(x), float(y)) * texelSize;
            result += SSAOMainTex.SampleLevel(TexSampler, texCoord + offset, 0.0).r;
        }
    }

    return result / (4.0f * 4.0f);
}

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.TexSize.x && threadID.y < Resources.TexSize.y) {

        float2 texCoord = float2(float(threadID.x) / Resources.TexSize.x, float(threadID.y) / Resources.TexSize.y);
        texCoord += (1.0f / Resources.TexSize) * 0.5f;

        float ssao = BlurSSAO(texCoord);

        float4 sampledColor = SceneColorTex.SampleLevel(TexSampler, texCoord, 0.0);
        OutTexture[threadID.xy] =  sampledColor * ssao;
    }
}