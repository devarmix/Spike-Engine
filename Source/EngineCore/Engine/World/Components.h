#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/World/Entity.h>

namespace Spike {

	class BaseEntityComponent {
	public:
		BaseEntityComponent(Entity* entity) : m_Entity(entity) {}
		virtual ~BaseEntityComponent() = default;

	protected:
		Entity* m_Entity;
		friend class Entity;
	};

	class StaticMeshProxy {
	public:
		StaticMeshProxy(RHIWorldProxy* wProxy, const Mat4x4& transform);
		~StaticMeshProxy();

		void OnTransformChange(const Mat4x4& newTransform);
		void SetMaterial(RHIMaterial* mat, uint32_t index, bool isNew = false);
		void PushMaterial(RHIMaterial* mat);
		void PopMaterial();
		void SetMesh(RHIMesh* mesh);

	private:
		void RemoveFromBatch(uint32_t idx);
		uint32_t FindBatch(RHIShader* shader);

	private:
		std::vector<uint32_t> m_DataIndices;
		std::vector<std::pair<RHIMaterial*, uint32_t>> m_Materials;
		Mat4x4 m_LastTransform;
		RHIWorldProxy* m_WorldProxy;
	};

	class StaticMeshComponent : public BaseEntityComponent {
	public:
		StaticMeshComponent(Entity* entity);
		virtual ~StaticMeshComponent() override;

		void SetMesh(Ref<Mesh> mesh);
		void SetMaterial(Ref<Material> mat, uint32_t index);
		void PushMaterial(Ref<Material> mat);
		void PopMaterial();

	private:

		Ref<Mesh> m_Mesh;
		std::vector<Ref<Material>> m_Materials;
		StaticMeshProxy* m_Proxy;

		uint8_t m_CallBackID;
	};

	class LightProxy {
	public:
		LightProxy(RHIWorldProxy* wProxy);
		~LightProxy();

		void SetIntensity(float value);
		void SetRange(float value);
		void SetColor(const Vec4& value);
		void SetDirection(const Vec3& dir);
		void SetLightLinear(float value);
		void SetLightConstant(float value);
		void SetLightQuadratic(float value);
		void SetType(uint32_t type);
		void SetInnerConeCos(float value);
		void SetOuterConeCos(float value);
		void OnPositionChange(const Vec3& pos);

	private:
		uint32_t m_DataIndex;
		RHIWorldProxy* m_WorldProxy;
	};

	enum class ELightType : uint8_t {

		ENone = 0,
		EPoint,
		ESpot
	};

	class LightComponent : public BaseEntityComponent {
	public:
		LightComponent(Entity* entity);
		virtual ~LightComponent() override;

		void SetIntensity(float value);
		void SetRange(float value);
		void SetColor(const Vec4& value);
		void SetDirection(const Vec3& value);
		void SetLightLinear(float value);
		void SetLightConstant(float value);
		void SetLightQuadratic(float value);
		void SetType(ELightType type);
		void SetInnerConeCos(float value);
		void SetOuterConeCos(float value);

	private:
		LightProxy* m_Proxy;
		uint8_t m_CallBackID;
	};

	class CameraComponent : public BaseEntityComponent {

	};
}