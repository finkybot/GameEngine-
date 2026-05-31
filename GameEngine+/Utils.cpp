/////////////////////////////////
// Utils.cpp - utility functions for the game engine, including floating-point comparisons, string comparisons, file reading, and JSON parsing for tile maps. These functions provide common utilities that can be used throughout the game engine for various purposes such as math 
// operations, string handling, and data serialization/deserialization.
/////////////////////////////////
 
 

/////////////////////////////////
// Includes 
#include "Utils.h"
#include "TileMap.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <iostream>

#include <fstream>
#include <sstream>
#include <iomanip>
/////////////////////////////////



/////////////////////////////////
// IsEqual - Utility function to compare two floating-point values for equality within a specified tolerance (EPSILON). This function is essential for handling the imprecision of floating-point arithmetic, allowing for reliable comparisons in game 
// logic and rendering calculations where exact equality may not be possible due to rounding errors.
bool IsEqual(float val1, float val2) {
	return fabsf(val1 - val2) < EPSILON;
}
/////////////////////////////////



/////////////////////////////////
// ParseJSONString - A simple ad-hoc JSON string parser that extracts the value of a specified key from a JSON-formatted string. This function searches for the key in the string, locates the corresponding value enclosed in double quotes, and returns it as an output parameter.
static bool ParseJSONString(const std::string& s, const std::string& key, std::string& out, size_t startPos = 0) {
	auto pos = s.find('"' + key + '"', startPos);
	if (pos == std::string::npos)
		return false;
	pos = s.find(':', pos);
	if (pos == std::string::npos)
		return false;
	pos = s.find('"', pos);
	if (pos == std::string::npos)
		return false;
	auto endq = s.find('"', pos + 1);
	if (endq == std::string::npos)
		return false;
	out = s.substr(pos + 1, endq - (pos + 1));
	return true;
}
/////////////////////////////////



/////////////////////////////////
// ParseJSONIntArrayFromPos - A simple ad-hoc JSON array parser that extracts an array of integers from a JSON-formatted string starting from a specified position. This function checks for the opening bracket, then iterates through the characters to parse integer values, 
// handling optional whitespace and commas, until it reaches the closing bracket.
static bool ParseJSONIntArrayFromPos(const std::string& s, size_t startBracketPos, std::vector<int>& out,
										  size_t& outPos) {
	if (startBracketPos >= s.size() || s[startBracketPos] != '[')
		return false;
	size_t pos = startBracketPos + 1;
	out.clear();
	while (pos < s.size()) {
		while (pos < s.size() && isspace((unsigned char)s[pos]))
			++pos;
		if (pos >= s.size())
			break;
		if (s[pos] == ']') {
			outPos = pos + 1;
			return true;
		}
		bool neg = false;
		if (s[pos] == '-') {
			neg = true;
			++pos;
		}
		if (pos >= s.size() || !isdigit((unsigned char)s[pos]))
			return false;
		int val = 0;
		while (pos < s.size() && isdigit((unsigned char)s[pos])) {
			val = val * 10 + (s[pos] - '0');
			++pos;
		}
		if (neg)
			val = -val;
		out.push_back(val);
		while (pos < s.size() && isspace((unsigned char)s[pos]))
			++pos;
		if (pos < s.size() && s[pos] == ',')
			++pos;
	}
	return false;
}
/////////////////////////////////



