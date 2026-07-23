/////////////////////////////////
// ChunkManagerV2.h - Trying a newer approach to chunk management with a simpler interface and potentially better performance. This is a work in progress and may not be fully functional yet.
/////////////////////////////////



/////////////////////////////////
#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include "Vec2.h"
#include "CChunkComponent.h"
#include "EntityManager.h"
/////////////////////////////////



/////////////////////////////////
// ChunkManagerV2 class - Manages the loading, saving, and rendering of chunks in the game world. It provides methods for accessing and modifying tile data, managing chunk streaming, and handling rendering properties.
//							|
//							|___________________________________________________________________________________
/////////////////////////////////
class ChunkManagerV2 {
	/////////////////////////////////
	// Public interface
public:
	/////////////////////////////////
	// Constructor
	explicit ChunkManagerV2(EntityManager& em, int chunkTilesWide, int chunkTilesHigh, int tileSize);
	/////////////////////////////////
	
	
	/////////////////////////////////
	// Chunk Lifecycle
	Entity* CreateChunk(int cx, int cy);
	void DestroyChunk(Entity* chunk);
	/////////////////////////////////



	/////////////////////////////////
	// Chunk Streaming
	void UpdateStreaming(const Vec2& cameraPos, float viewWidth, float viewHeight);
	void ActivateChunk(Entity* chunk);
	void DeactivateChunk(Entity* chunk);
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// Tile Access
	Tile& GetTile(Entity* chunk, int layerID, int localX, int localY);
	void SetTile(Entity* chunk, int layerID, int localX, int localY, const Tile& tile);
	/////////////////////////////////



	/////////////////////////////////
	// Collision
	bool IsTileSolid(Entity* chunk, int layerID, int localX, int localY);
	void BuildCollisionGrid(Entity* chunk);
	/////////////////////////////////


	/////////////////////////////////
	// Serialization
	void LoadChunkFromDisk(Entity* chunk);
	void SaveChunk(Entity* chunk);
	/////////////////////////////////



	/////////////////////////////////
	// Lookup 
	Entity* FindChunk(int cx, int cy);
	bool ChunkExists(int cx, int cy) const;
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods
private:
	/////////////////////////////////
	// PackChunkID - Packs the chunk coordinates (cx, cy) into a single 64-bit integer for use as a key in the chunk map. 
	// This allows for efficient storage and retrieval of chunks based on their coordinates.
	uint64_t PackChunkID(int cx, int cy) const;
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables
	private:
	EntityManager& m_em;
	////////////////////////////////_



	/////////////////////////////////
	// Map of chunk IDs to chunk entities for quick lookup and management of loaded chunks. The key is a 64-bit integer 
	// representing the packed chunk coordinates, and the value is a pointer to the corresponding chunk entity.
	std::unordered_map<uint64_t, Entity*> m_chunkMap;
	/////////////////////////////////



	/////////////////////////////////
	// Configuration for chunk dimensions and tile size. These values determine how many tiles are in each chunk and 
	// the size of each tile in world units.
	int m_chunkTilesWide;
	int m_chunkTilesHigh;
	int m_tileSize;
	int m_activeRadius = 3; // chunks around camera
	/////////////////////////////////
};
/////////////////////////////////
