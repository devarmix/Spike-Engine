#define USE_SCENE_RENDERING_DATA
#include "ShaderCommon.hlsli"

BEGIN_DECL_SHADER_RESOURCES(Resources)
    DECL_SHADER_BUFFER_UAV(DrawCommandsBuffer)
    DECL_SHADER_BUFFER_UAV(DrawCountsBuffer)
    DECL_SHADER_BUFFER_SRV(BatchOffsetsBuffer)
    DECL_SHADER_BUFFER_SRV(LastVisibilityBuffer)
    DECL_SHADER_BUFFER_SRV(SceneObjectsBuffer)
    DECL_SHADER_BUFFER_UAV(CurrentVisibilityBuffer)
    DECL_SHADER_TEXTURE_SRV(DepthPyramid)
    DECL_SHADER_UINT(IsPrepass)
    DECL_SHADER_UINT(PyramidSize)
	DECL_SHADER_SCALAR(CullZNear)

    DECL_SHADER_RESOURCES_STRUCT_PADDING(1)
END_DECL_SHADER_RESOURCES(Resources)


struct DrawIndirectCommand {

	uint VertexCount;
	uint InstanceCount;  
	uint FirstVertex; 
	uint FirstInstance; 
};

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

    BUFFER_SRV(objectsBuffer, Resources.SceneObjectsBuffer)
    SceneObjectGPUData objectData = objectsBuffer.LoadAtIndex<SceneObjectGPUData>(SceneDataBuffer.ObjectsOffset + objectID);

	float3 boundsCenter = objectData.BoundsOrigin.xyz;
	boundsCenter = mul(objectData.GlobalTransform, float4(boundsCenter, 1.f)).xyz;
	float boundsRadius = objectData.BoundsOrigin.w;

    BUFFER_SRV(lastVisibilityBuffer, Resources.LastVisibilityBuffer)

    uint lastVisibility = lastVisibilityBuffer.LoadAtIndex<uint>(objectData.LastVisibilityIndex);
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

                TEXTURE_2D_SRV(pyramid, Resources.DepthPyramid)

			    float occluderDepth = pyramid.SampleLevel(0.5f * (AABB.xy + AABB.zw), mipIndex).x;
			    float nearestBoundsDepth = zNear / (-centerViewSpace.z - boundsRadius);

			    bool bOcclusionCulled = occluderDepth >= nearestBoundsDepth;
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

        uint localDataIndex = SceneDataBuffer.ObjectsOffset + threadID.x;
        BUFFER_SRV(objectsBuffer, Resources.SceneObjectsBuffer)
        SceneObjectGPUData objectData = objectsBuffer.LoadAtIndex<SceneObjectGPUData>(localDataIndex);

        BUFFER_SRV(lastVisibilityBuffer, Resources.LastVisibilityBuffer)
        uint lastVisibility = lastVisibilityBuffer.LoadAtIndex<uint>(objectData.LastVisibilityIndex);
        bool drawMesh = prepass ? visible : visible && lastVisibility == 0;

        if (drawMesh) {

            BUFFER_SRV(batchOffsetsBuffer, Resources.BatchOffsetsBuffer)
            BUFFER_UAV(drawCountsBuffer, Resources.DrawCountsBuffer)
            BUFFER_UAV(drawCommandsbuffer, Resources.DrawCommandsBuffer)

            uint batchOffset = batchOffsetsBuffer.LoadAtIndex<uint>(SceneDataBuffer.MaterialsOffset + objectData.DrawBatchID);

            uint localIndex;
            drawCountsBuffer.InterlockedAddAtIndex(SceneDataBuffer.MaterialsOffset + batchOffset, 1, localIndex);
            uint localDrawCommandIndex = SceneDataBuffer.ObjectsOffset + batchOffset + localIndex;

            DrawIndirectCommand command;
            command.VertexCount = objectData.IndexCount;
            command.InstanceCount = 1;
            command.FirstVertex = 0;
            command.FirstInstance = localDataIndex;

            drawCommandsbuffer.StoreAtIndex<DrawIndirectCommand>(localDrawCommandIndex, command);
        }

        if (!prepass) {

            BUFFER_UAV(currentVisibilityBuffer, Resources.CurrentVisibilityBuffer)
            currentVisibilityBuffer.StoreAtIndex<uint>(objectData.CurrentVisibilityIndex, visible ? 1 : 0);
        }
    }
}