/////////////////////////////////
// ParseJSONLayers - A function to parse the "layers" array from a JSON-formatted string representing a tile map. This function looks for the "layers" key, then iterates through each layer object in the array, extracting the layer name and its corresponding tiles array. 
// The parsed layers are stored in an output vector: expects "layers": [ { "name": "..", "tiles": [..] }, ... ]
static bool ParseJSONLayers(const std::string& s, std::vector<TileMap::Layer>& out) {
	out.clear();
	auto pos = s.find("\"layers\"");
	if (pos == std::string::npos)
		return false;
	pos = s.find('[', pos);
	if (pos == std::string::npos)
		return false;
	size_t cur = pos + 1;
	while (cur < s.size()) {
		// find next '{'
		auto objStart = s.find('{', cur);
		if (objStart == std::string::npos)
			break;
		auto objEnd = s.find('}', objStart);
		if (objEnd == std::string::npos)
			break;
		TileMap::Layer layer;
		// parse name inside object
		std::string name;
		if (ParseJSONString(s, "name", name, objStart))
			layer.name = name;
		// find tiles array inside object
		auto tilesPos = s.find("\"tiles\"", objStart);
		if (tilesPos != std::string::npos && tilesPos < objEnd) {
			auto bracket = s.find('[', tilesPos);
			if (bracket != std::string::npos && bracket < objEnd) {
				size_t after;
				if (!ParseJSONIntArrayFromPos(s, bracket, layer.tiles, after))
					return false;
			}
		}
		out.push_back(std::move(layer));
		cur = objEnd + 1;
		// advance to next comma or closing bracket
		auto nextComma = s.find(',', cur);
		auto nextClose = s.find(']', cur);
		if (nextClose == std::string::npos)
			break;
		if (nextComma == std::string::npos || nextComma > nextClose)
			break; // no more objects
	}
	return !out.empty();
}
/////////////////////////////////



/////////////////////////////////
// SaveTileMapJSON - A function to save a TileMap object to a JSON-formatted file. This function writes the tile map's properties such as width, height, tile size, and optionally tileset metadata and layers to a file in a structured JSON format. 
// The function handles both the case where the tile map has multiple layers and the fallback case where it has a single tiles array.
bool SaveTileMapJSON(const TileMap& map, const std::string& path, std::string* outErr) {
	std::ofstream os(path, std::ios::binary);
	if (!os) {
		if (outErr)
			*outErr = "Failed to open file for writing";
		return false;
	}

	os << "{\n";
	os << "  \"version\": 1,\n";
	os << "  \"width\": " << map.width << ",\n";
	os << "  \"height\": " << map.height << ",\n";
	os << std::fixed << std::setprecision(6);
	os << "  \"tileSize\": " << map.tileSize << ",\n";
	// tileset metadata
	if (!map.tilesetKey.empty())
		os << "  \"tilesetKey\": \"" << map.tilesetKey << "\",\n";
	if (!map.tilesetImage.empty())
		os << "  \"tilesetImage\": \"" << map.tilesetImage << "\",\n";
	if (map.tilesetTileW > 0)
		os << "  \"tilesetTileW\": " << map.tilesetTileW << ",\n";
	if (map.tilesetTileH > 0)
		os << "  \"tilesetTileH\": " << map.tilesetTileH << ",\n";

	// layers support: if present, write layers array
	if (!map.layers.empty()) {
		os << "  \"layers\": [\n";
		for (size_t li = 0; li < map.layers.size(); ++li) {
			const auto& layer = map.layers[li];
			os << "    {\n";
			os << "      \"name\": \"" << layer.name << "\",\n";
			os << "      \"tiles\": [";
			const int total = map.width * map.height;
			for (int i = 0; i < total; ++i) {
				if (i)
					os << ", ";
				os << layer.tiles[i];
			}
			os << "]\n";
			os << "    }";
			if (li + 1 < map.layers.size())
				os << ",\n";
			else
				os << "\n";
		}
		os << "  ],\n";
	} else {
		// fallback single tiles array
		os << "  \"tiles\": [";
		const int total = map.width * map.height;
		for (int i = 0; i < total; ++i) {
			if (i)
				os << ", ";
			os << map.tiles[i];
		}
		os << "]\n";
	}
	os << "}\n";
	return true;
}
/////////////////////////////////



/////////////////////////////////
// ParseJSONInt - A simple ad-hoc JSON integer parser that extracts the value of a specified key from a JSON-formatted string. This function searches for the key in the string, locates the corresponding value after the colon, and parses it as an integer, 
// handling optional whitespace and negative signs.
static bool ParseJSONInt(const std::string& s, const std::string& key, int& out) {
	auto pos = s.find('"' + key + '"');
	if (pos == std::string::npos)
		return false;
	pos = s.find(':', pos);
	if (pos == std::string::npos)
		return false;
	++pos;
	while (pos < s.size() && isspace((unsigned char)s[pos]))
		++pos;
	bool neg = false;
	if (s[pos] == '-') {
		neg = true;
		++pos;
	}
	int val = 0;
	bool any = false;
	while (pos < s.size() && isdigit((unsigned char)s[pos])) {
		any = true;
		val = val * 10 + (s[pos] - '0');
		++pos;
	}
	if (!any)
		return false;
	out = neg ? -val : val;
	return true;
}
/////////////////////////////////



