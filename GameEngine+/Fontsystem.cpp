/////////////////////////////////
// Fontsystem.cpp - Implementation of the Fontsystem class
/////////////////////////////////



/////////////////////////////////
// Includes
#include "Fontsystem.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
/////////////////////////////////



/////////////////////////////////
// Fontsystem Implementation
bool Fontsystem::LoadBMFont(const std::string& fontName, const std::string& filePath, const std::string& pngPath) {
	
	// Create a new FontAsset and populate it with data
	FontAsset font;
	font.name = fontName;
	font.type = FontType::BMFont;

	// Parse the BMFont text file to extract font metrics and glyph data
	if (!ParseBMFontText(filePath, font)) {
		std::cerr << "[Fontsystem] Failed to parse BMFont text file: " << filePath << std::endl;
		return false;
	}

	// Validate the parsed font metrics to ensure they are positive values
	if (font.scaleW <= 0 || font.scaleH <= 0) {
		std::cerr << "[FontSystem] Invalid BMFont atlas size: " << font.scaleW << "x" << font.scaleH << "\n";
		return false;
	}
	
	// Load the associated texture for the BMFont
	if (!LoadBMFontTexture(pngPath, font)) {
		std::cerr << "[Fontsystem] Failed to load BMFont texture: " << pngPath << std::endl;
		return false;
	}

	// Add the loaded font to the font map
	m_fonts[fontName] = font;

	// Successfully loaded the font
	return true;
}
/////////////////////////////////



/////////////////////////////////
// GetFont - Retrieves a font asset by its name. Returns a pointer to the FontAsset if found, nullptr otherwise.
const FontAsset* Fontsystem::GetFont(const std::string& fontName) const {
	auto it = m_fonts.find(fontName);
	
	// If the font is found in the map, return a pointer to the FontAsset; otherwise, return nullptr
	if (it != m_fonts.end()) {
		return &(it->second); // Return a pointer to the found FontAsset
	}
	return nullptr;
}
/////////////////////////////////



/////////////////////////////////
// GetKerning - Retrieves the kerning amount for a specific pair of characters in a given font. Returns the kerning amount if found, 0.0f otherwise.
float Fontsystem::GetKerning(const FontAsset& font, int firstCharID, int secondCharID) const {
	// Iterate through the kerning pairs in the font asset
	for (const auto& pair : font.kerning) {
		if (pair.first == firstCharID && pair.second == secondCharID) {
			return pair.amount; // Return the kerning amount if the pair is found
		}
	}

	return 0.0f;
}
/////////////////////////////////



/////////////////////////////////
// GetGlyph - Retrieves a pointer to the Glyph for a specific character ID in a given font. Returns nullptr if the glyph is not found.
const Glyph* Fontsystem::GetGlyph(const FontAsset& font, int charID) const {
	auto it = font.glyphs.find(charID);
	if (it != font.glyphs.end()) {
		return &(it->second); // Return a pointer to the found Glyph
	}

	// Try fallback glyph if the requested glyph is not found
	auto fallback = font.glyphs.find(font.fallbackGlyph);

	// If the fallback glyph is found, return a pointer to it
	if (fallback != font.glyphs.end()) {
		return &(fallback->second); // Return a pointer to the fallback Glyph
	}

	// If neither the requested glyph nor the fallback glyph is found, return nullptr
	return nullptr;
}
/////////////////////////////////



/////////////////////////////////
// GetLoadedFonts - Retrieves a list of names of all loaded font assets.
std::vector<std::string> Fontsystem::GetLoadedFonts() const {
	// Create a vector to hold the names of loaded fonts
	std::vector<std::string> fontNames;
	fontNames.reserve(m_fonts.size());

	// Iterate through the font map and collect the names of loaded fonts
	for (const auto& pair : m_fonts) {
		fontNames.push_back(pair.first);
	}

	// Return the list of loaded font names
	return fontNames;
}
/////////////////////////////////



