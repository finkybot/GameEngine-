/////////////////////////////////
// TileMap.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "TileMap.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cctype>
/////////////////////////////////



/////////////////////////////////
// ParseJSONInt - A simple JSON parsing function that extracts an integer value associated with a specified key from a JSON-formatted string. The function searches for the key in the string, locates the corresponding value, and attempts to parse it as an integer, 
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
// ParseJSONDouble - Similar to ParseJSONInt, this function extracts a double value associated with a specified key from a JSON-formatted string. It locates the key, finds the corresponding value, and attempts to parse it as a double using std::stod, while handling optional whitespace.
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
// ParseJSONString - This function extracts a string value associated with a specified key from a JSON-formatted string. It searches for the key, locates the corresponding value, and attempts to parse it as a string by finding the enclosing double quotes and handling escaped characters.
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
// ParseJSONArrayFromPos - A helper function that parses an array of integers from a JSON-formatted string starting at a specified position. It checks for the opening bracket, iterates through the elements while handling whitespace and commas, 
// and populates the output vector with the parsed integer values.
static bool ParseJSONArrayFromPos(const std::string& s, size_t startBracketPos, std::vector<int>& out,
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
// ParseJSONIntArray - This function combines the functionality of finding a key in a JSON-formatted string and parsing the associated array of integers. It locates the key, finds the opening bracket for the array, and then calls ParseJSONArrayFromPos to 
// extract the integer values into the output vector.
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
		while (pos < s.size() && isspace((unsigned char)s[pos]))
			++pos;
		if (pos >= s.size())
			break;
		if (s[pos] == ']') {
			++pos;
			break;
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
	return true;
}
/////////////////////////////////



/////////////////////////////////
// ParseJSONLayers - This function parses the "layers" array from a JSON-formatted string and populates a vector of TileMap::Layer objects. It locates the "layers" key, iterates through each layer object in the array, extracts the layer name and tile data, 
// and adds the parsed layers to the output vector.
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
		auto objStart = s.find('{', cur);
		if (objStart == std::string::npos)
			break;
		auto objEnd = s.find('}', objStart);
		if (objEnd == std::string::npos)
			break;
		TileMap::Layer layer;
		std::string name;
		if (ParseJSONString(s, "name", name, objStart))
			layer.name = name;
		auto tilesPos = s.find("\"tiles\"", objStart);
		if (tilesPos != std::string::npos && tilesPos < objEnd) {
			auto bracket = s.find('[', tilesPos);
			if (bracket != std::string::npos && bracket < objEnd) {
				size_t after;
				if (!ParseJSONArrayFromPos(s, bracket, layer.tiles, after))
					return false;
			}
		}
		out.push_back(std::move(layer));
		cur = objEnd + 1;
		auto nextComma = s.find(',', cur);
		auto nextClose = s.find(']', cur);
		if (nextClose == std::string::npos)
			break;
		if (nextComma == std::string::npos || nextComma > nextClose)
			break;
	}
	return !out.empty();
}
/////////////////////////////////



/////////////////////////////////
// SaveToJSON - Saves the tile map data to a JSON file at the specified path. The method serializes the tile map's properties, including dimensions, tile size, tileset metadata, and layer information (if present), into a JSON format and writes it to the file. 
// It returns true if saving is successful, or false if an error occurs, with an error message provided in outErr.
bool TileMap::SaveToJSON(const std::string& path, std::string* outErr) const {
	std::ofstream os(path, std::ios::binary);
	if (!os) {
		if (outErr)
			*outErr = "Failed to open file for writing";
		return false;
	}

	os << "{\n";
	os << "  \"version\": 1,\n";
	os << "  \"width\": " << width << ",\n";
	os << "  \"height\": " << height << ",\n";
	os << std::fixed << std::setprecision(6);
	os << "  \"tileSize\": " << tileSize << ",\n";

	if (!tilesetKey.empty())
		os << "  \"tilesetKey\": \"" << tilesetKey << "\",\n";
	if (!tilesetImage.empty())
		os << "  \"tilesetImage\": \"" << tilesetImage << "\",\n";
	if (tilesetTileW > 0)
		os << "  \"tilesetTileW\": " << tilesetTileW << ",\n";
	if (tilesetTileH > 0)
		os << "  \"tilesetTileH\": " << tilesetTileH << ",\n";

	if (!layers.empty()) {
		os << "  \"layers\": [\n";
		for (size_t li = 0; li < layers.size(); ++li) {
			const auto& layer = layers[li];
			os << "    {\n";
			os << "      \"name\": \"" << layer.name << "\",\n";
			os << "      \"tiles\": [";
			const int total = width * height;
			for (int i = 0; i < total; ++i) {
				if (i)
					os << ", ";
				os << layer.tiles[i];
			}
			os << "]\n";
			os << "    }";
			if (li + 1 < layers.size())
				os << ",\n";
			else
				os << "\n";
		}
		os << "  ],\n";
	} else {
		os << "  \"tiles\": [";
		const int total = width * height;
		for (int i = 0; i < total; ++i) {
			if (i)
				os << ", ";
			os << tiles[i];
		}
		os << "]\n";
	}
	os << "}\n";
	return true;
}
/////////////////////////////////



/////////////////////////////////
// LoadFromJSON - Loads tile map data from a JSON file at the specified path. The method reads the file, parses the JSON content to extract the tile map's properties, including dimensions, tile size, tileset metadata, and layer information (if present), and constructs a TileMap object.
std::optional<TileMap> TileMap::LoadFromJSON(const std::string& path, std::string* outErr) {
	std::ifstream is(path, std::ios::binary);
	if (!is) {
		if (outErr)
			*outErr = "Failed to open file for reading";
		return std::nullopt;
	}
	std::ostringstream ss;
	ss << is.rdbuf();
	const std::string s = ss.str();

	int w = 0, h = 0;
	double ts = 0.0;
	if (!ParseJSONInt(s, "width", w) || !ParseJSONInt(s, "height", h) || !ParseJSONDouble(s, "tileSize", ts)) {
		if (outErr)
			*outErr = "Failed to parse width/height/tileSize from JSON";
		return std::nullopt;
	}

	std::vector<TileMap::Layer> parsedLayers;
	if (ParseJSONLayers(s, parsedLayers)) {
		if (!parsedLayers.empty()) {
			if ((int)parsedLayers[0].tiles.size() != w * h) {
				if (outErr)
					*outErr = "Layer 0 tiles count does not match width*height";
				return std::nullopt;
			}
			TileMap map(w, h, static_cast<float>(ts));
			map.tiles = std::move(parsedLayers[0].tiles);
			map.layers = std::move(parsedLayers);
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

	std::vector<int> tileArr;
	if (!ParseJSONIntArray(s, "tiles", tileArr)) {
		if (outErr)
			*outErr = "Failed to parse tiles array from JSON";
		return std::nullopt;
	}
	if ((int)tileArr.size() != w * h) {
		if (outErr)
			*outErr = "Tiles count does not match width*height";
		return std::nullopt;
	}
	TileMap map(w, h, static_cast<float>(ts));
	map.tiles = std::move(tileArr);
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
