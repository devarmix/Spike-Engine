#define MATERIAL_SHADER
#include "ShaderCommon.hlsli"

BEGIN_DECL_MATERIAL_RESOURCES()
    DECL_MATERIAL_TEXTURE_2D_SRV(AlbedoMap, 0)
    DECL_MATERIAL_TEXTURE_2D_SRV(NormalMap, 1)
    DECL_MATERIAL_TEXTURE_2D_SRV(MettalicMap, 2)
    DECL_MATERIAL_TEXTURE_2D_SRV(RoughnessMap, 3)
    DECL_MATERIAL_TEXTURE_2D_SRV(AOMap, 4)
END_DECL_MATERIAL_RESOURCES()

struct VSInput {

    uint VertexIndex : SV_VertexID;
    uint InstanceID : SV_InstanceID;
};

struct VSOutput {

    float3 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
    nointerpolation uint MaterialDataIndex : TEXCOORD1;

    float4 VertexPos : SV_POSITION;
};

VSOutput VSMain(VSInput input) {

    SceneObjectGPUData objectData = ObjectsBuffer[input.InstanceID];

    uint vIndex = vk::RawBufferLoad<uint>(objectData.IndexBufferAddress + ((input.VertexIndex + objectData.FirstIndex) * sizeof(uint)), 4);
    Vertex v = vk::RawBufferLoad<Vertex>(objectData.VertexBufferAddress + (vIndex * sizeof(Vertex)), 16);

    VSOutput output;

    float4 fragPos = mul(objectData.GlobalTransform, float4(v.Position.xyz, 1.0f));
    output.VertexPos = mul(SceneDataBuffer.ViewProj, fragPos);
    output.TexCoord = float2(v.Position.w, v.Normal.w);

    float4x4 transposeInverseTransform = transpose(objectData.InverseTransform);

    float3 tangent = normalize(mul(transposeInverseTransform, float4(v.Tangent.xyz, 0.0f)).xyz);
    output.Normal = v.Normal;
    tangent = normalize(tangent - dot(tangent, output.Normal) * output.Normal);

    output.Tangent = float4(tangent, v.Tangent.w);
    output.Color = v.Color;
    output.MaterialDataIndex = objectData.MaterialBufferIndex;

    return output;
}


struct PSOutput {

    float4 AlbedoColor : SV_TARGET0;
    float4 NormalColor : SV_TARGET1;
    float4 MaterialColor : SV_TARGET2;
};

PSOutput PSMain(VSOutput input) {

    float4 albedo = SampleMaterialTexture(input.MaterialDataIndex, AlbedoMap, input.TexCoord);
    float3 normalSample = SampleMaterialTexture(input.MaterialDataIndex, NormalMap, input.TexCoord).rgb;

    float ao = SampleMaterialTexture(input.MaterialDataIndex, AOMap, input.TexCoord).r;
    float metallic = SampleMaterialTexture(input.MaterialDataIndex, MettalicMap, input.TexCoord).r;
    float roughness = SampleMaterialTexture(input.MaterialDataIndex, RoughnessMap, input.TexCoord).r;

    if (albedo.a < 0.8f) {
        discard;
    }

    float3 B = cross(input.Normal, input.Tangent.xyz) * input.Tangent.w;
    float3x3 TBN = float3x3(input.Tangent.xyz, B, input.Normal);
    float3 N = normalize(mul(TBN, normalize(normalSample * 2.0f - 1.0f)));

    PSOutput output;
    output.AlbedoColor = albedo;
    output.NormalColor = float4(N, 1.0);
    output.MaterialColor = float4(roughness, metallic, ao, 1.0);
    return output;
}