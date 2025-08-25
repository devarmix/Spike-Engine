#define USE_SCENE_RENDERING_DATA
#include "ShaderCommon.hlsli"

BEGIN_DECL_SHADER_RESOURCES(Resources)
    DECL_SHADER_TEXTURE_SRV(SkyboxMap)

    DECL_SHADER_RESOURCES_STRUCT_PADDING(2)
END_DECL_SHADER_RESOURCES(Resources)

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

    CUBE_TEXTURE_SRV(skyboxTex, Resources.SkyboxMap)

    float3 color = skyboxTex.SampleLevel(normalize(input.ViewDir), 0.0).rgb;
    return float4(color, 1.0f);
}