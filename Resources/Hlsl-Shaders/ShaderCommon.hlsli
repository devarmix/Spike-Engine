#define BEGIN_DECL_SHADER_RESOURCES(name) struct Decl##name {
#define END_DECL_SHADER_RESOURCES(name) }; [[vk::push_constant]] Decl##name name;

#define DECL_SHADER_SCALAR(name) float name;
#define DECL_SHADER_UINT(name) uint name;
#define DECL_SHADER_FLOAT2(name) float2 name;
#define DECL_SHADER_FLOAT4(name) float4 name;
#define DECL_SHADER_MATRIX2x2(name) float2x2 name;
#define DECL_SHADER_MATRIX4x4(name) float4x4 name;

#define DECL_SHADER_TEXTURE_SRV(name) \
uint name;                            \
uint name##Sampler;

#define DECL_SHADER_TEXTURE_UAV(name) uint name;
#define DECL_SHADER_BUFFER_SRV(name) uint name;
#define DECL_SHADER_BUFFER_UAV(name) uint name;

#define DECL_SHADER_RESOURCES_STRUCT_PADDING(pad) float Padding##pad[pad];

[[vk::binding(0, 0)]] Texture2D Texture2DSRVTable[];
[[vk::binding(1, 0)]] RWTexture2D<float> Texture2DUAVFloatTable[];
[[vk::binding(2, 0)]] RWTexture2D<float2> Texture2DUAVFloat2Table[];
[[vk::binding(3, 0)]] RWTexture2D<float4> Texture2DUAVFloat4Table[];
[[vk::binding(4, 0)]] TextureCube CubeTextureSRVTable[];
[[vk::binding(5, 0)]] ByteAddressBuffer BufferSRVTable[];
[[vk::binding(6, 0)]] RWByteAddressBuffer BufferUAVTable[];
[[vk::binding(7, 0)]] SamplerState SamplerTable[];

#define INVALID_TABLE_INDEX 0xFFFFFFFFu

struct TextureSampler {

    uint ValueIndex;
    bool IsValid() { return ValueIndex != INVALID_TABLE_INDEX; }
};

struct Texture2DSRV {

    uint ValueIndex;
    bool IsValid() { return ValueIndex != INVALID_TABLE_INDEX; }

    TextureSampler UsedSampler;

    float4 Sample(float2 uv) {

        if (!(IsValid() && UsedSampler.IsValid())) 
            return float4(1.0f, 0.0f, 1.0f, 1.0f);

        //return float4(1.f, 0.f, 1.f, 1.f);
        return Texture2DSRVTable[NonUniformResourceIndex(ValueIndex)].Sample(SamplerTable[NonUniformResourceIndex(UsedSampler.ValueIndex)], uv);
    }

    float4 SampleLevel(float2 uv, float mipLevel) {

         if (!(IsValid() && UsedSampler.IsValid())) 
            return float4(1.0f, 0.0f, 1.0f, 1.0f);

        //return float4(1.f, 0.f, 1.f, 1.f);
        return Texture2DSRVTable[NonUniformResourceIndex(ValueIndex)].SampleLevel(SamplerTable[NonUniformResourceIndex(UsedSampler.ValueIndex)], uv, mipLevel);
    }
};

struct Texture2DUAVFloat {

    uint ValueIndex;
    bool IsValid() { return ValueIndex != INVALID_TABLE_INDEX; } 

    void Store(uint2 pos, float color) {

        if (!IsValid()) 
            return;

        Texture2DUAVFloatTable[NonUniformResourceIndex(ValueIndex)][pos] = color;
    }
};

struct Texture2DUAVFloat2 {

    uint ValueIndex;
    bool IsValid() { return ValueIndex != INVALID_TABLE_INDEX; }

    void Store(uint2 pos, float2 color) {

        if (!IsValid()) 
            return;

        Texture2DUAVFloat2Table[NonUniformResourceIndex(ValueIndex)][pos] = color;
    }
};

struct Texture2DUAVFloat4 {

    uint ValueIndex;
    bool IsValid() { return ValueIndex != INVALID_TABLE_INDEX; }

    void Store(uint2 pos, float4 color) {

        if (!IsValid()) 
            return;

        Texture2DUAVFloat4Table[NonUniformResourceIndex(ValueIndex)][pos] = color;
    }
};

struct CubeTextureSRV {

    uint ValueIndex;
    bool IsValid() { return ValueIndex != INVALID_TABLE_INDEX; }

    TextureSampler UsedSampler;

    float4 Sample(float3 uv) {

        if (!(IsValid() && UsedSampler.IsValid()))
            return float4(1.0f, 0.0f, 1.0f, 1.0f);

        //return float4(1.f, 0.f, 1.f, 1.f);
        return CubeTextureSRVTable[NonUniformResourceIndex(ValueIndex)].Sample(SamplerTable[NonUniformResourceIndex(UsedSampler.ValueIndex)], uv);
    }

    float4 SampleLevel(float3 uv, float mipLevel) {

        if (!(IsValid() && UsedSampler.IsValid()))
            return float4(1.0f, 0.0f, 1.0f, 1.0f);

        //return float4(1.f, 0.f, 1.f, 1.f);

        return CubeTextureSRVTable[NonUniformResourceIndex(ValueIndex)].SampleLevel(SamplerTable[NonUniformResourceIndex(UsedSampler.ValueIndex)], uv, mipLevel);
    }
};

