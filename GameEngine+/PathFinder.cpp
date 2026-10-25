/////////////////////////////////
// Pathfinder.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "PathFinder.h"
#include <queue>
#include <unordered_map>
#include <cmath>
/////////////////////////////////



/////////////////////////////////
// NodeKey and NodeKeyHash - Helper structures for A* pathfinding. NodeKey represents a tile coordinate (x, y) in the world, 
// and NodeKeyHash provides a hash function for using NodeKey in unordered maps.
struct NodeKey {
	int x, y;
	bool operator==(const NodeKey& o) const { return x == o.x && y == o.y; }
};
/////////////////////////////////



/////////////////////////////////
struct NodeKeyHash {
	size_t operator()(const NodeKey& k) const { return (static_cast<size_t>(k.x) << 32) ^ static_cast<size_t>(k.y); }
};
/////////////////////////////////



/////////////////////////////////
Pathfinder::Pathfinder(ChunkManager& cm) : m_chunkManager(cm) {}
/////////////////////////////////



/////////////////////////////////
bool Pathfinder::InBounds(int tx, int ty) const {
	return tx >= 0 && ty >= 0 && tx < m_chunkManager.worldWidth && ty < m_chunkManager.worldHeight;
}
/////////////////////////////////



/////////////////////////////////
bool Pathfinder::IsBlocked(int tx, int ty) const {
	const auto& mask = m_chunkManager.GetWorldMask();
	int w = m_chunkManager.worldWidth;
	return mask[ty * w + tx];
}
/////////////////////////////////



/////////////////////////////////
float Pathfinder::Heuristic(int x, int y, int gx, int gy) const {
	int dx = std::abs(x - gx);
	int dy = std::abs(y - gy);
	int minD = std::min(dx, dy);
	int maxD = std::max(dx, dy);
	return 1.41421356f * minD + (maxD - minD); // diagonal distance heuristic
}
/////////////////////////////////



/////////////////////////////////
// SmoothCollinear - Removes collinear points from a path to simplify it. This function takes a vector of Vec2 points representing a path and returns a 
// new vector with collinear points removed, keeping only the essential turning points.
std::vector<Vec2> Pathfinder::SmoothCollinear(const std::vector<Vec2>& path) {
	if (path.size() < 3)
		return path;

	std::vector<Vec2> out;
	out.push_back(path[0]);

	for (size_t i = 1; i + 1 < path.size(); ++i) {
		Vec2 a = out.back();
		Vec2 b = path[i];
		Vec2 c = path[i + 1];

		float abx = b.x - a.x;
		float aby = b.y - a.y;
		float bcx = c.x - b.x;
		float bcy = c.y - b.y;

		// If direction doesn't change, skip b
		if (std::abs(abx * bcy - aby * bcx) < 0.001f) {
			continue;
		}

		out.push_back(b);
	}

	out.push_back(path.back());
	return out;
}
/////////////////////////////////



/////////////////////////////////
// LineOfSight - Checks if there is a clear line of sight between two tile coordinates (x0, y0) and (x1, y1)
bool Pathfinder::LineOfSight(int x0, int y0, int x1, int y1) const {
	int dx = std::abs(x1 - x0);
	int dy = std::abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	while (true) {
		if (IsBlocked(x0, y0))
			return false;
		if (x0 == x1 && y0 == y1)
			break;

		int e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}

	return true;
}
/////////////////////////////////



/////////////////////////////////
// SmoothLineOfSight - Smooths a path by removing unnecessary points that are in line of sight of each other. This function takes a 
// vector of Vec2 points representing a path and returns a new vector with unnecessary points removed, keeping only the essential 
// turning points.
std::vector<Vec2> Pathfinder::SmoothLineOfSight(const std::vector<Vec2>& path) {
	if (path.size() < 3)
		return path;

	std::vector<Vec2> out;
	out.push_back(path[0]);

	int last = 0;

	for (int i = 1; i < path.size(); ++i) {
		int lx = (int)((out.back().x / m_chunkManager.GetTileSize()) - m_chunkManager.worldOffsetX);
		int ly = (int)((out.back().y / m_chunkManager.GetTileSize()) - m_chunkManager.worldOffsetY);

		int nx = (int)((path[i].x / m_chunkManager.GetTileSize()) - m_chunkManager.worldOffsetX);
		int ny = (int)((path[i].y / m_chunkManager.GetTileSize()) - m_chunkManager.worldOffsetY);

		if (!LineOfSight(lx, ly, nx, ny)) {
			out.push_back(path[i - 1]);
		}
	}

	out.push_back(path.back());
	return out;
}
/////////////////////////////////



