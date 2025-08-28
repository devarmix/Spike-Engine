#include "ShaderCommon.hlsli"

[[vk::binding(0, 0)]] Texture2D<float4> SampledTexture;
[[vk::binding(1, 0)]] SamplerState TexSampler;
[[vk::binding(2, 0)]] RWTexture2D<float4> OutTexture;

struct BloomConstants {

    float2 SrcSize;
    float2 OutSize;

    float Threadshold;
    float SoftThreadshold;
    uint MipLevel;
    float Padding0;
}; [[vk::push_constant]] BloomConstants Resources;

float3 PowFloat3(float3 v, float p) {

    return float3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

float3 ToSRGB(float3 v) {

    const float InvGamma = 1.0f / 2.2f;
    return PowFloat3(v, InvGamma);
}

float SRGBToLuma(float3 col) {

    return dot(col, float3(0.299f, 0.587f, 0.114f));
}

float KarisAverage(float3 col) {

    float luma = SRGBToLuma(ToSRGB(col)) * 0.25f;
    return 1.0f / (1.0f + luma);
}

float3 Prefilter(float3 col, float4 f) {

    float brightness = max(col.r, max(col.g, col.b));
    float soft = brightness - f.y;
    soft = clamp(soft, 0, f.z);
    soft = soft * soft * f.w;
    float contribution = max(soft, brightness - f.x);
    contribution /= max(brightness, 0.00001f);

    return col * contribution;
}

float3 DownSample13(float2 texCoord) {

    float2 srcTexelSize = 1.0f / Resources.SrcSize;
    float x = srcTexelSize.x;
    float y = srcTexelSize.y;

    float3 a = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x - 2*x, texCoord.y + 2*y), 0.0).rgb;
    float3 b = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x,       texCoord.y + 2*y), 0.0).rgb;
    float3 c = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x + 2*x, texCoord.y + 2*y), 0.0).rgb;

    float3 d = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x - 2*x, texCoord.y), 0.0).rgb;
    float3 e = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x,       texCoord.y), 0.0).rgb;
    float3 f = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x + 2*x, texCoord.y), 0.0).rgb;

    float3 g = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x - 2*x, texCoord.y - 2*y), 0.0).rgb;
    float3 h = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x,       texCoord.y - 2*y), 0.0).rgb;
    float3 i = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x + 2*x, texCoord.y - 2*y), 0.0).rgb;

    float3 j = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x - x, texCoord.y + y), 0.0).rgb;
    float3 k = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x + x, texCoord.y + y), 0.0).rgb;
    float3 l = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x - x, texCoord.y - y), 0.0).rgb;
    float3 m = SampledTexture.SampleLevel(TexSampler, float2(texCoord.x + x, texCoord.y - y), 0.0).rgb;

    float3 downsample;
    float3 groups[5];

    if (Resources.MipLevel == 0) {

        groups[0] = (a+b+d+e) * (0.125f/4.0f);
	    groups[1] = (b+c+e+f) * (0.125f/4.0f);
	    groups[2] = (d+e+g+h) * (0.125f/4.0f);
	    groups[3] = (e+f+h+i) * (0.125f/4.0f);
	    groups[4] = (j+k+l+m) * (0.5f/4.0f);
	    groups[0] *= KarisAverage(groups[0]);
	    groups[1] *= KarisAverage(groups[1]);
	    groups[2] *= KarisAverage(groups[2]);
	    groups[3] *= KarisAverage(groups[3]);
	    groups[4] *= KarisAverage(groups[4]);
	    downsample = groups[0]+groups[1]+groups[2]+groups[3]+groups[4];
	    downsample = max(downsample, 0.0001f);
    } else {

        downsample = e*0.125f;             
	    downsample += (a+c+g+i)*0.03125f;  
	    downsample += (b+d+f+h)*0.0625f;   
	    downsample += (j+k+l+m)*0.125f; 
    }

    return downsample;
}

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < uint(Resources.OutSize.x) && threadID.y < uint(Resources.OutSize.y)) {

        float2 imgSize = Resources.OutSize;
        float2 texCoord = float2(float(threadID.x) / imgSize.x, float(threadID.y) / imgSize.y);
        texCoord += (1.0f / imgSize) * 0.5f;

        float3 color = DownSample13(texCoord);
        if (Resources.MipLevel == 0) {
            // first downsample so prefilter color
            float knee = Resources.Threadshold * Resources.SoftThreadshold;

            float4 f;
            f.x = Resources.Threadshold;
            f.y = f.x - knee;
            f.z = 2.0f * knee;
            f.w = 0.25f / (knee + 0.00001f);

            color = Prefilter(color, f);
        }

        OutTexture[threadID.xy] = float4(color, 1.0f);
    }
}