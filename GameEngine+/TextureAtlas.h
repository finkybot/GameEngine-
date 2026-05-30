/////////////////////////////////
// TextureAtlas.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <string>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Rect.hpp>
/////////////////////////////////



/////////////////////////////////
// TextureAtlas: simple atlas that slices a texture into fixed-size tiles and exposes their rects.
class TextureAtlas {
	/////////////////////////////////
	// Public interface
public:
	/////////////////////////////////
	// Constructor and destructor for the TextureAtlas class. The default constructor initializes an empty texture atlas, while the destructor ensures that any loaded texture resources are properly cleaned up when the atlas is destroyed.
	TextureAtlas() = default;
	~TextureAtlas() = default;
	/////////////////////////////////



	/////////////////////////////////
	// LoadFromFile - Load texture and slice into tiles of size tileW x tileH. Returns false on error.
	bool LoadFromFile(const std::string& filePath, int tileW, int tileH);



	/////////////////////////////////
	// Get texture shared pointer (may be null if not loaded)
	std::shared_ptr<sf::Texture> GetTexture() const { return m_texture; }
	/////////////////////////////////



	/////////////////////////////////
	// TileCount - Number of tiles available
	size_t TileCount() const { return m_rects.size(); }
	/////////////////////////////////



	/////////////////////////////////
	// TileRect struct - Simple rect type for tile coordinates
	struct TileRect {
		int x;
		int y;
		int w;
		int h;
	};
	/////////////////////////////////



	/////////////////////////////////
	// GetRectForTile - Get rect for tile index (optional)
	std::optional<TileRect> GetRectForTile(size_t index) const;
	/////////////////////////////////



	/////////////////////////////////
	// GetSfFloatRectForTile - Convert stored TileRect to SFML FloatRect for rendering convenience
	std::optional<sf::FloatRect> GetSfFloatRectForTile(size_t index) const;
	/////////////////////////////////



	/////////////////////////////////
	// TileWidth - Get tile width
	int TileWidth() const { return m_tileW; }
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// TileHeight - Get tile height
	int TileHeight() const { return m_tileH; }
	/////////////////////////////////


	/////////////////////////////////
	// Private member variables 
private:
	/////////////////////////////////
	// Shared pointer to the loaded texture, allowing for shared ownership and automatic cleanup of the texture resource when it is no longer needed.
	std::shared_ptr<sf::Texture> m_texture;
	/////////////////////////////////



	/////////////////////////////////
	// Vector of TileRect structs representing the coordinates and dimensions of each tile in the atlas. This allows for easy retrieval of tile regions for rendering and other operations based on tile indices.
	std::vector<TileRect> m_rects;
	/////////////////////////////////



	/////////////////////////////////
	// Tile dimensions (width and height) used for slicing the texture into tiles. These values are set when loading the texture and are used to calculate the tile regions in the atlas.
	int m_tileW = 0;
	int m_tileH = 0;
	/////////////////////////////////
};
/////////////////////////////////
