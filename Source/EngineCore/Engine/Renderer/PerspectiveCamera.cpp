#include <Engine/Renderer/PerspectiveCamera.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Spike {

	Mat4x4 GetInfinitePerspectiveMatrix( float fov, float aspect, float nearProj) {

		float f = 1.0f / tanf(fov / 2.0f);
		return Mat4x4(
			f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, -f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, -1.0f,
			0.0f, 0.0f, nearProj, 0.0f);
	}


	Mat4x4 PerspectiveCamera::GetProjectionMatrix(float aspect) const {

		//return glm::perspective(glm::radians(m_FOV), aspect, m_Near, m_Far);

		return GetInfinitePerspectiveMatrix(glm::radians(m_FOV), aspect, m_Far);
	}

	Mat4x4 PerspectiveCamera::GetRotationMatrix() const {

		Quaternion pitchRotation = glm::angleAxis(m_Rotation.x, Vec3{ 1.f, 0.f, 0.f });
		Quaternion yawRotation = glm::angleAxis(m_Rotation.y, Vec3{ 0.f, -1.f, 0.f });
		Quaternion rollRotation = glm::angleAxis(m_Rotation.z, Vec3{ 0.f, 0.f, 1.f });

		return glm::toMat4(yawRotation * pitchRotation * rollRotation);

		//glm::vec3 forward{};
		//forward.x = glm::cos(glm::radians(m_Rotation.y)) * glm::cos(glm::radians(m_Rotation.x));
		//forward.y = glm::sin(glm::radians(m_Rotation.x));
		//forward.z = glm::sin(glm::radians(m_Rotation.y)) * glm::cos(glm::radians(m_Rotation.x));

		//return glm::lookAt(m_Position, m_Position + forward, glm::vec3(0.f, 1.f, 0.f));
	}

	glm::mat4 PerspectiveCamera::GetViewMatrix() const {

		Mat4x4 cameraTranslation = glm::translate(Mat4x4(1.f), m_Position);
		Mat4x4 cameraRotation = GetRotationMatrix();

		return glm::inverse(cameraTranslation * cameraRotation);

		//glm::vec3 forward{};
		//forward.x = glm::cos(glm::radians(m_Rotation.y)) * glm::cos(glm::radians(m_Rotation.x));
		//forward.y = glm::sin(glm::radians(m_Rotation.x));
		//forward.z = glm::sin(glm::radians(m_Rotation.y)) * glm::cos(glm::radians(m_Rotation.x));

		//return glm::lookAt(m_Position, m_Position + forward, glm::vec3(0.f, 1.f, 0.f));
	}
}