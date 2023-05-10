#pragma once

/// <summary>
/// 3次元ベクトル
/// </summary>
class Vector3 {
public:
	Vector3() {
		x = 0;
		y = 0;
		z = 0;
	}
	Vector3(float x, float y, float z) {
		this->x = x;
		this->y = y;
		this->z = z;
	}

	Vector3(const Vector3& obj) {
		x = obj.x;
		y = obj.y;
		z = obj.z;
	}

	// 変換
	Vector3& operator=(Vector3 obj) {
		x = obj.x;
		y = obj.y;
		z = obj.z;
		return *this;
	}

	// 加算
	Vector3 operator+(Vector3 obj) {
		Vector3 tmp;
		tmp.x = this->x + obj.x;
		tmp.y = this->y + obj.y;
		tmp.z = this->z + obj.z;
		return tmp;
	}
	void operator+=(Vector3 obj) {
		this->x = this->x + obj.x;
		this->y = this->y + obj.y;
		this->z = this->z + obj.z;
	}

	// 減算
	Vector3 operator-(Vector3 obj) {
		Vector3 tmp;
		tmp.x = this->x - obj.x;
		tmp.y = this->y - obj.y;
		tmp.z = this->z - obj.z;
		return tmp;
	}
	void operator-=(Vector3 obj) {
		this->x -= obj.x;
		this->y -= obj.y;
		this->z -= obj.z;
	}

	// スカラー

	// 乗算
	Vector3 operator*(float a) {
		Vector3 tmp;

		tmp.x = this->x * a;
		tmp.y = this->y * a;
		tmp.z = this->z * a;
		return tmp;
	}
	void operator*=(float a) {
		this->x *= a;
		this->y *= a;
		this->z *= a;
	}

	// 除算
	Vector3 operator/(float a) {
		Vector3 tmp;

		tmp.x = this->x / a;
		tmp.y = this->y / a;
		tmp.z = this->y / a;
		return tmp;
	}
	void operator/=(float a) {
		this->x /= a;
		this->y /= a;
		this->z /= a;
	}

public:
	float x;
	float y;
	float z;
};



