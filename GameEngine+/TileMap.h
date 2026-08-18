/////////////////////////////////
// TileMap.h - Defines the TileMap struct for representing 2D tile maps in the game engine.
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the TileMap struct. We include standard library headers for vector, string, and optional types, which are used to store tile data and metadata for the tile map.
#pragma once

#include <vector>
#include <string>
#include <optional>
/////////////////////////////////



/////////////////////////////////
// TileMap - logical representation of a 2D tile map used for raycasting, collision and rendering metadata.
//								|
//								|_______________________________________________________________________
struct TileMap {
	/////////////////////////////////
	// Public data members for TileMap
	int width = 0;
	int height = 0;
	float tileSize = 32.0f;
	std::vector<int> tiles; // 0 = empty, non-zero = solid
	/////////////////////////////////



	/////////////////////////////////
	// Tileset metadata (optional)
	std::string tilesetKey;	  // logical key to lookup atlas in TextureManager
	std::string tilesetImage; // optional image path (for export)
	int tilesetTileW = 0;
	int tilesetTileH = 0;
	/////////////////////////////////



	/////////////////////////////////
	// Optional layers support. If empty, 'tiles' is the single layer saved/loaded for compatibility.
	//								|
	//								|_______________________________________________________________________
	struct Layer {
		std::string name;
		std::vector<int> tiles;
	};
	std::vector<Layer> layers;
	/////////////////////////////////



	/////////////////////////////////
	// Constructors for the TileMap struct. The default constructor initializes an empty tile map, while the constructor with parameters allows for initializing the tile map with specified dimensions and tile size, creating a vector of tiles initialized to zero (empty).
	TileMap() = default;
	TileMap(int w, int h, float sz = 32.0f) : width(w), height(h), tileSize(sz), tiles(w * h, 0) {}
	/////////////////////////////////



	/////////////////////////////////
	// InBounds - checks if the given tile coordinates are within the bounds of the tile map. This method is used to prevent out-of-bounds access when getting or setting tile values, ensuring that tile operations are performed safely within the defined dimensions of the tile map.
	inline bool InBounds(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
	/////////////////////////////////



	/////////////////////////////////
	// GetTile - retrieves the value of a tile at the specified coordinates if they are within bounds, returning 0 (empty) if the coordinates are out of bounds. 
	// This method uses the InBounds method to check if the coordinates are valid before accessing the tile data, providing a safe way to query tile values without risking out-of-bounds access.
	inline int GetTile(int x, int y) const { return InBounds(x, y) ? tiles[y * width + x] : 0; }
	/////////////////////////////////



	/////////////////////////////////
	// SetTile - updates the value of a tile at the specified coordinates if they are within bounds. This method checks if the given coordinates are valid using the 
	// InBounds method before modifying the tile value, ensuring that tile updates do not cause out-of-bounds access or modify invalid memory.
	inline void SetTile(int x, int y, int v) {
		if (InBounds(x, y))
			tiles[y * width + x] = v;
	}
	/////////////////////////////////



	/////////////////////////////////
	// IsSolid - checks if a tile is solid (non-zero) at the specified coordinates, returning false if the coordinates are out of bounds. This method uses the GetTile 
	// method to retrieve the tile value and determine if it is solid, providing a convenient way to check for solid tiles in the tile map.
	inline bool IsSolid(int x, int y) const { return GetTile(x, y) != 0; }
	/////////////////////////////////



	/////////////////////////////////
	// SaveToJSON - saves the tile map data to a JSON file at the specified path. This method serializes the tile map properties and tile data into a JSON format, allowing 
	// for easy storage and retrieval of tile maps. The method returns true if the save operation was successful, or false if an error occurred, with an optional output parameter for error messages.
	bool SaveToJSON(const std::string& path, std::string* outErr = nullptr) const;
	/////////////////////////////////



	/////////////////////////////////
	// LoadFromJSON - loads tile map data from a JSON file at the specified path, returning an optional TileMap object. This method deserializes the JSON data to populate the tile map properties and tile data, 
	// allowing for easy loading of tile maps from files. If the load operation is successful, it returns a TileMap object; if an error occurs, it returns std::nullopt, with an optional output parameter for error messages.
	static std::optional<TileMap> LoadFromJSON(const std::string& path, std::string* outErr = nullptr);
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// SaveToJSON_Legacy - Legacy helper kept for compatibility (forward to the TileMap methods)
	bool SaveToJSON_Legacy(const std::string& path, std::string* outErr = nullptr) const {
		return SaveToJSON(path, outErr);
	}
	/////////////////////////////////



	/////////////////////////////////
	// LoadFromJSON_Legacy - Legacy helper kept for compatibility (forward to the TileMap methods)
	static std::optional<TileMap> LoadFromJSON_Legacy(const std::string& path, std::string* outErr = nullptr) {
		return LoadFromJSON(path, outErr);
	}
	/////////////////////////////////
};
/////////////////////////////////
