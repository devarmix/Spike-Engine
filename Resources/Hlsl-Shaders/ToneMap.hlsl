#include "ShaderCommon.hlsli"

BEGIN_DECL_SHADER_RESOURCES(Resources)
    DECL_SHADER_TEXTURE_SRV(InTexture)
    DECL_SHADER_TEXTURE_UAV(OutTexture)
    DECL_SHADER_SCALAR(Exposure)
    DECL_SHADER_FLOAT2(TexSize)

    DECL_SHADER_RESOURCES_STRUCT_PADDING(2)
END_DECL_SHADER_RESOURCES(Resources)

float3 Uncharted2Tonemap(float3 x) {

    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;

    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F)) - E/F;
}

float3 ToneMap(float3 color) {

    color = Uncharted2Tonemap(color * Resources.Exposure);
    float whitePoint = 11.2f;

    float3 whiteScale = 1.0f / Uncharted2Tonemap((float3)whitePoint);
    color *= whiteScale;

    return color;
}

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.TexSize.x && threadID.y < Resources.TexSize.y) {

        float2 texCoord = float2(float(threadID.x) / Resources.TexSize.x, float(threadID.y) / Resources.TexSize.y);
        texCoord += (1.0f / Resources.TexSize) * 0.5f;

        TEXTURE_2D_SRV(inTex, Resources.InTexture)
        TEXTURE_2D_UAV_FLOAT4(output, Resources.OutTexture)

        float3 sampledColor = inTex.SampleLevel(texCoord, 0.0).rgb;
        output.Store(threadID.xy, float4(ToneMap(sampledColor), 1.0f));
    }
}