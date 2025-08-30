#include "ShaderCommon.hlsli"

[[vk::binding(0, 0)]] Texture2D<float4> InTexture;
[[vk::binding(1, 0)]] SamplerState TexSampler;
[[vk::binding(2, 0)]] RWTexture2D<float4> OutTexture;

struct ToneMapConstants {

    float2 TexSize;
    float Exposure;
    float Padding0;
}; [[vk::push_constant]] ToneMapConstants Resources;


float3 Uncharted2Tonemap(float3 x) {

    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F)) - E/F;
}

float3 ToneMap(float3 color) {

    color = Uncharted2Tonemap(color * Resources.Exposure);
    float whitePoint = 11.2f;

    float3 whiteScale = 1.0f / Uncharted2Tonemap((float3)whitePoint);
    color *= whiteScale;

    return color;
}

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.TexSize.x && threadID.y < Resources.TexSize.y) {

        float2 texCoord = float2(float(threadID.x) / Resources.TexSize.x, float(threadID.y) / Resources.TexSize.y);
        texCoord += (1.0f / Resources.TexSize) * 0.5f;

        float3 sampledColor = InTexture.SampleLevel(TexSampler, texCoord, 0.0).rgb;

        //float gamma = 2.2f;
        //sampledColor = pow(sampledColor, (float3)1.0f/gamma);
        OutTexture[threadID.xy] = float4(ToneMap(sampledColor), 1.0f);
    }
}