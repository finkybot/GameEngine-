/////////////////////////////////
// Vec2.h - 2D vector class definition
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the Vec2 class.
#pragma once
#include <iostream>
/////////////////////////////////



/////////////////////////////////
// Vec2 class - represents a 2D vector with x and y components, along with various operations for vector arithmetic, normalization, projection, and rotation. 
// This class provides a convenient way to perform common vector operations needed for game development, such as calculating distances, angles, and reflections.
//								|
//								|_______________________________________________________________________
class Vec2 {
	/////////////////////////////////
	// Public member variables for the Vec2 class
public:
	/////////////////////////////////
	// Member variables for the x and y components of the vector.
	float x;
	float y;
	/////////////////////////////////



	/////////////////////////////////
	// Static constant for the zero vector. This provides a convenient way to represent a vector with no magnitude, which can be useful in various calculations and comparisons.
	static const Vec2 Zero;
	/////////////////////////////////



	/////////////////////////////////
	// Constructors for the Vec2 class. The default constructor initializes the vector to the zero vector (0, 0), while the constructor with parameters allows for initializing the vector with specific x and y values.
	Vec2() : Vec2(0, 0) {}
	Vec2(float x, float y) : x(x), y(y) {}
	/////////////////////////////////



	/////////////////////////////////
	// Inline helper methods for accessing and modifying the x and y components of the vector. These methods provide a convenient way to set and get the individual components of the vector, 
	// allowing for easy manipulation and retrieval of vector values in various calculations and operations.
	inline void SetX(float val) { x = val; }
	inline void SetY(float val) { y = val; }
	inline float GetX() const { return x; }
	inline float GetY() const { return y; }
	/////////////////////////////////



	/////////////////////////////////
	// Operator overloads for vector arithmetic and comparisons. These operators allow for intuitive usage of vector operations, such as addition, subtraction, scalar multiplication, and equality comparisons,
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
	/////////////////////////////////



	/////////////////////////////////
	// Methods for vector operations such as magnitude calculation, normalization, distance, dot product, projection, angle between vectors, reflection, and rotation. These methods provide the necessary functionality to perform common vector calculations needed in game development.
	float Mag2() const; // Returns the squared magnitude of the vector, which is more efficient than calculating the actual magnitude when only relative comparisons are needed.
	float Mag() const; // Returns the magnitude (length) of the vector, which is the square root of the squared magnitude.
	Vec2 GetUnitVec() const; // Returns a unit vector (vector with magnitude of 1) in the same direction as the original vector. This is useful for normalizing vectors for direction calculations.
	Vec2& Normalize(); // Normalizes the vector in place, modifying its components to have a magnitude of 1 while maintaining its direction. Returns a reference to the modified vector.

	float Distance(const Vec2& vector) const; // Returns the distance between this vector and another vector, calculated as the magnitude of the difference between the two vectors.
	float Dot(const Vec2& vector) const; // Returns the dot product of this vector and another vector, which is a measure of how much the two vectors point in the same direction. It is calculated as the sum of the products of their corresponding components.
	Vec2 ProjectOnto(const Vec2& vector) const; // Returns the projection of this vector onto another vector, which is the component of this vector that lies in the direction of the other vector. This is useful for calculating how much of one vector is aligned with another.
	float AngleBetween(const Vec2& vector) const; // Returns the angle between this vector and another vector in radians.

	Vec2 Reflect(const Vec2& normal) const; // Returns the reflection of this vector around a given normal vector.
	void Rotate(float angle, const Vec2& aroundPoint); // Rotates this vector around a given point by a specified angle in radians.
	Vec2 RotationResult(float angle, const Vec2& aroundPoint) const; // Returns the result of rotating this vector around a given point by a specified angle in radians without modifying the original vector.
	/////////////////////////////////



	/////////////////////////////////
	static Vec2	RandomDirection(); // Returns a random unit vector (direction) in 2D space. This can be useful for generating random movement directions or orientations in game development.
	/////////////////////////////////



	/////////////////////////////////
	// Friend functions for stream output and scalar multiplication. The stream output operator allows for easy printing of vector values to the console, while the scalar multiplication operator allows for multiplying a scalar value with a vector from the left side.
	friend std::ostream& operator<<(std::ostream& consoleOut, const Vec2& vector); // Overload the stream output operator to print the vector in a readable format (e.g., "(x, y)").
	friend Vec2 operator*(float scalar,	const Vec2&	vec); // Overload the scalar multiplication operator to allow multiplying a scalar value with a vector from the left side (e.g., "scalar * vector").
	/////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// Vec3 struct - represents a 3D vector with x, y, and z components. This struct is used for representing 3D positions, colors, or any other data that requires three components. It provides a convenient way to store and manipulate 3D vector data in the game engine.
//								|
//								|_______________________________________________________________________
struct Vec3 {
	float x;
	float y;
	float z;

	Vec3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};
/////////////////////////////////
