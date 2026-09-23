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
//	|	Portal struct - Represents a portal in the pathfinding algorithm, defined by its left and right Vec2 points. Portals are used in the funneling process to smooth paths and reduce unnecessary turns, allowing for more efficient navigation through the tile-based world.
//	|_______________________________________________________________________
struct Portal {
	Vec2 left;
	Vec2 right;
};
/////////////////////////////////




/////////////////////////////////
//	|	Pathfinder class - Implements a basic A* pathfinding algorithm for finding paths in a tile-based world managed by ChunkManager.
//	|_______________________________________________________________________
class Pathfinder {
public:
	explicit Pathfinder(ChunkManager& cm);

	std::optional<Path> FindPath(int startTileX, int startTileY, int goalTileX, int goalTileY);

private:
	ChunkManager& m_chunkManager;

	// Helper methods for pathfinding smoothing and line-of-sight checks...plus funneling
	std::vector<Vec2> SmoothCollinear(const std::vector<Vec2>& path);
	bool LineOfSight(int x0, int y0, int x1, int y1) const;
	std::vector<Vec2> SmoothLineOfSight(const std::vector<Vec2>& path);
	std::vector<Portal> BuildPortals(const std::vector<Vec2>& tileCenters);
	std::vector<Vec2> Funnel(const std::vector<Vec2>& path);

	bool InBounds(int tx, int ty) const;
	bool IsBlocked(int tx, int ty) const;
	float Heuristic(int x, int y, int gx, int gy) const;
};
///////////////////////////////