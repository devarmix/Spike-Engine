struct SceneGPUData {

    float4x4 View;
    float4x4 Proj;
    float4x4 InverseView;
    float4x4 InverseProj;
    float4x4 ViewProj;

    float4 CameraPos;
    float4 FrustumPlanes[6];

    float P00;
    float P11;
    float NearProj;
    float FarProj;

    uint LightsCount;
    uint ObjectsCount;

    float Padding0[2];
    float4 Padding1[3];
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

	int LastVisibilityIndex;
	int CurrentVisibilityIndex;

    float Padding0[2];
};

struct SceneLightGPUData {

    float4 Position;
    float4 Direction; // for directional and spot
    float4 Color;

    float Intensity;
    int Type; // 0 = directional, 1 = point, 2 = spot
    float InnerConeCos;
    float OuterConeCos;
};

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

    float4 Position;           // w - UV_x
    float4 Normal;             // w - UV_y
    float4 Color;
    float4 Tangent;            // w - handedness
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
[[vk::binding(1, 1)]] Texture2D<float4> TextureTable[];
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