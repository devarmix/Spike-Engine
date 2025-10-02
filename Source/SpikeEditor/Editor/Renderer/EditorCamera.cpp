#include <Editor/Renderer/EditorCamera.h>
#include <Engine/Utils/MathUtils.h>
#include <Engine/Core/Log.h>

#include <Engine/Core/Application.h>

namespace Spike {

	void EditorCamera::Tick(float deltaTime) {

		{
			if (m_ViewportHovered) {
				SDL_SetRelativeMouseMode(SDL_TRUE);
				SDL_ShowCursor(SDL_DISABLE);

				m_IsControlled = true;
			}

			if (GInput->GetMouseButtonReleased(2) && m_IsControlled) {
				SDL_SetRelativeMouseMode(SDL_FALSE);
				SDL_ShowCursor(SDL_ENABLE);

				m_IsControlled = false;
			}
		}
		{
			if (m_IsControlled) {
				m_SpeedScrollMultiplier += GInput->GetMouseScroll() * m_ScrollSpeedDeltaFactor;
				m_SpeedScrollMultiplier = MathUtils::ClampF(m_SpeedScrollMultiplier, 0.01f, m_MaxSpeedScrollMultiplier);
			}
			else if (m_ViewportHovered) {
				float distance = GInput->GetMouseScroll() * m_ScrollSpeed;

				if (distance > 0) {
					Mat4x4 cameraRotation = GetRotationMatrix();
					m_Position += Vec3(cameraRotation * Vec4(0.f, 0.f, -distance, 0.f));
				}
			}
		}

		if (m_IsControlled) {

			if (GInput->GetKeyDown(EKey::W)) {
				m_TargetVelocity.z = -1.f;
			}
			else if (GInput->GetKeyDown(EKey::S)) {
				m_TargetVelocity.z = 1.0f;
			}
			else {
				m_TargetVelocity.z = 0;
			}

			if (GInput->GetKeyDown(EKey::A)) {
				m_TargetVelocity.x = -1.f;
			}
			else if (GInput->GetKeyDown(EKey::D)) {
				m_TargetVelocity.x = 1.0f;
			}
			else {
				m_TargetVelocity.x = 0;
			}

			if (GInput->GetKeyDown(EKey::Q)) {
				m_TargetVelocity.y = -1.f;
			}
			else if (GInput->GetKeyDown(EKey::E)) {
				m_TargetVelocity.y = 1.0f;
			}
			else {
				m_TargetVelocity.y = 0;
			}

			m_Yaw += GInput->GetMouseDeltaX() / 200.f;
			m_Pitch += GInput->GetMouseDeltaY() / 200.f;
			m_Rotation = Vec3(m_Pitch, m_Yaw, 0.f);
		}

		// if not controlling camera, reset velocity
		if (!m_IsControlled) m_TargetVelocity = Vec3(0.f);
		//if (m_CurrentVelocity == Vector3() && !m_IsControlled) return;

		m_CurrentVelocity = MathUtils::MoveTowardsVec3(m_CurrentVelocity, m_TargetVelocity, m_AccelerationSpeed * deltaTime);
		Vec3 v = m_CurrentVelocity * deltaTime * 7.f * m_Speed * m_SpeedMultiplier * m_SpeedScrollMultiplier;

		Mat4x4 cameraRotation = GetRotationMatrix();
		m_Position += Vec3(cameraRotation * Vec4(v.x, v.y, v.z, 0.f));
	}

	void EditorCamera::SetSpeed(float value) {
		m_Speed = MathUtils::ClampF(value, 1.f, m_MaxSpeed);
	}

	void EditorCamera::SetSpeedMultiplier(float value) {
		m_SpeedMultiplier = MathUtils::ClampF(value, 1.f, m_MaxSpeedMultiplier);
	}
}