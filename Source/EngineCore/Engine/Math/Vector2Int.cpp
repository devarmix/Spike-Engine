#include <Engine/Math/Vector2Int.h>
#include <Engine/Math/Mathf.h>

namespace SpikeEngine {

	Vector2Int Vector2Int::zeroVector = Vector2Int(0, 0);

	Vector2Int Vector2Int::oneVector = Vector2Int(1, 1);

	Vector2Int Vector2Int::upVector = Vector2Int(0, 1);

	Vector2Int Vector2Int::downVector = Vector2Int(0, -1);

	Vector2Int Vector2Int::leftVector = Vector2Int(-1, 0);

	Vector2Int Vector2Int::rightVector = Vector2Int(1, 0);


	float Vector2Int::Magnitude() const {

		return Mathf::Sqrt(float(x * x + y * y));
	}

	int Vector2Int::SqrMagnitude() const {

		return x * x + y * y;
	}


	const Vector2Int Vector2Int::Zero() {

		return zeroVector;
	}

	const Vector2Int Vector2Int::One() {

		return oneVector;
	}

	const Vector2Int Vector2Int::Up() {

		return upVector;
	}

	const Vector2Int Vector2Int::Down() {

		return downVector;
	}

	const Vector2Int Vector2Int::Left() {

		return leftVector;
	}

	const Vector2Int Vector2Int::Right() {

		return rightVector;
	}


	void Vector2Int::Set(int newX, int newY) {

		x = newX;
		y = newY;
	}

	float Vector2Int::Distance(Vector2Int a, Vector2Int b) {

		float num = float(a.x - b.x);
		float num2 = float(a.y - b.y);
		return Mathf::Sqrt(num * num + num2 * num2);
	}

	Vector2Int Vector2Int::Min(Vector2Int lhs, Vector2Int rhs) {

		return Vector2Int(Mathf::Min(lhs.x, rhs.x), Mathf::Min(lhs.y, rhs.y));
	}

	Vector2Int Vector2Int::Max(Vector2Int lhs, Vector2Int rhs) {

		return Vector2Int(Mathf::Max(lhs.x, rhs.x), Mathf::Max(lhs.y, rhs.y));
	}

	Vector2Int Vector2Int::Scale(Vector2Int a, Vector2Int b) {

		return Vector2Int(a.x * b.x, a.y * b.y);
	}

	void Vector2Int::Scale(Vector2Int scale) {

		x *= scale.x;
		y *= scale.y;
	}

	void Vector2Int::Clamp(Vector2Int min, Vector2Int max) {

		x = Mathf::Max(min.x, x);
		x = Mathf::Min(max.x, x);
		y = Mathf::Max(min.y, y);
		y = Mathf::Min(max.y, y);
	}


	Vector2Int Vector2Int::FloorToInt(Vector2 v) {

		return Vector2Int(Mathf::FloorToInt(v.x), Mathf::FloorToInt(v.y));
	}

	Vector2Int Vector2Int::CeilToInt(Vector2 v) {

		return Vector2Int(Mathf::CeilToInt(v.x), Mathf::CeilToInt(v.y));
	}

	Vector2Int Vector2Int::RoundToInt(Vector2 v) {

		return Vector2Int(Mathf::RoundToInt(v.x), Mathf::RoundToInt(v.y));
	}

	bool Vector2Int::Equals(Vector2Int other) const {

		return x == other.x && y == other.y;
	}
}