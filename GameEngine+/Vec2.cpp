/////////////////////////////////
// Vec2.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "Vec2.h"
#include "Utils.h"
#include <cassert>
#include <cmath>
/////////////////////////////////



/////////////////////////////////
// Static constant for the zero vector. This provides a convenient way to represent a vector with no magnitude, which can be useful in various calculations and comparisons.
const Vec2 Vec2::Zero;
/////////////////////////////////



/////////////////////////////////
// Operator overloads for vector arithmetic and comparisons. These operators allow for intuitive usage of vector operations, such as addition, subtraction, scalar multiplication, and equality comparisons,
bool Vec2::operator==(const Vec2& vector) const {
	return IsEqual(x, vector.x) && IsEqual(y, vector.y);
}
/////////////////////////////////



/////////////////////////////////
// The inequality operator is implemented in terms of the equality operator, returning true if the vectors are not equal and false if they are equal. This provides a convenient way to compare vectors for 
// inequality without needing to duplicate the logic for comparing the individual components.
bool Vec2::operator!=(const Vec2& vector) const {
	return !(*this == vector);
}
/////////////////////////////////



/////////////////////////////////
// The unary negation operator returns a new Vec2 instance with both the x and y components negated. This allows for easily obtaining the inverse of a vector, which can be useful in various calculations such as reflecting a vector or changing its direction.
Vec2 Vec2::operator-() const {
	return Vec2(-x, -y);
}
/////////////////////////////////



/////////////////////////////////
// The scalar multiplication operator multiplies both the x and y components of the vector by the given scalar value and returns a new Vec2 instance with the resulting values. This allows for easily scaling a vector by a certain factor, 
// which can be useful in various operations such as resizing,
Vec2 Vec2::operator*(float scalar) const {
	return Vec2(x * scalar, y * scalar);
}
/////////////////////////////////



/////////////////////////////////
// The scalar division operator divides both the x and y components of the vector by the given scalar value and returns a new Vec2 instance with the resulting values. This allows for easily scaling a vector by the inverse of a certain factor, 
// which can be useful in operations such as normalization.
Vec2 Vec2::operator/(float scalar) const {
	assert(fabsf(scalar) > EPSILON);
	return Vec2(x / scalar, y / scalar);
}
/////////////////////////////////



/////////////////////////////////
// The compound assignment operators for scalar multiplication and division modify the current vector by multiplying or dividing its components by the given scalar value, respectively. These operators allow for more concise code when performing 
// scaling operations on a vector, as they eliminate the need to create a new instance for the result.
Vec2& Vec2::operator*=(float scalar) {
	*this = *this * scalar;
	return *this;
}
/////////////////////////////////



/////////////////////////////////
// The compound assignment operator for scalar division modifies the current vector by dividing its components by the given scalar value. It includes an assertion to ensure that the scalar value is not too close to zero, 
// which would lead to undefined behavior due to division by zero.
Vec2& Vec2::operator/=(float scalar) {
	assert(fabsf(scalar) > EPSILON);
	*this = *this / scalar;
	return *this;
}
/////////////////////////////////



/////////////////////////////////
// The addition operator returns a new Vec2 instance that is the result of adding the corresponding components of the two vectors together. This allows for easily combining two vectors to produce a new vector that represents their sum.
Vec2 Vec2::operator+(const Vec2& vector) const {
	return Vec2(x + vector.x, y + vector.y);
}
/////////////////////////////////



/////////////////////////////////
// The subtraction operator returns a new Vec2 instance that is the result of subtracting the corresponding components of the given vector from the current vector. This allows for easily calculating the difference between two vectors,
Vec2 Vec2::operator-(const Vec2& vector) const {
	return Vec2(x - vector.x, y - vector.y);
}
/////////////////////////////////



/////////////////////////////////
// The compound assignment operators for vector addition and subtraction modify the current vector by adding or subtracting the corresponding components of the given vector, respectively. These operators allow for more concise code when performing
Vec2& Vec2::operator+=(const Vec2& vector) {
	*this = *this + vector;
	return *this;
}
/////////////////////////////////



/////////////////////////////////
// The compound assignment operator for vector subtraction modifies the current vector by subtracting the corresponding components of the given vector. This allows for more concise code when performing subtraction operations on a vector, as it eliminates 
// the need to create a new instance for the result.
Vec2& Vec2::operator-=(const Vec2& vector) {
	*this = *this - vector;
	return *this;
}
/////////////////////////////////



/////////////////////////////////
// Mag2 - calculates the squared magnitude (or length) of the vector by taking the dot product of the vector with itself. This provides a measure of the vector's length without the computational cost of a square root operation, which can be useful for performance-sensitive calculations.
float Vec2::Mag2() const {
	return Dot(*this);
}
/////////////////////////////////



