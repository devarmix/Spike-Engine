#include "ShaderCommon.hlsli"

BEGIN_DECL_SHADER_RESOURCES(Resources)
    DECL_SHADER_TEXTURE_UAV(OutTexture)
    DECL_SHADER_TEXTURE_SRV(SampledTexture)
    DECL_SHADER_UINT(CubeFaceIndex)
    DECL_SHADER_UINT(FilterType)
    DECL_SHADER_UINT(OutSize)
    DECL_SHADER_SCALAR(Roughness)

    DECL_SHADER_RESOURCES_STRUCT_PADDING(1)
END_DECL_SHADER_RESOURCES(Resources)

#define FILTER_TYPE_NONE 0
#define FILTER_TYPE_IRRADIANCE_MAP 1
#define FILTER_TYPE_RADIANCE_MAP 2

#define PI 3.1415926536
#define NUM_RADIANCE_SAMPLES 128u
#define IRRADIANCE_SAMPLE_DELTA 0.004f

float3 SampleDirection(float2 uv) {

    // [0,1] → [-1,1]
    uv = uv * 2.0f - 1.0f;

    float3 dir;
    if (Resources.CubeFaceIndex == 0)      dir = normalize(float3(1.0f, -uv.y, -uv.x));  // +X
    else if (Resources.CubeFaceIndex == 1) dir = normalize(float3(-1.0f, -uv.y, uv.x));  // -X
    else if (Resources.CubeFaceIndex == 2) dir = normalize(float3(uv.x, 1.0f, uv.y));    // +Y
    else if (Resources.CubeFaceIndex == 3) dir = normalize(float3(uv.x, -1.0f, -uv.y));  // -Y
    else if (Resources.CubeFaceIndex == 4) dir = normalize(float3(uv.x, -uv.y, 1.0f));   // +Z
    else if (Resources.CubeFaceIndex == 5) dir = normalize(float3(-uv.x, -uv.y, -1.0f)); // -Z

    return dir;
}

float2 DirToEquirectUV(float3 dir) {

    float phi = atan2(dir.z, dir.x);
    float theta = acos(clamp(dir.y, -1.0f, 1.0f));
    return float2(phi / (2.0f * PI) + 0.5f, theta / PI);
}

// glsl style mod
float mod(float a, float b) {
    return a - b * floor(a / b);
}

float Random(float2 co) {

    float a = 12.9898;
	float b = 78.233;
	float c = 43758.5453;
	float dt = dot(co.xy, float2(a,b));
	float sn = mod(dt, 3.14);
	return frac(sin(sn) * c);
}

float2 Hammersley2D(uint i, uint N) {

    uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rdi = float(bits) * 2.3283064365386963e-10;
	return float2(float(i) /float(N), rdi);
}

float3 ImportanceSampleGGX(float2 Xi, float roughness, float3 normal) {

    // Maps a 2D point to a hemisphere with spread based on roughness
	float alpha = roughness * roughness;
	float phi = 2.0 * PI * Xi.x + Random(normal.xz) * 0.1;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	float3 H = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

	// Tangent space
	float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangentX = normalize(cross(up, normal));
	float3 tangentY = normalize(cross(normal, tangentX));

	// Convert to world Space
	return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);
}

float DGGX(float dotNH, float roughness) {

	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = dotNH * dotNH * (alpha2 - 1.0f) + 1.0f;
	return (alpha2)/(PI * denom*denom); 
}

float3 FilterRadiance(float3 R) {

	float3 N = R;
	float3 V = R;
	float3 color = (float3)0.0;
	float totalWeight = 0.0;
	float envMapDim = float(Resources.OutSize);

    CUBE_TEXTURE_SRV(texture, Resources.SampledTexture)
	for(uint i = 0u; i < NUM_RADIANCE_SAMPLES; i++) {

		float2 Xi = Hammersley2D(i, NUM_RADIANCE_SAMPLES);
		float3 H = ImportanceSampleGGX(Xi, Resources.Roughness, N);
		float3 L = 2.0 * dot(V, H) * H - V;
		float dotNL = clamp(dot(N, L), 0.0, 1.0);

		if(dotNL > 0.0) {

			// Filtering based on https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/
			float dotNH = clamp(dot(N, H), 0.0, 1.0);
			float dotVH = clamp(dot(V, H), 0.0, 1.0);

			// Probability Distribution Function
			float pdf = DGGX(dotNH, Resources.Roughness) * dotNH / (4.0 * dotVH) + 0.0001;
			// Slid angle of current smple
			float omegaS = 1.0 / (float(NUM_RADIANCE_SAMPLES) * pdf);
			// Solid angle of 1 pixel across all cube faces
			float omegaP = 4.0 * PI / (6.0 * envMapDim * envMapDim);
			// Biased (+1.0) mip level for better result
			float mipLevel = Resources.Roughness == 0.0 ? 0.0 : max(0.5 * log2(omegaS / omegaP) + 1.0, 0.0f);

            color += texture.SampleLevel(L, mipLevel).rgb * dotNL;
			totalWeight += dotNL;
		}
	}

	return (color / totalWeight);
}

float3 FilterIrradiance(float3 N) {

    float3 irradiance = (float3)0.0;  

    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    CUBE_TEXTURE_SRV(texture, Resources.SampledTexture)
    float nrSamples = 0.0f;

    for(float phi = 0.0; phi < 2.0 * PI; phi += IRRADIANCE_SAMPLE_DELTA) {

        for(float theta = 0.0; theta < 0.5 * PI; theta += IRRADIANCE_SAMPLE_DELTA) {

            // spherical to cartesian (in tangent space)
            float3 tangentSample = float3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

            irradiance += texture.SampleLevel(sampleVec, 0.0).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    return irradiance;
}

float3 FilterNone(float3 N) {

    float2 eqUV = DirToEquirectUV(N);

    TEXTURE_2D_SRV(texture, Resources.SampledTexture)
    return texture.SampleLevel(eqUV, 0.0).rgb;
}

[numthreads(8, 8, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < Resources.OutSize && threadID.y < Resources.OutSize) {

        float2 uv = float2(threadID.xy) / float(Resources.OutSize); 
        float3 dir = SampleDirection(uv);
        float3 color;

        if (Resources.FilterType == FILTER_TYPE_NONE) {

            color = FilterNone(dir);
        } else if (Resources.FilterType == FILTER_TYPE_RADIANCE_MAP) {

            color = FilterRadiance(dir);
        } else if (Resources.FilterType == FILTER_TYPE_IRRADIANCE_MAP) {

            color = FilterIrradiance(dir);
        }

        TEXTURE_2D_UAV_FLOAT4(output, Resources.OutTexture)
        output.Store(threadID.xy, float4(color, 1.0f));
    }
}