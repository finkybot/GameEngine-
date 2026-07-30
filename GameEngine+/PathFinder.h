/////////////////////////////////
// PathfinderSystem.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <functional>
#include <vector>
#include <queue>
#include <unordered_map>
#include "Vec2.h"
#include "ChunkManager.h"
/////////////////////////////////



/////////////////////////////////
// Path type definition - Represents a path as a vector of Vec2 points, where each point corresponds to a tile coordinate in the world.
using Path = std::vector<Vec2>;
/////////////////////////////////


/////////////////////////////////
// Pathfinder class - Implements a basic A* pathfinding algorithm for finding paths in a tile-based world managed by ChunkManager.
//								|
//								|_______________________________________________________________________
class Pathfinder {
	//////////////////////////////
	// Public interface for the Pathfinder class, including methods for finding paths and checking tile properties.
public:
	explicit Pathfinder(ChunkManager& cm);

	std::optional<Path> FindPath(int startTileX, int startTileY, int goalTileX, int goalTileY);

	//////////////////////////////
	// Private member variables and helper methods for the Pathfinder class, including a reference to the ChunkManager and methods for checking tile bounds,
private:
	ChunkManager& m_chunkManager;

	bool InBounds(int tx, int ty) const;
	bool IsBlocked(int tx, int ty) const;
	float Heuristic(int x, int y, int gx, int gy) const;
	//////////////////////////////
};
///////////////////////////////