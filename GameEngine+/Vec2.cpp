// Vec2.cpp


// Includes.
#include "Vec2.h"
#include "Utils.h"
#include <cassert>  
#include <cmath>


// Constants.
const Vec2 Vec2::Zero;


// Method definitions.

/// <summary>
/// Operator overriding; is equal (==). Return a Boolean based on Comparison (to a given tolerence) of both the calling Vec2 and referenced Vec2 vectors.
/// </summary>
/// <param name="vector"> - Reference to a constant Vec2 object.</param>
/// <returns>Boolean value; true if equal; false otherwise.</returns>
bool Vec2::operator==(const Vec2& vector) const
{
	return IsEqual(x, vector.x) && IsEqual(y, vector.y); // compares the values of the calling Vec2.
}


/// <summary>
/// Operator overriding; not equal (!=). Return a Boolean based on Comparison (to a given tolerence) of both the calling Vec2 and referenced Vec2 vectors.
/// </summary>
/// <param name="vector"> - Reference to a constant Vec2 object.</param>
/// <returns>Boolean value; true if not equal; false otherwise.</returns>
bool Vec2::operator!=(const Vec2& vector) const
{
	return !(*this == vector);
}


/// <summary>
/// Operator overriding; negation (-). Return a negated copy of a Vec2 object.
/// </summary>
/// <returns>Vec2 object.</returns>
Vec2 Vec2::operator-() const
{
	return Vec2(-x, -y);
}


/// <summary>
/// Operator overriding; scaler multiplicaton (*). Returns a (copy) vector multiplied by a scalar value.
/// </summary>
/// <param name="scalar"> - Floating point value.</param>
/// <returns>Vector2D object; copy of calling Vec2.</returns>
Vec2 Vec2::operator*(float scalar) const
{
	return Vec2(x*scalar, y*scalar);
}


/// <summary>
/// Operator overriding; scaler division (/). Return a (copy) vector divided by a scalar value.
/// </summary>
/// <param name="scalar"> - Floating point value.</param>
/// <returns>Vec2 object.</returns>
Vec2 Vec2::operator/(float scalar) const
{
	assert(fabsf(scalar) > EPSILON); // Guard - will crash program with scalar value is zero (don't divide by zero chump).
	return Vec2(x/scalar, y/scalar);
}


/// <summary>
/// Operator overriding; scaler multiplying (*=). Return a reference to the calling vector that is modified by muliplying it with a scalar value.
/// </summary>
/// <param name="scalar"> - Floating point value; scalar value.</param>
/// <returns>Reference to the calling Vec2 object.</returns>
Vec2& Vec2::operator*=(float scalar)
{
	*this = *this * scalar;	// Uses our overridden multiply operator.
	return *this;
}


/// <summary>
/// Operator overriding; scaler division. Return a reference to the calling vector that is modified by dividing it with a scalar value.
/// </summary>
/// <param name="scalar"> - floating point value.</param>
/// <returns>Reference to the calling Vec2 object.</returns>
Vec2& Vec2::operator/=(float scalar)
{
	assert(fabsf(scalar) > EPSILON); // Guard - will crash program with scalar value is zero.

	*this = *this / scalar;	// Uses our overridden divide operator.
	return *this;
}


/// <summary>
/// Operator overriding; addition (+). Return a (copy) vector based on the addition of the calling vector and the referenced vector.
/// </summary>
/// <param name="vector"> - Reference to a constant Vec2 object.</param>
/// <returns>Vec2 object.</returns>
Vec2 Vec2::operator+(const Vec2& vector) const
{
	return Vec2(x+vector.x, y+vector.y);
}


/// <summary>
/// Operator overriding; subtracting (-). Return a (copy) vector based on the subtraction of the calling vector and the refereneced vector.
/// </summary>
/// <param name="vector"> - Reference to a constant Vec2 object.</param>
/// <returns>Vec2 object.</returns>
Vec2 Vec2::operator-(const Vec2& vector) const
{
	return Vec2(x - vector.x, y - vector.y);
}


