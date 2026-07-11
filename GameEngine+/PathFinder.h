/////////////////////////////////
// PathfinderSystem.h
/////////////////////////////////



/////////////////////////////////
#pragma once
#include <functional>
#include <vector>
#include <queue>
#include <unordered_map>
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
// Using declarations for pathfinding types
using Path = std::vector<Vec2>;				// Path is a vector of Vec2 representing a sequence of world positions that form a path.
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
	/////////////////////////////////
	 
	 
	
	/////////////////////////////////
	// LocalAStar - Performs local A* pathfinding within a single chunk, returning an optional vector of Vec2 containing world positions if a path is found,
	std::optional<std::vector<Vec2>> LocalAStar(const Chunk& chunk, int startTileX, int startTileY, int goalTileX, int goalTileY);
	/////////////////////////////////
	
	

	/////////////////////////////////
	// HeuristicChunk - Expose chunk heuristic for external incremental search scheduling
	float HeuristicChunk(int chunkX, int chunkY, int targetChunkX, int targetChunkY) const;
	/////////////////////////////////



	/////////////////////////////////
	// private member variables for the PathfinderSystem class, including a reference to the ChunkManager instance for managing chunk data.
private:
	/////////////////////////////////
	// Member variables
	ChunkManager& m_chunkManager; // Reference to the ChunkManager instance for managing chunk data.
	const int m_dirs[8][2] = {{1, 0}, {-1, 0}, {0, 1},	{0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}}; // 8-directional movement (dx, dy) for A* search
	/////////////////////////////////



	/////////////////////////////////
	// FindChunkPath - High-level A* pathfinding on the chunk graph, returning a list of chunk coordinates that form the path from the 
	// start chunk to the goal chunk; (returns chunk coordinate list, or at least it will do when i write the code)
	bool FindChunkPath(int startTileX, int startTileY, int goalTileX, int goalTileY,
					   std::vector<std::pair<int, int>>& outChunks);
	/////////////////////////////////



	/////////////////////////////////
	// CheckDiagonalNeighbour - Checks if a diagonal neighboring tile is walkable (not blocked) and updates the return flag accordingly. I pass a reference to retFlag 
	// so that the caller can know if the neighbor is walkable or not, and I return a bool to indicate if the neighbor is valid (in bounds) or not.
	bool CheckDiagonalNeighbour(int dx, int dy, int cx, const int chunkW, int cy, const int chunkH, int nx, int ny,
								std::function<bool(int, int)> isBlocked, bool& retFlag);
	/////////////////////////////////


	
	/////////////////////////////////
	// CheckOrthogonalNeighbour - Checks if a neighboring tile in the specified direction is walkable (not blocked) and updates the return flag accordingly. I pass a 
	// reference to retFlag so that the caller can know if the neighbor is walkable or not, and I return a bool to indicate if the neighbor is valid (in bounds) or not.
	bool CheckOrthogonalNeighbour(int dx, int dy, int cx, const int chunkW, int nx, const int chunkH, int cy,
								  std::function<bool(int, int)> isBlocked, int ny, bool& retFlag);
	/////////////////////////////////



	/////////////////////////////////
	// Helpers
	void EnsureChunksForPath(int startTileX, int startTileY, int goalTileX, int goalTileY);
	/////////////////////////////////
};
/////////////////////////////////
