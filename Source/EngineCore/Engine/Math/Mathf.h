#pragma once

namespace SpikeEngine {

	class Mathf {
	public:

		static const float PI;
		static const float E;
		static const float Deg2Rad;
		static const float Rad2Deg;

		static float Sin(float f);
		static float Cos(float f);
		static float Tan(float f);
		static float Asin(float f);
		static float Acos(float f);
		static float Atan(float f);
		static float Atan2(float y, float x);
		static float Sqrt(float f);
		static float Abs(float f);
		static int Abs(int i);
		static float Min(float a, float b);
		static int Min(int a, int b);
		static float Max(float a, float b);
		static int Max(int a, int b);
		static float Pow(float f, float p);
		static float Exp(float power);
		static float Log(float f);
		static float Log10(float f);
		static float Ceil(float f);
		static float Floor(float f);
		static float Round(float f);
		static int CeilToInt(float f);
		static int FloorToInt(float f);
		static int RoundToInt(float f);
		static float Sign(float f);
		static float Clamp(float value, float min, float max);
		static int Clamp(int value, int min, int max);
		static float Clamp01(float value);
		static float Lerp(float a, float b, float t);
		static float LerpUnclamped(float a, float b, float t);
		static float LerpAngle(float a, float b, float t);
		static float MoveTowards(float current, float target, float maxDelta);
		static float MoveTowardsAngle(float current, float target, float maxDelta);
		static float SmoothStep(float from, float to, float t);
		static float Gamma(float value, float absmax, float gamma);
		static float SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime);
		static float SmoothDampAngle(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime);
		static float Repeat(float t, float length);
		static float PingPong(float t, float length);
		static float InverseLerp(float a, float b, float value);
		static float DeltaAngle(float current, float target);
	};
}
