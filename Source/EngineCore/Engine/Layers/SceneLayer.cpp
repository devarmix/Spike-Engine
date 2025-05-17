#include <Engine/Layers/SceneLayer.h>

#include <Engine/Core/Stats.h>

#include <chrono>

namespace Spike {

	SceneLayer::~SceneLayer() {

		//m_LoadedModels.clear();
	}

	void SceneLayer::OnUpdate(float deltaTime) {

		// reset counters
		auto start = std::chrono::system_clock::now();

		m_ActiveScene->OnUpdate();

		auto end = std::chrono::system_clock::now();

		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
		Stats::stats.scene_update_time = elapsed.count() / 1000.f;
	}

    void SceneLayer::OnAttach() {

		m_ActiveScene = CreateRef<Scene>();

		// load scene
		//std::string structurePath = { "res\\models\\structure.glb" };
		//auto structureFile = LoadGLTFGameObject(structurePath, m_ActiveScene);

		//assert(structureFile.has_value());

		//m_LoadedModels.push_back(*structureFile);
	}

	void SceneLayer::OnDetach() {}

	void SceneLayer::OnEvent(Event& event) {}
}