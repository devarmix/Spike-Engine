#pragma once

namespace SpikeEngine {

	struct Vector2 {

		Vector2() : x(0.f), y(0.f) {}
		Vector2(float X, float Y) : x(X), y(Y) {}

		void Set(float newX, float newY);

		float x;
		float y;

		Vector2 Normalized() const;
		float Magnitude() const;

		static const Vector2 Zero();
		static const Vector2 One();
		static const Vector2 Up();
		static const Vector2 Down();
		static const Vector2 Left();
		static const Vector2 Right();

		static Vector2 Lerp(Vector2 a, Vector2 b, float t);
		static Vector2 LerpUnclamped(Vector2 a, Vector2 b, float t);
		static Vector2 MoveTowards(Vector2 current, Vector2 target, float maxDistanceDelta);
		static Vector2 Scale(Vector2 a, Vector2 b);

		void Scale(Vector2 scale);
		void Normalize();

		bool Equals(Vector2 other) const;
		static Vector2 Reflect(Vector2 inDirection, Vector2 inNormal);
		static Vector2 Perpendicular(Vector2 inDirection);

		static float Dot(Vector2 lhs, Vector2 rhs);
		static float Angle(Vector2 from, Vector2 to);
		static float SignedAngle(Vector2 from, Vector2 to);

		static float Distance(Vector2 a, Vector2 b);
		static Vector2 ClampMagnitude(Vector2 vector, float maxLength);
		static float SqrMagnitude(Vector2 a);
		float SqrMagnitude() const;

		static Vector2 Min(Vector2 lhs, Vector2 rhs);
		static Vector2 Max(Vector2 lhs, Vector2 rhs);

		static Vector2 SmoothDamp(Vector2 current, Vector2 target, Vector2& currentVelocity, float smoothTime, float maxSpeed, float deltaTime);

        Vector2 operator+(const Vector2& other) const {

            return Vector2(x + other.x, y + other.y);
        }

        Vector2 operator-(const Vector2& other) const {

            return Vector2(x - other.x, y - other.y);
        }

        Vector2 operator*(const Vector2& other) const {

            return Vector2(x * other.x, y * other.y);
        }

        Vector2 operator/(const Vector2& other) const {

            return Vector2(x / other.x, y / other.y);
        }

        Vector2 operator-() const {

            return Vector2(0.f - x, 0.f - y);
        }

        Vector2 operator*(float d) const {

            return Vector2(x * d, y * d);
        }

        Vector2 operator/(float d) const {
			
            return Vector2(x / d, y / d);
        }

		Vector2& operator/=(float d) {

			x /= d;
			y /= d;

			return *this;
		}

		Vector2& operator*=(float d) {

			x *= d;
			y *= d;

			return *this;
		}

		Vector2& operator+=(const Vector2& other) {

			x += other.x;
			y += other.y;

			return *this;
		}

		Vector2& operator-=(const Vector2& other) {

			x -= other.x;
			y -= other.y;

			return *this;
		}

		Vector2& operator/=(const Vector2& other) {

			x /= other.x;
			y /= other.y;

			return *this;
		}

		Vector2& operator*=(const Vector2& other) {

			x *= other.x;
			y *= other.y;

			return *this;
		}

        bool operator ==(const Vector2& other)
        {
            float num = x - other.x;
            float num2 = y - other.y;
            return num * num + num2 * num2 < 9.99999944E-11f;
        }

        bool operator !=(const Vector2& other)
        {
            return !(*this == other);
        }

	private:

		static Vector2 zeroVector;

		static Vector2 oneVector;

		static Vector2 upVector;

		static Vector2 downVector;

		static Vector2 leftVector;

		static Vector2 rightVector;
	};
}
