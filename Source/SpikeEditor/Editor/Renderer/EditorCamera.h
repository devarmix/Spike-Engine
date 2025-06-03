#pragma once

#include <Engine/SpikeEngine.h>
#include <Engine/Events/Event.h>
#include <Engine/Renderer/PerspectiveCamera.h>
using namespace SpikeEngine;

#include <Engine/Events/ApplicationEvents.h>

namespace SpikeEditor {

	class EditorCamera : public PerspectiveCamera {
	public:
		EditorCamera();
		~EditorCamera();

		void OnUpdate(float deltaTime);
		void PollEvents(const Spike::GenericEvent& event);

		void SetViewportHovered(bool value = true) { m_ViewportHovered = value; }

		void SetSpeed(float value);
		void SetSpeedMultiplier(float value);

		float GetSpeed() const  { return m_Speed;}
		float GetSpeedMultiplier() const { return m_SpeedMultiplier; }

	private:

		bool OnKeyPress(const Spike::KeyPressEvent& event);
		bool OnKeyRelease(const Spike::KeyReleaseEvent& event);

		bool OnMouseMotion(const Spike::MouseMotionEvent& event);
		bool OnMouseButtonPress(const Spike::MouseButtonPressEvent& event);
		bool OnMouseButtonRelease(const Spike::MouseButtonReleaseEvent& event);
		bool OnMouseScroll(const Spike::MouseScrollEvent& event);

	private:
		
		glm::vec3 m_Position{ 0, 0, 0 };

		Vector3 m_TargetVelocity;
		Vector3 m_CurrentVelocity;

		float m_AccelerationSpeed = 5.f;
		float m_ScrollSpeed = 0.4f;
		float m_ScrollSpeedDeltaFactor = 0.05f;

		float m_Speed = 1.f;
		float m_SpeedMultiplier = 1.f;
		float m_SpeedScrollMultiplier = 1.f;

		float m_MaxSpeed = 10.f;
		float m_MaxSpeedMultiplier = 100.f;
		float m_MaxSpeedScrollMultiplier = 8.f;
		
		float m_Pitch = 0;
		float m_Yaw = 0;

		bool m_MouseRightButton = false;
		bool m_ViewportHovered = false;
	};
}
