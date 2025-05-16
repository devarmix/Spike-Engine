#include <Engine/Math/Vector3Int.h>
#include <Engine/Math/Mathf.h>

namespace SpikeEngine {

    Vector3Int Vector3Int::zeroVector = Vector3Int(0, 0, 0);

    Vector3Int Vector3Int::oneVector = Vector3Int(1, 1, 1);

    Vector3Int Vector3Int::upVector = Vector3Int(0, 1, 0);

    Vector3Int Vector3Int::downVector = Vector3Int(0, -1, 0);

    Vector3Int Vector3Int::leftVector = Vector3Int(-1, 0, 0);

    Vector3Int Vector3Int::rightVector = Vector3Int(1, 0, 0);

    Vector3Int Vector3Int::forwardVector = Vector3Int(0, 0, 1);

    Vector3Int Vector3Int::backVector = Vector3Int(0, 0, -1);


    float Vector3Int::Magnitude() const {

        return Mathf::Sqrt(float(x * x + y * y + z * z));
    }

    int Vector3Int::SqrMagnitude() const {

        return x * x + y * y + z * z;
    }


    Vector3Int Vector3Int::Zero() {

        return zeroVector;
    }

    Vector3Int Vector3Int::One() {

        return oneVector;
    }

    Vector3Int Vector3Int::Up() {

        return upVector;
    }

    Vector3Int Vector3Int::Down() {

        return downVector;
    }

    Vector3Int Vector3Int::Left() {

        return leftVector;
    }

    Vector3Int Vector3Int::Right() {

        return rightVector;
    }

    Vector3Int Vector3Int::Forward() {

        return forwardVector;
    }

    Vector3Int Vector3Int::Back() {

        return backVector;
    }


    void Vector3Int::Set(int newX, int newY, int newZ) {

        x = newX;
        y = newY;
        z = newZ;
    }

    float Vector3Int::Distance(Vector3Int a, Vector3Int b) {

        return (a - b).Magnitude();
    }

    Vector3Int Vector3Int::Min(Vector3Int lhs, Vector3Int rhs) {

        return Vector3Int(Mathf::Min(lhs.x, rhs.x), Mathf::Min(lhs.y, rhs.y), Mathf::Min(lhs.z, rhs.z));
    }

    Vector3Int Vector3Int::Max(Vector3Int lhs, Vector3Int rhs) {

        return Vector3Int(Mathf::Max(lhs.x, rhs.x), Mathf::Max(lhs.y, rhs.y), Mathf::Max(lhs.z, rhs.z));
    }

    Vector3Int Vector3Int::Scale(Vector3Int a, Vector3Int b) {

        return Vector3Int(a.x * b.x, a.y * b.y, a.z * b.z);
    }

    void Vector3Int::Scale(Vector3Int scale) {

        x *= scale.x;
        y *= scale.y;
        z *= scale.z;
    }

    void Vector3Int::Clamp(Vector3Int min, Vector3Int max) {

        x = Mathf::Max(min.x, x);
        x = Mathf::Min(max.x, x);
        y = Mathf::Max(min.y, y);
        y = Mathf::Min(max.y, y);
        z = Mathf::Max(min.z, z);
        z = Mathf::Min(max.z, z);
    }

    Vector3Int Vector3Int::FloorToInt(Vector3 v) {

        return Vector3Int(Mathf::FloorToInt(v.x), Mathf::FloorToInt(v.y), Mathf::FloorToInt(v.z));
    }

    Vector3Int Vector3Int::CeilToInt(Vector3 v) {

        return Vector3Int(Mathf::CeilToInt(v.x), Mathf::CeilToInt(v.y), Mathf::CeilToInt(v.z));
    }

    Vector3Int Vector3Int::RoundToInt(Vector3 v) {

        return Vector3Int(Mathf::RoundToInt(v.x), Mathf::RoundToInt(v.y), Mathf::RoundToInt(v.z));
    }

    bool Vector3Int::Equals(Vector3Int other) const {

        return *this == other;
    }
}