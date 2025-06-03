#include <Engine/Layers/SceneLayer.h>

#include <Engine/Core/Stats.h>

#include <Engine/Core/Profiler.h>

namespace Spike {

	SceneLayer::~SceneLayer() {

		//m_LoadedModels.clear();
	}

	void SceneLayer::OnUpdate(float deltaTime) {

		EXECUTION_TIME_PROFILER_START

		m_ActiveScene->OnUpdate();

		EXECUTION_TIME_PROFILER_END

		Stats::Data.SceneUpdateTime = GET_EXECUTION_TIME_MS;
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

	void SceneLayer::OnEvent(const GenericEvent& event) {}
}