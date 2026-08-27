/////////////////////////////////
// ChunkComponent.h - Header file for the CChunkComponent class, which represents a chunk of tiles in a tile-based game world. It includes definitions for tile properties, layer management, collision handling, and rendering metadata.
#pragma once
/////////////////////////////////



/////////////////////////////////
// Includes
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
# include "Component.h"
/////////////////////////////////



/////////////////////////////////
// Enums
/////////////////////////////////
// TileFlags enum - Represents various flags that can be associated with a tile, indicating its properties and behavior in the game world. Each flag is represented as a bit in a 16-bit unsigned
// integer, allowing for efficient storage and manipulation of multiple flags for each tile.
enum TileFlags : uint16_t { Solid = 1 << 0, Water = 1 << 1, Animated = 1 << 2, BlocksVision = 1 << 3 };
enum class BlendMode : uint8_t { Normal, Additive, Multiply, Screen, Overlay, Subtract, AlphaBlend };

/////////////////////////////////



/////////////////////////////////
// MeshHandle struct - Represents a handle to a GPU resource (mesh) used for rendering tile layers. It contains an ID for the GPU resource and a validity flag to quickly check if the handle is valid.
//							|
//							|___________________________________________________________________________________
/////////////////////////////////
struct MeshHandle {
	/////////////////////////////////
	uint32_t ID;  // GPU resource ID
	bool IsValid; // quick check
				  /////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// Color struct - Represents a color with red, green, blue, and alpha components. Each component is an 8-bit unsigned integer (0-255), allowing for a wide range of colors and transparency levels.
//							|
//							|___________________________________________________________________________________
/////////////////////////////////
struct Color {
	/////////////////////////////////
	float R;
	float G;
	float B;
	float A; // opacity
	/////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// Tile struct - Represents a single tile in the tile map, containing information about its type, properties, and rendering details. Each tile can have various flags indicating its behavior
// (e.g., solid, water), a collision type for physics interactions, a light level for lighting calculations, and indices for animation frames and metadata.
//							|
//							|___________________________________________________________________________________
/////////////////////////////////
struct Tile {
	/////////////////////////////////
	uint16_t tileID;	   // Which tile type
	uint16_t Flags;		   // Bitmask: solid, water, etc.
	uint8_t CollisionType; // 0 = none, 1 = solid, 2 = water, etc.
	uint8_t LightLevel;	   // 0-255 light level for this tile
	uint8_t FrameIndex;	   // For animated tiles, which frame to display
	uint8_t MetadataIndex; // Index into a metadata table for this tile
						   /////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// CollisionGrid struct - Represents a grid of collision data for a tile layer, containing information about its dimensions, cell storage, and precomputed flags for quick collision checks.
// 							|
//							|___________________________________________________________________________________
/////////////////////////////////
struct CollisionGrid {
	/////////////////////////////////
	// Dimensions
	int Width;	// tiles
	int Height; // tiles
	/////////////////////////////////

	/////////////////////////////////
	// Each cell stores a collision type (0 = none)
	std::vector<uint8_t> Cells;
	/////////////////////////////////

	/////////////////////////////////
	// Optional: precomputed flags
	bool HasAnyCollision; // quick skip for empty layers
						  /////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// TileLayer struct - Represents a layer of tiles in the tile map, containing information about its identity, dimensions, tile storage, rendering properties, collision grid, animation state, and metadata.
//							|
//							|___________________________________________________________________________________
/////////////////////////////////
struct TileLayer {
	/////////////////////////////////
	// Identity
	int LayerID;
	std::string Name;
	/////////////////////////////////

	/////////////////////////////////
	// Dimensions
	int TilesWide;
	int TilesHigh;
	/////////////////////////////////

	/////////////////////////////////
	// Tile Storage
	std::vector<Tile> Tiles;
	/////////////////////////////////

	/////////////////////////////////
	// Layer Properties
	bool Visible;
	bool HasCollision;
	bool IsAnimated;
	bool IsFogLayer;
	bool IsLightLayer;
	/////////////////////////////////

	/////////////////////////////////
	// Render Metadata
	MeshHandle Mesh;
	bool NeedsRebuild;
	float Opacity;
	Color Tint;
	BlendMode Mode;
	/////////////////////////////////

	/////////////////////////////////
	// Collision
	CollisionGrid CollisionMask;
	/////////////////////////////////

	/////////////////////////////////
	// Animation
	uint8_t AnimationFrame;
	float AnimationTimer;
	/////////////////////////////////

	/////////////////////////////////
	// Metadata
	std::vector<uint16_t> Metadata;
	/////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// ChunkComponent struct - Represents a chunk of tiles in the game world, containing information about its identity, dimensions, and storage for the individual tiles. Each chunk is identified by its
//							|
//							|___________________________________________________________________________________
/////////////////////////////////
class CChunk : public Component {
	/////////////////////////////////
public:
	/////////////////////////////////
	// Identity
	int ChunkX = 0;
	int ChunkY = 0;
	uint64_t ChunkID = 0;
	/////////////////////////////////



	/////////////////////////////////
	// Dimensions
	int TilesWide = 0;
	int TilesHigh = 0;
	float TileSize = 32.0f;
	/////////////////////////////////



	/////////////////////////////////
	// Tile Storage
	std::vector<Tile> Tiles;	   // 1D array of tiles, size = TilesWide * TilesHigh
	std::vector<TileLayer> Layers; // Optional layers for multi-layered tile maps
	/////////////////////////////////



	/////////////////////////////////
	// World Bounds
	float WorldMinX;
	float WorldMinY;
	float WorldMaxX;
	float WorldMaxY;
	/////////////////////////////////



	/////////////////////////////////
	// Streaming State
	bool IsLoaded;
	bool IsActive;
	bool IsGenerated;
	bool IsDirty; // needs to be saved
	/////////////////////////////////



	/////////////////////////////////
	CollisionGrid CollisionMask; // Optional collision grid for this chunk
	bool HasCollision;			 // quick check for collision presence
	/////////////////////////////////
};
/////////////////////////////////
