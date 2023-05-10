#pragma once
#include "Vector3.h"

class Matrix4x4 {
public:
	static Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

	static Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

	static Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

	static Matrix4x4 Multiply(const float& num, const Matrix4x4& m);

	static Vector3 Multiply(const Vector3& vector, const Matrix4x4& m);

	static Matrix4x4 Inverse(const Matrix4x4& m);

	// 転置行列
	static Matrix4x4 Transpose(const Matrix4x4& m);

	// 単位行列
	static Matrix4x4 MakeIdentity4x4();

	static Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

	static Matrix4x4 MakeScaleMatrix(const Vector3& scale);

	static Vector3 Transform(const Vector3& vector, const Matrix4x4& m);

	static Matrix4x4 MakeRotateXMatrix(float radian);

	static Matrix4x4 MakeRotateYMatrix(float radian);

	static Matrix4x4 MakeRotateZMatrix(float radian);

	static Matrix4x4 MakeRotateXYZMatrix(const Vector3& rotate);

	static Matrix4x4 MakeAffinMatrix(
	    const Vector3& scale, const Vector3& rotate, const Vector3& translate);

public:
	float m[4][4];
};