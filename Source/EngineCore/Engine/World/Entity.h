#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Core/Log.h>
#include <Engine/Asset/UUID.h>
#include <Engine/World/World.h>

namespace Spike {

	class TransformComponent;
	class HierarchyComponent;

	class Entity {
	public:
		Entity() : m_World(nullptr), m_Handle(entt::null) {}
		Entity(World* world, entt::entity handle) : m_World(world), m_Handle(handle) {}
		~Entity() {}

		World* GetWorld() { return m_World; }
		entt::entity GetHandle() const { return m_Handle; }
		void Destroy() { m_World->DestroyEntity(*this); }

		template<typename T>
		bool HasComponent() {
			return m_World->EntityHasComponent<T>(m_Handle);
		}

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args) {
			if (HasComponent<T>()) ENGINE_ERROR("Entity already has the component!");

			T& component = m_World->RegisterEntityComponent<T>(m_Handle, *this, std::forward<Args>(args)...);
			return component;
		}

		template<typename T>
		T& GetComponent() {
			if (!HasComponent<T>()) ENGINE_ERROR("Entity does not have the component to get!");
			return m_World->GetEntityComponent<T>(m_Handle);
		}

		template<typename T>
		void DestroyComponent() {
			if (!HasComponent<T>()) ENGINE_ERROR("Entity does not have the component to destroy!");
			m_World->DestroyEntityComponent<T>(m_Handle);
		}

		template<>
		void DestroyComponent<TransformComponent>() {
			assert(false && "Destuction forbidden!");
		}

		template<>
		void DestroyComponent<HierarchyComponent>() {
			assert(false && "Destuction forbidden!");
		}

		bool operator==(const Entity& other) const {
			return m_Handle == other.m_Handle && m_World == other.m_World;
		}

		bool operator!=(const Entity& other) const {
			return !(*this == other);
		}

		operator bool() const {
			return m_World != nullptr && m_Handle != entt::null;
		}

	private:
		World* m_World;
		entt::entity m_Handle;
 	};
}
