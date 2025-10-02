#pragma once

#include <Engine/Core/Core.h>

using Quaternion = glm::quat;
using Vec2 = glm::vec2;
using Vec2Int = glm::ivec2;
using Vec2Uint = glm::uvec2;
using Vec3 = glm::vec3;
using Vec3Int = glm::ivec3;
using Vec3Uint = glm::uvec3;
using Vec4 = glm::vec4;
using Vec4Int = glm::ivec4;
using Mat2x2 = glm::mat2;
using Mat3x3 = glm::mat3;
using Mat4x4 = glm::mat4;

struct PackedHalf {

	uint32_t A;
	uint32_t B;
};

namespace Spike {

	namespace MathUtils {

		constexpr float PI = 3.14159274f;
		constexpr float E = 2.71828175f;
		constexpr float DEG_TO_RAD = PI / 180;
		constexpr float RAD_TO_DEG = 57.29578f;

		float ClampF(float value, float min, float max);
		int ClampI(int value, int min, int max);
		uint32_t ClampU(uint32_t value, uint32_t min, uint32_t max);

		float LerpF(float a, float b, float t);
		float LerpUnclampedF(float a, float b, float t);
		float MoveTowardsF(float current, float target, float maxDelta);
		float SmoothDampF(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime);

		Vec2 UpVe2();
		Vec2 DownVec2();
		Vec2 LeftVec2();
		Vec2 RightVec2();

		Vec2 LerpVec2(const Vec2& a, const Vec2& b, float t);
		Vec2 LerpUnclampedVec2(const Vec2& a, const Vec2& b, float t);
		Vec2 MoveTowardsVec2(const Vec2& current, const Vec2& target, float maxDistanceDelta);
		Vec2 SmoothDampVec2(const Vec2& current, Vec2 target, Vec2& currentVelocity, float smoothTime, float maxSpeed, float deltaTime);

		Vec3 UpVec3();
		Vec3 DownVec3();
		Vec3 LeftVec3();
		Vec3 RightVec3();
		Vec3 BackVec3();
		Vec3 ForwardVec3();

		Vec3 LerpVec3(const Vec3& a, const Vec3& b, float t);
		Vec3 LerpUnclampedVec3(const Vec3& a, const Vec3& b, float t);
		Vec3 MoveTowardsVec3(const Vec3& current, const Vec3& target, float maxDistanceDelta);
		Vec3 SmoothDampVec3(const Vec3& current, Vec3 target, Vec3& currentVelocity, float smoothTime, float maxSpeed, float deltaTime);

		Vec4 LerpVec4(const Vec4& a, const Vec4& b, float t);
		Vec4 LerpUnclampedVec4(const Vec4& a, const Vec4& b, float t);
		Vec4 MoveTowardsVec4(const Vec4& current, const Vec4& target, float maxDistanceDelta);

		void HashCombine(size_t& hash1, size_t hash2);

		uint32_t PackUnsignedVec4ToUint(const Vec4& v);
		Vec4 UnpackUintToUnsignedVec4(uint32_t packed);
		PackedHalf PackSignedVec4ToHalf(const Vec4& v);
		Vec4 UnpackHalfToSignedVec4(const PackedHalf& packed);
		Mat4x4 GetInfinitePerspectiveMatrix(float fov, float aspect, float nearProj);
	}
}