/////////////////////////////////
// Mag - calculates the magnitude (or length) of the vector by taking the square root of the squared magnitude (Mag2). This provides the actual length of the vector in 2D space, which can be useful for various calculations such as normalization, distance, and physics simulations.
float Vec2::Mag() const {
	return sqrt(Mag2());
}
/////////////////////////////////



/////////////////////////////////
// GetUnitVec - returns a new Vec2 instance that is the unit vector (or normalized vector) in the same direction as the current vector. It calculates the magnitude of the vector and divides the vector by its magnitude to obtain the unit vector.
Vec2 Vec2::GetUnitVec() const {
	float mag = Mag();
	if (mag > EPSILON)
		return *this / mag;
	return Vec2::Zero;
}
/////////////////////////////////



/////////////////////////////////
// Normalize - modifies the current vector to become a unit vector (or normalized vector) in the same direction. It calculates the magnitude of the vector and divides the vector by its magnitude to normalize it. If the magnitude is too small (less than EPSILON),
Vec2& Vec2::Normalize() {
	float mag = Mag();
	if (mag > EPSILON)
		*this /= mag;
	return *this;
}
/////////////////////////////////



/////////////////////////////////
// Distance - calculates the distance between the current vector and another vector by computing the difference in their x and y components, squaring those differences, summing them, and then taking the square root of that sum. 
// This provides the straight-line distance between the two vectors in 2D space.
float Vec2::Distance(const Vec2& vector) const {
	float delX = x - vector.x;
	float delY = y - vector.y;
	return sqrt(delX * delX + delY * delY);
}
/////////////////////////////////



/////////////////////////////////
// Dot - calculates the dot product of the current vector and another vector by multiplying their corresponding components (x and y) and summing those products. The dot product is a fundamental operation in vector mathematics 
// that can be used to determine the angle between vectors,
float Vec2::Dot(const Vec2& vector) const {
	return (x * vector.x) + (y * vector.y);
}
/////////////////////////////////



/////////////////////////////////
// ProjectOnto - projects the current vector onto another vector by calculating the dot product of the two vectors, dividing it by the squared magnitude of the other vector, and then multiplying that result by the other vector.
Vec2 Vec2::ProjectOnto(const Vec2& vector) const {
	return (Dot(vector) / vector.Mag2()) * vector;
}
/////////////////////////////////



/////////////////////////////////
// AngleBetween - calculates the angle between the current vector and another vector by using the dot product and magnitudes of the vectors. It computes the cosine of the angle using the dot product divided by the product of the magnitudes,
float Vec2::AngleBetween(const Vec2& vector) const {
	return acosf(Dot(vector) / (Mag() * vector.Mag()));
}
/////////////////////////////////



/////////////////////////////////
// Reflect - this method calculates the reflection of the current vector across a given normal vector. It does this by subtracting twice the projection of the current vector onto the normal vector from the current vector itself.
Vec2 Vec2::Reflect(const Vec2& normal) const {
	return *this - (normal * 2.0f * Dot(normal));
}
/////////////////////////////////



/////////////////////////////////
// Rotate - this method rotates the current vector by a specified angle (in radians) around a given point. It first translates the vector to be relative to the aroundPoint, then applies the rotation using the RotationResult method, and finally translates it back to the original position.
void Vec2::Rotate(float angle, const Vec2& aroundPoint) {
	*this -= aroundPoint;
	*this = RotationResult(angle, Vec2::Zero);
	*this += aroundPoint;
}
/////////////////////////////////



/////////////////////////////////
// RotationResult - this method calculates the result of rotating the current vector by a specified angle (in radians) around a given point without modifying the original vector. It first translates the vector to be relative to the aroundPoint,
Vec2 Vec2::RotationResult(float angle, const Vec2& aroundPoint) const {
	float cosA = cosf(angle);
	float sinA = sinf(angle);
	Vec2 result = *this - aroundPoint;
	float newX = (result.x * cosA) - (result.y * sinA);
	float newY = (result.x * sinA) + (result.y * cosA);
	return Vec2(newX, newY) + aroundPoint;
}
/////////////////////////////////



/////////////////////////////////
// Friend functions for stream output and scalar multiplication. The stream output operator allows for easy printing of vector values to the console, while the scalar multiplication operator allows for multiplying a scalar value with a vector from the left side.
std::ostream& operator<<(std::ostream& consoleOut, const Vec2& vector) {
	consoleOut << "[ " << vector.x << " , " << vector.y << " ]";
	return consoleOut;
}

Vec2 operator*(float scalar, const Vec2& vec) {
	return vec * scalar;
}
/////////////////////////////////
