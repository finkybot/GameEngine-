/////////////////////////////////
// CTexture.h: defines the CTexture component class, which holds information about a texture to be rendered for an entity.
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the CTexture component. We include the base Component class for ECS architecture and string for handling texture atlas keys.
#pragma once
#include "Component.h"
#include <string>
/////////////////////////////////



/////////////////////////////////
// CTexture component - represents a texture to be rendered for an entity, with properties for the texture atlas key, tile index, visibility, z-order for rendering control, and optional area dimensions for rendering larger textures that cover multiple tiles.
class CTexture : public Component {
	/////////////////////////////////
	// Public data members for CTexture.
public:
	/////////////////////////////////
	// Member variables
	std::string atlasKey; // key used with TextureManager
	int tileIndex = 0;	  // index into the atlas (0-based)
	bool visible = true;
	float zOrder = 0.0f;
	/////////////////////////////////



	/////////////////////////////////
	// Area (world) covered by this texture. If zero, texture represents a single tile sized by the atlas.
	float areaW = 0.0f;
	float areaH = 0.0f;
	/////////////////////////////////



	/////////////////////////////////
	// Constructors for the CTexture component. The default constructor initializes the texture with default properties, while the constructor with parameters allows for specifying the atlas key, tile index, and optional area dimensions for rendering larger textures.
	CTexture() = default;
	/////////////////////////////////



	/////////////////////////////////
	// Constructor with parameters - initializes the texture with a specified atlas key and tile index, with optional area dimensions for rendering larger textures. If area dimensions are not provided, the texture will be rendered as a single tile based on the atlas.
	explicit CTexture(const std::string& key, int idx = 0) : atlasKey(key), tileIndex(idx) {}
	/////////////////////////////////



	/////////////////////////////////
	// Constructor with area dimensions - initializes the texture with a specified atlas key, tile index, and area dimensions for rendering larger textures that cover multiple tiles. This allows for more flexible rendering of textures that may not fit within a single tile in the atlas.
	explicit CTexture(const std::string& key, int idx, float w, float h)
		: atlasKey(key), tileIndex(idx), areaW(w), areaH(h) {}
	/////////////////////////////////
};
/////////////////////////////////