/////////////////////////////////
// ParseJSONDouble - A simple ad-hoc JSON double parser that extracts the value of a specified key from a JSON-formatted string. This function searches for the key in the string, locates the corresponding value after the colon, and parses it as a double using std::stod,
static bool ParseJSONDouble(const std::string& s, const std::string& key, double& out) {
	auto pos = s.find('"' + key + '"');
	if (pos == std::string::npos)
		return false;
	pos = s.find(':', pos);
	if (pos == std::string::npos)
		return false;
	++pos;
	while (pos < s.size() && isspace((unsigned char)s[pos]))
		++pos;
	std::size_t end;
	try {
		out = std::stod(s.substr(pos), &end);
		return true;
	} catch (...) {
		return false;
	}
}
/////////////////////////////////



/////////////////////////////////
// ParseJSONIntArray - A simple ad-hoc JSON array parser that extracts an array of integers from a JSON-formatted string based on a specified key. This function searches for the key in the string, locates the corresponding array enclosed in square brackets, 
// and parses the integer values within it,
static bool ParseJSONIntArray(const std::string& s, const std::string& key, std::vector<int>& out) {
	auto pos = s.find('"' + key + '"');
	if (pos == std::string::npos)
		return false;
	pos = s.find('[', pos);
	if (pos == std::string::npos)
		return false;
	++pos;
	out.clear();
	while (pos < s.size()) {
		// skip whitespace
		while (pos < s.size() && isspace((unsigned char)s[pos]))
			++pos;
		if (pos >= s.size())
			break;
		if (s[pos] == ']') {
			++pos;
			break;
		}
		// parse int (allow negative)
		bool neg = false;
		if (s[pos] == '-') {
			neg = true;
			++pos;
		}
		if (pos >= s.size() || !isdigit((unsigned char)s[pos]))
			return false;
		int val = 0;
		while (pos < s.size() && isdigit((unsigned char)s[pos])) {
			val = val * 10 + (s[pos] - '0');
			++pos;
		}
		if (neg)
			val = -val;
		out.push_back(val);
		// skip whitespace and optional comma
		while (pos < s.size() && isspace((unsigned char)s[pos]))
			++pos;
		if (pos < s.size() && s[pos] == ',')
			++pos;
	}
	return true;
}
/////////////////////////////////



/////////////////////////////////
// LoadTileMapJSON - A function to load a TileMap object from a JSON-formatted file. This function reads the file content, parses the JSON string to extract the tile map's properties such as width, height, tile size, and optionally tileset metadata and layers.
std::optional<TileMap> LoadTileMapJSON(const std::string& path, std::string* outErr) {
	std::ifstream is(path, std::ios::binary);
	if (!is) {
		if (outErr)
			*outErr = "Failed to open file for reading";
		return std::nullopt;
	}
	std::ostringstream ss;
	ss << is.rdbuf();
	const std::string s = ss.str();

	int width = 0, height = 0;
	double tileSize = 0.0;
	if (!ParseJSONInt(s, "width", width) || !ParseJSONInt(s, "height", height) ||
		!ParseJSONDouble(s, "tileSize", tileSize)) {
		if (outErr)
			*outErr = "Failed to parse width/height/tileSize from JSON";
		return std::nullopt;
	}

	// try to parse layers first
	std::vector<TileMap::Layer> layers;
	if (ParseJSONLayers(s, layers)) {
		// use first layer as primary tiles for logic (collision) if exists
		if (!layers.empty()) {
			if ((int)layers[0].tiles.size() != width * height) {
				if (outErr)
					*outErr = "Layer 0 tiles count does not match width*height";
				return std::nullopt;
			}
			TileMap map(width, height, static_cast<float>(tileSize));

			map.tiles = std::move(layers[0].tiles);
			map.layers = std::move(layers);
			// optional tileset metadata

			ParseJSONString(s, "tilesetKey", map.tilesetKey);
			ParseJSONString(s, "tilesetImage", map.tilesetImage);
			int tw = 0, th = 0;

			ParseJSONInt(s, "tilesetTileW", tw);
			ParseJSONInt(s, "tilesetTileH", th);

			map.tilesetTileW = tw;
			map.tilesetTileH = th;

			return map;
		}
	}

	// fallback: single 'tiles' array
	std::vector<int> tiles;
	if (!ParseJSONIntArray(s, "tiles", tiles)) {
		if (outErr)
			*outErr = "Failed to parse tiles array from JSON";
		return std::nullopt;
	}

	if ((int)tiles.size() != width * height) {
		if (outErr)
			*outErr = "Tiles count does not match width*height";
		return std::nullopt;
	}

	TileMap map(width, height, static_cast<float>(tileSize));
	map.tiles = std::move(tiles);
	// optional tileset metadata
	ParseJSONString(s, "tilesetKey", map.tilesetKey);
	ParseJSONString(s, "tilesetImage", map.tilesetImage);

	int tw = 0, th = 0;

	ParseJSONInt(s, "tilesetTileW", tw);
	ParseJSONInt(s, "tilesetTileH", th);

	map.tilesetTileW = tw;
	map.tilesetTileH = th;
	return map;
}
/////////////////////////////////



