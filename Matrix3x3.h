#pragma once
#include "Vector2.h"

class Matrix3x3 {
public:
	static Matrix3x3 Add(const Matrix3x3 m1, const Matrix3x3& m2);

	static Matrix3x3 Subtract(const Matrix3x3& m1, const Matrix3x3& m2);

	static Matrix3x3 Multiply(const Matrix3x3& m1, const Matrix3x3& m2);

	static Matrix3x3 Multiply(const float& num, const Matrix3x3& m);

	static Vector2 Multiply(const Vector2& vector, const Matrix3x3& m);

	static Matrix3x3
	    MakeAffineTransformMatrix(Vector2& scale, float& radian, Vector2& translate);

	static float SarrasRule(const Matrix3x3& m);

public:
	float m[3][3];
};
