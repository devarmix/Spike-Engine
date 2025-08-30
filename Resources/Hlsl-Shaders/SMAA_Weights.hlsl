struct SMAAConstants {

    float4 ScreenSize;
    float4 SubSampleIndices;
}; [[vk::push_constant]] SMAAConstants Resources;

[[vk::binding(0, 0)]] SamplerState LinearSampler;
[[vk::binding(1, 0)]] SamplerState PointSampler;

#define SMAA_RT_METRICS Resources.ScreenSize
#define SMAA_HLSL_4_1 1

#define SMAA_INCLUDE_CS 1

#define SMAA_FLIP_Y 0
#define SMAA_PRESET_ULTRA 1
#define SMAA_HLSL_NO_SAMPLERS 1

#include "SMAA.hlsli"

[[vk::binding(2, 0)]] SMAATexture2D(EdgesTex);
[[vk::binding(3, 0)]] SMAATexture2D(AreaTex);
[[vk::binding(4, 0)]] SMAATexture2D(SearchTex);
[[vk::binding(5, 0)]] SMAAWriteImage2D(OutTex);

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.ScreenSize.z && threadID.y < Resources.ScreenSize.w) {

        SMAABlendingWeightCalculationCS(int2(threadID.xy), OutTex, EdgesTex, AreaTex, SearchTex, Resources.SubSampleIndices);
    }
}