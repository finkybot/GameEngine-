// Utils.h
// Contains ultility function declarations
#pragma once


// Includes.
#include <string>


// Constants.
static const float EPSILON = 0.0001f; // Tolerence value for floating point value comparisons

const float PI = 3.14159f;
const float TWO_PI = 2.0f * PI;

// Structure declarations.

/// <summary>
/// Size structure declaration; holds width and height
/// </summary>
struct Size
{
	unsigned int width = 0, height = 0;
};


// Utility functions.

/// <summary>
/// Returns a comparison; if the float val1 is equal to the float val2 (to the tolerance EPSILON).
/// </summary>
/// <param name="val1">Floating point value.</param>
/// <param name="val2">Floating point value.</param>
/// <returns>Boolean value; true if floats are equal within the EPSILON tolerance.</returns>
bool IsEqual(float val1, float val2);

/// <summary>
/// Returns a comparison; if the float val1 is greater than or equal to the float val2 (to the tolerance EPSILON).
/// </summary>
/// <param name="val1">Floating point value.</param>
/// <param name="val2">Floating point value.</param>
/// <returns>Boolean value; true if value1 is greater than or equal to value2.</returns>
bool IsGreaterThanOrEqual(float val1, float val2);

/// <summary>
/// Returns a comparison; if the float val1 is less than or equal to the float val2 (to the tolerance EPSILON).
/// </summary>
/// <param name="val1">Floating point value.</param>
/// <param name="val2">Floating point value.</param>
/// <returns>Boolean value; true if value1 is less than or equal to value2.</returns>
bool IsLessThanOrEqual(float val1, float val2);

/// <summary>
/// Converts a value of milliseconds into seconds.
/// </summary>
/// <param name="milliseconds">Unsigned integer; time in milliseconds.</param>
/// <returns>Floating point; time in seconds.</returns>
float MillisecondsToSeconds(unsigned int milliseconds);

/// <summary>
/// Get an index for a pixel at a given position in a 2D array.
/// </summary>
/// <param name="width">Unsigned integer value; width of the array.</param>
/// <param name="row">Unsigned integer value; row index.</param>
/// <param name="col">Unsigned integer value; column index.</param>
/// <returns>Integer value (row * width + col).</returns>
unsigned int GetIndex(unsigned int width, unsigned int row, unsigned int col);

/// <summary>
/// Compares two strings (case insensitive).
/// </summary>
/// <param name="a">String to compare.</param>
/// <param name="b">String to compare.</param>
/// <returns>Boolean value; true if strings are equal (case insensitive); false otherwise.</returns>
bool StringCompare(const std::string& a, const std::string& b);

/// <summary>
/// Clamps a value between minimum and maximum bounds.
/// </summary>
/// <param name="val">Floating point value to clamp.</param>
/// <param name="min">Floating point minimum value.</param>
/// <param name="max">Floating point maximum value.</param>
/// <returns>Floating point value; min if val < min, max if val > max, val otherwise.</returns>
float Clamp(float val, float min, float max);