#include "ShaderCommon.hlsli"

[[vk::binding(0, 0)]] Texture2D<float> InImage;
[[vk::binding(1, 0)]] SamplerState DepthSampler;
[[vk::binding(2, 0)]] RWTexture2D<float> OutImage;

struct PyramidConstants {

    uint MipSize;
    float Padding0[3];
}; [[vk::push_constant]] PyramidConstants Resources;

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.MipSize && threadID.y < Resources.MipSize) {

        float2 uv = (0.5f + float2(threadID.xy)) / float(Resources.MipSize);

        float depth = InImage.SampleLevel(DepthSampler, uv, 0.0).r;
        OutImage[threadID.xy] =  depth;
    }
}