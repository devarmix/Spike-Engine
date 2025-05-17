#include <Engine/Entity/Scene.h>
#include <Engine/Entity/GameObject.h>

#include <Engine/Entity/Components/MeshRenderer/MeshRenderer.h>
//#include <Engine/Renderer/VK_Renderer.h>

namespace SpikeEngine {

	Scene::Scene() {}

	Scene::~Scene() {}


	const Ref<GameObject> Scene::CreateGameObject() {
		
		Ref<GameObject> object = CreateRef<GameObject>(m_Registry.create(), this);

		// Add default components;
		object->AddComponent<Transform>();
		object->AddComponent<GameObjectInfo>("New GameObject", "None", "Default");

		ENGINE_INFO("Created object!");

		return object;
	}
	
	void Scene::DestroyGameObject(Ref<GameObject> object) {

		//m_Registry.destroy(object);
	}


	void Scene::OnUpdate() {

		// draw meshes
		{
			auto group = m_Registry.group<Transform>(entt::get<MeshRenderer>);
			for (auto entity : group)
			{
				auto [transform, mesh] = group.get<Transform, MeshRenderer>(entity);

				//Spike::VK_Renderer::SubmitMesh(mesh.mesh, transform.worldTransform);
			}
		}
	}
}