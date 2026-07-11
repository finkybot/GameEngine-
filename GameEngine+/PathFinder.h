/////////////////////////////////
// PathfinderSystem.h
/////////////////////////////////



/////////////////////////////////
#pragma once
#include <vector>
#include <optional>
#include "Vec2.h"
#include "ChunkManager.h"
/////////////////////////////////



/////////////////////////////////
// Simple tile node used for local A*
struct TileNode {
	int x, y;
	float g, h;
	int parentIdx;
};
/////////////////////////////////



/////////////////////////////////
// Result path as world positions
using Path = std::vector<Vec2>;
/////////////////////////////////



/////////////////////////////////
// Pathfinder - A system responsible for finding paths in a tile-based world using A* algorithm. It operates on a chunked world managed by ChunkManager, 
// allowing for efficient pathfinding across large maps.
//								|
//								|_______________________________________________________________________
class Pathfinder {
	/////////////////////////////////
	// Public interface for the PathfinderSystem class, including methods for finding paths and managing chunk data.
public:
	/////////////////////////////////
	// Constructor for the Pathfinder class, taking a reference to a ChunkManager instance for managing chunk data.
	Pathfinder(ChunkManager& cm) : m_chunkManager(cm) {}
	/////////////////////////////////
	 
	 
	
	/////////////////////////////////
	// FindPath - Finds a path from the start tile to the goal tile using A* algorithm. Returns an optional Path containing world positions if a path is found, 
	// or std::nullopt if no path exists; start/end are world tile coordinates
	std::optional<Path> FindPath(int startTileX, int startTileY, int goalTileX, int goalTileY);

	// Expose LocalAStar for callers that stitch chunk-level paths
	std::optional<std::vector<Vec2>> LocalAStar(const Chunk& chunk, int startTileX, int startTileY, int goalTileX, int goalTileY);
	// Expose chunk heuristic for external incremental search scheduling
	float HeuristicChunk(int chunkX, int chunkY, int targetChunkX, int targetChunkY) const;
	/////////////////////////////////



	/////////////////////////////////
	// private member variables for the PathfinderSystem class, including a reference to the ChunkManager instance for managing chunk data.
private:
	/////////////////////////////////
	// Reference to the ChunkManager instance for managing chunk data
	ChunkManager& m_chunkManager;
	/////////////////////////////////



	/////////////////////////////////
	// FindChunkPath - High-level A* pathfinding on the chunk graph, returning a list of chunk coordinates that form the path from the 
	// start chunk to the goal chunk; (returns chunk coordinate list, or at least it will do when i write the code)
	bool FindChunkPath(int startTileX, int startTileY, int goalTileX, int goalTileY, std::vector<std::pair<int, int>>& outChunks);
	/////////////////////////////////



	/////////////////////////////////
	// Helpers
	void EnsureChunksForPath(int startTileX, int startTileY, int goalTileX, int goalTileY);
	/////////////////////////////////
};
/////////////////////////////////
