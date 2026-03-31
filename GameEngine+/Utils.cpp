// Utils.cpp

// ***** Includes *****
#include"Utils.h"
#include "Raycast.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <iostream>

#include <fstream>
#include <sstream>




// ***** Utility Function definitions *****
bool IsEqual(float val1, float val2)
{
	return fabsf(val1 - val2) < EPSILON;
}

// --- Ad-hoc JSON save/load for TileMap ---

bool SaveTileMapJSON(const TileMap& map, const std::string& path, std::string* outErr)
{
	std::ofstream os(path, std::ios::binary);
	if (!os) {
		if (outErr) *outErr = "Failed to open file for writing";
		return false;
	}

	os << "{\n";
	os << "  \"version\": 1,\n";
	os << "  \"width\": " << map.width << ",\n";
	os << "  \"height\": " << map.height << ",\n";
	os << std::fixed << std::setprecision(6);
	os << "  \"tileSize\": " << map.tileSize << ",\n";
	os << "  \"tiles\": [";

	const int total = map.width * map.height;
	for (int i = 0; i < total; ++i) {
		if (i) os << ", ";
		os << map.tiles[i];
	}
	os << "]\n";
	os << "}\n";
	return true;
}

// very small ad-hoc parser
static bool parse_json_int(const std::string& s, const std::string& key, int& out) {
	auto pos = s.find('"' + key + '"');
	if (pos == std::string::npos) return false;
	pos = s.find(':', pos);
	if (pos == std::string::npos) return false;
	++pos;
	while (pos < s.size() && isspace((unsigned char)s[pos])) ++pos;
	bool neg = false;
	if (s[pos] == '-') { neg = true; ++pos; }
	int val = 0; bool any = false;
	while (pos < s.size() && isdigit((unsigned char)s[pos])) { any = true; val = val * 10 + (s[pos]-'0'); ++pos; }
	if (!any) return false;
	out = neg ? -val : val;
	return true;
}

static bool parse_json_double(const std::string& s, const std::string& key, double& out) {
	auto pos = s.find('"' + key + '"');
	if (pos == std::string::npos) return false;
	pos = s.find(':', pos);
	if (pos == std::string::npos) return false;
	++pos;
	while (pos < s.size() && isspace((unsigned char)s[pos])) ++pos;
	std::size_t end;
	try {
		out = std::stod(s.substr(pos), &end);
		return true;
	} catch (...) {
		return false;
	}
}

static bool parse_json_int_array(const std::string& s, const std::string& key, std::vector<int>& out) {
	auto pos = s.find('"' + key + '"');
	if (pos == std::string::npos) return false;
	pos = s.find('[', pos);
	if (pos == std::string::npos) return false;
	++pos;
	out.clear();
	while (pos < s.size()) {
		// skip whitespace
		while (pos < s.size() && isspace((unsigned char)s[pos])) ++pos;
		if (pos >= s.size()) break;
		if (s[pos] == ']') { ++pos; break; }
		// parse int (allow negative)
		bool neg = false;
		if (s[pos] == '-') { neg = true; ++pos; }
		if (pos >= s.size() || !isdigit((unsigned char)s[pos])) return false;
		int val = 0;
		while (pos < s.size() && isdigit((unsigned char)s[pos])) { val = val*10 + (s[pos]-'0'); ++pos; }
		if (neg) val = -val;
		out.push_back(val);
		// skip whitespace and optional comma
		while (pos < s.size() && isspace((unsigned char)s[pos])) ++pos;
		if (pos < s.size() && s[pos] == ',') ++pos;
	}
	return true;
}

std::optional<TileMap> LoadTileMapJSON(const std::string& path, std::string* outErr)
{
	std::ifstream is(path, std::ios::binary);
	if (!is) {
		if (outErr) *outErr = "Failed to open file for reading";
		return std::nullopt;
	}
	std::ostringstream ss;
	ss << is.rdbuf();
	const std::string s = ss.str();

	int width=0, height=0;
	double tileSize=0.0;
	if (!parse_json_int(s, "width", width) || !parse_json_int(s, "height", height) || !parse_json_double(s, "tileSize", tileSize)) {
		if (outErr) *outErr = "Failed to parse width/height/tileSize from JSON";
		return std::nullopt;
	}

	std::vector<int> tiles;
	if (!parse_json_int_array(s, "tiles", tiles)) {
		if (outErr) *outErr = "Failed to parse tiles array from JSON";
		return std::nullopt;
	}

	if ((int)tiles.size() != width * height) {
		if (outErr) *outErr = "Tiles count does not match width*height";
		return std::nullopt;
	}

	TileMap map(width, height, static_cast<float>(tileSize));
	map.tiles = std::move(tiles);
	return map;
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