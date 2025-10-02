struct SceneGPUData {

    float4x4 View;
    float4x4 Proj;
    float4x4 InverseView;
    float4x4 InverseProj;
    float4x4 ViewProj;

    float4 CameraPos;
    float4 FrustumPlanes[6];
    float4 SunColor;
    float4 SunDirection;
    float SunIntensity;

    float P00;
    float P11;
    float NearProj;
    float FarProj;

    uint LightsCount;
    uint ObjectsCount;

    float Padding0[5];
};

struct SceneObjectGPUData {

	float4 BoundsOrigin; // w - bounds radius
	float4 BoundsExtents;
	float4x4 GlobalTransform;
    float4x4 InverseTransform;

	uint FirstIndex;
	uint IndexCount;

	uint64_t IndexBufferAddress;
	uint64_t VertexBufferAddress;

	uint MaterialBufferIndex;
	uint DrawBatchID;

	int VisibilityIdx;
    float Padding0[3];
};

struct SceneLightGPUData {

    float4 Position;
    float4 Direction; // for spot
    float4 Color;

    int Type; // 0 = directional, 1 = point, 2 = spot
    float Intensity;
    float InnerConeCos;
    float OuterConeCos;

    float LightConstant;
    float LightQuadratic;
    float LightLinear;
    float Range;
};

float4 UnpackUintToUnsignedVec4(uint packed) {
    float r = float((packed & 0x000000ff) >> 0) / 255.f;
    float g = float((packed & 0x0000ff00) >> 8) / 255.f;
    float b = float((packed & 0x00ff0000) >> 16) / 255.f;
    float a = float((packed & 0xff000000) >> 24) / 255.f;

    return float4(r, g, b, a);
}

struct PackedHalf {
    uint A;
    uint B;
};

float4 UnpackHalfToSignedVec4(PackedHalf packed) {
	float x = (float((packed.A & 0x0000ffff) >> 0) / 65535.f) * 2.f - 1.f;
	float y = (float((packed.A & 0xffff0000) >> 16) / 65535.f) * 2.f - 1.f;
	float z = (float((packed.B & 0x0000ffff) >> 0) / 65535.f) * 2.f - 1.f;
	float w = (float((packed.B & 0xffff0000) >> 16) / 65535.f) * 2.f - 1.f;

	return float4(x, y, z, w);
}

#ifdef MATERIAL_SHADER

#define BEGIN_DECL_MATERIAL_RESOURCES()
#define END_DECL_MATERIAL_RESOURCES()

#define DECL_MATERIAL_SCALAR(name, binding) static const uint name =  binding;
#define DECL_MATERIAL_UINT(name, binding) static const uint name = binding;
#define DECL_MATERIAL_FLOAT2(name, binding) static const uint name = binding;
#define DECL_MATERIAL_FLOAT4(name, binding) static const uint name = binding;

#define DECL_MATERIAL_TEXTURE_2D_SRV(name, binding) \
static const uint name = binding;                   \
static const uint name##Sampler = binding;

struct Vertex {
    float Position[3];
    uint Color;

    float2 UV0;
    float2 UV1;

    PackedHalf Tangent;
    PackedHalf Normal;
};

struct MaterialData {

    float ScalarData[16];
    uint UintData[16];
    float2 Float2Data[16];
    float4 Float4Data[16];
    uint TextureData[16];
    uint SamplerData[16];
};

[[vk::binding(0, 0)]] ConstantBuffer<SceneGPUData> SceneDataBuffer;
[[vk::binding(1, 0)]] StructuredBuffer<SceneObjectGPUData> ObjectsBuffer;

[[vk::binding(0, 1)]] StructuredBuffer<MaterialData> MaterialDataBuffer;
[[vk::binding(1, 1)]] Texture2D TextureTable[];
[[vk::binding(2, 1)]] SamplerState SamplerTable[];

#define INVALID_TABLE_INDEX 0xFFFFFFFFu

float4 SampleMaterialTexture(uint dataIndex, uint res, float2 uv) {

    uint tex = MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].TextureData[res];
    uint sam = MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].SamplerData[res];

    if (tex == INVALID_TABLE_INDEX || sam == INVALID_TABLE_INDEX)
        return float4(1.0f, 0.0f, 1.0f, 1.0f);
    return TextureTable[NonUniformResourceIndex(tex)].Sample(SamplerTable[NonUniformResourceIndex(sam)], uv);
}

float GetMaterialScalar(uint dataIndex, uint res) {

    return MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].ScalarData[res];
}

float GetMaterialUint(uint dataIndex, uint res) {

    return MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].UintData[res];
}

float2 GetMaterialFloat2(uint dataIndex, uint res) {

    return MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].Float2Data[res];
}

float4 GetMaterialFloat4(uint dataIndex, uint res) {

    return MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].Float4Data[res];
}
#endif