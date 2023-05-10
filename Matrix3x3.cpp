#include "Matrix3x3.h"
#include <assert.h>
#include <cmath>

Matrix3x3 Matrix3x3::Add(const Matrix3x3 m1, const Matrix3x3& m2) {
	Matrix3x3 result = {};
	for (int row = 0; row < 3; row++) {
		for (int column = 0; column < 3; column++) {
			result.m[row][column] = m1.m[row][column] + m2.m[row][column];
		}
	}
	return result;
}

Matrix3x3 Matrix3x3::Subtract(const Matrix3x3& m1, const Matrix3x3& m2) {
	Matrix3x3 result = {};
	for (int row = 0; row < 3; row++) {
		for (int column = 0; column < 3; column++) {
			result.m[row][column] = m1.m[row][column] - m2.m[row][column];
		}
	}
	return result;
}

Matrix3x3 Matrix3x3::Multiply(const Matrix3x3& m1, const Matrix3x3& m2) {
	Matrix3x3 result = {};
	for (int i = 0; i < 3; i++) {
		for (int row = 0; row < 3; row++) {
			for (int column = 0; column < 3; column++) {
				result.m[i][row] += m1.m[i][column] * m2.m[column][row];
			}
		}
	}
	return result;
}

Matrix3x3 Matrix3x3::Multiply(const float& num, const Matrix3x3& m) {
	Matrix3x3 result = {};
	for (int row = 0; row < 3; row++) {
		for (int column = 0; column < 3; column++) {
			result.m[row][column] = m.m[row][column] * num;
		}
	}
	return result;
}

Vector2 Matrix3x3::Multiply(const Vector2& vector, const Matrix3x3& m) {
	Vector2 result = {};
	result.x = vector.x * m.m[0][0] + vector.y * m.m[1][0] + 1.0f * m.m[2][0];
	result.y = vector.x * m.m[0][1] + vector.y * m.m[1][1] + 1.0f * m.m[2][1];
	float w = vector.x * m.m[0][2] + vector.y * m.m[1][2] + 1.0f * m.m[2][2];
	assert(w != 0.0f);
	result.x /= w;
	result.y /= w;

	return result;
}

Matrix3x3
    Matrix3x3::MakeAffineTransformMatrix(Vector2& scale, float& radian, Vector2& translate) {
	Matrix3x3 matrix = {
	    {{float(scale.x * std::cos(radian)), float(scale.x * std::sin(radian)), 0},
	     {float(-scale.y * std::sin(radian)), float(scale.y * std::cos(radian)), 0},
	     {translate.x, translate.y, 1}}
    };
	return matrix;
}

float Matrix3x3::SarrasRule(const Matrix3x3& m) {
	float result = m.m[0][0] * m.m[1][1] * m.m[2][2] + m.m[0][1] * m.m[1][2] * m.m[2][0] +
	               m.m[0][2] * m.m[1][0] * m.m[2][1] - m.m[0][2] * m.m[1][1] * m.m[2][0] -
	               m.m[0][1] * m.m[1][0] * m.m[2][2] - m.m[0][0] * m.m[1][2] * m.m[2][1];
	return result;
}