/////////////////////////////////
// BuildPortals - Constructs portals from a sequence of tile centers. This function takes a vector of Vec2 points representing 
// the centers of tiles in a path and returns a vector of Portal structures,
std::vector<Portal> Pathfinder::BuildPortals(const std::vector<Vec2>& tileCenters) {
	std::vector<Portal> portals;

	for (int i = 0; i + 1 < tileCenters.size(); ++i) {
		Vec2 a = tileCenters[i];
		Vec2 b = tileCenters[i + 1];

		Vec2 dir = b - a;

		// Horizontal
		if (dir.x > 0 && dir.y == 0) {
			portals.push_back({Vec2(a.x, a.y + 16), Vec2(a.x, a.y - 16)});
		} else if (dir.x < 0 && dir.y == 0) {
			portals.push_back({Vec2(a.x, a.y - 16), Vec2(a.x, a.y + 16)});
		}
		// Vertical
		else if (dir.y > 0 && dir.x == 0) {
			portals.push_back({Vec2(a.x + 16, a.y), Vec2(a.x - 16, a.y)});
		} else if (dir.y < 0 && dir.x == 0) {
			portals.push_back({Vec2(a.x - 16, a.y), Vec2(a.x + 16, a.y)});
		}
		// Diagonal
		else {
			// Build diagonal portals
			Vec2 perp(dir.y, -dir.x);
			perp = perp.Normalise() * 16;

			portals.push_back({a + perp, a - perp});
		}
	}

	return portals;
}
/////////////////////////////////



/////////////////////////////////
// Funnel - Implements the funnel algorithm to further smooth a path by removing unnecessary points that are within a "funnel" 
// defined by the path's edges.
std::vector<Vec2> Pathfinder::Funnel(const std::vector<Vec2>& path) {
	// Check if the path has enough points to apply the funnel algorithm
	if (path.size() < 3)
		return path; // Not enough points to apply the funnel algorithm

	// Initialize the output path with the first point of the input path
	std::vector<Vec2> output;
	output.push_back(path[0]);

	// Initialize the apex, left, and right points of the funnel
	Vec2 apex = path[0];
	Vec2 left = path[1];
	Vec2 right = path[1];

	// Initialize indices for the apex, left, and right points
	int apexIndex = 0;
	int leftIndex = 1;
	int rightIndex = 1;

	// Lambda function to calculate the cross product of two vectors
	auto cross = [](const Vec2& a, const Vec2& b) { return a.x * b.y - a.y * b.x; };

    for (int i = 2; i < path.size(); ++i) {
		Vec2 p = path[i];

		// Update right boundary
		if (cross(p - apex, right - apex) <= 0) {
			if (cross(p - apex, left - apex) < 0) {
				// Funnel collapse
				output.push_back(left);
				apex = left;
				apexIndex = leftIndex;
				left = apex;
				right = apex;
				leftIndex = apexIndex;
				rightIndex = apexIndex;
				i = apexIndex + 1;
				continue;
			}
			right = p;
			rightIndex = i;
		}

		// Update left boundary
		if (cross(p - apex, left - apex) >= 0) {
			if (cross(p - apex, right - apex) > 0) {
				// Funnel collapse
				output.push_back(right);
				apex = right;
				apexIndex = rightIndex;
				left = apex;
				right = apex;
				leftIndex = apexIndex;
				rightIndex = apexIndex;
				i = apexIndex + 1;
				continue;
			}
			left = p;
			leftIndex = i;
		}
	}


	output.push_back(path.back());
	return output;
}
/////////////////////////////////



