#pragma once

#include <entt.hpp>

#include <Engine/Entity/Components/Transform/Transform.h>
#include <Engine/Entity/Components/GameObjectInfo/GameObjectInfo.h>

#include <Engine/Core/Core.h>


namespace SpikeEngine {

	class GameObject;

	class Scene {
	public:
		Scene();
		~Scene();

		const Ref<GameObject> CreateGameObject();
		void DestroyGameObject(Ref<GameObject> object);

		void OnUpdate();

		entt::registry m_Registry;

	private:

		friend class GameObject;
	};
}