/////////////////////////////////
// ParseBMFontText - Parses a BMFont text file and populates the FontAsset with glyph + kerning data.
bool Fontsystem::ParseBMFontText(const std::string& filePath, FontAsset& fontAsset) {
	// Open the BMFont text file for reading
	std::ifstream file(filePath);

	// Check if the file was successfully opened
	if (!file.is_open()) {
		std::cerr << "[Fontsystem] Failed to open BMFont text file: " << filePath << std::endl;
		return false;
	}

	// Read the file line by line and parse the relevant data
	std::string line;

	// Iterate through each line of the file
	while (std::getline(file, line)) {

		// ---------------------------------------
		// Parse "common" block (atlas + metrics)
		// ---------------------------------------
		
		// Check if the line contains the "common" block, which contains font metrics and atlas size
		if (line.find("common") != std::string::npos) {
			// Use a stringstream to tokenize the line and extract key-value pairs
			std::stringstream ss(line);
			std::string token;

			// Iterate through each token in the line
			while (ss >> token) {
				// Find the position of the '=' character to separate key and value
				auto pos = token.find('=');

				// If '=' is not found, skip this token
				if (pos == std::string::npos) continue;

				// Extract the key and value from the token
				std::string key = token.substr(0, pos);
				float value = std::stof(token.substr(pos + 1));

				// Assign the extracted values to the corresponding fields in the FontAsset
				if (key == "lineHeight")				fontAsset.lineHeight = value;
				if (key == "base")						fontAsset.base = value;
				if (key == "scaleW")					fontAsset.scaleW = value;
				if (key == "scaleH")					fontAsset.scaleH = value;
			}
		}

		// ---------------------------------------
		// Parse glyph definitions
		// ---------------------------------------

		// Check if the line contains a glyph definition (starts with "char id")
		if (line.find("char id") != std::string::npos) {
			// Create a new Glyph object to hold the parsed data
			Glyph glyph{};
			std::stringstream ss(line);
			std::string token;
			
			// Iterate through each token in the line
			while (ss >> token) {
				// Find the position of the '=' character to separate key and value
				auto pos = token.find('=');

				// If '=' is not found, skip this token
				if (pos == std::string::npos) continue;

				// Extract the key and value from the token
				std::string key = token.substr(0, pos);
				float value = std::stof(token.substr(pos + 1));

				// Assign the extracted values to the corresponding fields in the Glyph object
				if (key == "id")					glyph.id = static_cast<int>(value);
				if (key == "x")						glyph.x = value;
				if (key == "y")						glyph.y = value;
				if (key == "width")					glyph.w = value;
				if (key == "height")				glyph.h = value;
				if (key == "xoffset")				glyph.xoffset = value;
				if (key == "yoffset")				glyph.yoffset = value;
				if (key == "xadvance")				glyph.xadvance = value;
			}

			// Compute UVs
			glyph.u0 = glyph.x / fontAsset.scaleW;
			glyph.v0 = glyph.y / fontAsset.scaleH;
			glyph.u1 = (glyph.x + glyph.w) / fontAsset.scaleW;
			glyph.v1 = (glyph.y + glyph.h) / fontAsset.scaleH;

			// Add the parsed glyph to the FontAsset's glyph map
			fontAsset.glyphs[glyph.id] = glyph;
		}

		// ---------------------------------------
		// Parse kerning pairs
		// ---------------------------------------

		// Check if the line contains a kerning pair definition (starts with "kerning")
		if (line.find("kerning") != std::string::npos) {
			// Create a new KerningPair object to hold the parsed data
			KerningPair kp{};
			std::stringstream ss(line);
			std::string token;

			// Iterate through each token in the line
			while (ss >> token) {
				// Find the position of the '=' character to separate key and value
				auto pos = token.find('=');

				// If '=' is not found, skip this token
				if (pos == std::string::npos) continue;

				// Extract the key and value from the token
				std::string key = token.substr(0, pos);
				float value = std::stof(token.substr(pos + 1));

				// Assign the extracted values to the corresponding fields in the KerningPair object
				if (key == "first")					kp.first = static_cast<int>(value);
				if (key == "second")				kp.second = static_cast<int>(value);
				if (key == "amount")				kp.amount = value;
			}

			// Add the parsed kerning pair to the FontAsset's kerning vector
			fontAsset.kerning.push_back(kp);
		}
	}

	//---------------------------------------
	// Close the file after parsing is complete (good practice to release resources but not strictly necessary due to RAII)
	file.close();

	// Successfully parsed the BMFont text file and populated the FontAsset
	return true;
}
/////////////////////////////////



/////////////////////////////////
// LoadBMFontTexture - Loads the texture associated with a BMFont and populates the FontAsset with texture data.
bool Fontsystem::LoadBMFontTexture(const std::string& pngPath, FontAsset& fontAsset) {

	int width, height, channels;
	unsigned char* data = stbi_load(pngPath.c_str(), &width, &height, &channels, 4); // Force 4 channels (RGBA)

	if (!data) {
		std::cerr << "[Fontsystem] Failed to load BMFont texture: " << pngPath << std::endl;
		return false;
	}

	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // Use linear filtering for minification with mipmaps, might remove this later for pixel-perfect rendering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);

	fontAsset.textureID = textureID;

	return true;
}
/////////////////////////////////