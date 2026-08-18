/////////////////////////////////
// Utils.h - Header file for utility functions and structures used throughout the game engine. This file contains declarations for various
// utility functions and structures that are commonly used across different parts of the game engine.
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for utility functions and structures. We include standard library headers for string and optional types, which are used in some of the utility functions declared in this file.
#pragma once
#include <string>
#include <optional>
/////////////////////////////////



/////////////////////////////////
// Constants for utility functions. These constants are used for various calculations and comparisons in the utility functions, such as floating-point value comparisons and angle calculations.
static const float EPSILON = 0.0001f; // Tolerence value for floating point value comparisons
const float PI = 3.14159f; // Its PI, you know, the circle ratio constant!!! Used for various calculations involving angles and rotations in the game engine.
const float TWO_PI = 2.0f * PI; // Precomputed value for 2 * PI, used for efficiency in calculations that require a full circle (e.g., angle normalization).
/////////////////////////////////



/////////////////////////////////
// Size struct represents a simple width and height pair, commonly used for dimensions of objects, textures, or other 2D elements in the game engine.
//								|
//								|_______________________________________________________________________
struct Size {
	unsigned int width = 0, height = 0;
};
/////////////////////////////////



/////////////////////////////////
// IsEqual - Returns true if the two float values are equal within a small tolerance defined by EPSILON, accounting for floating-point precision issues. This function is used to compare floating-point values in a way that accounts for the inherent imprecision of floating-point arithmetic, 
// allowing for more reliable comparisons in situations where exact equality may not be possible due to rounding errors.
bool IsEqual(float val1, float val2);
/////////////////////////////////



/////////////////////////////////
// IsGreaterThanOrEqual - Returns true if the float val1 is greater than or equal to the float val2, considering a small tolerance defined by EPSILON. This function is used to compare floating-point values in a way that accounts for the inherent imprecision of floating-point arithmetic, 
// allowing for more reliable comparisons when determining if one value is greater than or approximately equal to another.
bool IsGreaterThanOrEqual(float val1, float val2);
/////////////////////////////////



/////////////////////////////////
// IsLessThanOrEqual - Returns true if the float val1 is less than or equal to the float val2, considering a small tolerance defined by EPSILON. This function is used to compare floating-point values in a way that accounts for the inherent imprecision of floating-point arithmetic,
bool IsLessThanOrEqual(float val1, float val2);
/////////////////////////////////



/////////////////////////////////
// DegreesToRadians - Converts an angle from degrees to radians by multiplying the input value by PI and dividing by 180.0f, returning the result as a float. This function is commonly used in calculations involving angles and rotations, as many mathematical functions in C++ expect angles to be in radians.
float MillisecondsToSeconds(unsigned int milliseconds);



/////////////////////////////////
// GetIndex - Converts 2D grid coordinates (row, col) into a 1D index based on the provided width of the grid. This function is used to access elements in a 1D array that represents a 2D grid, allowing for efficient storage and retrieval of grid data without needing to use a 2D array structure.
unsigned int GetIndex(unsigned int width, unsigned int row, unsigned int col);
/////////////////////////////////



/////////////////////////////////
bool StringCompare(const std::string& a, const std::string& b); // Compares two strings for equality in a case-insensitive manner. Returns true if the strings are of the same length and contain the same characters regardless of case, otherwise returns false.
/////////////////////////////////



/////////////////////////////////
float Clamp(float val, float min, float	max); // Clamps a float value between a minimum and maximum range; if the value is less than the minimum, the minimum is returned; if the value is greater than the maximum, the maximum is returned; otherwise, the original value is returned.
/////////////////////////////////
 


/////////////////////////////////
// Forward declaration for TileMap (defined in TileMap.h)
struct TileMap;
/////////////////////////////////



/////////////////////////////////
// ReadFile - Reads the contents of a file specified by the filePath and returns it as a C-style string (const char*). The caller is responsible for managing the memory of the returned string, which should be freed when no longer needed. 
// This function is used to read text files, such as JSON files for tile maps, and return their contents for further processing.
// (TileMap JSON helpers moved into TileMap.*) Use TileMap::SaveToJSON / TileMap::LoadFromJSON
const char* ReadFile(const char* filePath); // Can you guess what it does? Really hard to figure it out from the name, I know. Reads the contents of a file specified by the filePath and returns it as a C-style string (const char*). You better take responsiblity for managing the memory of the returned string.
/////////////////////////////////