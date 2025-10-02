#pragma once

#include <entt/entt.hpp>
#include <Engine/Utils/MathUtils.h>
#include <Engine/Renderer/Shader.h>

namespace Spike {

	class Entity;

	struct alignas(16) ObjectGPUData {

		Vec4 BoundsOrigin; // w - bounds radius

		Mat4x4 GlobalTransform;
		Mat4x4 InverseTransform;

		uint32_t FirstIndex;
		uint32_t IndexCount;

		uint64_t IndexBufferAddress;
		uint64_t VertexBufferAddress;

		uint32_t MaterialBufferIndex;
		uint32_t DrawBatchID;

		uint32_t VisibilityIdx;
		float Padding0[3];
	};

	struct alignas(16) LightGPUData {

		Vec4 Position;
		Vec4 Direction; // for spot
		Vec4 Color;

		uint32_t Type; // 1 = point, 2 = spot
		float Intensity;
		float InnerConeCos;
		float OuterConeCos;

		float LightConstant;
		float LightQuadratic;
		float LightLinear;
		float Range;
	};

	struct alignas(16) WorldGPUData {

		Mat4x4 View;
		Mat4x4 Proj;
		Mat4x4 InverseView;
		Mat4x4 InverseProj;
		Mat4x4 ViewProj;

		Vec4 CameraPos;
		Vec4 FrustumPlanes[6];
		Vec4 SunColor; // w - intensity
		Vec4 SunDirection;
		float SunIntensity;

		float P00;
		float P11;
		float NearProj;
		float FarProj;

		uint32_t LightsCount;
		uint32_t ObjectsCount;

		float Padding0[5];
	};

	constexpr uint32_t MAX_DRAW_OBJECTS_PER_WORLD = 100000;
	constexpr uint32_t MAX_LIGHTS_PER_WORLD = 500;
	constexpr uint32_t MAX_SHADERS_PER_WORLD = 100;

	struct WorldDrawBatch {

		uint32_t CommandsCount;
		RHIShader* Shader;
	};

	class RHIWorldProxy : public RHIResource {
	public:
		RHIWorldProxy();
		virtual ~RHIWorldProxy() override {};

		virtual void InitRHI() override;
		virtual void ReleaseRHI() override;

	public:
		struct {
			Vec3 Direction{};
			Vec3 Color{};
			float Intensity{};
		} SunData;

		RHIBuffer* DrawCommandsBuffer;
		RHIBuffer* DrawCountsBuffer;
		RHIBuffer* VisibilityBuffer;
		RHIBuffer* ObjectsBuffer;
		RHIBuffer* LightsBuffer;

		DenseBuffer<ObjectGPUData> ObjectsVB;
		DenseBuffer<LightGPUData> LightsVB;

		std::vector<WorldDrawBatch> Batches;
		IndexQueue VisibilityQueue;
	};

	class World {
	public:
		World();
		~World();

		void Tick();
		RHIWorldProxy* GetProxy() { return m_Proxy; }

	private:

		entt::registry m_Registry;
		//std::vector<Entity> m_RootEntities;

		//std::vector<std::pair<RHIMesh*, >> m_DirtyRenderables;
		//std::vector<Entity> m_DirtyLightObjects;

		RHIWorldProxy* m_Proxy;
		friend class Entity;
	};
}
