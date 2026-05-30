/////////////////////////////////
// CTileMap.h - Component wrapping TileMap data
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the CTileMap component. We include the base Component class for ECS architecture and the TileMap struct for storing tile map data.
#pragma once

#include "Component.h"
#include "TileMap.h"
/////////////////////////////////



/////////////////////////////////
// CTileMap component - stores a TileMap inside an entity so systems can operate on tilemaps via ECS
class CTileMap : public Component {
	/////////////////////////////////
	// Public data members for CTileMap
public:
	/////////////////////////////////
	// Member variables
	TileMap map;			  // underlying tile data
	bool m_processed = false; // whether the map has been converted to collider entities
	bool m_dirty = true;	  // whether the map needs processing (set true when map is created or modified)
	/////////////////////////////////



	/////////////////////////////////
	// Constructors for the CTileMap component. The default constructor initializes the tile map with default properties, while the constructors with parameters allow for initializing the tile map with a given TileMap object, either by copy or by move semantics.
	CTileMap() = default;
	explicit CTileMap(const TileMap& m) : map(m) {}
	explicit CTileMap(TileMap&& m) : map(std::move(m)) {}
	/////////////////////////////////



	/////////////////////////////////
	// Inline helper methods for accessing tile map properties and manipulating tiles. These methods provide convenient access to the underlying TileMap data, allowing systems to easily query and modify the tile map as needed.
	inline int GetWidth() const { return map.width; }
	inline int GetHeight() const { return map.height; }
	inline float GetTileSize() const { return map.tileSize; }
	/////////////////////////////////



	/////////////////////////////////
	// Inline helper methods for accessing and modifying tiles in the tile map. These methods allow systems to get the value of a tile at specific coordinates, set the value of a tile, and check if a tile is solid (non-zero) based on the underlying TileMap data.
	inline int GetTile(int x, int y) const { return map.GetTile(x, y); }
	inline void SetTile(int x, int y, int v) { map.SetTile(x, y, v); }
	inline bool IsSolid(int x, int y) const { return map.IsSolid(x, y); }
	/////////////////////////////////
};
/////////////////////////////////
