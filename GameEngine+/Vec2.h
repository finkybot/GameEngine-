// ***** Vector2D.h - Vec2 class definition *****
#pragma once

// Includes.
#include<iostream>


// Class declaration.

// Vec2 class represents a 2D vector with x and y components.
class Vec2
{
	// ***** Public Member Variables *****
public:
	float x;
	float y;

	// ***** Public Methods *****
public:
	static const Vec2 Zero;					// Static constant representing the zero vector (0, 0)

	Vec2() : Vec2(0, 0) {}					// Default constructor initializes to zero vector
	Vec2(float x, float y) : x(x), y(y) {}	// Constructor to initialize vector with specific x and y values

	inline void SetX(float x) { x = x; }	// Setter for x component
	inline void SetY(float y) { y = y; }	// Setter for y component

	inline float GetX() const { return x; } // Getter for x component
	inline float GetY() const { return y; } // Getter for y component

	bool operator==(const Vec2& vector) const;	// Is Equal operator
	bool operator!=(const Vec2& vector) const;	// Is Not Equal operator
	Vec2 operator-() const;						// Negatation operator
	Vec2 operator*(float scalar) const;			// Scalar multiplication (growing vectors) works with calling  Vector2D * scalar 
	Vec2 operator/(float scalar) const;			// Scalar division (growing vectors)
	Vec2& operator*=(float scalar);				// Scalar multiply equals
	Vec2& operator/=(float scalar);				// Scalar divide equals
	Vec2 operator+(const Vec2& vector) const;	// Vector addition
	Vec2 operator-(const Vec2& vector) const;	// Vector subtraction
	Vec2& operator+=(const Vec2& vector);		// Vector add equals
	Vec2& operator-=(const Vec2& vector);		// Vector subtract equals

	float Mag2() const;	// Get the squared magnitude (length) of the vector, which is more efficient than Mag() when you only need to compare magnitudes or avoid the cost of a square root operation.
	float Mag() const;	// Get the magnitude (length) of the vector, calculated as the square root of the sum of squares of x and y components.

	Vec2 GetUnitVec() const;	// Get a unit vector (normalized) from calling vector if magnitude > 0, else return zero vector.
	Vec2& Normalize();			// Normalize the calling vector in place (modifies the vector to have a magnitude of 1 while maintaining its direction). If the magnitude is zero, the vector remains unchanged to avoid division by zero.

	float Distance(const Vec2& vector) const;		// Calculate the distance between the calling vector and another vector, which is the magnitude of the difference between the two vectors.
	float Dot(const Vec2& vector) const;			// Calculate the dot product of the calling vector and another vector, which is a scalar value representing the magnitude of one vector projected onto another. It is calculated as (x1 * x2) + (y1 * y2).
	Vec2 ProjectOnto(const Vec2& vector) const;		// Project the calling vector onto another vector, which results in a new vector that represents the component of the calling vector in the direction of the other vector. It is calculated using the formula: (dot_product / magnitude_squared) * other_vector.
	float AngleBetween(const Vec2& vector) const;	// Calculate the angle in radians between the calling vector and another vector, which is derived from the dot product and magnitudes of the two vectors using the formula: acos(dot_product / (magnitude1 * magnitude2)). The result is in the range [0, π].

	Vec2 Reflect(const Vec2& normal) const;								// Reflect the calling vector across a surface with the given normal vector, which results in a new vector that represents the direction of the reflected vector. It is calculated using the formula: reflected_vector = original_vector - (2 * (original_vector . normal) * normal), where "." denotes the dot product.
	void Rotate(float angle, const Vec2& aroundPoint);					// Rotate the calling vector by a specified angle (in radians) around a given point. This modifies the vector's position by applying a rotation transformation based on the angle and the pivot point.
	Vec2 RotationResult(float angle, const Vec2& aroundPoint) const;	// Get the result of rotating the calling vector by a specified angle (in radians) around a given point. This returns a new vector representing the rotated position without modifying the original vector.

	// Friends.
	friend std::ostream& operator<<(std::ostream& consoleOut, const Vec2& vector);	// Insertion operator (friend)
	friend Vec2 operator*(float scalar, const Vec2& vec);							// Scalar multiplication (friend) so we can multiply scalar * Vector2D 
};

// Vec3 struct represents a simple 3D vector with x, y, and z components. It is defined as a struct for simplicity and is not intended to be used as a full-featured vector class like Vec2. It can be used for basic 3D vector operations or as a utility type in the game engine where a simple 3D vector is needed.
    struct Vec3  
    {  
       float x;  
       float y;  
       float z;  

       // Constructor to initialize Vec3 with x, y, z values  
       Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}  
    };
