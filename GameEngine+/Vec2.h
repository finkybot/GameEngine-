// Vector2D.h
#pragma once


// Includes.
#include<iostream>


// Class declaration.

/// <summary>
/// Vec2 class.
/// </summary>
class Vec2
{
public:
	float x;
	float y;


public:
	// Static member variables.
	static const Vec2 Zero;


	// Constructors ~ Destructors.
	Vec2() : Vec2(0, 0) {} // Default
	Vec2(float x, float y) : x(x), y(y) {}

	// Methods.
	// Helpers.
	inline void SetX(float x) { x = x; }
	inline void SetY(float y) { y = y; }

	inline float GetX() const { return x; }
	inline float GetY() const { return y; }


	// Operator Overridding.
	bool operator==(const Vec2& vector) const; // Is Equal operator
	bool operator!=(const Vec2& vector) const; // Is Not Equal operator

	Vec2 operator-() const;	// Negatation operator
	
	Vec2 operator*(float scalar) const;	// Scalar multiplication (growing vectors) works with calling  Vector2D * scalar 
	Vec2 operator/(float scalar) const;	// Scalar division (growing vectors)

	Vec2& operator*=(float scalar);	// Scalar multiply equals
	Vec2& operator/=(float scalar);	// Scalar divide equals

	Vec2 operator+(const Vec2& vector) const;
	Vec2 operator-(const Vec2& vector) const;

	Vec2& operator+=(const Vec2& vector);
	Vec2& operator-=(const Vec2& vector);


	// Vector Magnitude
	float Mag2() const;
	float Mag() const;

	// Unit Vectors
	Vec2 GetUnitVec() const;
	Vec2& Normalize();

	float Distance(const Vec2& vector) const; // Vector Distance
	float Dot(const Vec2& vector) const; // Dot Product
	Vec2 ProjectOnto(const Vec2& vector) const;	// Vector Projection
	float AngleBetween(const Vec2& vector) const; // Get the angle between two vectors 
	Vec2 Reflect(const Vec2& normal) const;	// Reflect calling vector of the normal vector

	void Rotate(float angle, const Vec2& aroundPoint); // Rotate calling vector around a point (calling vector modified)
	Vec2 RotationResult(float angle, const Vec2& aroundPoint) const; // Rotate calling vector around a point (new vector returned)

	// Friends.
	friend std::ostream& operator<<(std::ostream& consoleOut, const Vec2& vector); // Insertion operator (friend)
	friend Vec2 operator*(float scalar, const Vec2& vec); // Scalar multiplication (friend) so we can multiply scalar * Vector2D 
};

    struct Vec3  
    {  
       float x;  
       float y;  
       float z;  

       // Constructor to initialize Vec3 with x, y, z values  
       Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}  
    };
