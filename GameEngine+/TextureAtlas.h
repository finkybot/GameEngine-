#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Rect.hpp>

// TextureAtlas: simple atlas that slices a texture into fixed-size tiles and exposes their rects.
class TextureAtlas {
public:
	TextureAtlas() = default;
	~TextureAtlas() = default;

	// Load texture and slice into tiles of size tileW x tileH. Returns false on error.
	bool LoadFromFile(const std::string& filePath, int tileW, int tileH);

	// Get texture shared pointer (may be null if not loaded)
	std::shared_ptr<sf::Texture> GetTexture() const { return m_texture; }

	// Number of tiles available
	size_t TileCount() const { return m_rects.size(); }

	// Simple rect type for tile coordinates
	struct TileRect {
		int x;
		int y;
		int w;
		int h;
	};

	// Get rect for tile index (optional)
	std::optional<TileRect> GetRectForTile(size_t index) const;

	// Convert stored TileRect to SFML FloatRect for rendering convenience
	std::optional<sf::FloatRect> GetSfFloatRectForTile(size_t index) const;

	int TileWidth() const { return m_tileW; }
	int TileHeight() const { return m_tileH; }

private:
	std::shared_ptr<sf::Texture> m_texture;
	std::vector<TileRect> m_rects;
	int m_tileW = 0;
	int m_tileH = 0;
};
