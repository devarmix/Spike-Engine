#include "ShaderCommon.hlsli"

BEGIN_DECL_SHADER_RESOURCES(Resources)
    DECL_SHADER_FLOAT2(SrcSize)
    DECL_SHADER_FLOAT2(OutSize)
    DECL_SHADER_UINT(MipLevel)
    DECL_SHADER_SCALAR(Threadshold)
    DECL_SHADER_SCALAR(SoftThreadshold)
    DECL_SHADER_TEXTURE_SRV(SampledTextureA)
    DECL_SHADER_TEXTURE_SRV(SampledTextureB)
    DECL_SHADER_TEXTURE_UAV(OutTexture)
    DECL_SHADER_UINT(BloomStage)
    DECL_SHADER_SCALAR(BloomIntensity)
    DECL_SHADER_SCALAR(FilterRadius)
    
    DECL_SHADER_RESOURCES_STRUCT_PADDING(1)
END_DECL_SHADER_RESOURCES(Resources)

#define BLOOM_STAGE_DOWN_SAMPLE 0
#define BLOOM_STAGE_UP_SAMPLE 1
#define BLOOM_STAGE_COMPOSITE 2

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

    TEXTURE_2D_SRV(texture, Resources.SampledTextureA)
    float3 a = texture.SampleLevel(float2(texCoord.x - 2*x, texCoord.y + 2*y), 0.0).rgb;
    float3 b = texture.SampleLevel(float2(texCoord.x,       texCoord.y + 2*y), 0.0).rgb;
    float3 c = texture.SampleLevel(float2(texCoord.x + 2*x, texCoord.y + 2*y), 0.0).rgb;

    float3 d = texture.SampleLevel(float2(texCoord.x - 2*x, texCoord.y), 0.0).rgb;
    float3 e = texture.SampleLevel(float2(texCoord.x,       texCoord.y), 0.0).rgb;
    float3 f = texture.SampleLevel(float2(texCoord.x + 2*x, texCoord.y), 0.0).rgb;

    float3 g = texture.SampleLevel(float2(texCoord.x - 2*x, texCoord.y - 2*y), 0.0).rgb;
    float3 h = texture.SampleLevel(float2(texCoord.x,       texCoord.y - 2*y), 0.0).rgb;
    float3 i = texture.SampleLevel(float2(texCoord.x + 2*x, texCoord.y - 2*y), 0.0).rgb;

    float3 j = texture.SampleLevel(float2(texCoord.x - x, texCoord.y + y), 0.0).rgb;
    float3 k = texture.SampleLevel(float2(texCoord.x + x, texCoord.y + y), 0.0).rgb;
    float3 l = texture.SampleLevel(float2(texCoord.x - x, texCoord.y - y), 0.0).rgb;
    float3 m = texture.SampleLevel(float2(texCoord.x + x, texCoord.y - y), 0.0).rgb;

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

float3 UpSample9(float2 texCoord) {

    float x = Resources.FilterRadius;
    float y = Resources.FilterRadius;

    TEXTURE_2D_SRV(texture, Resources.SampledTextureA)
    float3 a = texture.SampleLevel(float2(texCoord.x - x, texCoord.y + y), 0.0).rgb;
    float3 b = texture.SampleLevel(float2(texCoord.x,     texCoord.y + y), 0.0).rgb;
    float3 c = texture.SampleLevel(float2(texCoord.x + x, texCoord.y + y), 0.0).rgb;

    float3 d = texture.SampleLevel(float2(texCoord.x - x, texCoord.y), 0.0).rgb;
    float3 e = texture.SampleLevel(float2(texCoord.x,     texCoord.y), 0.0).rgb;
    float3 f = texture.SampleLevel(float2(texCoord.x + x, texCoord.y), 0.0).rgb;

    float3 g = texture.SampleLevel(float2(texCoord.x - x, texCoord.y - y), 0.0).rgb;
    float3 h = texture.SampleLevel(float2(texCoord.x,     texCoord.y - y), 0.0).rgb;
    float3 i = texture.SampleLevel(float2(texCoord.x + x, texCoord.y - y), 0.0).rgb;

    float3 upsample = e*4.0f;
	upsample += (b+d+f+h)*2.0f;
	upsample += (a+c+g+i);
	upsample *= 1.0f / 16.0f;

    return upsample;
}

[numthreads(32, 32, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < uint(Resources.OutSize.x) && threadID.y < uint(Resources.OutSize.y)) {

        float2 imgSize = Resources.OutSize;
        float2 texCoord = float2(float(threadID.x) / imgSize.x, float(threadID.y) / imgSize.y);
        texCoord += (1.0f / imgSize) * 0.5f;

        float3 color;

        if (Resources.BloomStage == BLOOM_STAGE_DOWN_SAMPLE) {

            color = DownSample13(texCoord);
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
        } else if (Resources.BloomStage == BLOOM_STAGE_UP_SAMPLE) {

            color = UpSample9(texCoord);
            TEXTURE_2D_SRV(texture, Resources.SampledTextureB)

            float3 colorB = texture.SampleLevel(texCoord, 0.0).rgb;
            color += colorB;
        } else if (Resources.BloomStage == BLOOM_STAGE_COMPOSITE) {

            color = UpSample9(texCoord);
            TEXTURE_2D_SRV(texture, Resources.SampledTextureB)

            float3 colorB = texture.SampleLevel(texCoord, 0.0).rgb;
            color = lerp(colorB, color, Resources.BloomIntensity);
        }

        TEXTURE_2D_UAV_FLOAT4(output, Resources.OutTexture)
        output.Store(threadID.xy, float4(color, 1.0f));
    }
}