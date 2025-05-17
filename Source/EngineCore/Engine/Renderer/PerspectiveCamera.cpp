#include <Engine/Renderer/PerspectiveCamera.h>
#include <Engine/Core/Log.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace SpikeEngine {

	void PerspectiveCamera::SetCameraPosition(const glm::vec3& position) {

		m_Position = position;
	}

	void PerspectiveCamera::SetCameraRotation(const glm::vec3& rotation) {

		m_Rotation = rotation;
	}


	const glm::mat4 PerspectiveCamera::GetProjectionMatrix(float aspect) {

		return glm::perspective(glm::radians(m_FOV), aspect, m_Near, m_Far);
	}

	const glm::mat4 PerspectiveCamera::GetRotationMatrix() {

		glm::quat pitchRotation = glm::angleAxis(m_Rotation.x, glm::vec3{ 1.f, 0.f, 0.f });
		glm::quat yawRotation = glm::angleAxis(m_Rotation.y, glm::vec3{ 0.f, -1.f, 0.f });
		glm::quat rollRotation = glm::angleAxis(m_Rotation.z, glm::vec3{ 0.f, 0.f, 1.f });

		return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation) * glm::toMat4(rollRotation);
	}

	const glm::mat4 PerspectiveCamera::GetViewMatrix() {

		glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), m_Position);
		glm::mat4 cameraRotation = GetRotationMatrix();

		return glm::inverse(cameraTranslation * cameraRotation);
	}
}