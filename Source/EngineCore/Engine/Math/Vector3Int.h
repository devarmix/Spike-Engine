#pragma once

#include <Engine/Math/Vector3.h>

namespace SpikeEngine {

	struct Vector3Int {

		Vector3Int() : x(0), y(0), z(0) {}
		Vector3Int(int X, int Y, int Z) : x(X), y(Y), z(Z) {}
		Vector3Int(int X, int Y) : x(X), y(Y), z(0) {}

		int x;
		int y;
		int z;

		float Magnitude() const;
		int SqrMagnitude() const;

		static Vector3Int Zero();
		static Vector3Int One();
		static Vector3Int Up();
		static Vector3Int Down();
		static Vector3Int Left();
		static Vector3Int Right();
		static Vector3Int Forward();
		static Vector3Int Back();

		void Set(int newX, int newY, int newZ);
		static float Distance(Vector3Int a, Vector3Int b);

		static Vector3Int Min(Vector3Int lhs, Vector3Int rhs);
		static Vector3Int Max(Vector3Int lhs, Vector3Int rhs);

		static Vector3Int Scale(Vector3Int a, Vector3Int b);
		void Scale(Vector3Int scale);

		void Clamp(Vector3Int min, Vector3Int max);

		static Vector3Int FloorToInt(Vector3 v);
		static Vector3Int CeilToInt(Vector3 v);
		static Vector3Int RoundToInt(Vector3 v);

		bool Equals(Vector3Int other) const;


		Vector3Int operator-() const {

			return Vector3Int(-x, -y, -z);
		}

		Vector3Int operator+(const Vector3Int& other) const {

			return Vector3Int(x + other.x, y + other.y, z + other.z);
		}

		Vector3Int operator-(const Vector3Int& other) const {

			return Vector3Int(x - other.x, y - other.y, z - other.z);
		}

		Vector3Int operator*(const Vector3Int& other) const {

			return Vector3Int(x * other.x, y * other.y, z * other.z);
		}

		Vector3Int operator*(int b) const {

			return Vector3Int(x * b, y * b, z * b);
		}

		Vector3Int operator/(int b) const {

			return Vector3Int(x / b, y / b, z / b);
		}

		Vector3Int& operator+=(const Vector3Int& other) {

			x += other.x;
			y += other.y;
			z += other.z;

			return *this;
		}

		Vector3Int& operator-=(const Vector3Int& other) {

			x -= other.x;
			y -= other.y;
			z -= other.z;

			return *this;
		}

		Vector3Int& operator*=(const Vector3Int& other) {

			x *= other.x;
			y *= other.y;
			z *= other.z;

			return *this;
		}

		Vector3Int& operator*=(int b) {

			x *= b;
			y *= b;
			z *= b;

			return *this;
		}

		Vector3Int& operator/=(int b) {

			x /= b;
			y /= b;
			z /= b;

			return *this;
		}

		bool operator==(const Vector3Int& other) const {

			return x == other.x && y == other.y && z == other.z;
		}

		bool operator!=(const Vector3Int& other) const {

			return !(*this == other);
		}

	private:

		static Vector3Int zeroVector;

		static Vector3Int oneVector;

		static Vector3Int upVector;

		static Vector3Int downVector;

		static Vector3Int leftVector;

		static Vector3Int rightVector;

		static Vector3Int forwardVector;

		static Vector3Int backVector;
	};
}