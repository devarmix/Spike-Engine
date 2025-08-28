#include "ShaderCommon.hlsli"

[[vk::binding(0, 0)]] TextureCube<float4> SkyboxMap;
[[vk::binding(1, 0)]] SamplerState TexSampler;
[[vk::binding(2, 0)]] ConstantBuffer<SceneGPUData> SceneDataBuffer;

struct VSOutput {

    float3 ViewDir : TEXCOORD;
    float4 VertexPos : SV_POSITION;
};

VSOutput VSMain(uint vertexID : SV_VertexID) {

    float2 texCoord = float2((vertexID << 1) & 2, (vertexID & 2));

    VSOutput output;
    output.VertexPos = float4(texCoord * 2.0f + -1.0f, 0.0f, 1.0f);
    output.ViewDir = mul((float3x3)SceneDataBuffer.InverseView, mul(SceneDataBuffer.InverseProj, output.VertexPos).xyz); 

    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET0 {

    float3 color = SkyboxMap.SampleLevel(TexSampler, normalize(input.ViewDir), 0.0).rgb;
    return float4(color, 1.0f);
}