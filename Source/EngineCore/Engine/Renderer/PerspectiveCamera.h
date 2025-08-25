#pragma once

#include <Engine/Core/Core.h>

namespace Spike {

	class PerspectiveCamera {
	public:

		Mat4x4 GetViewMatrix() const;
		Mat4x4 GetRotationMatrix() const;
		Mat4x4 GetProjectionMatrix(float aspect) const;

		Vec3 GetPosition() const { return m_Position; }

		void SetCameraFOV(float fov) { m_FOV = fov; }

		void SetCameraNearProj(float n) { m_Near = n; }
		void SetCameraFarProj(float f) { m_Far = f; }

		float GetCameraNearProj() const { return m_Near; }
		float GetCameraFarProj() const { return m_Far; }

	protected:

		float m_FOV = 70;
		float m_Near = 10000.f;
		float m_Far = 0.01f;

		Vec3 m_Position = { 0, 0, 0 };
		Vec3 m_Rotation = { 0, 0, 0 };
	};
}