// modified version of https://github.com/turanszkij/WickedEngine/blob/master/WickedEngine/shaders/blockcompressCS_BC1.hlsl

#pragma dxc diagnostic push
#pragma dxc diagnostic ignored "-Wambig-lit-shift"
#pragma dxc diagnostic ignored "-Wunused-value"
#define ASPM_HLSL
#include "AMDCompressonator/CommonKernel.hlsli"
#pragma dxc diagnostic pop

#if !defined(BC3) && !defined(BC5)
#define BC1
#endif

[[vk::binding(0, 0)]] Texture2D Input;
[[vk::binding(1, 0)]] SamplerState TexSampler;

#ifdef BC1
[[vk::binding(2, 0)]] RWStructuredBuffer<uint2> Output;
#else
[[vk::binding(2, 0)]] RWStructuredBuffer<uint4> Output;
#endif

static const half BayerMatrix8[8][8] =
{
	{ 1.0 / 65.0, 49.0 / 65.0, 13.0 / 65.0, 61.0 / 65.0, 4.0 / 65.0, 52.0 / 65.0, 16.0 / 65.0, 64.0 / 65.0 },
	{ 33.0 / 65.0, 17.0 / 65.0, 45.0 / 65.0, 29.0 / 65.0, 36.0 / 65.0, 20.0 / 65.0, 48.0 / 65.0, 32.0 / 65.0 },
	{ 9.0 / 65.0, 57.0 / 65.0, 5.0 / 65.0, 53.0 / 65.0, 12.0 / 65.0, 60.0 / 65.0, 8.0 / 65.0, 56.0 / 65.0 },
	{ 41.0 / 65.0, 25.0 / 65.0, 37.0 / 65.0, 21.0 / 65.0, 44.0 / 65.0, 28.0 / 65.0, 40.0 / 65.0, 24.0 / 65.0 },
	{ 3.0 / 65.0, 51.0 / 65.0, 15.0 / 65.0, 63.0 / 65.0, 2.0 / 65.0, 50.0 / 65.0, 14.0 / 65.0, 62.0 / 65.0 },
	{ 35.0 / 65.0, 19.0 / 65.0, 47.0 / 65.0, 31.0 / 65.0, 34.0 / 65.0, 18.0 / 65.0, 46.0 / 65.0, 30.0 / 65.0 },
	{ 11.0 / 65.0, 59.0 / 65.0, 7.0 / 65.0, 55.0 / 65.0, 10.0 / 65.0, 58.0 / 65.0, 6.0 / 65.0, 54.0 / 65.0 },
	{ 43.0 / 65.0, 27.0 / 65.0, 39.0 / 65.0, 23.0 / 65.0, 42.0 / 65.0, 26.0 / 65.0, 38.0 / 65.0, 22.0 / 65.0 }
};

half DitherMask8(in min16uint2 pixel) {
	return BayerMatrix8[pixel.x % 8][pixel.y % 8];
}

half Dither(in min16uint2 pixel) {
	return DitherMask8(pixel);
}

#define DITHER(color) (color + (Dither((float2)threadID.xy) - 0.5f) / 64.0f)

