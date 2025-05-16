#pragma once

#include <Engine/Entity/Object.h>
#include <Engine/Entity/Scene.h>

#include <Engine/Core/Log.h>

namespace SpikeEngine {

	class GameObject : public Object {
	public:
		GameObject() = default;
		GameObject(entt::entity handle, Scene* scene);
		GameObject(const GameObject& other) = default;

		Transform& GetTransform() { return GetComponent<Transform>(); }

		std::string& GetName() { return GetComponent<GameObjectInfo>().name; }
		std::string& GetTag() { return GetComponent<GameObjectInfo>().tag; }
		std::string& GetLayer() { return GetComponent<GameObjectInfo>().layer; }

		std::weak_ptr<GameObject> GetParent() { return GetComponent<GameObjectInfo>().parent; }
		std::vector<Ref<GameObject>> GetChildren() { return GetComponent<GameObjectInfo>().children; }

		void SetParent(Ref<GameObject> object);

		void RefreshTransform(const glm::mat4& parentTransform);


		template<typename T>
		bool HasComponent() {

			return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
		}

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args) {

			if (HasComponent<T>()) ENGINE_ERROR("Gameobject already has the component!");
			T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
			return component;
		}

		template<typename T>
		T& GetComponent() {

			if (!HasComponent<T>()) ENGINE_ERROR("Gameobject does not have the component to get!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		void DestroyComponent() {

			if (!HasComponent<T>()) ENGINE_ERROR("Gameobject does not have the component to destroy!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		template<>
		void DestroyComponent<Transform>() {

			ENGINE_WARN("Cannot destroy default component - Transform!");
		}

		template<>
		void DestroyComponent<GameObjectInfo>() {

			ENGINE_WARN("Cannot destroy default compoment - GameObjectInfo!");
		}


		bool operator==(const GameObject& other) const {
			
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		}

		bool operator!=(const GameObject& other) const {

			return !(*this == other);
		}

	private:
		entt::entity m_EntityHandle;
		Scene* m_Scene;
	};
}
