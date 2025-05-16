#pragma once

#include <Engine/Core/Layer.h>
#include <Engine/Entity/Scene.h>

//#include <Engine/Renderer/VK_Renderer.h>
using namespace SpikeEngine;

namespace Spike {

	class SceneLayer : public Layer {
	public:
		SceneLayer() : Layer("Scene Layer") {}
		~SceneLayer() override;

		virtual void OnUpdate(float deltaTime) override;
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnEvent(Event& event) override;

		Ref<Scene> GetActiveScene() const { return m_ActiveScene; }

	private:
		Ref<Scene> m_ActiveScene;

		//std::vector<Ref<LoadedModel>> m_LoadedModels;
	};
}