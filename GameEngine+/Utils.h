// ***** Utility function declarations *****
// Contains utility function declarations
#pragma once

// ***** Includes *****
#include <string>
#include <optional>

// ***** Constants *****
static const float EPSILON = 0.0001f; // Tolerence value for floating point value comparisons
const float PI =
	3.14159f; // Its PI, you know, the circle ratio constant!!! Used for various calculations involving angles and rotations in the game engine.
const float TWO_PI =
	2.0f *
	PI; // Precomputed value for 2 * PI, used for efficiency in calculations that require a full circle (e.g., angle normalization).

// ***** Structure declarations *****
// Size struct represents a simple width and height pair, commonly used for dimensions of objects, textures, or other 2D elements in the game engine.
struct Size {
	unsigned int width = 0, height = 0;
};

// ***** Utility functions *****
bool IsEqual(
	float val1,
	float
		val2); // Returns true if the two float values are equal within a small tolerance defined by EPSILON, accounting for floating-point precision issues.
bool IsGreaterThanOrEqual(
	float val1,
	float
		val2); // Returns true if the float val1 is greater than or equal to the float val2 (to the tolerance EPSILON).
bool IsLessThanOrEqual(
	float val1,
	float val2); // Returns true if the float val1 is less than or equal to the float val2 (to the tolerance EPSILON).
float MillisecondsToSeconds(
	unsigned int
		milliseconds); // Converts a time value in milliseconds to seconds by dividing the input value by 1000.0f, returning the result as a float.
unsigned int
GetIndex(unsigned int width, unsigned int row,
		 unsigned int
			 col); // Converts 2D grid coordinates (row, col) into a 1D index based on the provided width of the grid.
bool StringCompare(
	const std::string& a,
	const std::string&
		b); // Compares two strings for equality in a case-insensitive manner. Returns true if the strings are of the same length and contain the same characters regardless of case, otherwise returns false.
float Clamp(
	float val, float min,
	float
		max); // Clamps a float value between a minimum and maximum range; if the value is less than the minimum, the minimum is returned; if the value is greater than the maximum, the maximum is returned; otherwise, the original value is returned.
// Forward declaration for TileMap (defined in TileMap.h)
struct TileMap;

// (TileMap JSON helpers moved into TileMap.*) Use TileMap::SaveToJSON / TileMap::LoadFromJSON
const char* readFile(
	const char*
		filePath); // Can you guess what it does? Really hard to figure it out from the name, I know. Reads the contents of a file specified by the filePath and returns it as a C-style string (const char*). You better take responsiblity for managing the memory of the returned string.
const char* readFile(
	const char*
		filePath); // Can you guess what it does? Really hard to figure it out from the name, I know. Reads the contents of a file specified by the filePath and returns it as a C-style string (const char*). You better take responsiblity for managing the memory of the returned string.