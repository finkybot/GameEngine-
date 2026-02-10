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


	float Mag2() const;
	/// <summary>
	/// Get the magnitude (length) of the vector using Pythagorean theorem.
	/// </summary>
	/// <returns>Floating point value representing the length of the vector.</returns>
	float Mag() const;

	/// <summary>
	/// Get a unit vector (normalized) from calling vector if magnitude > 0, else return zero vector.
	/// </summary>
	/// <returns>Unit Vec2 object or zero vector if magnitude is zero.</returns>
	Vec2 GetUnitVec() const;

	/// <summary>
	/// Normalize the calling vector (modify in place) if magnitude > 0.
	/// </summary>
	/// <returns>Reference to the normalized calling Vec2 object.</returns>
	Vec2& Normalize();

	/// <summary>
	/// Calculate the distance between this vector and another vector.
	/// </summary>
	/// <param name="vector">Reference to another Vec2 object.</param>
	/// <returns>Floating point distance value.</returns>
	float Distance(const Vec2& vector) const;

	/// <summary>
	/// Calculate the dot product of two vectors.
	/// </summary>
	/// <param name="vector">Reference to another Vec2 object.</param>
	/// <returns>Floating point dot product value.</returns>
	float Dot(const Vec2& vector) const;

	/// <summary>
	/// Project the calling vector onto another vector.
	/// </summary>
	/// <param name="vector">Reference to the vector to project onto.</param>
	/// <returns>Projected Vec2 object.</returns>
	Vec2 ProjectOnto(const Vec2& vector) const;

	/// <summary>
	/// Calculate the angle (in radians) between two vectors.
	/// </summary>
	/// <param name="vector">Reference to another Vec2 object.</param>
	/// <returns>Floating point angle in radians.</returns>
	float AngleBetween(const Vec2& vector) const;

	/// <summary>
	/// Reflect the calling vector off a normal vector.
	/// </summary>
	/// <param name="normal">Reference to the normal vector to reflect off.</param>
	/// <returns>Reflected Vec2 object.</returns>
	Vec2 Reflect(const Vec2& normal) const;

	/// <summary>
	/// Rotate the calling vector around a point (modifies calling vector in place).
	/// </summary>
	/// <param name="angle">Rotation angle in radians.</param>
	/// <param name="aroundPoint">Point to rotate around.</param>
	void Rotate(float angle, const Vec2& aroundPoint);

	/// <summary>
	/// Get the result of rotating the calling vector around a point (returns new vector).
	/// </summary>
	/// <param name="angle">Rotation angle in radians.</param>
	/// <param name="aroundPoint">Point to rotate around.</param>
	/// <returns>Rotated Vec2 object.</returns>
	Vec2 RotationResult(float angle, const Vec2& aroundPoint) const;

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
