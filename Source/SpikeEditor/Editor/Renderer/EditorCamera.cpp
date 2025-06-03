#include <Editor/Renderer/EditorCamera.h>
#include <Engine/Core/Log.h>

namespace SpikeEditor {

	EditorCamera::EditorCamera() {}

	EditorCamera::~EditorCamera() {}

	void EditorCamera::OnUpdate(float deltaTime) {

		// if not holding right mouse button, reset velocity
		if (!m_MouseRightButton) m_TargetVelocity = Vector3(0.f, 0.f, 0.f);

		m_CurrentVelocity = Vector3::MoveTowards(m_CurrentVelocity, m_TargetVelocity, m_AccelerationSpeed * deltaTime);

		glm::mat4 cameraRotation = GetRotationMatrix();

		Vector3 v = m_CurrentVelocity * 0.05f * m_Speed * m_SpeedMultiplier * m_SpeedScrollMultiplier;
		m_Position += glm::vec3(cameraRotation * glm::vec4(v.x, v.y, v.z, 0.f));

		SetCameraPosition(m_Position);
	}

	void EditorCamera::PollEvents(const Spike::GenericEvent& event) {

		Spike::EventHandler handler(event);
		handler.Handle<Spike::MouseButtonPressEvent>(BIND_FUNCTION(EditorCamera::OnMouseButtonPress));
		handler.Handle<Spike::MouseButtonReleaseEvent>(BIND_FUNCTION(EditorCamera::OnMouseButtonRelease));
		handler.Handle<Spike::MouseScrollEvent>(BIND_FUNCTION(EditorCamera::OnMouseScroll));

		// poll movement events, only when right mouse button is pressed
		if (m_MouseRightButton) {

			handler.Handle<Spike::KeyPressEvent>(BIND_FUNCTION(EditorCamera::OnKeyPress));
			handler.Handle<Spike::KeyReleaseEvent>(BIND_FUNCTION(EditorCamera::OnKeyRelease));
			handler.Handle<Spike::MouseMotionEvent>(BIND_FUNCTION(EditorCamera::OnMouseMotion));
		}
	}

	bool EditorCamera::OnKeyPress(const Spike::KeyPressEvent& event) {

		if (event.GetKey() == SDLK_w) { m_TargetVelocity.z = -1; }
		else if (event.GetKey() == SDLK_s) { m_TargetVelocity.z = 1; }
		else if (event.GetKey() == SDLK_a) { m_TargetVelocity.x = -1; }
		else if (event.GetKey() == SDLK_d) { m_TargetVelocity.x = 1; }
		else if (event.GetKey() == SDLK_q) { m_TargetVelocity.y = -1; }
		else if (event.GetKey() == SDLK_e) { m_TargetVelocity.y = 1; }

		return false;
	}

	bool EditorCamera::OnKeyRelease(const Spike::KeyReleaseEvent& event) {

		if (event.GetKey() == SDLK_w && m_TargetVelocity.z < 0) { m_TargetVelocity.z = 0; }
		else if (event.GetKey() == SDLK_s && m_TargetVelocity.z > 0) { m_TargetVelocity.z = 0; }
		else if (event.GetKey() == SDLK_a && m_TargetVelocity.x < 0) { m_TargetVelocity.x = 0; }
		else if (event.GetKey() == SDLK_d && m_TargetVelocity.x > 0) { m_TargetVelocity.x = 0; }
		else if (event.GetKey() == SDLK_q && m_TargetVelocity.y < 0) { m_TargetVelocity.y = 0; }
		else if (event.GetKey() == SDLK_e && m_TargetVelocity.y > 0) { m_TargetVelocity.y = 0; }

		return false;
	}

	bool EditorCamera::OnMouseMotion(const Spike::MouseMotionEvent& event) {

		m_Yaw += event.GetDeltaX() / 200.f;
		m_Pitch -= event.GetDeltaY() / 200.f;

		//m_Yaw = Mathf::Clamp(m_Yaw, 0.f, 90.f);

		//SE_CORE_WARN("Yaw: {0}", m_Yaw);
		//SE_CORE_WARN("Pitch: {0}", m_Pitch);

		SetCameraRotation(glm::vec3{ m_Pitch, m_Yaw, 0.f });

		return false;
	}

	bool EditorCamera::OnMouseButtonPress(const Spike::MouseButtonPressEvent& event) {

		if (event.GetButton() == SDL_BUTTON_RIGHT && m_ViewportHovered) {

			// hide cursor to control camera
			SDL_SetRelativeMouseMode(SDL_TRUE);
			SDL_ShowCursor(SDL_DISABLE);

			m_MouseRightButton = true;
		}

		return false;
	}

	bool EditorCamera::OnMouseButtonRelease(const Spike::MouseButtonReleaseEvent& event) {

		if (event.GetButton() == SDL_BUTTON_RIGHT) {

			// show cursor back
			SDL_SetRelativeMouseMode(SDL_FALSE);
			SDL_ShowCursor(SDL_ENABLE);

			m_MouseRightButton = false;
		}

		return false;
	}

	bool EditorCamera::OnMouseScroll(const Spike::MouseScrollEvent& event) {

		if (m_MouseRightButton) {

			// change scroll speed multiplier
			m_SpeedScrollMultiplier += event.GetScrollY() * m_ScrollSpeedDeltaFactor;
			m_SpeedScrollMultiplier = Mathf::Clamp(m_SpeedScrollMultiplier, 0.01f, m_MaxSpeedScrollMultiplier);

		} else if (m_ViewportHovered) {

			// scroll camera
			float distance = event.GetScrollY() * m_ScrollSpeed;
			glm::mat4 cameraRotation = GetRotationMatrix();

			m_Position += glm::vec3(cameraRotation * glm::vec4(0.f, 0.f, -distance, 0.f));

			SetCameraPosition(m_Position);
		}

		return false;
	}

	void EditorCamera::SetSpeed(float value) {

		m_Speed = Mathf::Clamp(value, 1.f, m_MaxSpeed);
	}

	void EditorCamera::SetSpeedMultiplier(float value) {

		m_SpeedMultiplier = Mathf::Clamp(value, 1.f, m_MaxSpeedMultiplier);
	}
}