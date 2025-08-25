#include "ShaderCommon.hlsli"

BEGIN_DECL_SHADER_RESOURCES(Resources)
    DECL_SHADER_TEXTURE_SRV(InImage)
    DECL_SHADER_TEXTURE_UAV(OutImage)
    DECL_SHADER_UINT(MipSize)
END_DECL_SHADER_RESOURCES(Resources)

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.MipSize && threadID.y < Resources.MipSize) {

        float2 uv = (0.5f + float2(threadID.xy)) / float(Resources.MipSize);

        TEXTURE_2D_SRV(inDepth, Resources.InImage)
        TEXTURE_2D_UAV_FLOAT(output, Resources.OutImage)

        float depth = inDepth.SampleLevel(uv, 0.0).r;
        output.Store(threadID.xy, depth);
    }
}