/// <summary>
/// Operator overriding; addition (+=). Return a reference to the calling vector after modifying it by adding the passed referenced vector.
/// </summary>
/// <param name="vector"> - Reference to a constant Vec2 object.</param>
/// <returns>Reference to the calling Vec2 object.</returns>
Vec2& Vec2::operator+=(const Vec2& vector)
{
	*this = *this + vector;
	return *this;
}


/// <summary>
/// Operator overriding; subtraction (-=). Return a reference to the calling vector after modifying it by subtracting the passed referenced vector.
/// </summary>
/// <param name="vector">Reference to a constant Vec2 object.</param>
/// <returns>Reference to the calling Vec2 object.</returns>
Vec2& Vec2::operator-=(const Vec2& vector)
{
	*this = *this - vector;
	return *this;
}


/// <summary>
/// Return the squared length the calling vector using Pythagoras theorm.
/// </summary>
/// <returns>Floating point value.</returns>
float Vec2::Mag2() const
{
	return Dot(*this); // Return the squared length of the calling vector.
}


/// <summary>
/// Return the squareroot of the value calculated via the Mag2 method.
/// </summary>
/// <returns>Floating point value.</returns>
float Vec2::Mag() const
{
	return sqrt(Mag2()); // Returns the length of the calling vector (result will always be positive).
}


/// <summary>
/// Return a unit vector based on of the calling vector if the vector is greater than 0, else return a 0 vector.
/// </summary>
/// <returns>Vec2 object.</returns>
Vec2 Vec2::GetUnitVec() const
{
	float mag = Mag(); // Get the magnitude of the calling vector
	
	// If magnitude of the calling vector is greater than 0 (divide by zero check) then return the calling vector divided by the magnitude otherwise return the static zero vector.
	if (mag > EPSILON)
	{
		return *this / mag; 
	}
	
	return Vec2::Zero;
}


/// <summary>
/// Return a reference to the calling vector after it has been normalised.
/// </summary>
/// <returns>Reference to the calling Vec2 object.</returns>
Vec2& Vec2::Normalize()
{
	float mag = Mag(); // Get the magnitude of the calling vector.

	// If magnitude of the calling vector is greater than 0 (divide by zero check) then convert calling vector into a unit vector otherwise return the vector (its at zero).
	if (mag > EPSILON)
	{
		*this /= mag; 
	}

	return *this;
}


/// <summary>
/// Returns the distance between two vectors.
/// </summary>
/// <param name="vector"> - Reference to a constant Vec2 object.</param>
/// <returns>Floating point value.</returns>
float Vec2::Distance(const Vec2& vector) const
{
	return (vector - *this).Mag(); // Return the magnitude of the difference of a passed vector and the calling vector.
}


/// <summary>
/// Return the dot product of two vectors as a scalar value.
/// </summary>
/// <param name="vector"> - Reference to a constant Vec2 object.</param>
/// <returns>Floating point value.</returns>
float Vec2::Dot(const Vec2& vector) const
{
	return x * vector.x + y * vector.y;
}


/// <summary>
/// Return a vector that is Projection of a unit vector onto another vector.
/// </summary>
/// <param name="vector"> - Reference to a constant Vec2 object.</param>
/// <returns>Vec2 object.</returns>
Vec2 Vec2::ProjectOnto(const Vec2& vector) const
{
	Vec2 unitVector = vector.GetUnitVec(); // Get the unit vector of the passed vector.
	float dot = Dot(unitVector); // Get the dot product of the unit vector.

	return unitVector * dot; // Return the unit vector multiplied by the dot product.
}