[numthreads(8, 8, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID)
{
	uint2 dim;
	Input.GetDimensions(dim.x, dim.y);
	uint2 block_dim = (dim + 3) / 4;

	[branch]
	if (any(threadID.xy >= block_dim))
		return;

	const float2 dimRcp = rcp(dim);
	const float2 uv = float2(threadID.xy * 4 + 1) * dimRcp;

#ifdef BC1
	float3 block[16];
#endif

#ifdef BC3
	float3 block[16];
	float block_a[16];
#endif

#ifdef BC4
	float block_r[16];
#endif

#ifdef BC5
	float block_u[16];
	float block_v[16];
#endif

	//SUB-BLOCK///////////////////////////////////////////////////////////////////////

	float4 red = Input.GatherRed(TexSampler, uv, int2(0, 0));
	float4 green = Input.GatherGreen(TexSampler, uv, int2(0, 0));
	float4 blue = Input.GatherBlue(TexSampler, uv, int2(0, 0));
	float4 alpha = Input.GatherAlpha(TexSampler, uv, int2(0, 0));

#if defined(BC1) || defined(BC3)
	block[0] = DITHER(float3(red[3], green[3], blue[3]));
	block[1] = DITHER(float3(red[2], green[2], blue[2]));
	block[4] = DITHER(float3(red[0], green[0], blue[0]));
	block[5] = DITHER(float3(red[1], green[1], blue[1]));
#endif

#ifdef BC3
	block_a[0] = alpha[3];
	block_a[1] = alpha[2];
	block_a[4] = alpha[0];
	block_a[5] = alpha[1];
#endif

#ifdef BC5
	block_u[0] = red[3];
	block_u[1] = red[2];
	block_u[4] = red[0];
	block_u[5] = red[1];
	block_v[0] = green[3];
	block_v[1] = green[2];
	block_v[4] = green[0];
	block_v[5] = green[1];
#endif

	//SUB-BLOCK///////////////////////////////////////////////////////////////////////

	red = Input.GatherRed(TexSampler, uv, int2(2, 0));
	green = Input.GatherGreen(TexSampler, uv, int2(2, 0));
	blue = Input.GatherBlue(TexSampler, uv, int2(2, 0));
	alpha = Input.GatherAlpha(TexSampler, uv, int2(2, 0));

#if defined(BC1) || defined(BC3)
	block[2] = DITHER(float3(red[3], green[3], blue[3]));
	block[3] = DITHER(float3(red[2], green[2], blue[2]));
	block[6] = DITHER(float3(red[0], green[0], blue[0]));
	block[7] = DITHER(float3(red[1], green[1], blue[1]));
#endif

#ifdef BC3
	block_a[2] = alpha[3];
	block_a[3] = alpha[2];
	block_a[6] = alpha[0];
	block_a[7] = alpha[1];
#endif

#ifdef BC5
	block_u[2] = red[3];
	block_u[3] = red[2];
	block_u[6] = red[0];
	block_u[7] = red[1];
	block_v[2] = green[3];
	block_v[3] = green[2];
	block_v[6] = green[0];
	block_v[7] = green[1];
#endif

	//SUB-BLOCK///////////////////////////////////////////////////////////////////////

	red = Input.GatherRed(TexSampler, uv, int2(0, 2));
	green = Input.GatherGreen(TexSampler, uv, int2(0, 2));
	blue = Input.GatherBlue(TexSampler, uv, int2(0, 2));
	alpha = Input.GatherAlpha(TexSampler, uv, int2(0, 2));

#if defined(BC1) || defined(BC3)
	block[8] = DITHER(float3(red[3], green[3], blue[3]));
	block[9] = DITHER(float3(red[2], green[2], blue[2]));
	block[12] = DITHER(float3(red[0], green[0], blue[0]));
	block[13] = DITHER(float3(red[1], green[1], blue[1]));
#endif

#ifdef BC3
	block_a[8] = alpha[3];
	block_a[9] = alpha[2];
	block_a[12] = alpha[0];
	block_a[13] = alpha[1];
#endif

#ifdef BC5
	block_u[8] = red[3];
	block_u[9] = red[2];
	block_u[12] = red[0];
	block_u[13] = red[1];
	block_v[8] = green[3];
	block_v[9] = green[2];
	block_v[12] = green[0];
	block_v[13] = green[1];
#endif

	//SUB-BLOCK///////////////////////////////////////////////////////////////////////

	red = Input.GatherRed(TexSampler, uv, int2(2, 2));
	green = Input.GatherGreen(TexSampler, uv, int2(2, 2));
	blue = Input.GatherBlue(TexSampler, uv, int2(2, 2));
	alpha = Input.GatherAlpha(TexSampler, uv, int2(2, 2));

#if defined(BC1) || defined(BC3)
	block[10] = DITHER(float3(red[3], green[3], blue[3]));
	block[11] = DITHER(float3(red[2], green[2], blue[2]));
	block[14] = DITHER(float3(red[0], green[0], blue[0]));
	block[15] = DITHER(float3(red[1], green[1], blue[1]));
#endif

#ifdef BC3
	block_a[10] = alpha[3];
	block_a[11] = alpha[2];
	block_a[14] = alpha[0];
	block_a[15] = alpha[1];
#endif

#ifdef BC5
	block_u[10] = red[3];
	block_u[11] = red[2];
	block_u[14] = red[0];
	block_u[15] = red[1];
	block_v[10] = green[3];
	block_v[11] = green[2];
	block_v[14] = green[0];
	block_v[15] = green[1];
#endif

	//COMPRESS-WRITE///////////////////////////////////////////////////////////////////
    uint dataOffset = threadID.x + threadID.y * block_dim.x;

#ifdef BC1
	Output[dataOffset] = CompressBlockBC1_UNORM(block, CMP_QUALITY2, /*isSRGB =*/ false);
#endif

#ifdef BC3
	Output[dataOffset] = CompressBlockBC3_UNORM(block, block_a, CMP_QUALITY2, /*isSRGB =*/ false);
#endif

#ifdef BC5
	Output[dataOffset] = CompressBlockBC5_UNORM(block_u, block_v, CMP_QUALITY2);
#endif
}