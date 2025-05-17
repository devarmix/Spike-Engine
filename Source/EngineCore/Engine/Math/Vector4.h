#pragma once

namespace SpikeEngine {

	struct Vector4 {

		Vector4() : x(0.f), y(0.f), z(0.f), w(0.f) {}
		Vector4(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W) {}
		Vector4(float X, float Y, float Z) : x(X), y(Y), z(Z), w(0.f) {}
		Vector4(float X, float Y) : x(X), y(Y), z(0.f), w(0.f) {}

		float x;
		float y;
		float z;
		float w;

		Vector4 Normalized() const;
		float Magnitude() const;
		float SqrMagnitude() const;

		static float SqrMagnitude(Vector4 a);

		static const Vector4 Zero();
		static const Vector4 One();

		void Set(float newX, float newY, float newZ, float newW);
		static Vector4 Lerp(Vector4 a, Vector4 b, float t);
		static Vector4 LerpUnclamped(Vector4 a, Vector4 b, float t);

		static Vector4 MoveTowards(Vector4 current, Vector4 target, float maxDistanceDelta);
		static Vector4 Scale(Vector4 a, Vector4 b);
		void Scale(Vector4 scale);

		bool Equals(Vector4 other) const;
		static Vector4 Normalize(Vector4 a);
		void Normalize();

		static float Dot(Vector4 a, Vector4 b);
		static Vector4 Project(Vector4 a, Vector4 b);

		static float Distance(Vector4 a, Vector4 b);
		static float Magnitude(Vector4 a);

		static Vector4 Min(Vector4 lhs, Vector4 rhs);
		static Vector4 Max(Vector4 lhs, Vector4 rhs);

		Vector4 operator+(const Vector4& other) const {

			return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
		}

		Vector4 operator-(const Vector4& other) const {

			return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
		}

		Vector4 operator-() const {

			return Vector4(0.f - x, 0.f - y, 0.f - z, 0.f - w);
		}

		Vector4 operator*(float d) const {

			return Vector4(x * d, y * d, z * d, w * d);
		}

		Vector4 operator/(float d) const {

			return Vector4(x / d, y / d, z / d, w / d);
		}

		Vector4& operator+=(const Vector4& other) {

			x += other.x;
			y += other.y;
			z += other.z;
			w += other.w;

			return *this;
		}

		Vector4& operator-=(const Vector4& other) {

			x -= other.x;
			y -= other.y;
			z -= other.z;
			w -= other.w;

			return *this;
		}

		Vector4& operator*=(float d) {

			x *= d;
			y *= d;
			z *= d;
			w *= d;

			return *this;
		}

		Vector4& operator/=(float d) {

			x /= d;
			y /= d;
			z /= d;
			w /= d;

			return *this;
		}

		bool operator==(const Vector4& other) const {

			float num = x - other.x;
			float num2 = y - other.y;
			float num3 = z - other.z;
			float num4 = w - other.w;
			float num5 = num * num + num2 * num2 + num3 * num3 + num4 * num4;
			return num5 < 9.99999944E-11f;
		}

		bool operator!=(const Vector4& other) const {

			return !(*this == other);
		}

	private:

		static Vector4 zeroVector;
		static Vector4 oneVector;
	};
}