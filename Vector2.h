#pragma once
struct Vector2 {
public:

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
