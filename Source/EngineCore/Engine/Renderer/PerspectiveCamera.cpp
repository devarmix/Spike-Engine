#include <Engine/Renderer/PerspectiveCamera.h>

namespace Spike {

	Mat4x4 PerspectiveCamera::GetProjectionMatrix(float aspect) const {
		return MathUtils::GetInfinitePerspectiveMatrix(glm::radians(m_FOV), aspect, m_Far);
	}

	Mat4x4 PerspectiveCamera::GetRotationMatrix() const {

		Quaternion pitchRotation = glm::angleAxis(m_Rotation.x, Vec3{ 1.f, 0.f, 0.f });
		Quaternion yawRotation = glm::angleAxis(m_Rotation.y, Vec3{ 0.f, -1.f, 0.f });
		Quaternion rollRotation = glm::angleAxis(m_Rotation.z, Vec3{ 0.f, 0.f, 1.f });

		return glm::toMat4(yawRotation * pitchRotation * rollRotation);
	}

	Mat4x4 PerspectiveCamera::GetViewMatrix() const {

		Mat4x4 cameraTranslation = glm::translate(Mat4x4(1.f), m_Position);
		Mat4x4 cameraRotation = GetRotationMatrix();

		return glm::inverse(cameraTranslation * cameraRotation);
	}
}