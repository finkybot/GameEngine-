/////////////////////////////////
// TextureAtlas.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "TextureAtlas.h"
#include <SFML/Graphics.hpp>

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>

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
// LoadFromFile - Loads a texture from a file and slices it into tiles of specified width and height. Returns true if successful, false otherwise.
bool TextureAtlas::LoadFromFile(const std::string& filePath, int tileW, int tileH) {
	if (tileW <= 0 || tileH <= 0)
		return false;

	sf::Image img;
	if (!img.loadFromFile(filePath)) {
		std::cerr << "TextureAtlas: failed to load image '" << filePath << "'\n";
		return false;
	}

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

	// Store tile size
	m_tileW = tileW;
	m_tileH = tileH;
	m_rects.clear();

	// Decide whether padding should be applied
	auto isPow2 = [](int x) { return (x & (x - 1)) == 0; };

	applyPadding = (!conditionalPadding) ||		  // pad ALL tiles if conditionalPadding == false
				   (isPow2(tileW) && isPow2(tileH)); // otherwise pad only power-of-two tiles

	unsigned int cols = img.getSize().x / tileW;
	unsigned int rows = img.getSize().y / tileH;

	if (applyPadding) {
		const int pad = 1; // 1px extruded padding

		unsigned int newW = cols * (tileW + pad * 2);
		unsigned int newH = rows * (tileH + pad * 2);

		// SFML 3.1.0: must use constructor with Vector2u
		sf::Image padded(sf::Vector2u(newW, newH), sf::Color::Transparent);

		for (unsigned int r = 0; r < rows; ++r) {
			for (unsigned int c = 0; c < cols; ++c) {

				int srcX = c * tileW;
				int srcY = r * tileH;

				int dstX = c * (tileW + pad * 2) + pad;
				int dstY = r * (tileH + pad * 2) + pad;

				// Copy tile interior
				for (int y = 0; y < tileH; ++y) {
					for (int x = 0; x < tileW; ++x) {
						padded.setPixel(sf::Vector2u(dstX + x, dstY + y),
										img.getPixel(sf::Vector2u(srcX + x, srcY + y)));
					}
				}

				// Extrude top/bottom edges
				for (int x = 0; x < tileW; ++x) {
					padded.setPixel(sf::Vector2u(dstX + x, dstY - pad), img.getPixel(sf::Vector2u(srcX + x, srcY)));
					padded.setPixel(sf::Vector2u(dstX + x, dstY + tileH),
									img.getPixel(sf::Vector2u(srcX + x, srcY + tileH - 1)));
				}

				// Extrude left/right edges
				for (int y = 0; y < tileH; ++y) {
					padded.setPixel(sf::Vector2u(dstX - pad, dstY + y), img.getPixel(sf::Vector2u(srcX, srcY + y)));
					padded.setPixel(sf::Vector2u(dstX + tileW, dstY + y),
									img.getPixel(sf::Vector2u(srcX + tileW - 1, srcY + y)));
				}

				// Extrude corners
				padded.setPixel(sf::Vector2u(dstX - pad, dstY - pad), img.getPixel(sf::Vector2u(srcX, srcY)));

				padded.setPixel(sf::Vector2u(dstX + tileW, dstY - pad),
								img.getPixel(sf::Vector2u(srcX + tileW - 1, srcY)));

				padded.setPixel(sf::Vector2u(dstX - pad, dstY + tileH),
								img.getPixel(sf::Vector2u(srcX, srcY + tileH - 1)));

				padded.setPixel(sf::Vector2u(dstX + tileW, dstY + tileH),
								img.getPixel(sf::Vector2u(srcX + tileW - 1, srcY + tileH - 1)));

				// Store padded rect
				TileRect rect;
				rect.x = dstX;
				rect.y = dstY;
				rect.w = tileW;
				rect.h = tileH;
				m_rects.push_back(rect);
			}
		}

		// Replace texture with padded version
		m_texture = std::make_shared<sf::Texture>();
		if (!m_texture->loadFromImage(padded)) {
			std::cerr << "TextureAtlas: failed to create padded texture\n";
			return false;
		}

		m_texture->setSmooth(false);
		m_texture->setRepeated(false);

		return true;
	}

	// --- NO PADDING: original slicing ---
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

	// Use original texture
	m_texture = std::make_shared<sf::Texture>();
	if (!m_texture->loadFromImage(img)) {
		std::cerr << "TextureAtlas: failed to create texture from image '" << filePath << "'\n";
		return false;
	}

	m_texture->setSmooth(false);
	m_texture->setRepeated(false);

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

	const int pad = 1; // must match LoadFromFile()

	for (size_t i = 0; i < TileCount(); ++i) {
		auto frOpt = GetSfFloatRectForTile(i);
		if (!frOpt)
			continue;

		const sf::Rect<float>& fr = *frOpt;

		QuadTemplate q;

		// If padding was applied, skip padded border
		float px = applyPadding ? pad : 0;
		float py = applyPadding ? pad : 0;

		// Compute interior tile bounds (correct!)
		float interiorX0 = fr.position.x + px;
		float interiorY0 = fr.position.y + py;

		float interiorX1 = fr.position.x + px + m_tileW;
		float interiorY1 = fr.position.y + py + m_tileH;

		// Convert to UVs
		float u0 = interiorX0 / texW;
		float v0 = interiorY0 / texH;
		float u1 = interiorX1 / texW;
		float v1 = interiorY1 / texH;

		// Quad size in world space
		float w = static_cast<float>(m_tileW);
		float h = static_cast<float>(m_tileH);

		// Two triangles forming a quad
		q.v[0] = {{0.f, 0.f}, sf::Color::White, {u0, v0}};
		q.v[1] = {{w, 0.f}, sf::Color::White, {u1, v0}};
		q.v[2] = {{w, h}, sf::Color::White, {u1, v1}};
		q.v[3] = {{0.f, 0.f}, sf::Color::White, {u0, v0}};
		q.v[4] = {{w, h}, sf::Color::White, {u1, v1}};
		q.v[5] = {{0.f, h}, sf::Color::White, {u0, v1}};

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