/////////////////////////////////
// (TileMap::SaveToJSON / LoadFromJSON implemented in TileMap.cpp)
/////////////////////////////////



/////////////////////////////////
// IsGreaterThanOrEqual and IsLessThanOrEqual - Utility functions to compare two floating-point values for greater than or equal and less than or equal, respectively, using the IsEqual function to account for floating-point imprecision. These functions provide reliable 
// comparisons for game logic and rendering calculations where
bool IsGreaterThanOrEqual(float val1, float val2) {
	return val1 > val2 || IsEqual(val1, val2);
}

bool IsLessThanOrEqual(float val1, float val2) {
	return val1 < val2 || IsEqual(val1, val2);
}
/////////////////////////////////



/////////////////////////////////
// MillisecondsToSeconds - A utility function to convert a time value from milliseconds to seconds. This function takes an unsigned integer representing milliseconds and returns a floating-point value representing the equivalent time in seconds, which is commonly used 
// in game timing and animation calculations.
float MillisecondsToSeconds(unsigned int milliseconds) {
	return static_cast<float>(milliseconds) / 1000.0f;
}
/////////////////////////////////



/////////////////////////////////
// GetIndex - A utility function to calculate the index in a one-dimensional array representing a two-dimensional grid. This function takes the width of the grid, the row and column coordinates, and returns the corresponding index in the array, which is commonly used for
unsigned int GetIndex(unsigned int width, unsigned int row, unsigned int col) {
	return row * width + col;
}
/////////////////////////////////



/////////////////////////////////
// StringCompare - A utility function to compare two strings for equality in a case-insensitive manner. This function checks if the lengths of the two strings are equal, and if so, it uses std::equal with a custom comparison function that converts characters 
// to lowercase before comparing them.
bool StringCompare(const std::string& a, const std::string& b) {
	if (a.length() == b.length()) {
		return std::equal(b.begin(), b.end(), a.begin(),
						  [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
	}
	return false;
}
/////////////////////////////////



/////////////////////////////////
// Clamp - A utility function to clamp a floating-point value between a specified minimum and maximum range. This function checks if the value exceeds the maximum or is below the minimum, and returns the appropriate boundary value if so; otherwise, it returns the original value.
float Clamp(float val, float min, float max) {
	if (val > max) {
		return max;
	} else if (val < min) {
		return min;
	}
	return val;
}
/////////////////////////////////



/////////////////////////////////
// ReadFile - A utility function to read the contents of a file into a dynamically allocated character buffer. This function takes the file path as input, attempts to open the file, and if successful, reads its contents into a buffer that is null-terminated. 
// The caller is responsible for freeing the allocated memory when it is no longer needed.
const char* ReadFile(const char* filePath) {
	FILE* file = nullptr;
	fopen_s(&file, filePath, "r");

	if (!file) {
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
/////////////////////////////////