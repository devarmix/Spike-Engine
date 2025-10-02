#include <Engine/Utils/MathUtils.h>

float Spike::MathUtils::ClampF(float value, float min, float max) {

	if (value < min) value = min;
	else if (value > max) value = max;

	return value;
}

int Spike::MathUtils::ClampI(int value, int min, int max) {

	if (value < min) value = min;
	else if (value > max) value = max;

	return value;
}

uint32_t Spike::MathUtils::ClampU(uint32_t value, uint32_t min, uint32_t max) {

	if (value < min) value = min;
	else if (value > max) value = max;

	return value;
}

float Spike::MathUtils::LerpF(float a, float b, float t) {
	return a + (b - a) * ClampF(t, 0.f, 1.f);
}

float Spike::MathUtils::LerpUnclampedF(float a, float b, float t) {
	return a + (b - a) * t;
}

float Spike::MathUtils::MoveTowardsF(float current, float target, float maxDelta) {

	if (std::abs(target - current) <= maxDelta) {
		return target;
	}

	float sign = (target - current) < 0 ? -1.f : 1.f;

	return current + sign * maxDelta;
}

float Spike::MathUtils::SmoothDampF(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime) {

	smoothTime = std::max(0.0001f, smoothTime);
	float num = 2 / smoothTime;
	float num2 = num * deltaTime;
	float num3 = 1 / (1 + num2 + 0.48f * num2 * num2 + 0.235f * num2 * num2 * num2);
	float value = current - target;
	float num4 = target;
	float num5 = maxSpeed * smoothTime;
	value = ClampF(value, 0 - num5, num5);
	target = current - value;
	float num6 = (currentVelocity + num * value) * deltaTime;
	currentVelocity = (currentVelocity - num * num6) * num3;
	float num7 = target + (value + num6) * num3;
	if (num4 - current > 0 == num7 > num4)
	{
		num7 = num4;
		currentVelocity = (num7 - num4) / deltaTime;
	}

	return num7;
}

Vec2 Spike::MathUtils::UpVe2() { return Vec2(0.f, 1.f); }

Vec2 Spike::MathUtils::DownVec2() { return Vec2(0.f, -1.f); }

Vec2 Spike::MathUtils::LeftVec2() { return Vec2(-1.f, 0.f); }

Vec2 Spike::MathUtils::RightVec2() { return Vec2(1.f, 0.f); }

