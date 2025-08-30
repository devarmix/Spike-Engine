struct SMAAConstants {

    float4 ScreenSize;
}; [[vk::push_constant]] SMAAConstants Resources;

[[vk::binding(0, 0)]] SamplerState LinearSampler;
[[vk::binding(1, 0)]] SamplerState PointSampler;

#define SMAA_RT_METRICS Resources.ScreenSize
#define SMAA_HLSL_4_1 1

#define SMAA_INCLUDE_CS 1

#define SMAA_FLIP_Y 0
#define SMAA_PRESET_ULTRA 1
#define SMAA_HLSL_NO_SAMPLERS 1
#define SMAA_PREDICATION 1

#include "SMAA.hlsli"

[[vk::binding(2, 0)]] SMAATexture2D(ColorTex);
[[vk::binding(3, 0)]] SMAAWriteImage2D(OutTex);
[[vk::binding(4, 0)]] SMAATexture2D(PredicationTex);

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.ScreenSize.z && threadID.y < Resources.ScreenSize.w) {

        SMAALumaEdgeDetectionCS(int2(threadID.xy), OutTex, ColorTex, PredicationTex);
    }
}