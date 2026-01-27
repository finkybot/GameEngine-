// Utils.cpp


// Includes.
#include"Utils.h"
#include <cmath>
#include <algorithm>
#include <cctype>


// Function definitions.

/// <summary>
/// Returns a comparison; if the float val1 is equal to the float val2 (to the tolerence EPSILON).
/// </summary>
/// <param name="val1"> - Floating point value.</param>
/// <param name="val2"> - Floating point value.</param>
/// <returns>Boolean value; true if floats are equal within the EPSILON tolerence.</returns>
bool IsEqual(float val1, float val2)
{
	return fabsf(val1 - val2) < EPSILON;
}


/// <summary>
/// Returns a comparision; if the float val1 is greater than or equal to the float val2 (to the tolerence EPSILON); returns true if value 1 is greater than value 2; otherwise returns false.
/// </summary>
/// <param name="val1"> - Floating point value.</param>
/// <param name="val2"> - Floating point value.</param>
/// <returns>Boolean value.</returns>
bool IsGreaterThanOrEqual(float val1, float val2)
{
	return val1 > val2 || IsEqual(val1, val2);
}


/// <summary>
/// Returns a comparison; if the float val1 is less than or equal to the float val2 (to the tolerence EPSILON); return true if value 1 is less than value 2; otherwise return false.
/// </summary>
/// <param name="val1"> - Floating point value.</param>
/// <param name="val2"> - Floating point value.</param>
/// <returns>Boolean value.</returns>
bool IsLessThanOrEqual(float val1, float val2)
{
	return val1 < val2 || IsEqual(val1, val2);
}


/// <summary>
/// Converts a value of milliseconds into seconds and returns the seconds value as a float.
/// </summary>
/// <param name="milliseconds">Unsigned integer; time in milliseconds.</param>
/// <returns>Floating point; time in seconds.</returns>
float MillisecondsToSeconds(unsigned int milliseconds)
{
	return static_cast<float>(milliseconds) / 1000.0f;
}


/// <summary>
/// Get an index for a pixel (at a given position) from an BMPImage.
/// </summary>
/// <param name="width"> - Unsigned integer value; Width of BMPImage</param>
/// <param name="row"> - Unsigned integer value.</param>
/// <param name="col"> - Unsigned integer value.</param>
/// <returns>Integer value (row * width + col)</returns>
unsigned int GetIndex(unsigned int width, unsigned int row, unsigned int col)
{
	return row * width + col;
}


/// <summary>
/// Compares two standard strings (case insensitive); returns true they are the same; otherwise returns false.
/// </summary>
/// <param name="a">string for comparing, 'a' string is compared to 'b' string.</param>
/// <param name="b">string for comparing, 'a' string is compared to 'b' string.</param>
/// <returns>Boolean value; true if 'a' string is the same as 'b' string; false otherwise.</returns>
bool StringCompare(const std::string& a, const std::string& b)
{
	if (a.length() == b.length())
	{
		return std::equal(b.begin(), b.end(), a.begin(), [](unsigned char a, unsigned char b){ return std::tolower(a) == std::tolower(b); });
	}

	return false;
}


/// <summary>
/// Returns a value (either minimum or maximum) if the passed value is outside the bounds of the min or max values; otherwise just returns the passed value.
/// </summary>
/// <param name="val">Floating point value; given value to test against the min/max value.</param>
/// <param name="min">Floating point value; minimum value.</param>
/// <param name="max">Floating point value; maximum value.</param>
/// <returns>Floating point value; min if val is less than min, max if val is greater than max, val if val lies between min and max.</returns>
float Clamp(float val, float min, float max)
{
	if (val > max)	// Return the max value if passed value is greater than max.
	{
		return max;
	}
	else if(val < min) // Return the min value if passed value is less than min.
	{
		return min;
	}

	return val;	// Otherwise return the passed value.
}