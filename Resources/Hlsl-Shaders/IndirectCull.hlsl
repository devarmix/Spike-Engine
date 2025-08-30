#include "ShaderCommon.hlsli"

struct DrawIndirectCommand {

	uint VertexCount;
	uint InstanceCount;  
	uint FirstVertex; 
	uint FirstInstance; 
};

[[vk::binding(0, 0)]] RWStructuredBuffer<DrawIndirectCommand> DrawCommandsBuffer;
[[vk::binding(1, 0)]] RWByteAddressBuffer DrawCountsBuffer;
[[vk::binding(2, 0)]] StructuredBuffer<uint> BatchOffsetsBuffer;
[[vk::binding(3, 0)]] StructuredBuffer<uint> LastVisibilityBuffer;
[[vk::binding(4, 0)]] RWStructuredBuffer<uint> CurrentVisibilityBuffer;
[[vk::binding(5, 0)]] Texture2D<float> DepthPyramid;
[[vk::binding(6, 0)]] SamplerState PyramidSampler;
[[vk::binding(7, 0)]] StructuredBuffer<SceneObjectGPUData> ObjectsBuffer;
[[vk::binding(8, 0)]] ConstantBuffer<SceneGPUData> SceneDataBuffer;

struct CullConstants {

	uint IsPrepass;
	uint PyramidSize;
	float CullZNear;
	float Padding0;
}; [[vk::push_constant]] CullConstants Resources;

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere
// https://jcgt.org/published/0002/02/05/
bool TryCalculateSphereBounds(float3 center, float radius, float zNear, float P00, float P11, out float4 AABB)
{
	if (-center.z < radius + zNear)
	{
		return false;
	}

	float2 centerXZ = -center.xz;
	float2 vX = float2(sqrt(dot(centerXZ, centerXZ) - radius * radius), radius);
	float2 minX = mul(float2x2(float2(vX.x, vX.y), float2(-vX.y, vX.x)), centerXZ);
	float2 maxX = mul(float2x2(float2(vX.x, -vX.y), float2(vX.y, vX.x)), centerXZ);

	float2 centerYZ = -center.yz;
	float2 vY = float2(sqrt(dot(centerYZ, centerYZ) - radius * radius), radius);
	float2 minY = mul(float2x2(float2(vY.x, vY.y), float2(-vY.y, vY.x)), centerYZ);
	float2 maxY = mul(float2x2(float2(vY.x, -vY.y), float2(vY.y, vY.x)), centerYZ);

	AABB = 0.5 - 0.5 * float4(
		minX.x / minX.y * P00, minY.x / minY.y * P11,
		maxX.x / maxX.y * P00, maxY.x / maxY.y * P11);

	return true;
}

bool IsVisible(uint objectID, bool prepass) {

	SceneObjectGPUData objectData = ObjectsBuffer[objectID];

	float3 boundsCenter = objectData.BoundsOrigin.xyz;
	boundsCenter = mul(objectData.GlobalTransform, float4(boundsCenter, 1.f)).xyz;
	float boundsRadius = objectData.BoundsOrigin.w;

    uint lastVisibility = LastVisibilityBuffer[objectData.LastVisibilityIndex];
	bool visible = prepass ? lastVisibility == 1 : true;

	if (visible) {

		for (uint i = 0u; i < 6u; i++) {

		    float4 plane = SceneDataBuffer.FrustumPlanes[i];
		    float d = dot(plane.xyz, boundsCenter) + plane.w;

		    if (d < -boundsRadius) {

			    visible = false;
			    break;
		    } 
		}
	}

	// occlusion culling
	if (!prepass) {

		if (visible) {

		    float3 centerViewSpace = mul(SceneDataBuffer.View, float4(boundsCenter, 1.0f)).xyz;
		    float zNear = Resources.CullZNear;
		    float4 AABB;

		    if (TryCalculateSphereBounds(centerViewSpace, boundsRadius, zNear, SceneDataBuffer.P00, SceneDataBuffer.P11, AABB))
		    {
			    float boundsWidth = (AABB.z - AABB.x) * float(Resources.PyramidSize);
			    float boundsHeight = (AABB.w - AABB.y) * float(Resources.PyramidSize);
			    float mipIndex = floor(log2(max(boundsWidth, boundsHeight)));

			    float occluderDepth = DepthPyramid.SampleLevel(PyramidSampler, 0.5f * (AABB.xy + AABB.zw), mipIndex).x;
			    float nearestBoundsDepth = zNear / (-centerViewSpace.z - boundsRadius);

				const float eps = 0.000025f;
			    bool bOcclusionCulled = occluderDepth >= nearestBoundsDepth + eps;
			    visible = !bOcclusionCulled;
		    }
	    }
	}

	return visible;
}

[numthreads(256, 1, 1)]
void CSMain(uint3 threadID : SV_DispatchThreadID) {

    if (threadID.x < SceneDataBuffer.ObjectsCount) {

        bool prepass = Resources.IsPrepass == 1;
        bool visible = IsVisible(threadID.x, prepass);

        uint localDataIndex = threadID.x;
        SceneObjectGPUData objectData = ObjectsBuffer[localDataIndex];

        uint lastVisibility = LastVisibilityBuffer[objectData.LastVisibilityIndex];
        bool drawMesh = prepass ? visible : visible && lastVisibility == 0;

        if (drawMesh) {

            uint batchOffset = BatchOffsetsBuffer[objectData.DrawBatchID];

            uint localIndex;
			DrawCountsBuffer.InterlockedAdd(objectData.DrawBatchID * 4, 1, localIndex);
            uint localDrawCommandIndex = localIndex + batchOffset;

            DrawIndirectCommand command;
            command.VertexCount = objectData.IndexCount;
            command.InstanceCount = 1;
            command.FirstVertex = 0;
            command.FirstInstance = localDataIndex;

            DrawCommandsBuffer[localDrawCommandIndex] = command;
        }

        if (!prepass) {
            CurrentVisibilityBuffer[objectData.CurrentVisibilityIndex] = visible ? 1 : 0;
        }
    }
}