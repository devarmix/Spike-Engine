#include "ShaderCommon.hlsli"

[[vk::binding(0, 0)]] RWTexture2D<float4> OutTexture;
[[vk::binding(1, 0)]] Texture2D<float4> SampledTexture;
[[vk::binding(2, 0)]] SamplerState TexSampler;

struct CubeMapGenConstants {

    uint FaceIndex;
    uint OutSize;

    float Padding0[2];
}; [[vk::push_constant]] CubeMapGenConstants Resources;

float3 SampleDirection(float2 uv) {

    // [0,1] → [-1,1]
    uv = uv * 2.0f - 1.0f;

    float3 dir;
    if (Resources.FaceIndex == 0)      dir = normalize(float3(1.0f, -uv.y, -uv.x));  // +X
    else if (Resources.FaceIndex == 1) dir = normalize(float3(-1.0f, -uv.y, uv.x));  // -X
    else if (Resources.FaceIndex == 2) dir = normalize(float3(uv.x, 1.0f, uv.y));    // +Y
    else if (Resources.FaceIndex == 3) dir = normalize(float3(uv.x, -1.0f, -uv.y));  // -Y
    else if (Resources.FaceIndex == 4) dir = normalize(float3(uv.x, -uv.y, 1.0f));   // +Z
    else if (Resources.FaceIndex == 5) dir = normalize(float3(-uv.x, -uv.y, -1.0f)); // -Z

    return dir;
}

#define PI 3.1415926536

float2 DirToEquirectUV(float3 dir) {

    float phi = atan2(dir.z, dir.x);
    float theta = acos(clamp(dir.y, -1.0f, 1.0f));
    return float2(phi / (2.0f * PI) + 0.5f, theta / PI);
}

[numthreads(8, 8, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.OutSize && threadID.y < Resources.OutSize) {

        float2 uv = float2(threadID.xy) / float(Resources.OutSize); 
        float3 dir = SampleDirection(uv);

        float2 eqUV = DirToEquirectUV(dir);
        float3 color = SampledTexture.SampleLevel(TexSampler, eqUV, 0.0).rgb;
        OutTexture[threadID.xy] =  float4(color, 1.0f);
    }
}