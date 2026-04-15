// Vec2.h - 2D vector class definition
#pragma once

#include <iostream>

class Vec2 {
public:
	float x;
	float y;

	static const Vec2 Zero;

	Vec2() : Vec2(0, 0) {}
	Vec2(float x, float y) : x(x), y(y) {}

	inline void SetX(float val) { x = val; }
	inline void SetY(float val) { y = val; }
	inline float GetX() const { return x; }
	inline float GetY() const { return y; }

	bool operator==(const Vec2& vector) const;
	bool operator!=(const Vec2& vector) const;
	Vec2 operator-() const;
	Vec2 operator*(float scalar) const;
	Vec2 operator/(float scalar) const;
	Vec2& operator*=(float scalar);
	Vec2& operator/=(float scalar);
	Vec2 operator+(const Vec2& vector) const;
	Vec2 operator-(const Vec2& vector) const;
	Vec2& operator+=(const Vec2& vector);
	Vec2& operator-=(const Vec2& vector);

	float Mag2() const;
	float Mag() const;
	Vec2 GetUnitVec() const;
	Vec2& Normalize();

	float Distance(const Vec2& vector) const;
	float Dot(const Vec2& vector) const;
	Vec2 ProjectOnto(const Vec2& vector) const;
	float AngleBetween(const Vec2& vector) const;

	Vec2 Reflect(const Vec2& normal) const;
	void Rotate(float angle, const Vec2& aroundPoint);
	Vec2 RotationResult(float angle, const Vec2& aroundPoint) const;

	friend std::ostream& operator<<(std::ostream& consoleOut, const Vec2& vector);
	friend Vec2 operator*(float scalar, const Vec2& vec);
};

struct Vec3 {
	float x;
	float y;
	float z;

	Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};