struct BufferSRV {

    uint ValueIndex;
    bool IsValid() { return ValueIndex != INVALID_TABLE_INDEX; }

    template<typename T>
    T LoadAtIndex(uint index) {

        // return empty if not valid
        if (!IsValid()) 
            return (T)0;

        return BufferSRVTable[NonUniformResourceIndex(ValueIndex)].Load<T>(sizeof(T) * index);
    }
};

struct BufferUAV {

    uint ValueIndex;
    bool IsValid() { return ValueIndex != INVALID_TABLE_INDEX; }

    template<typename T>
    T LoadAtIndex(uint index) {

        // return empty if not valid
        if (!IsValid()) 
            return (T)0;

        return BufferUAVTable[NonUniformResourceIndex(ValueIndex)].Load<T>(sizeof(T) * index);
    }

    template<typename T>
    void StoreAtIndex(uint index, T data) {

        if (!IsValid()) 
            return;

        BufferUAVTable[NonUniformResourceIndex(ValueIndex)].Store<T>(sizeof(T) * index, data);
    }

    void InterlockedAddAtIndex(uint index, uint data, out uint oldData) {

        if (!IsValid()) {
            oldData = 0;
            return;
        }

        BufferUAVTable[NonUniformResourceIndex(ValueIndex)].InterlockedAdd(index * 4, data, oldData);
    }
};


#define TEXTURE_2D_SRV(name, resource) \
Texture2DSRV name;                     \
name.ValueIndex = resource;            \
name.UsedSampler.ValueIndex = resource##Sampler;

#define TEXTURE_2D_UAV_FLOAT(name, resource) \
Texture2DUAVFloat name;                      \
name.ValueIndex = resource;

#define TEXTURE_2D_UAV_FLOAT2(name, resource) \
Texture2DUAVFloat2 name;                      \
name.ValueIndex = resource;

#define TEXTURE_2D_UAV_FLOAT4(name, resource) \
Texture2DUAVFloat4 name;                      \
name.ValueIndex = resource;

#define CUBE_TEXTURE_SRV(name, resource) \
CubeTextureSRV name;                     \
name.ValueIndex = resource;      \
name.UsedSampler.ValueIndex = resource##Sampler;

#define BUFFER_SRV(name, resource) \
BufferSRV name;                    \
name.ValueIndex = resource;

#define BUFFER_UAV(name, resource) \
BufferUAV name;                    \
name.ValueIndex = resource;


#ifdef USE_MATERIAL_RENDERING_DATA
#define USE_SCENE_RENDERING_DATA

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
[[vk::binding(8, 0)]] StructuredBuffer<MaterialData> MaterialDataBuffer;

#define BEGIN_DECL_MATERIAL_RESOURCES()
#define END_DECL_MATERIAL_RESOURCES()

#define DECL_MATERIAL_SCALAR(name, binding) static const uint name =  binding;
#define DECL_MATERIAL_UINT(name, binding) static const uint name = binding;
#define DECL_MATERIAL_FLOAT2(name, binding) static const uint name = binding;
#define DECL_MATERIAL_FLOAT4(name, binding) static const uint name = binding;

#define DECL_MATERIAL_TEXTURE_2D_SRV(name, binding) \
static const uint name = binding;                   \
static const uint name##Sampler = binding;

#define MATERIAL_TEXTURE_2D_SRV(dataIndex, name, resource)                                              \
Texture2DSRV name;                                                                                      \
name.ValueIndex = MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].TextureData[resource];         \
name.UsedSampler.ValueIndex = MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].SamplerData[resource##Sampler];

float GetMaterialScalar(uint dataIndex, uint resource) {

    return MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].ScalarData[resource];
}

uint GetMaterialUint(uint dataIndex, uint resource) {

    return MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].UintData[resource];
}

float2 GetMaterialFloat2(uint dataIndex, uint resource) {

    return MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].Float2Data[resource];
}

float4 GetMaterialFloat4(uint dataIndex, uint resource) {

    return MaterialDataBuffer[NonUniformResourceIndex(dataIndex)].Float4Data[resource];
}

#endif

#ifdef USE_SCENE_RENDERING_DATA
struct SceneData {

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

    uint LightsOffset;
    uint ObjectsOffset;
    uint MaterialsOffset;
    uint LightsCount;
    uint ObjectsCount;

    float Padding0[3];
    float4 Padding1[2];
};
[[vk::binding(0, 1)]] ConstantBuffer<SceneData> SceneDataBuffer;

struct SceneObjectGPUData {

	float4 BoundsOrigin; // w - bounds radius
	float4 BoundsExtents;
	float4x4 GlobalTransform;
    float4x4 InverseTransform;

	uint FirstIndex;
	uint IndexCount;

	uint IndexBuffer;
	uint VertexBuffer;

	uint MaterialBufferIndex;
	uint DrawBatchID;

	int LastVisibilityIndex;
	int CurrentVisibilityIndex;
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
#endif
