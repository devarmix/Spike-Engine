#include <Engine/Math/Vector2.h>
#include <Engine/Math/Mathf.h>

namespace SpikeEngine {

	Vector2 Vector2::zeroVector = Vector2(0.f, 0.f);

	Vector2 Vector2::oneVector = Vector2(1.f, 1.f);

	Vector2 Vector2::upVector = Vector2(0.f, 1.f);

	Vector2 Vector2::downVector = Vector2(0.f, -1.f);

	Vector2 Vector2::leftVector = Vector2(-1.f, 0.f);

	Vector2 Vector2::rightVector = Vector2(1.f, 0.f);
	

	Vector2 Vector2::Normalized() const {

		Vector2 result = Vector2(x, y);
		result.Normalize();

		return result;
	}

	float Vector2::Magnitude() const {

		return Mathf::Sqrt(x * x + y * y);
	}

	float Vector2::SqrMagnitude() const {

		return x * x + y * y;
	}


	Vector2 Vector2::Zero() {

		return zeroVector;
	}

	Vector2 Vector2::One() {

		return oneVector;
	}

	Vector2 Vector2::Up() {

		return upVector;
	}

	Vector2 Vector2::Down() {

		return downVector;
	}

	Vector2 Vector2::Right() {

		return rightVector;
	}

	Vector2 Vector2::Left() {

		return leftVector;
	}

	
	void Vector2::Set(float newX, float newY) {

		x = newX;
		y = newY;
	}

	Vector2 Vector2::Lerp(Vector2 a, Vector2 b, float t) {

		t = Mathf::Clamp01(t);
		return Vector2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
	}

	Vector2 Vector2::LerpUnclamped(Vector2 a, Vector2 b, float t) {

		return Vector2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
	}

	Vector2 Vector2::MoveTowards(Vector2 current, Vector2 target, float maxDistanceDelta) {

		float num = target.x - current.x;
		float num2 = target.y - current.y;
		float num3 = num * num + num2 * num2;
		if (num3 == 0.f || (maxDistanceDelta >= 0.f && num3 <= maxDistanceDelta * maxDistanceDelta))
		{
			return target;
		}

		float num4 = Mathf::Sqrt(num3);
		return Vector2(current.x + num / num4 * maxDistanceDelta, current.y + num2 / num4 * maxDistanceDelta);
	}

	Vector2 Vector2::Scale(Vector2 a, Vector2 b) {

		return Vector2(a.x * b.x, a.y * b.y);
	}

	void Vector2::Scale(Vector2 scale) {

		x *= scale.x;
		y *= scale.y;
	}

	void Vector2::Normalize() {

		float num = Magnitude();
		if (num > 1E-05f)
		{
			*this /= num;

		} else
		{
			*this = Zero();
		}
	}

	bool Vector2::Equals(Vector2 other) const {

		return x == other.x && y == other.y;
	}

	Vector2 Vector2::Reflect(Vector2 inDirection, Vector2 inNormal) {

		float num = -2.f * Dot(inNormal, inDirection);
		return Vector2(num * inNormal.x + inDirection.x, num * inNormal.y + inDirection.y);
	}

	Vector2 Vector2::Perpendicular(Vector2 inDirection) {

		return Vector2(0.f - inDirection.y, inDirection.x);
	}

	float Vector2::Dot(Vector2 lhs, Vector2 rhs) {

		return lhs.x * rhs.x + lhs.y * rhs.y;
	}

	float Vector2::Angle(Vector2 from, Vector2 to) {

		float num = Mathf::Sqrt(from.SqrMagnitude() * to.SqrMagnitude());
		if (num < 1E-15f)
		{
			return 0.f;
		}

		float num2 = Mathf::Clamp(Dot(from, to) / num, -1.f, 1.f);
		return Mathf::Acos(num2) * 57.29578f;
	}

	float Vector2::SignedAngle(Vector2 from, Vector2 to) {

		float num = Angle(from, to);
		float num2 = Mathf::Sign(from.x * to.y - from.y * to.x);
		return num * num2;
	}

	float Vector2::Distance(Vector2 a, Vector2 b) {

		float num = a.x - b.x;
		float num2 = a.y - b.y;
		return Mathf::Sqrt(num * num + num2 * num2);
	}

	Vector2 Vector2::ClampMagnitude(Vector2 vector, float maxLength) {

		float num = vector.SqrMagnitude();
		if (num > maxLength * maxLength)
		{
			float num2 = Mathf::Sqrt(num);
			float num3 = vector.x / num2;
			float num4 = vector.y / num2;
			return Vector2(num3 * maxLength, num4 * maxLength);
		}

		return vector;
	}

	float Vector2::SqrMagnitude(Vector2 a) {

		return a.x * a.x + a.y * a.y;
	}

	Vector2 Vector2::Min(Vector2 lhs, Vector2 rhs) {

		return Vector2(Mathf::Min(lhs.x, rhs.x), Mathf::Min(lhs.y, rhs.y));
	}

	Vector2 Vector2::Max(Vector2 lhs, Vector2 rhs) {

		return Vector2(Mathf::Max(lhs.x, rhs.x), Mathf::Max(lhs.y, rhs.y));
	}

	Vector2 Vector2::SmoothDamp(Vector2 current, Vector2 target, Vector2& currentVelocity, float smoothTime, float maxSpeed, float deltaTime) {

		smoothTime = Mathf::Max(0.0001f, smoothTime);
		float num = 2.f / smoothTime;
		float num2 = num * deltaTime;
		float num3 = 1.f / (1.f + num2 + 0.48f * num2 * num2 + 0.235f * num2 * num2 * num2);
		float num4 = current.x - target.x;
		float num5 = current.y - target.y;
		Vector2 vector = target;
		float num6 = maxSpeed * smoothTime;
		float num7 = num6 * num6;
		float num8 = num4 * num4 + num5 * num5;
		if (num8 > num7)
		{
			float num9 = Mathf::Sqrt(num8);
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

		return Vector2(num12, num13);
	}
}