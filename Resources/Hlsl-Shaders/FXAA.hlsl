#define FXAA_PC 1
#define FXAA_HLSL_5 1

#define FXAA_QUALITY_PRESET 20

#include "FXAA.hlsli"

[[vk::binding(0, 0)]] Texture2D ColorTex;
[[vk::binding(1, 0)]] SamplerState TexSampler;
[[vk::binding(2, 0)]] RWTexture2D<float4> OutTexture;

struct FXAAConstants {

    float4 ScreenSize;
}; [[vk::push_constant]] FXAAConstants Resources;
 

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.ScreenSize.z && threadID.y < Resources.ScreenSize.w) {

        float2 texCoord = float2(threadID.xy);
        texCoord += float2(0.5, 0.5);
        texCoord *= Resources.ScreenSize.xy;

        float4 vZero = float4(0.0, 0.0, 0.0, 0.0);
        FxaaTex t;
        t.smpl = TexSampler;
        t.tex = ColorTex;
        
        float4 pixel = FxaaPixelShader(texCoord, vZero, t, t, t, Resources.ScreenSize.xy, vZero, vZero, vZero, 0.75, 0.125, 0.0833, 8.0, 0.125, 0.05, vZero);
        OutTexture[threadID.xy] = pixel;
    }
}