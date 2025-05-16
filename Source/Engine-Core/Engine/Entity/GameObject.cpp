#include <Engine/Entity/GameObject.h>

namespace SpikeEngine {

	GameObject::GameObject(entt::entity handle, Scene* scene)
	    : m_EntityHandle(handle), m_Scene(scene) {

	}

	void GameObject::SetParent(Ref<GameObject> object) {

		GetComponent<GameObjectInfo>().parent = object;
		//object->GetComponent<GameObjectInfo>().children.push_back();
	}


	void GameObject::RefreshTransform(const glm::mat4& parentTransform) {

		GetTransform().worldTransform = parentTransform * GetTransform().localTransform;
		for (auto c : GetChildren()) {

			c->RefreshTransform(GetTransform().worldTransform);
		}
	}
}