#pragma once

#include <Engine/Core/Layer.h>
#include <Engine/World/World.h>

namespace Spike {

	class WorldLayer : public Layer {
	public:
		WorldLayer() : Layer("World Layer") {}
		virtual ~WorldLayer() override {}

		virtual void Tick(float deltaTime) override;
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		void LoadWorld(const BinaryReadStream& stream) { m_Current = World::Create(stream); }
		virtual void OnEvent(const GenericEvent& event) override;

	private:
		Ref<World> m_Current;
	};
}