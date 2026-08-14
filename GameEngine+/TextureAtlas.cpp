/////////////////////////////////
// TextureAtlas.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "TextureAtlas.h"
#include <SFML/Graphics.hpp>

#include <SFML/Graphics/Image.hpp>
#include <iostream>
/////////////////////////////////



/////////////////////////////////
// GetQuadTemplate
// Retrieves the precomputed quad template for the specified tile index. Each quad template contains vertex positions and texture 
// coordinates for rendering the corresponding tile from the texture atlas. This allows for efficient rendering of tiles without recalculating vertex data each time.
const QuadTemplate& TextureAtlas::GetQuadTemplate(size_t tileIndex) const {
	return quadTemplates[tileIndex];
}
/////////////////////////////////



/////////////////////////////////
// LoadFromFile
// Loads a texture from a file and slices it into tiles of specified width and height. Returns true if successful, false otherwise.
bool TextureAtlas::LoadFromFile(const std::string& filePath, int tileW, int tileH) {
	if (tileW <= 0 || tileH <= 0)
		return false;

	sf::Image img;
	if (!img.loadFromFile(filePath)) {
		std::cerr << "TextureAtlas: failed to load image '" << filePath << "'\n";
		return false;
	}

	m_texture = std::make_shared<sf::Texture>();
	if (!m_texture->loadFromImage(img)) {
		std::cerr << "TextureAtlas: failed to create texture from image '" << filePath << "'\n";
		m_texture.reset();
		return false;
	}

	m_texture->setSmooth(false);
	m_texture->setRepeated(false);

	// Detect alpha
	m_hasAlpha = false;
	const unsigned char* pixels = img.getPixelsPtr();
	if (pixels) {
		unsigned int width = img.getSize().x;
		unsigned int height = img.getSize().y;
		for (unsigned int i = 0; i < width * height; ++i) {
			if (pixels[i * 4 + 3] != 255) {
				m_hasAlpha = true;
				break;
			}
		}
	}

	// Slice into tiles
	m_tileW = tileW;
	m_tileH = tileH;
	m_rects.clear();

	unsigned int cols = img.getSize().x / static_cast<unsigned int>(tileW);
	unsigned int rows = img.getSize().y / static_cast<unsigned int>(tileH);

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
// BuildQuadTemplates
// Builds quad templates for each tile in the atlas, precomputing vertex positions and texture coordinates for efficient rendering. Each tile is represented as two triangles 
// forming a quad, with vertices defined in a clockwise order. The texture coordinates are normalized based on the size of the texture to ensure correct mapping of the tile's 
// image onto the quad.
void TextureAtlas::BuildQuadTemplates() {
	quadTemplates.resize(TileCount());

	float texW = static_cast<float>(m_texture->getSize().x);
	float texH = static_cast<float>(m_texture->getSize().y);

	for (size_t i = 0; i < TileCount(); ++i) {
		auto frOpt = GetSfFloatRectForTile(i);
		if (!frOpt)
			continue;

		const sf::Rect<float>& fr = *frOpt;

		QuadTemplate q;

		float u0 = fr.position.x / texW;
		float v0 = fr.position.y / texH;
		float u1 = (fr.position.x + fr.size.x) / texW;
		float v1 = (fr.position.y + fr.size.y) / texH;

		q.v[0] = {{0.f, 0.f}, sf::Color::White, {u0, v0}};
		q.v[1] = {{fr.size.x, 0.f}, sf::Color::White, {u1, v0}};
		q.v[2] = {{fr.size.x, fr.size.y}, sf::Color::White, {u1, v1}};
		q.v[3] = {{0.f, 0.f}, sf::Color::White, {u0, v0}};
		q.v[4] = {{fr.size.x, fr.size.y}, sf::Color::White, {u1, v1}};
		q.v[5] = {{0.f, fr.size.y}, sf::Color::White, {u0, v1}};

		quadTemplates[i] = q;
	}
}
/////////////////////////////////



/////////////////////////////////
// GetRectForTile
// Get the TileRect for a tile index, returning std::nullopt if the index is out of bounds.
std::optional<TextureAtlas::TileRect> TextureAtlas::GetRectForTile(size_t index) const {
	if (index >= m_rects.size())
		return std::nullopt;
	return m_rects[index];
}
/////////////////////////////////



/////////////////////////////////
// GetSfFloatRectForTile
// Get the SFML FloatRect for a tile index, converting from the stored TileRect. Returns std::nullopt if the index is out of bounds.
std::optional<sf::Rect<float>> TextureAtlas::GetSfFloatRectForTile(size_t index) const {
	if (index >= m_rects.size())
		return std::nullopt;

	const TileRect& t = m_rects[index];

	sf::Rect<float> r;
	r.position.x = static_cast<float>(t.x);
	r.position.y = static_cast<float>(t.y);
	r.size.x = static_cast<float>(t.w);
	r.size.y = static_cast<float>(t.h);

	return r;
}
/////////////////////////////////