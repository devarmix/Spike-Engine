#pragma once

#include <Engine/Math/Vector2.h>

namespace SpikeEngine {

	struct Vector2Int {

		Vector2Int() : x(0), y(0) {}
		Vector2Int(int X, int Y) : x(X), y(Y) {}

		int x;
		int y;

		float Magnitude() const;
		int SqrMagnitude() const;

		static const Vector2Int Zero();
		static const Vector2Int One();
		static const Vector2Int Up();
		static const Vector2Int Down();
		static const Vector2Int Right();
		static const Vector2Int Left();

		void Set(int newX, int newY);
		static float Distance(Vector2Int a, Vector2Int b);

		static Vector2Int Min(Vector2Int lhs, Vector2Int rhs);
		static Vector2Int Max(Vector2Int lhs, Vector2Int rhs);

		static Vector2Int Scale(Vector2Int a, Vector2Int b);
		void Scale(Vector2Int scale);

		void Clamp(Vector2Int min, Vector2Int max);

		static Vector2Int FloorToInt(Vector2 v);
		static Vector2Int CeilToInt(Vector2 v);
		static Vector2Int RoundToInt(Vector2 v);

		bool Equals(Vector2Int other) const;

		Vector2Int operator-() const {

			return Vector2Int(-x, -y);
		}

		Vector2Int operator+(const Vector2Int& other) const {

			return Vector2Int(x + other.x, y + other.y);
		}

		Vector2Int operator-(const Vector2Int& other) const {

			return Vector2Int(x - other.x, y - other.y);
		}

		Vector2Int operator*(const Vector2Int& other) const {

			return Vector2Int(x * other.x, y * other.y);
		}

		Vector2Int operator*(int b) const {

			return Vector2Int(x * b, y * b);
		}

		Vector2Int operator/(int b) const {

			return Vector2Int(x / b, y / b);
		}

		Vector2Int& operator+=(const Vector2Int& other) {

			x += other.x;
			y += other.y;

			return *this;
		}

		Vector2Int& operator-=(const Vector2Int& other) {

			x -= other.x;
			y -= other.y;

			return *this;
		}

		Vector2Int& operator*=(const Vector2Int& other) {

			x *= other.x;
			y *= other.y;

			return *this;
		}

		Vector2Int& operator*=(int b) {

			x *= b;
			y *= b;

			return *this;
		}

		Vector2Int& operator/=(int b) {

			x /= b;
			y /= b;

			return *this;
		}

		bool operator==(const Vector2Int& other) const {

			return x == other.x && y == other.y;
		}

		bool operator!=(const Vector2Int& other) const {

			return !(*this == other);
		}

	private:

		static Vector2Int zeroVector;

		static Vector2Int oneVector;

		static Vector2Int upVector;

		static Vector2Int downVector;

		static Vector2Int leftVector;

		static Vector2Int rightVector;
	};
}
