#pragma once

#include <glm/glm.hpp>
#include <Engine/Core/Core.h>

namespace SpikeEngine {

	class PerspectiveCamera {
	public:

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetRotationMatrix() const;
		glm::mat4 GetProjectionMatrix(float aspect) const;

		glm::vec3 GetPosition() const { return m_Position; }

		void SetCameraFOV(float fov) { m_FOV = fov; }

		void SetCameraNearProj(float n) { m_Near = n; }
		void SetCameraFarProj(float f) { m_Far = f; }

		float GetCameraNearProj() const { return m_Near; }
		float GetCameraFarProj() const { return m_Far; }

	protected:

		float m_FOV = 70;
		float m_Near = 10000.f;
		float m_Far = 0.01f;

		glm::vec3 m_Position = { 0, 0, 0 };
		glm::vec3 m_Rotation = { 0, 0, 0 };
	};
}