#pragma once

namespace SpikeEngine {

	struct Vector3 {

		Vector3() : x(0.f), y(0.f), z(0.f) {}
		Vector3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
		Vector3(float X, float Y) : x(X), y(Y), z(0.f) {}

		float x;
		float y;
		float z;

		Vector3 Normalized() const;
		float Magnitude() const;
		float SqrMagnitude() const;

		static const Vector3 Zero();
		static const Vector3 One();
		static const Vector3 Forward();
		static const Vector3 Back();
		static const Vector3 Up();
		static const Vector3 Down();
		static const Vector3 Left();
		static const Vector3 Right();

		static Vector3 Lerp(Vector3 a, Vector3 b, float t);
		static Vector3 LerpUnclamped(Vector3 a, Vector3 b, float t);

		static Vector3 MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta);
		static Vector3 SmoothDamp(Vector3 current, Vector3 target, Vector3& currentVelocity, float smoothTime, float maxSpeed, float deltaTime);

		void Set(float newX, float newY, float newZ);
		static Vector3 Scale(Vector3 a, Vector3 b);
		void Scale(Vector3 scale);

		static Vector3 Cross(Vector3 lhs, Vector3 rhs);
		bool Equals(Vector3 other) const;

		static Vector3 Reflect(Vector3 inDirection, Vector3 inNormal);
		static Vector3 Normalize(Vector3 value);
		void Normalize();

		static float Dot(Vector3 lhs, Vector3 rhs);

		static float Angle(Vector3 from, Vector3 to);
		static float SignedAngle(Vector3 from, Vector3 to, Vector3 axis);

		static float Distance(Vector3 a, Vector3 b);
		static Vector3 ClampMagnitude(Vector3 vector, float maxLength);
		static float Magnitude(Vector3 vector);
		static float SqrMagnitude(Vector3 vector);

		static Vector3 Min(Vector3 lhs, Vector3 rhs);
		static Vector3 Max(Vector3 lhs, Vector3 rhs);

		Vector3 operator+(const Vector3& other) const {

			return Vector3(x + other.x, y + other.y, z + other.z);
		}

		Vector3 operator-(const Vector3& other) const {

			return Vector3(x - other.x, y - other.y, z - other.z);
		}

		Vector3 operator-() const {

			return Vector3(0.f - x, 0.f - y, 0.f - z);
		}

		Vector3 operator*(float d) const {

			return Vector3(x * d, y * d, z * d);
		}

		Vector3 operator/(float d) const {

			return Vector3(x / d, y / d, z / d);
		}

		Vector3& operator+=(const Vector3& other) {

			x += other.x;
			y += other.y;
			z += other.z;

			return *this;
		}

		Vector3& operator-=(const Vector3& other) {

			x -= other.x;
			y -= other.y;
			z -= other.z;

			return *this;
		}

		Vector3& operator/=(float d) {

			x /= d;
			y /= d;
			z /= d;

			return *this;
		}

		Vector3& operator*=(float d) {

			x *= d;
			y *= d;
			z *= d;

			return *this;
		}

		bool operator==(const Vector3& other) const {

			float num = x - other.x;
			float num2 = y - other.y;
			float num3 = z - other.z;
			float num4 = num * num + num2 * num2 + num3 * num3;
			return num4 < 9.99999944E-11f;
		}

		bool operator!=(const Vector3& other) const {

			return !(*this == other);
		}

	private:

		static Vector3 zeroVector;
		
		static Vector3 oneVector;

		static Vector3 upVector;

		static Vector3 downVector;

		static Vector3 leftVector;

		static Vector3 rightVector;

		static Vector3 forwardVector;

		static Vector3 backVector;
	};
}
