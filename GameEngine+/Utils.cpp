// Utils.cpp

// ***** Includes *****
#include"Utils.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <iostream>



// ***** Utility Function definitions *****
bool IsEqual(float val1, float val2)
{
	return fabsf(val1 - val2) < EPSILON;
}

bool IsGreaterThanOrEqual(float val1, float val2)
{
	return val1 > val2 || IsEqual(val1, val2);
}

bool IsLessThanOrEqual(float val1, float val2)
{
	return val1 < val2 || IsEqual(val1, val2);
}


float MillisecondsToSeconds(unsigned int milliseconds)
{
	return static_cast<float>(milliseconds) / 1000.0f;
}

unsigned int GetIndex(unsigned int width, unsigned int row, unsigned int col)
{
	return row * width + col;
}

bool StringCompare(const std::string& a, const std::string& b)
{
	if (a.length() == b.length())
	{
		return std::equal(b.begin(), b.end(), a.begin(), [](unsigned char a, unsigned char b){ return std::tolower(a) == std::tolower(b); });
	}

	return false;
}

float Clamp(float val, float min, float max)
{
	if (val > max)
	{
		return max;
	}
	else if(val < min)
	{
		return min;
	}

	return val;
}

const char* readFile(const char* filePath)
{
	FILE* file = nullptr;
	fopen_s(&file, filePath, "r");

	if (!file)
	{
		std::cerr << "Failed to open file: " << filePath << std::endl;
		return nullptr;
	}

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);

	rewind(file);
	char* buffer = new char[size + 1];
	fread(buffer, sizeof(char), size, file);
	buffer[size] = '\0'; // Null-terminate the string

	fclose(file);

	return buffer;
}