#pragma once

#include <Engine/Core/Core.h>
#include <Engine/Events/Event.h>

namespace Spike {

	class Layer {
	public:
		Layer(const std::string& name = "New Layer");
		virtual ~Layer() = default;

		virtual void OnUpdate(float deltaTime) {}
		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnEvent(Event& event) {}

		std::string GetName() const { return m_Name; }

	private:
		std::string m_Name;
	};
}

