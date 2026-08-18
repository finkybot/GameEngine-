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
	/////////////////////////////////



	/////////////////////////////////
	// Friend functions for stream output and scalar multiplication. The stream output operator allows for easy printing of vector values to the console, while the scalar multiplication operator allows for multiplying a scalar value with a vector from the left side.
	friend std::ostream& operator<<(std::ostream& consoleOut, const Vec2& vector);
	friend Vec2 operator*(float scalar, const Vec2& vec);
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
