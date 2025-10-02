#include "ShaderCommon.hlsli"

struct Vertex {
    float Pos[2];
    float UV[2];
    uint Color;
};
[[vk::binding(0, 0)]] StructuredBuffer<Vertex> VtxBuffer;
[[vk::binding(0, 1)]] Texture2D SampledTex;
[[vk::binding(1, 1)]] SamplerState TexSampler;

struct GUIConstants {

    float2 Scale;
    float2 Translate;
    uint BaseVtx;
    float Padding0[3];
}; [[vk::push_constant]] GUIConstants Resources;

struct VSOutput {
    float4 VertexPos : SV_POSITION;
    float4 VertexColor : COLOR0;
    float2 TexCoord : TEXCOORD0;
};

VSOutput VSMain(uint idx : SV_VertexID) {
    Vertex vtx = VtxBuffer[idx + Resources.BaseVtx];

    VSOutput output;
    output.VertexColor = UnpackUintToUnsignedVec4(vtx.Color);
    output.TexCoord = float2(vtx.UV[0], vtx.UV[1]);
    output.VertexPos = float4(float2(vtx.Pos[0], vtx.Pos[1]) * Resources.Scale + Resources.Translate, 0, 1);

    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET0 {
    return input.VertexColor * SampledTex.Sample(TexSampler, input.TexCoord);
}