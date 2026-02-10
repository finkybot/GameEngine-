// Vec2.cpp

// Includes.
#include "Vec2.h"
#include "Utils.h"
#include <cassert>  
#include <cmath>

// Constants.
const Vec2 Vec2::Zero;

// Method definitions.

bool Vec2::operator==(const Vec2& vector) const
{
	return IsEqual(x, vector.x) && IsEqual(y, vector.y);
}

bool Vec2::operator!=(const Vec2& vector) const
{
	return !(*this == vector);
}

Vec2 Vec2::operator-() const
{
	return Vec2(-x, -y);
}

Vec2 Vec2::operator*(float scalar) const
{
	return Vec2(x*scalar, y*scalar);
}

Vec2 Vec2::operator/(float scalar) const
{
	assert(fabsf(scalar) > EPSILON);
	return Vec2(x/scalar, y/scalar);
}

Vec2& Vec2::operator*=(float scalar)
{
	*this = *this * scalar;
	return *this;
}

Vec2& Vec2::operator/=(float scalar)
{
	assert(fabsf(scalar) > EPSILON);
	*this = *this / scalar;
	return *this;
}

Vec2 Vec2::operator+(const Vec2& vector) const
{
	return Vec2(x+vector.x, y+vector.y);
}

Vec2 Vec2::operator-(const Vec2& vector) const
{
	return Vec2(x - vector.x, y - vector.y);
}

Vec2& Vec2::operator+=(const Vec2& vector)
{
	*this = *this + vector;
	return *this;
}

Vec2& Vec2::operator-=(const Vec2& vector)
{
	*this = *this - vector;
	return *this;
}

float Vec2::Mag2() const
{
	return Dot(*this);
}

float Vec2::Mag() const
{
	return sqrt(Mag2());
}

Vec2 Vec2::GetUnitVec() const
{
	float mag = Mag();
	
	if (mag > EPSILON)
	{
		return *this / mag; 
	}
	
	return Vec2::Zero;
}

Vec2& Vec2::Normalize()
{
	float mag = Mag();

	if (mag > EPSILON)
	{
		*this /= mag; 
	}

	return *this;
}

float Vec2::Distance(const Vec2& vector) const
{
	float delX = x - vector.x;
	float delY = y - vector.y;

	return sqrt(delX * delX + delY * delY);
}

float Vec2::Dot(const Vec2& vector) const
{
	return (x * vector.x) + (y * vector.y);
}

Vec2 Vec2::ProjectOnto(const Vec2& vector) const
{
	return (Dot(vector) / vector.Mag2()) * vector;
}

float Vec2::AngleBetween(const Vec2& vector) const
{
	return acosf(Dot(vector) / (Mag() * vector.Mag()));
}

Vec2 Vec2::Reflect(const Vec2& normal) const
{
	return *this - (normal * 2.0f * Dot(normal));
}

void Vec2::Rotate(float angle, const Vec2& aroundPoint)
{
	*this -= aroundPoint;
	*this = RotationResult(angle, Vec2::Zero);
	*this += aroundPoint;
}

Vec2 Vec2::RotationResult(float angle, const Vec2& aroundPoint) const
{
	float cosA = cosf(angle);
	float sinA = sinf(angle);

	Vec2 result = *this - aroundPoint;
	float newX = (result.x * cosA) - (result.y * sinA);
	float newY = (result.x * sinA) + (result.y * cosA);

	return Vec2(newX, newY) + aroundPoint;
}

std::ostream& operator<<(std::ostream& consoleOut, const Vec2& vector)
{
	consoleOut << "[ " << vector.x << " , " << vector.y << " ]";
	return consoleOut;
}

Vec2 operator*(float scalar, const Vec2& vec)
{
	return vec * scalar;
}
