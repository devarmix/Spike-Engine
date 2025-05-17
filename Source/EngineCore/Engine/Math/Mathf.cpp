#include <cmath>
#include "Mathf.h"

namespace SpikeEngine {

	const float Mathf::PI = 3.14159274f;
	const float Mathf::E = 2.71828175f;
	const float Mathf::Deg2Rad = PI / 180;
	const float Mathf::Rad2Deg = 57.29578f;

	float Mathf::Sin(float f) {
		return sin(f);
	}

	float Mathf::Cos(float f) {
		return cos(f);
	}

	float Mathf::Tan(float f) {
		return tan(f);
	}

	float Mathf::Asin(float f) {
		return asin(f);
	}

	float Mathf::Acos(float f) {
		return acos(f);
	}

	float Mathf::Atan(float f) {
		return atan(f);
	}

	float Mathf::Atan2(float y, float x) {
		return atan2(y, x);
	}

	float Mathf::Sqrt(float f) {
		return sqrt(f);
	}

	float Mathf::Abs(float f) {
		return abs(f);
	}

	int Mathf::Abs(int i) {
		return abs(i);
	}

	float Mathf::Min(float a, float b) {
		return (a < b) ? a : b;
	}

	int Mathf::Min(int a, int b) {
		return (a < b) ? a : b;
	}

	float Mathf::Max(float a, float b) {
		return (a > b) ? a : b;
	}

	int Mathf::Max(int a, int b) {
		return (a > b) ? a : b;
	}

	float Mathf::Pow(float f, float p) {
		return pow(f, p);
	}

	float Mathf::Exp(float power) {
		return exp(power);
	}

	float Mathf::Log(float f) {
		return log(f);
	}

	float Mathf::Log10(float f) {
		return log10(f);
	}

	float Mathf::Ceil(float f) {
		return ceil(f);
	}

	float Mathf::Floor(float f) {
		return floor(f);
	}

	float Mathf::Round(float f) {
		return round(f);
	}

	int Mathf::CeilToInt(float f) {
		return (int)ceil(f);
	}

	int Mathf::FloorToInt(float f) {
		return (int)floor(f);
	}

	int Mathf::RoundToInt(float f) {
		return (int)round(f);
	}

	float Mathf::Sign(float f) {
		return (f >= 0) ? 1.f : (-1.f);
	}

	float Mathf::Clamp(float value, float min, float max) {

		if (value < min) {
			value = min;
		}
		else if (value > max) {
			value = max;
		}

		return value;
	}

	int Mathf::Clamp(int value, int min, int max) {

		if (value < min) {
			value = min;
		}
		else if (value > max) {
			value = max;
		}

		return value;
	}

	float Mathf::Clamp01(float value) {

		if (value < 0) {
			return 0;
		}

		if (value > 1) {
			return 1;
		}

		return value;
	}

	float Mathf::Lerp(float a, float b, float t) {
		return a + (b - a) * Clamp01(t);
	}

	float Mathf::LerpUnclamped(float a, float b, float t) {
		return a + (b - a) * t;
	}

	float Mathf::LerpAngle(float a, float b, float t) {

		float num = Repeat(b - a, 360);
		if (num > 180) {
			num -= 360;
		}

		return a + num * Clamp01(t);
	}

	float Mathf::MoveTowards(float current, float target, float maxDelta) {

		if (Abs(target - current) <= maxDelta) {
			return target;
		}

		return current + Sign(target - current) * maxDelta;
	}

	float Mathf::MoveTowardsAngle(float current, float target, float maxDelta) {

		float num = DeltaAngle(current, target);
		if (0 - maxDelta < num && num < maxDelta) {
			return target;
		}

		target = current + num;
		return MoveTowards(current, target, maxDelta);
	}

	float Mathf::SmoothStep(float from, float to, float t) {

		t = Clamp01(t);
		t = -2 * t * t * t + 3 * t * t;
		return to * t + from * (1 - t);
	}

	float Mathf::Gamma(float value, float absmax, float gamma) {

		bool flag = value < 0;
		float num = Abs(value);
		if (num > absmax)
		{
			return flag ? (0 - num) : num;
		}

		float num2 = Pow(num / absmax, gamma) * absmax;
		return flag ? (0 - num2) : num2;
	}

	float Mathf::SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
	{
		smoothTime = Max(0.0001f, smoothTime);
		float num = 2 / smoothTime;
		float num2 = num * deltaTime;
		float num3 = 1 / (1 + num2 + 0.48f * num2 * num2 + 0.235f * num2 * num2 * num2);
		float value = current - target;
		float num4 = target;
		float num5 = maxSpeed * smoothTime;
		value = Clamp(value, 0 - num5, num5);
		target = current - value;
		float num6 = (currentVelocity + num * value) * deltaTime;
		currentVelocity = (currentVelocity - num * num6) * num3;
		float num7 = target + (value + num6) * num3;
		if (num4 - current > 0 == num7 > num4)
		{
			num7 = num4;
			currentVelocity = (num7 - num4) / deltaTime;
		}

		return num7;
	}

	float Mathf::SmoothDampAngle(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
	{
		target = current + DeltaAngle(current, target);
		return SmoothDamp(current, target, currentVelocity, smoothTime, maxSpeed, deltaTime);
	}

	float Mathf::Repeat(float t, float length) {
		return Clamp(t - Floor(t / length) * length, 0.0f, length);
	}

	float Mathf::PingPong(float t, float length) {

		t = Repeat(t, length * 2);
		return length - Abs(t - length);
	}

	float Mathf::InverseLerp(float a, float b, float value) {

		if (a != b) {
			return Clamp01((value - a) / (b - a));
		}

		return 0;
	}

	float Mathf::DeltaAngle(float current, float target) {

		float num = Repeat(target - current, 360);
		if (num > 180) {
			num -= 360;
		}

		return num;
	}
}