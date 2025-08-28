#include "ShaderCommon.hlsli"

[[vk::binding(0, 0)]] Texture2D<float> DepthTexture;
[[vk::binding(1, 0)]] Texture2D<float4> NormalTexture;
[[vk::binding(2, 0)]] Texture2D<float4> NoiseTexture;
[[vk::binding(3, 0)]] SamplerState NoiseSampler;
[[vk::binding(4, 0)]] SamplerState TexSampler;
[[vk::binding(5, 0)]] RWTexture2D<float> OutTexture;

struct KernelData { float4 Data[64]; };
[[vk::binding(6, 0)]] ConstantBuffer<KernelData> KernelBuffer;
[[vk::binding(7, 0)]] ConstantBuffer<SceneGPUData> SceneDataBuffer;

struct SSAOConstants {

    float Radius;
    float Bias;
    float Intensity;
    uint NumSamples;
    float2 TexSize;

    float Padding0[2];
}; [[vk::push_constant]] SSAOConstants Resources;

float3 GetViewPosFromDepth(float2 texCoord) {

    float depth = DepthTexture.SampleLevel(TexSampler, texCoord, 0.0).r;
    const float4 ndc = float4(texCoord * 2.f - 1.f, depth, 1.f);

    float4 posVS = mul(SceneDataBuffer.InverseProj, ndc);
    return posVS.xyz / posVS.w; 
}

float GenSSAO(float2 texCoord) {

    const float2 noiseScale = Resources.TexSize / 4.0f;
    const float3 randomVec = normalize(NoiseTexture.SampleLevel(NoiseSampler, texCoord * noiseScale, 0.0).xyz);
    const float3 viewPos = GetViewPosFromDepth(texCoord);

    float3 viewNormal = mul((float3x3)SceneDataBuffer.View, NormalTexture.SampleLevel(TexSampler, texCoord, 0.0).xyz);

    const float3 T = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    const float3 B = cross(viewNormal, T);
    const float3x3 TBN = float3x3(T, B, viewNormal);

    float occlusion = 0.0f;
    for (int i = 0; i < Resources.NumSamples; i++) {

        float3 samplePos = mul(TBN, KernelBuffer.Data[i].xyz);
        samplePos = viewPos + samplePos * Resources.Radius;

        float4 offset = float4(samplePos, 1.0);
        offset = mul(SceneDataBuffer.Proj, offset);
        offset.xy = (offset.xy / offset.w) * 0.5 + 0.5;

        if (offset.x < 0.0f || offset.x > 1.0f || offset.y < 0.0f || offset.y > 1.0f)
            continue; 

        const float reconPos = GetViewPosFromDepth(offset.xy).z;
        const float rangeCheck = smoothstep(0.0f, 1.0f, Resources.Radius / abs(viewPos.z - reconPos));
        occlusion += step(samplePos.z + Resources.Bias, reconPos) * rangeCheck;
    }
    
    occlusion = 1.0 - (occlusion / float(Resources.NumSamples));
    return pow(abs(occlusion), Resources.Intensity);
}

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.TexSize.x && threadID.y < Resources.TexSize.y) {

        float2 texCoord = float2(float(threadID.x) / Resources.TexSize.x, float(threadID.y) / Resources.TexSize.y);
        texCoord += (1.0f / Resources.TexSize) * 0.5f;

        float ssao = GenSSAO(texCoord);
        OutTexture[threadID.xy] = ssao;
    }
}