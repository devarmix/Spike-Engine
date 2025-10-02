#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Core/Log.h>
#include <Engine/World/World.h>
#include <Engine/Asset/UUID.h>

namespace Spike {

	using EntityHandle = entt::entity;

	class Entity {
	public:
		Entity(World* world);
		~Entity();

		Vec3 GetPosition() const { return m_Position; }
		Quaternion GetRotation() const { return m_Rotation; }
		Vec3 GetScale() const { return m_Scale; }
		const Mat4x4& GetTransform() const { return m_WorldTransform; }

		void SetScale(const Vec3& scale);
		void SetPosition(const Vec3& pos);
		void SetRotation(const Vec3& rot);

		using TransformCallBack = std::function<void(const Vec3&, const Vec3&, const Vec3&, const Mat4x4&)>;
		uint8_t AddTransformCallBack(TransformCallBack&& func);
		void RemoveTransformCallBack(uint8_t id);
		void DispatchTransformCallBack();

		Entity* GetParent() const { return m_Parent; }
		void SetParent(Entity* parent);
		const std::vector<Entity*>& GetChildren() const { return m_Children; }
		UUID GetID() const { return m_ID; }
		World* GetWorld() { return m_World; }

		template<typename T>
		bool HasComponent() {
			return m_World->m_Registry.all_of<T>(m_Handle);
		}

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args) {

			if (HasComponent<T>()) ENGINE_ERROR("Gameobject already has the component!");
			T& component = m_World->m_Registry.emplace<T>(m_Handle, std::forward<Args>(args)..., this);
			component.m_Entity = this;
			return component;
		}

		template<typename T>
		T& GetComponent() {

			if (!HasComponent<T>()) ENGINE_ERROR("Gameobject does not have the component to get!");
			return m_World->m_Registry.get<T>(m_Handle);
		}

		template<typename T>
		void DestroyComponent() {

			if (!HasComponent<T>()) ENGINE_ERROR("Gameobject does not have the component to destroy!");
			m_World->m_Registry.remove<T>(m_Handle);
		}

		bool operator==(const Entity& other) const {
			return m_Handle == other.m_Handle && m_World == other.m_World;
		}

		bool operator!=(const Entity& other) const {
			return !(*this == other);
		}

	private:
		void UpdateWorldTransform();

	private:

		Entity* m_Parent;
		std::vector<Entity*> m_Children;
		std::vector<TransformCallBack> m_TransformCallbacks;

		Vec3 m_Position;
		Vec3 m_Rotation;
		Vec3 m_Scale;
		Mat4x4 m_WorldTransform;

		World* m_World;
		UUID m_ID;
		EntityHandle m_Handle;
 	};
}
