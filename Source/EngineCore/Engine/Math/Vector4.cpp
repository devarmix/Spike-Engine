#include <Engine/Math/Vector4.h>
#include <Engine/Math/Mathf.h>

namespace SpikeEngine {

	Vector4 Vector4::zeroVector = Vector4(0.f, 0.f, 0.f, 0.f);

	Vector4 Vector4::oneVector = Vector4(1.f, 1.f, 1.f, 1.f);


	Vector4 Vector4::Normalized() const {

		return Normalize(*this);
	}

	float Vector4::Magnitude() const {

		return Mathf::Sqrt(Dot(*this, *this));
	}

	float Vector4::SqrMagnitude() const {

		return Dot(*this, *this);
	}

	const Vector4 Vector4::Zero() {

		return zeroVector;
	}

	const Vector4 Vector4::One() {

		return oneVector;
	}

	void Vector4::Set(float newX, float newY, float newZ, float newW) {

		x = newX;
		y = newY;
		z = newZ;
		w = newW;
	}

	Vector4 Vector4::Lerp(Vector4 a, Vector4 b, float t) {

		t = Mathf::Clamp01(t);
		return Vector4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
	}

	Vector4 Vector4::LerpUnclamped(Vector4 a, Vector4 b, float t) {

		return Vector4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
	}

	Vector4 Vector4::MoveTowards(Vector4 current, Vector4 target, float maxDistanceDelta) {

		float num = target.x - current.x;
		float num2 = target.y - current.y;
		float num3 = target.z - current.z;
		float num4 = target.w - current.w;
		float num5 = num * num + num2 * num2 + num3 * num3 + num4 * num4;
		if (num5 == 0.f || (maxDistanceDelta >= 0.f && num5 <= maxDistanceDelta * maxDistanceDelta))
		{
			return target;
		}

		float num6 = Mathf::Sqrt(num5);
		return Vector4(current.x + num / num6 * maxDistanceDelta, current.y + num2 / num6 * maxDistanceDelta, current.z + num3 / num6 * maxDistanceDelta, current.w + num4 / num6 * maxDistanceDelta);
	}


	Vector4 Vector4::Scale(Vector4 a, Vector4 b) {

		return Vector4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
	}

	void Vector4::Scale(Vector4 scale) {

		x *= scale.x;
		y *= scale.y;
		z *= scale.z;
		w *= scale.w;
	}

	bool Vector4::Equals(Vector4 other) const {

		return x == other.x && y == other.y && z == other.z && w == other.w;
	}

	Vector4 Vector4::Normalize(Vector4 a) {

		float num = Magnitude(a);
		if (num > 1E-05f)
		{
			return a / num;
		}

		return Zero();
	}

	void Vector4::Normalize() {

		float num = Magnitude(*this);
		if (num > 1E-05f)
		{
			*this /= num;
		}
		else
		{
			*this = Zero();
		}
	}

	float Vector4::Dot(Vector4 a, Vector4 b) {

		return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	}

	Vector4 Vector4::Project(Vector4 a, Vector4 b) {

		return b * (Dot(a, b) / Dot(b, b));
	}

	float Vector4::Distance(Vector4 a, Vector4 b) {

		return Magnitude(a - b);
	}

	float Vector4::Magnitude(Vector4 a) {

		return Mathf::Sqrt(Dot(a, a));
	}

	Vector4 Vector4::Min(Vector4 lhs, Vector4 rhs) {

		return Vector4(Mathf::Min(lhs.x, rhs.x), Mathf::Min(lhs.y, rhs.y), Mathf::Min(lhs.z, rhs.z), Mathf::Min(lhs.w, rhs.w));
	}

	Vector4 Vector4::Max(Vector4 lhs, Vector4 rhs) {

		return Vector4(Mathf::Max(lhs.x, rhs.x), Mathf::Max(lhs.y, rhs.y), Mathf::Max(lhs.z, rhs.z), Mathf::Max(lhs.w, rhs.w));
	}

	float Vector4::SqrMagnitude(Vector4 a) {

	    return Dot(a, a);
	}
}