/// <summary>
/// Return a float that is angle between two vectors: cos theta = a/length of a dot a/length of b.
/// </summary>
/// <param name="vector">Reference to constant Vec2 object.</param>
/// <returns>Floating point object.</returns>
float Vec2::AngleBetween(const Vec2& vector) const
{
	return acosf(GetUnitVec().Dot(vector.GetUnitVec())); // Return the inverse cosine of dot product of the unit vectors of the calling vector and the passed vector.
}


/// <summary>
/// Return the reflection of the calling vector upon the referenced vector.
/// </summary>
/// <param name="normal"> - Reference to constant Vec2 object.</param>
/// <returns>Vec2 object.</returns>
Vec2 Vec2::Reflect(const Vec2& normal) const
{
	return *this - 2 * ProjectOnto(normal);	// Calculation equivalent to v - 2(v dot n)n, we can use the ProjectOnto to do the calculation.
}


/// <summary>
/// Rotate the calling Vec2 around another Vec2; calling Vec2 object is modified.
/// </summary>
/// <param name="angle"> - Floating point value.</param>
/// <param name="aroundPoint"> - Reference to a constant Vec2 object.</param>
void Vec2::Rotate(float angle, const Vec2& aroundPoint)
{
	float cosine = cosf(angle);	// Get the cosine of the angle (radians).
	float sine = sinf(angle); // Get the sine of the angle (radians).

	Vec2 vector(x, y); // Create a new vector based on the calling vector.

	vector -= aroundPoint; // Modify the vector by subtracting the aroundPoint vector.

	float xRot = vector.x * cosine - vector.y * sine;	// Calculate the x rotation.
	float yRot = vector.x * sine + vector.y * cosine;	// Calculate the y rotation.

	Vec2 rotVector = Vec2(xRot, yRot); // Create a vector based on the x and y rotation.

	*this = rotVector + aroundPoint; // Modify the calling vector and add back in the aroundPoint vector.
}


/// <summary>
/// Create and return a new Vec2 by rotating the calling Vec2 around another Vector2D.
/// </summary>
/// <param name="angle"> - Floating point value; amount to rotate by.</param>
/// <param name="aroundPoint">Reference to constant Vec2 object; to rotate around.</param>
/// <returns>Vec2 object.</returns>
Vec2 Vec2::RotationResult(float angle, const Vec2& aroundPoint) const
{
	float cosine = cosf(angle);	// Get the cosine of the angle (radians).
	float sine = sinf(angle); // Get the sine of the angle (radians).

	Vec2 vector(x, y); // Create a new vector based on the calling vector.

	vector -= aroundPoint; // Modify the vector by subtracting the aroundPoint vector.

	float xRot = vector.x * cosine - vector.y * sine;	// Calculate the x rotation.
	float yRot = vector.x * sine + vector.y * cosine;	// Calculate the y rotation.

	Vec2 rotVector = Vec2(xRot, yRot); // Create a vector based on the x and y rotation.

	return rotVector + aroundPoint;	// Return the resulting rotation with the aroundPoint vector added back in.
}


/// <summary>
/// Operator override; (<<). Append the values of the Vector to the console out (cout), returns ostream memory location to allow for further appending.
/// </summary>
/// <param name="consoleOut"> - Reference to ostream object.</param>
/// <param name="vector"> - Reference to constant Vec2 object.</param>
/// <returns>Reference to ostream object.</returns>
std::ostream& operator<<(std::ostream& consoleOut, const Vec2& vector)
{
	consoleOut << "(X: " << vector.x << ", Y: " << vector.y << ")";
	return consoleOut;
}


/// <summary>
/// Operator override; (*). Friend operator method. Multiply scalar value as first value by scalar Vec2 object and returns new Vec2 result.
/// </summary>
/// <param name="scalar"> - Floating point value.</param>
/// <param name="vec"> - Reference to a constant Vec2 object.</param>
/// <returns></returns>
Vec2 operator*(float scalar, const Vec2& vec)
{
	return vec * scalar; // Return muliplied vector (uses the overridden multiply Vec2).
}