#include <Engine/Math/Vector3.h>
#include <Engine/Math/Mathf.h>

namespace SpikeEngine {

    Vector3 Vector3::zeroVector = Vector3(0.f, 0.f, 0.f);

	Vector3 Vector3::oneVector = Vector3(1.f, 1.f, 1.f);

    Vector3 Vector3::upVector = Vector3(0.f, 1.f, 0.f);

    Vector3 Vector3::downVector = Vector3(0.f, -1.f, 0.f);

    Vector3 Vector3::leftVector = Vector3(-1.f, 0.f, 0.f);

    Vector3 Vector3::rightVector = Vector3(1.f, 0.f, 0.f);

    Vector3 Vector3::forwardVector = Vector3(0.f, 0.f, 1.f);

    Vector3 Vector3::backVector = Vector3(0.f, 0.f, -1.f);


    Vector3 Vector3::Normalized() const {

        return Normalize(*this);
    }

    float Vector3::Magnitude() const {

        return Mathf::Sqrt(x * x + y * y + z * z);
    }

    float Vector3::SqrMagnitude() const {

        return x * x + y * y + z * z;
    }

    const Vector3 Vector3::Zero() {

        return zeroVector;
    }

    const Vector3 Vector3::One() {

        return oneVector;
    }

    const Vector3 Vector3::Forward() {

        return forwardVector;
    }

    const Vector3 Vector3::Back() {

        return backVector;
    }

    const Vector3 Vector3::Up() {

        return upVector;
    }

    const Vector3 Vector3::Down() {

        return downVector;
    }

    const Vector3 Vector3::Left() {

        return leftVector;
    }

    const Vector3 Vector3::Right() {

        return rightVector;
    }


    Vector3 Vector3::Lerp(Vector3 a, Vector3 b, float t) {

        t = Mathf::Clamp01(t);
        return Vector3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
    }

    Vector3 Vector3::LerpUnclamped(Vector3 a, Vector3 b, float t) {

        return Vector3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
    }

    Vector3 Vector3::MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta) {

        float num = target.x - current.x;
        float num2 = target.y - current.y;
        float num3 = target.z - current.z;
        float num4 = num * num + num2 * num2 + num3 * num3;
        if (num4 == 0.f || (maxDistanceDelta >= 0.f && num4 <= maxDistanceDelta * maxDistanceDelta))
        {
            return target;
        }

        float num5 = Mathf::Sqrt(num4);
        return Vector3(current.x + num / num5 * maxDistanceDelta, current.y + num2 / num5 * maxDistanceDelta, current.z + num3 / num5 * maxDistanceDelta);
    }

    Vector3 Vector3::SmoothDamp(Vector3 current, Vector3 target, Vector3& currentVelocity, float smoothTime, float maxSpeed, float deltaTime) {

        float num = 0.f;
        float num2 = 0.f;
        float num3 = 0.f;
        smoothTime = Mathf::Max(0.0001f, smoothTime);
        float num4 = 2.f / smoothTime;
        float num5 = num4 * deltaTime;
        float num6 = 1.f / (1.f + num5 + 0.48f * num5 * num5 + 0.235f * num5 * num5 * num5);
        float num7 = current.x - target.x;
        float num8 = current.y - target.y;
        float num9 = current.z - target.z;
        Vector3 vector = target;
        float num10 = maxSpeed * smoothTime;
        float num11 = num10 * num10;
        float num12 = num7 * num7 + num8 * num8 + num9 * num9;
        if (num12 > num11)
        {
            float num13 = Mathf::Sqrt(num12);
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

        return Vector3(num, num2, num3);
    }

    void Vector3::Set(float newX, float newY, float newZ) {

        x = newX;
        y = newY;
        z = newZ;
    }

    Vector3 Vector3::Scale(Vector3 a, Vector3 b) {

        return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
    }

    void Vector3::Scale(Vector3 scale) {

        x *= scale.x;
        y *= scale.y;
        z *= scale.z;
    }

    Vector3 Vector3::Cross(Vector3 lhs, Vector3 rhs) {

        return Vector3(lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x);
    }
     
    bool Vector3::Equals(Vector3 other) const {

        return x == other.x && y == other.y && z == other.z;
    }

    Vector3 Vector3::Reflect(Vector3 inDirection, Vector3 inNormal) {

        float num = -2.f * Dot(inNormal, inDirection);
        return Vector3(num * inNormal.x + inDirection.x, num * inNormal.y + inDirection.y, num * inNormal.z + inDirection.z);
    }

    Vector3 Vector3::Normalize(Vector3 value) {

        float num = Magnitude(value);
        if (num > 1E-05f)
        {
            return value / num;
        }

        return Zero();
    }

    void Vector3::Normalize() {

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

    float Vector3::Dot(Vector3 lhs, Vector3 rhs) {

        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    float Vector3::Angle(Vector3 from, Vector3 to) {

        float num = Mathf::Sqrt(from.SqrMagnitude() * to.SqrMagnitude());
        if (num < 1E-15f)
        {
            return 0.f;
        }

        float num2 = Mathf::Clamp(Dot(from, to) / num, -1.f, 1.f);
        return Mathf::Acos(num2) * 57.29578f;
    }

    float Vector3::SignedAngle(Vector3 from, Vector3 to, Vector3 axis) {

        float num = Angle(from, to);
        float num2 = from.y * to.z - from.z * to.y;
        float num3 = from.z * to.x - from.x * to.z;
        float num4 = from.x * to.y - from.y * to.x;
        float num5 = Mathf::Sign(axis.x * num2 + axis.y * num3 + axis.z * num4);
        return num * num5;
    }

    float Vector3::Distance(Vector3 a, Vector3 b) {

        float num = a.x - b.x;
        float num2 = a.y - b.y;
        float num3 = a.z - b.z;
        return Mathf::Sqrt(num * num + num2 * num2 + num3 * num3);
    }

    Vector3 Vector3::ClampMagnitude(Vector3 vector, float maxLength) {

        float num = vector.SqrMagnitude();
        if (num > maxLength * maxLength)
        {
            float num2 = Mathf::Sqrt(num);
            float num3 = vector.x / num2;
            float num4 = vector.y / num2;
            float num5 = vector.z / num2;
            return Vector3(num3 * maxLength, num4 * maxLength, num5 * maxLength);
        }

        return vector;
    }

    float Vector3::Magnitude(Vector3 vector) {

        return Mathf::Sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
    }

    float Vector3::SqrMagnitude(Vector3 vector) {

        return vector.x * vector.x + vector.y * vector.y + vector.z * vector.z;
    }

    Vector3 Vector3::Min(Vector3 lhs, Vector3 rhs) {

        return Vector3(Mathf::Min(lhs.x, rhs.x), Mathf::Min(lhs.y, rhs.y), Mathf::Min(lhs.z, rhs.z));
    }

    Vector3 Vector3::Max(Vector3 lhs, Vector3 rhs) {

        return Vector3(Mathf::Max(lhs.x, rhs.x), Mathf::Max(lhs.y, rhs.y), Mathf::Max(lhs.z, rhs.z));
    }
}