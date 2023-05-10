#pragma once
class Vector2 {
public:
	Vector2() {
		x = 0;
		y = 0;
	}
	Vector2(float x, float y) {
		this->x = x;
		this->y = y;
	}

	Vector2(const Vector2& obj) {
		x = obj.x;
		y = obj.y;
	}

	Vector2& operator=(Vector2 obj) {
		x = obj.x;
		y = obj.y;
		return *this;
	}

	Vector2 operator+(Vector2 obj) {
		Vector2 tmp;
		tmp.x = this->x + obj.x;
		tmp.y = this->y + obj.y;
		return tmp;
	}

	void operator+=(Vector2 obj) {
		this->x = this->x + obj.x;
		this->y = this->y + obj.y;
	}

	Vector2 operator-(Vector2 obj) {
		Vector2 tmp;
		tmp.x = this->x - obj.x;
		tmp.y = this->y - obj.y;
		return tmp;
	}

	void operator-=(Vector2 obj) {
		this->x -= obj.x;
		this->y -= obj.y;
	}

	Vector2 operator*(float a) {
		Vector2 tmp;

		tmp.x = this->x * a;
		tmp.y = this->y * a;
		return tmp;
	}

	void operator*=(float a) {
		this->x *= a;
		this->y *= a;
	}

	Vector2 operator/(float a) {
		Vector2 tmp;

		tmp.x = this->x / a;
		tmp.y = this->y / a;
		return tmp;
	}

	void operator/=(float a) {
		this->x /= a;
		this->y /= a;
	}

public:
	float x;
	float y;
};
