#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Renderer/Mesh.h>
#include <Engine/World/Entity.h>

namespace Spike {

	class BaseEntityComponent {
	public:
		BaseEntityComponent(Entity entity) : m_Self(entity) {}
		virtual ~BaseEntityComponent() = default;

		virtual void OnEntityMove(const Vec3& pos, const Vec3& rot, const Vec3& scale, const Mat4x4& world) {}

	protected:
		Entity m_Self;
	};

	class HierarchyComponent : public BaseEntityComponent {
	public:
		HierarchyComponent(Entity self, World* owner)
			: BaseEntityComponent(self), m_Owner(owner), m_LayerMask(0) {}
		virtual ~HierarchyComponent() override;

		void SetName(const std::string& value) { m_Name = value; }
		const std::string& GetName() const { return m_Name; }

		uint32_t GetLayerMask() const { return m_LayerMask; }
		void AddLayer(uint32_t layer) { m_LayerMask |= layer; }
		void RemoveLayer(uint32_t layer) { m_LayerMask &= ~layer; }

		Entity GetParent() const { return m_Parent; }
		void SetParent(Entity parent);

		const std::vector<Entity>& GetChildren() const { return m_Children; }
		World* GetOwnerWorld() { return m_Owner; }

	private:
		std::string m_Name;
		uint32_t m_LayerMask;

		Entity m_Parent;
		std::vector<Entity> m_Children;  

		World* m_Owner;
	};

	class TransformComponent : public BaseEntityComponent {
	public:
		TransformComponent(Entity self);
		virtual ~TransformComponent() override {}

		const Vec3& GetPosition() const { return m_Position; }
		const Vec3& GetRotation() const { return m_Rotation; }
		const Vec3& GetScale() const { return m_Scale; }
		const Mat4x4& GetWorldTranform() const { return m_WorldTransform; }

		void SetPosition(const Vec3& value);
		void SetRotation(const Vec3& value);
		void SetScale(const Vec3& value);

		void UpdateParentTransform(const Mat4x4& mat);

	private:
		void UpdateTransformMatrix(const Mat4x4* parentMat = nullptr);

    private:
		Vec3 m_Position;
		Vec3 m_Rotation;
		Vec3 m_Scale;
		Mat4x4 m_WorldTransform;
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
		StaticMeshComponent(Entity entity);
		virtual ~StaticMeshComponent() override;

		void SetMesh(Ref<Mesh> mesh);
		void SetMaterial(Ref<Material> mat, uint32_t index);
		void PushMaterial(Ref<Material> mat);
		void PopMaterial();

		virtual void OnEntityMove(const Vec3& pos, const Vec3& rot, const Vec3& scale, const Mat4x4& world) override;

	private:

		Ref<Mesh> m_Mesh;
		std::vector<Ref<Material>> m_Materials;
		StaticMeshProxy* m_Proxy;
	};

	class LightProxy {
	public:
		LightProxy(RHIWorldProxy* wProxy);
		~LightProxy();

		void Init(const Vec3& pos, const Vec3& rot);

		void SetIntensity(float value);
		void SetRange(float value);
		void SetColor(const Vec4& value);
		void SetDirection(const Vec3& dir);
		void SetPosition(const Vec3& pos);
		void SetLightLinear(float value);
		void SetLightConstant(float value);
		void SetLightQuadratic(float value);
		void SetType(uint32_t type);
		void SetInnerConeCos(float value);
		void SetOuterConeCos(float value);

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
		LightComponent(Entity entity);
		virtual ~LightComponent() override;

		void SetIntensity(float value);
		void SetRange(float value);
		void SetColor(const Vec4& value);
		void SetLightLinear(float value);
		void SetLightConstant(float value);
		void SetLightQuadratic(float value);
		void SetType(ELightType type);
		void SetInnerConeCos(float value);
		void SetOuterConeCos(float value);

		virtual void OnEntityMove(const Vec3& pos, const Vec3& rot, const Vec3& scale, const Mat4x4& world) override;

	private:
		LightProxy* m_Proxy;
	};

	class CameraComponent : public BaseEntityComponent {

	};
}