Vec2 Spike::MathUtils::LerpVec2(const Vec2& a, const Vec2& b, float t) {

	t = ClampF(t, 0.f, 1.f);
	return Vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

Vec2 Spike::MathUtils::LerpUnclampedVec2(const Vec2& a, const Vec2& b, float t) {
	return Vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

Vec2 Spike::MathUtils::MoveTowardsVec2(const Vec2& current, const Vec2& target, float maxDistanceDelta) {

	float num = target.x - current.x;
	float num2 = target.y - current.y;
	float num3 = num * num + num2 * num2;
	if (num3 == 0.f || (maxDistanceDelta >= 0.f && num3 <= maxDistanceDelta * maxDistanceDelta))
	{
		return target;
	}

	float num4 = std::sqrt(num3);
	return Vec2(current.x + num / num4 * maxDistanceDelta, current.y + num2 / num4 * maxDistanceDelta);
}

Vec2 Spike::MathUtils::SmoothDampVec2(const Vec2& current, Vec2 target, Vec2& currentVelocity, float smoothTime, float maxSpeed, float deltaTime) {

	smoothTime = std::max(0.0001f, smoothTime);
	float num = 2.f / smoothTime;
	float num2 = num * deltaTime;
	float num3 = 1.f / (1.f + num2 + 0.48f * num2 * num2 + 0.235f * num2 * num2 * num2);
	float num4 = current.x - target.x;
	float num5 = current.y - target.y;
	Vec2 vector = target;
	float num6 = maxSpeed * smoothTime;
	float num7 = num6 * num6;
	float num8 = num4 * num4 + num5 * num5;
	if (num8 > num7)
	{
		float num9 = std::sqrt(num8);
		num4 = num4 / num9 * num6;
		num5 = num5 / num9 * num6;
	}

	target.x = current.x - num4;
	target.y = current.y - num5;
	float num10 = (currentVelocity.x + num * num4) * deltaTime;
	float num11 = (currentVelocity.y + num * num5) * deltaTime;
	currentVelocity.x = (currentVelocity.x - num * num10) * num3;
	currentVelocity.y = (currentVelocity.y - num * num11) * num3;
	float num12 = target.x + (num4 + num10) * num3;
	float num13 = target.y + (num5 + num11) * num3;
	float num14 = vector.x - current.x;
	float num15 = vector.y - current.y;
	float num16 = num12 - vector.x;
	float num17 = num13 - vector.y;
	if (num14 * num16 + num15 * num17 > 0.f)
	{
		num12 = vector.x;
		num13 = vector.y;
		currentVelocity.x = (num12 - vector.x) / deltaTime;
		currentVelocity.y = (num13 - vector.y) / deltaTime;
	}

	return Vec2(num12, num13);
}

Vec3 Spike::MathUtils::UpVec3() { return Vec3(0.f, 1.f, 0.f); }

Vec3 Spike::MathUtils::DownVec3() { return Vec3(0.f, -1.f, 0.f); }

Vec3 Spike::MathUtils::LeftVec3() { return Vec3(-1.f, 0.f, 0.f); }

Vec3 Spike::MathUtils::RightVec3() { return Vec3(1.f, 0.f, 0.f); }

Vec3 Spike::MathUtils::BackVec3() { return Vec3(0.f, 0.f, -1.f); }

Vec3 Spike::MathUtils::ForwardVec3() { return Vec3(0.f, 0.f, 1.f); }

Vec3 Spike::MathUtils::LerpVec3(const Vec3& a, const Vec3& b, float t) {

	t = ClampF(t, 0.f, 1.f);
	return Vec3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}

Vec3 Spike::MathUtils::LerpUnclampedVec3(const Vec3& a, const Vec3& b, float t) {
	return Vec3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}

Vec3 Spike::MathUtils::MoveTowardsVec3(const Vec3& current, const Vec3& target, float maxDistanceDelta) {

	float num = target.x - current.x;
	float num2 = target.y - current.y;
	float num3 = target.z - current.z;
	float num4 = num * num + num2 * num2 + num3 * num3;
	if (num4 == 0.f || (maxDistanceDelta >= 0.f && num4 <= maxDistanceDelta * maxDistanceDelta))
	{
		return target;
	}

	float num5 = std::sqrt(num4);
	return Vec3(current.x + num / num5 * maxDistanceDelta, current.y + num2 / num5 * maxDistanceDelta, current.z + num3 / num5 * maxDistanceDelta);
}

Vec3 Spike::MathUtils::SmoothDampVec3(const Vec3& current, Vec3 target, Vec3& currentVelocity, float smoothTime, float maxSpeed, float deltaTime) {

	float num = 0.f;
	float num2 = 0.f;
	float num3 = 0.f;
	smoothTime = std::max(0.0001f, smoothTime);
	float num4 = 2.f / smoothTime;
	float num5 = num4 * deltaTime;
	float num6 = 1.f / (1.f + num5 + 0.48f * num5 * num5 + 0.235f * num5 * num5 * num5);
	float num7 = current.x - target.x;
	float num8 = current.y - target.y;
	float num9 = current.z - target.z;
	Vec3 vector = target;
	float num10 = maxSpeed * smoothTime;
	float num11 = num10 * num10;
	float num12 = num7 * num7 + num8 * num8 + num9 * num9;
	if (num12 > num11)
	{
		float num13 = std::sqrt(num12);
		num7 = num7 / num13 * num10;
		num8 = num8 / num13 * num10;
		num9 = num9 / num13 * num10;
	}

	target.x = current.x - num7;
	target.y = current.y - num8;
	target.z = current.z - num9;
	float num14 = (currentVelocity.x + num4 * num7) * deltaTime;
	float num15 = (currentVelocity.y + num4 * num8) * deltaTime;
	float num16 = (currentVelocity.z + num4 * num9) * deltaTime;
	currentVelocity.x = (currentVelocity.x - num4 * num14) * num6;
	currentVelocity.y = (currentVelocity.y - num4 * num15) * num6;
	currentVelocity.z = (currentVelocity.z - num4 * num16) * num6;
	num = target.x + (num7 + num14) * num6;
	num2 = target.y + (num8 + num15) * num6;
	num3 = target.z + (num9 + num16) * num6;
	float num17 = vector.x - current.x;
	float num18 = vector.y - current.y;
	float num19 = vector.z - current.z;
	float num20 = num - vector.x;
	float num21 = num2 - vector.y;
	float num22 = num3 - vector.z;
	if (num17 * num20 + num18 * num21 + num19 * num22 > 0.f)
	{
		num = vector.x;
		num2 = vector.y;
		num3 = vector.z;
		currentVelocity.x = (num - vector.x) / deltaTime;
		currentVelocity.y = (num2 - vector.y) / deltaTime;
		currentVelocity.z = (num3 - vector.z) / deltaTime;
	}

	return Vec3(num, num2, num3);
}

Vec4 Spike::MathUtils::LerpVec4(const Vec4& a, const Vec4& b, float t) {

	t = ClampF(t, 0.f, 1.f);
	return Vec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

Vec4 Spike::MathUtils::LerpUnclampedVec4(const Vec4& a, const Vec4& b, float t) {

	return Vec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
}

Vec4 Spike::MathUtils::MoveTowardsVec4(const Vec4& current, const Vec4& target, float maxDistanceDelta) {

	float num = target.x - current.x;
	float num2 = target.y - current.y;
	float num3 = target.z - current.z;
	float num4 = target.w - current.w;
	float num5 = num * num + num2 * num2 + num3 * num3 + num4 * num4;
	if (num5 == 0.f || (maxDistanceDelta >= 0.f && num5 <= maxDistanceDelta * maxDistanceDelta))
	{
		return target;
	}

	float num6 = std::sqrt(num5);
	return Vec4(current.x + num / num6 * maxDistanceDelta, current.y + num2 / num6 * maxDistanceDelta, current.z + num3 / num6 * maxDistanceDelta, current.w + num4 / num6 * maxDistanceDelta);
}

void Spike::MathUtils::HashCombine(size_t& hash1, size_t hash2) {

	hash1 ^= hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2);
}

uint32_t Spike::MathUtils::PackUnsignedVec4ToUint(const Vec4& v) {

	uint8_t r = uint8_t(v.r * 255.f + 0.5f);
	uint8_t g = uint8_t(v.g * 255.f + 0.5f);
	uint8_t b = uint8_t(v.b * 255.f + 0.5f);
	uint8_t a = uint8_t(v.a * 255.f + 0.5f);

	return (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

Vec4 Spike::MathUtils::UnpackUintToUnsignedVec4(uint32_t packed) {

	float r = float((packed & 0x000000ff) >> 0) / 255.f;
	float g = float((packed & 0x0000ff00) >> 8) / 255.f;
	float b = float((packed & 0x00ff0000) >> 16) / 255.f;
	float a = float((packed & 0xff000000) >> 24) / 255.f;

	return glm::vec4(r, g, b, a);
}

PackedHalf Spike::MathUtils::PackSignedVec4ToHalf(const Vec4& v) {

	uint16_t x = uint16_t((v.x + 1.f) * 32767.5f + 0.5f);
	uint16_t y = uint16_t((v.y + 1.f) * 32767.5f + 0.5f);
	uint16_t z = uint16_t((v.z + 1.f) * 32767.5f + 0.5f);
	uint16_t w = uint16_t((v.w + 1.f) * 32767.5f + 0.5f);

	PackedHalf out{};
	out.A = (x << 0) | (y << 16);
	out.B = (z << 0) | (w << 16);

	return out;
}

Vec4 Spike::MathUtils::UnpackHalfToSignedVec4(const PackedHalf& packed) {

	float x = (float((packed.A & 0x0000ffff) >> 0) / 65535.f) * 2.f - 1.f;
	float y = (float((packed.A & 0xffff0000) >> 16) / 65535.f) * 2.f - 1.f;
	float z = (float((packed.B & 0x0000ffff) >> 0) / 65535.f) * 2.f - 1.f;
	float w = (float((packed.B & 0xffff0000) >> 16) / 65535.f) * 2.f - 1.f;

	return glm::vec4(x, y, z, w);
}

Mat4x4 Spike::MathUtils::GetInfinitePerspectiveMatrix(float fov, float aspect, float nearProj) {

	float f = 1.0f / tanf(fov / 2.0f);
	return Mat4x4(
		f / aspect, 0.0f, 0.0f, 0.0f,
		0.0f, -f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, -1.0f,
		0.0f, 0.0f, nearProj, 0.0f);
}