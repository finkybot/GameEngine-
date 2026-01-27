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


// Utiltity functions.
bool IsEqual(float val1, float val2);

bool IsGreaterThanOrEqual(float val1, float val2);

bool IsLessThanOrEqual(float val1, float val2);

float MillisecondsToSeconds(unsigned int milliseconds);

unsigned int GetIndex(unsigned int width, unsigned int row, unsigned int col);

bool StringCompare(const std::string& a, const std::string& b);

float Clamp(float val, float min, float max);