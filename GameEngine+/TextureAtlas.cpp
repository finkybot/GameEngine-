/////////////////////////////////
// TextureAtlas.cpp
/////////////////////////////////



/////////////////////////////////
// Includes and namespace aliases for the TextureAtlas implementation. We include necessary headers for file I/O, memory management, and SFML graphics.
#include "TextureAtlas.h"
#include <SFML/Graphics/Image.hpp>
#include <iostream>
/////////////////////////////////



/////////////////////////////////
// LoadFromFile - Loads a texture atlas from the specified file path and slices it into tiles of the given width and height. If loading fails, an error message is logged and false is returned.
bool TextureAtlas::LoadFromFile(const std::string& filePath, int tileW, int tileH) {
	if (tileW <= 0 || tileH <= 0) return false; // Guard against invalid tile sizes

	// Load the image from file using SFML's Image class. If loading fails, log an error and return false.
	sf::Image img;
	if (!img.loadFromFile(filePath)) {
		std::cerr << "TextureAtlas: failed to load image '" << filePath << "'\n";
		return false;
	}

	// Create a texture from the loaded image. If creation fails, log an error, reset the texture pointer, and return false.
	m_texture = std::make_shared<sf::Texture>();
	if (!m_texture->loadFromImage(img)) {
		std::cerr << "TextureAtlas: failed to create texture from image '" << filePath << "'\n";
		m_texture.reset();
		return false;
	}

	// Ensure sampling state is appropriate for texture atlases
	m_texture->setSmooth(false);
	m_texture->setRepeated(false);

	// Detect whether image has any transparent pixels (alpha < 255)
	m_hasAlpha = false;
	const unsigned char* pixels = img.getPixelsPtr();
	if (pixels) {
		unsigned int width = img.getSize().x;
		unsigned int height = img.getSize().y;
		// each pixel has 4 components (RGBA)
		for (unsigned int i = 0; i < width * height; ++i) {
			const unsigned char a = pixels[i * 4 + 3];
			if (a != 255) { m_hasAlpha = true; break; }
		}
	}

	// Slice the image into tiles based on the specified tile width and height. Store the tile rectangles in the m_rects vector.
	m_tileW = tileW;
	m_tileH = tileH;
	m_rects.clear();

	// Calculate the number of columns and rows of tiles in the image, and iterate through them to create TileRect entries for each tile. The rectangles are stored in row-major order.
	unsigned int cols = img.getSize().x / static_cast<unsigned int>(tileW);
	unsigned int rows = img.getSize().y / static_cast<unsigned int>(tileH);

	// Iterate through each row and column to create TileRect entries for each tile in the atlas. The rectangles are stored in row-major order.
	for (unsigned int r = 0; r < rows; ++r) {
		for (unsigned int c = 0; c < cols; ++c) {
			TileRect rect;
			rect.x = static_cast<int>(c * tileW);
			rect.y = static_cast<int>(r * tileH);
			rect.w = tileW;
			rect.h = tileH;
			m_rects.push_back(rect);
		}
	}

	return !m_rects.empty();
}
/////////////////////////////////



/////////////////////////////////
// GetRectForTile - Retrieves the TileRect for the specified tile index. If the index is out of bounds, std::nullopt is returned. Otherwise, the TileRect is returned.
std::optional<TextureAtlas::TileRect> TextureAtlas::GetRectForTile(size_t index) const {
	if (index >= m_rects.size())
		return std::nullopt;
	return m_rects[index];
}
/////////////////////////////////



/////////////////////////////////
// GetSfFloatRectForTile - Retrieves the SFML FloatRect for the specified tile index by converting the stored TileRect. If the index is out of bounds, std::nullopt is returned. Otherwise, the converted FloatRect is returned.
std::optional<sf::FloatRect> TextureAtlas::GetSfFloatRectForTile(size_t index) const {
	if (index >= m_rects.size())
		return std::nullopt;
	const TileRect& t = m_rects[index];
	sf::FloatRect r;
	r.position.x = static_cast<float>(t.x);
	r.position.y = static_cast<float>(t.y);
	r.size.x = static_cast<float>(t.w);
	r.size.y = static_cast<float>(t.h);
	return r;
}
/////////////////////////////////