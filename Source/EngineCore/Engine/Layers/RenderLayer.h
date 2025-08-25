#pragma once

#include <Engine/Core/Layer.h>

namespace Spike {

	class RenderLayer : public Layer {
	public:
		RenderLayer() : Layer("Render Layer") {}
		virtual ~RenderLayer() override {}

		virtual void OnUpdate(float deltaTime) override;
		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void OnEvent(const GenericEvent& event) override;
	};
}