/////////////////////////////////
// FindPath - Implements the A* pathfinding algorithm to find a path from a start tile to a goal tile. It returns 
// an optional Path, which is a vector of Vec2 points representing the path in world coordinates. If no path is 
// found, it returns std::nullopt.
std::optional<Path> Pathfinder::FindPath(int startTileX, int startTileY, int goalTileX, int goalTileY) {
	SearchState state;
	SearchStatus status = StartSearch(state, startTileX, startTileY, goalTileX, goalTileY);
	while (status == SearchStatus::Running)
		status = AdvanceSearch(state, 100000);
	if (status == SearchStatus::Succeeded)
		return std::move(state.result);
	return std::nullopt;
}
/////////////////////////////////

Pathfinder::SearchStatus Pathfinder::StartSearch(SearchState& state, int startTileX, int startTileY, int goalTileX, int goalTileY) {
	state = SearchState{};
	state.offsetX = m_chunkManager.worldOffsetX;
	state.offsetY = m_chunkManager.worldOffsetY;
	state.goalX = goalTileX - state.offsetX;
	state.goalY = goalTileY - state.offsetY;

	const int sx = startTileX - state.offsetX;
	const int sy = startTileY - state.offsetY;

	if (!InBounds(sx, sy) || !InBounds(state.goalX, state.goalY))
		return SearchStatus::Failed;
	if (IsBlocked(sx, sy) || IsBlocked(state.goalX, state.goalY))
		return SearchStatus::Failed;

	PathNodeKey startKey{sx, sy};
	PathNode startNode{sx, sy, 0.0f, Heuristic(sx, sy, state.goalX, state.goalY), -1, -1};
	state.nodes[startKey] = startNode;
	state.open.push({startNode.f, startKey});
	state.initialized = true;
	return SearchStatus::Running;
}

Pathfinder::SearchStatus Pathfinder::AdvanceSearch(SearchState& state, int nodeBudget) {
	if (!state.initialized)
		return SearchStatus::Failed;

	const int dirs8[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

	for (int expanded = 0; expanded < std::max(1, nodeBudget); ++expanded) {
		if (state.open.empty())
			return SearchStatus::Failed;

		auto top = state.open.top();
		state.open.pop();
		PathNodeKey curKey = top.second;

		if (state.closed[curKey])
			continue;
		state.closed[curKey] = true;

		PathNode cur = state.nodes[curKey];
		if (cur.x == state.goalX && cur.y == state.goalY) {
			std::vector<PathNode> tileNodes;
			PathNode k = cur;
			while (true) {
				tileNodes.push_back(k);
				if (k.parentX == -1 && k.parentY == -1)
					break;
				PathNodeKey pk{k.parentX, k.parentY};
				auto it = state.nodes.find(pk);
				if (it == state.nodes.end())
					break;
				k = it->second;
			}

			std::reverse(tileNodes.begin(), tileNodes.end());

			Path path;
			float ts = m_chunkManager.GetTileSize();
			for (auto& n : tileNodes) {
				float wx = (float(n.x + state.offsetX) + 0.5f) * ts;
				float wy = (float(n.y + state.offsetY) + 0.5f) * ts;
				path.emplace_back(wx, wy);
			}

			path = SmoothCollinear(path);
			path = SmoothLineOfSight(path);
			state.result = std::move(path);
			return SearchStatus::Succeeded;
		}

		for (auto& d : dirs8) {
			int nx = cur.x + d[0];
			int ny = cur.y + d[1];

			if (!InBounds(nx, ny) || IsBlocked(nx, ny))
				continue;

			if (d[0] != 0 && d[1] != 0) {
				if (IsBlocked(cur.x + d[0], cur.y) || IsBlocked(cur.x, cur.y + d[1])) {
					continue;
				}
			}

			float ng = cur.g + ((d[0] == 0 || d[1] == 0) ? 1.0f : 1.41421356f);
			PathNodeKey nk{nx, ny};
			auto nit = state.nodes.find(nk);

			if (nit == state.nodes.end() || ng < nit->second.g) {
				PathNode n;
				n.x = nx;
				n.y = ny;
				n.g = ng;
				n.f = ng + Heuristic(nx, ny, state.goalX, state.goalY);
				n.parentX = cur.x;
				n.parentY = cur.y;
				state.nodes[nk] = n;
				state.open.push({n.f, nk});
			}
		}
	}

	return SearchStatus::Running;
}
