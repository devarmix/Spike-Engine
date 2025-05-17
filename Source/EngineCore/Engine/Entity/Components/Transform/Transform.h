#pragma once

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace SpikeEngine {

	class Transform {
	public:

		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;

		glm::mat4 localTransform;
		glm::mat4 worldTransform;
	};
}
