#include "TileMap.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cctype>

// Minimal ad-hoc JSON helpers dedicated to TileMap serialization.
static bool parse_json_int(const std::string& s, const std::string& key, int& out) {
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

static bool parse_json_double(const std::string& s, const std::string& key, double& out) {
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

static bool parse_json_string(const std::string& s, const std::string& key, std::string& out, size_t startPos = 0) {
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

static bool parse_json_int_array_from_pos(const std::string& s, size_t startBracketPos, std::vector<int>& out,
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

static bool parse_json_int_array(const std::string& s, const std::string& key, std::vector<int>& out) {
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

static bool parse_json_layers(const std::string& s, std::vector<TileMap::Layer>& out) {
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
		if (parse_json_string(s, "name", name, objStart))
			layer.name = name;
		auto tilesPos = s.find("\"tiles\"", objStart);
		if (tilesPos != std::string::npos && tilesPos < objEnd) {
			auto bracket = s.find('[', tilesPos);
			if (bracket != std::string::npos && bracket < objEnd) {
				size_t after;
				if (!parse_json_int_array_from_pos(s, bracket, layer.tiles, after))
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
	if (!parse_json_int(s, "width", w) || !parse_json_int(s, "height", h) || !parse_json_double(s, "tileSize", ts)) {
		if (outErr)
			*outErr = "Failed to parse width/height/tileSize from JSON";
		return std::nullopt;
	}

	std::vector<TileMap::Layer> parsedLayers;
	if (parse_json_layers(s, parsedLayers)) {
		if (!parsedLayers.empty()) {
			if ((int)parsedLayers[0].tiles.size() != w * h) {
				if (outErr)
					*outErr = "Layer 0 tiles count does not match width*height";
				return std::nullopt;
			}
			TileMap map(w, h, static_cast<float>(ts));
			map.tiles = std::move(parsedLayers[0].tiles);
			map.layers = std::move(parsedLayers);
			parse_json_string(s, "tilesetKey", map.tilesetKey);
			parse_json_string(s, "tilesetImage", map.tilesetImage);
			int tw = 0, th = 0;
			parse_json_int(s, "tilesetTileW", tw);
			parse_json_int(s, "tilesetTileH", th);
			map.tilesetTileW = tw;
			map.tilesetTileH = th;
			return map;
		}
	}

	std::vector<int> tileArr;
	if (!parse_json_int_array(s, "tiles", tileArr)) {
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
	parse_json_string(s, "tilesetKey", map.tilesetKey);
	parse_json_string(s, "tilesetImage", map.tilesetImage);
	int tw = 0, th = 0;
	parse_json_int(s, "tilesetTileW", tw);
	parse_json_int(s, "tilesetTileH", th);
	map.tilesetTileW = tw;
	map.tilesetTileH = th;
	return map;
}
