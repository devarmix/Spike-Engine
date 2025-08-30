#include <Editor/Renderer/EditorCamera.h>
#include <Engine/Utils/MathUtils.h>
#include <Engine/Core/Log.h>

#include <Engine/Core/Application.h>

namespace SpikeEditor {

	EditorCamera::EditorCamera() {}

	EditorCamera::~EditorCamera() {}

	void EditorCamera::OnUpdate(float deltaTime) {

		// if not controlling camera, reset velocity
		if (!m_IsControlled) m_TargetVelocity = Vec3(0.f);
		//if (m_CurrentVelocity == Vector3() && !m_IsControlled) return;

		m_CurrentVelocity = MathUtils::MoveTowardsVec3(m_CurrentVelocity, m_TargetVelocity, m_AccelerationSpeed * deltaTime);
		Vec3 v = m_CurrentVelocity * deltaTime * 7.f * m_Speed * m_SpeedMultiplier * m_SpeedScrollMultiplier;

		Mat4x4 cameraRotation = GetRotationMatrix();
		m_Position += Vec3(cameraRotation * Vec4(v.x, v.y, v.z, 0.f));
	}

	void EditorCamera::OnEvent(const GenericEvent& event) {
		 
		EventHandler handler(event);
		handler.Handle<MouseButtonPressEvent>(BIND_FUNCTION(EditorCamera::OnMouseButtonPress));
		handler.Handle<MouseButtonReleaseEvent>(BIND_FUNCTION(EditorCamera::OnMouseButtonRelease));
		handler.Handle<MouseScrollEvent>(BIND_FUNCTION(EditorCamera::OnMouseScroll));

		// poll movement events, only when controlling camera
		if (m_IsControlled) {

			handler.Handle<KeyPressEvent>(BIND_FUNCTION(EditorCamera::OnKeyPress));
			handler.Handle<KeyReleaseEvent>(BIND_FUNCTION(EditorCamera::OnKeyRelease));
			handler.Handle<MouseMotionEvent>(BIND_FUNCTION(EditorCamera::OnMouseMotion));
		}
	}

	bool EditorCamera::OnKeyPress(const KeyPressEvent& event) {

		KeyPressEvent e = event;

		EXECUTE_ON_RENDER_THREAD(([=, this]() {

			if (e.GetKey() == SDLK_w) { m_TargetVelocity.z = -1; }
			else if (e.GetKey() == SDLK_s) { m_TargetVelocity.z = 1; }
			else if (e.GetKey() == SDLK_a) { m_TargetVelocity.x = -1; }
			else if (e.GetKey() == SDLK_d) { m_TargetVelocity.x = 1; }
			else if (e.GetKey() == SDLK_q) { m_TargetVelocity.y = -1; }
			else if (e.GetKey() == SDLK_e) { m_TargetVelocity.y = 1; }
			}));

		return false;
	}

	bool EditorCamera::OnKeyRelease(const KeyReleaseEvent& event) {

		KeyReleaseEvent e = event;

		EXECUTE_ON_RENDER_THREAD(([=, this]() {

			if (e.GetKey() == SDLK_w && m_TargetVelocity.z < 0) { m_TargetVelocity.z = 0; }
			else if (e.GetKey() == SDLK_s && m_TargetVelocity.z > 0) { m_TargetVelocity.z = 0; }
			else if (e.GetKey() == SDLK_a && m_TargetVelocity.x < 0) { m_TargetVelocity.x = 0; }
			else if (e.GetKey() == SDLK_d && m_TargetVelocity.x > 0) { m_TargetVelocity.x = 0; }
			else if (e.GetKey() == SDLK_q && m_TargetVelocity.y < 0) { m_TargetVelocity.y = 0; }
			else if (e.GetKey() == SDLK_e && m_TargetVelocity.y > 0) { m_TargetVelocity.y = 0; }
			}));

		return false;
	}

	bool EditorCamera::OnMouseMotion(const MouseMotionEvent& event) {

		MouseMotionEvent e = event;

		EXECUTE_ON_RENDER_THREAD(([=, this]() {

			m_Yaw += e.GetDeltaX() / 200.f;
			m_Pitch -= e.GetDeltaY() / 200.f;

			//m_Yaw = Mathf::Clamp(m_Yaw, 0.f, 90.f);

			//SE_CORE_WARN("Yaw: {0}", m_Yaw);
			//SE_CORE_WARN("Pitch: {0}", m_Pitch);

			//SetCameraRotation(glm::vec3{ m_Pitch, m_Yaw, 0.f });

			m_Rotation = glm::vec3(m_Pitch, m_Yaw, 0.f);
			}));

		return false;
	}

	bool EditorCamera::OnMouseButtonPress(const MouseButtonPressEvent& event) { 

		MouseButtonPressEvent e = event;

		EXECUTE_ON_RENDER_THREAD(([=, this]() {

			if (e.GetButton() == SDL_BUTTON_RIGHT && m_ViewportHovered) {

				// hide cursor to control camera
				SDL_SetRelativeMouseMode(SDL_TRUE);
				SDL_ShowCursor(SDL_DISABLE);

				m_IsControlled = true;
			}
			}));

		return false;
	}

	bool EditorCamera::OnMouseButtonRelease(const MouseButtonReleaseEvent& event) {

		MouseButtonReleaseEvent e = event;

		EXECUTE_ON_RENDER_THREAD(([=, this]() {

			if (e.GetButton() == SDL_BUTTON_RIGHT && m_IsControlled) {

				// show cursor back
				SDL_SetRelativeMouseMode(SDL_FALSE);
				SDL_ShowCursor(SDL_ENABLE);

				m_IsControlled = false;
			}
			}));

		return false;
	}

	bool EditorCamera::OnMouseScroll(const MouseScrollEvent& event) {

		MouseScrollEvent e = event;

		EXECUTE_ON_RENDER_THREAD(([=, this]() {

			if (m_IsControlled) {

				// change scroll speed multiplier
				m_SpeedScrollMultiplier += e.GetScrollY() * m_ScrollSpeedDeltaFactor;
				m_SpeedScrollMultiplier = MathUtils::ClampF(m_SpeedScrollMultiplier, 0.01f, m_MaxSpeedScrollMultiplier);

			}
			else if (m_ViewportHovered) {

				// scroll camera
				float distance = e.GetScrollY() * m_ScrollSpeed;
				glm::mat4 cameraRotation = GetRotationMatrix();

				m_Position += glm::vec3(cameraRotation * glm::vec4(0.f, 0.f, -distance, 0.f));
			}
			}));

		return false;
	}

	void EditorCamera::SetSpeed(float value) {

		m_Speed = MathUtils::ClampF(value, 1.f, m_MaxSpeed);
	}

	void EditorCamera::SetSpeedMultiplier(float value) {

		m_SpeedMultiplier = MathUtils::ClampF(value, 1.f, m_MaxSpeedMultiplier);
	}
}