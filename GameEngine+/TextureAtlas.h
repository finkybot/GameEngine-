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
#include <SFML/Graphics.hpp>
/////////////////////////////////



/////////////////////////////////
// OpenGL unsigned int type for texture IDs and other OpenGL handles
typedef unsigned int GLuint; 
/////////////////////////////////



/////////////////////////////////
//	|	QuadTemplate struct - Represents a quad made of 6 vertices (2 triangles) for rendering a tile from the texture atlas. Each vertex contains position and texture coordinates.
//	|_______________________________________________________________________
struct QuadTemplate {
	sf::Vertex v[6];
};
/////////////////////////////////



/////////////////////////////////
//	|	TextureAtlas: simple atlas that slices a texture into fixed-size tiles and exposes their rects.
//	|_______________________________________________________________________
class TextureAtlas {
public:
	// Constructor and destructor for the TextureAtlas class. The default constructor initializes an empty texture atlas, while the destructor ensures that any loaded texture resources are properly cleaned up when the atlas is destroyed.
	TextureAtlas() = default;
	~TextureAtlas() = default;

	// ----------------------------------------------
	// SFML API for loading and slicing texture atlases
	// ----------------------------------------------


	bool LoadFromFile(const std::string& filePath, int tileW, int tileH);	// LoadFromFile - Load texture and slice into tiles of size tileW x tileH. Returns false on error.
	void BuildQuadTemplates();												// BuildQuadTemplates - Precomputes quad templates for each tile in the atlas, allowing
	const QuadTemplate& GetQuadTemplate(size_t tileIndex) const;			// GetQuadTemplate - Retrieves the precomputed quad template for the specified tile index.

	uint64_t GetBindlessHandle() const {return m_bindlessHandle;}			// GetBindlessHandle - Returns the OpenGL bindless texture handle for the atlas, allowing for efficient binding of the texture in OpenGL shaders and rendering pipelines.



	////////////////////////////////
	//	|	TileRect struct - Simple rect type for tile coordinates
	//	|_______________________________________________________________________
	struct TileRect {
		int x;
		int y;
		int w;
		int h;
	};
	////////////////////////////////



	std::shared_ptr<sf::Texture> GetTexture() const { return m_texture; }	// Get texture shared pointer (may be null if not loaded)
	size_t TileCount() const { return m_rects.size(); }						// TileCount - Number of tiles available


	bool conditionalPadding = false; 	// whether to apply conditional padding to the tile rects (for example, to avoid bleeding when rendering tiles with linear filtering)
	bool applyPadding = false;			// whether padding was applied when slicing the texture (set internally after loading)


	std::optional<TileRect> GetRectForTile(size_t index) const;				// GetRectForTile - Get rect for tile index (optional)
	std::optional<sf::FloatRect> GetSfFloatRectForTile(size_t index) const;	// GetSfFloatRectForTile - Convert stored TileRect to SFML FloatRect for rendering convenience


	int TileWidth() const { return m_tileW; }				// TileWidth - Get tile width
	int TileHeight() const { return m_tileH; }				// TileHeight - Get tile height
	public: bool HasAlpha() const { return m_hasAlpha; }	// Query whether atlas image contains alpha


	// ----------------------------------------------
	// OpenGL API for binding the texture atlas to a specific texture unit for rendering. This allows the texture to be used in OpenGL shaders and rendering pipelines.
	// ----------------------------------------------

	bool LoadGLTexture();	// LoadGLTexture - Load the texture into OpenGL and return true on success
	void BuildGLUVRects();	// BuildGLUVRects - Precompute OpenGL UV coordinates for each tile in the atlas


	// GL UV rect struct - Represents the normalized texture coordinates (UVs) for a tile in the atlas, used for OpenGL rendering. Each tile has its own UV rect that maps to the corresponding region of the texture.
	struct UVRect {
		float u0, v0; // bottom left corner
		float u1, v1; // top right corner
	};


	// Accessors for GL renderer
	GLuint GetGLHandle() const { return m_glHandle; }
	const UVRect& GetGLUVRect(size_t tileIndex) const { return m_glUVRects[tileIndex]; }



private:
	// ----------------------------------------------
	// SFML - only data
	// ----------------------------------------------

	std::shared_ptr<sf::Texture> m_texture;		// Shared pointer to the loaded texture, allowing for shared ownership and automatic cleanup of the texture resource when it is no longer needed.
	std::vector<TileRect> m_rects;				// Vector of TileRect structs representing the coordinates and dimensions of each tile in the atlas. This allows for easy retrieval of tile regions for rendering and other operations based on tile indices.
	std::vector<QuadTemplate> quadTemplates;	// Vector of QuadTemplate structs representing the precomputed quad templates for each tile in the atlas. This allows for efficient rendering of tiles by reusing the precomputed vertex data.


	// Tile dimensions (width and height) used for slicing the texture into tiles. These values are set when loading the texture and are used to calculate the tile regions in the atlas.
	int m_tileW = 0;			// Tile width
	int m_tileH = 0;			// Tile height
	bool m_hasAlpha = false;	// whether the loaded image contains any transparent pixel

	// ----------------------------------------------
	// OpenGL - only data
	// ----------------------------------------------

	uint64_t m_bindlessHandle = 0;
	GLuint m_glHandle =	0;				// OpenGL texture handle (GLuint) for the loaded texture, used for binding the texture in OpenGL rendering pipelines.
	std::vector<UVRect>	m_glUVRects;	// Vector of UVRect structs representing the normalized texture coordinates (UVs) for each tile in the atlas. This allows for efficient rendering of tiles in OpenGL by mapping the correct texture coordinates to the corresponding tile regions.
};
/////////////////////////////////
