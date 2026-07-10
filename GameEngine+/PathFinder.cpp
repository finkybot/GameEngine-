/////////////////////////////////
// Pathfinder.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "PathFinder.h"
#include <queue>
#include <mutex>
#include <limits>
#include <cmath>
#include <functional>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <limits>
#include <cmath>
/////////////////////////////////



/////////////////////////////////
// Type alias for priority queue items used in A* search. Each item is a pair consisting of the f-cost (float) and the index (int) of the node in the search 
// space. The f-cost is used to prioritize nodes in the open set, with lower f-costs being processed first.
using PQItem = std::pair<float, int>;
/////////////////////////////////



/////////////////////////////////
// FindPath - Finds a path from the start tile to the goal tile using A* algorithm. Returns an optional Path containing world positions if a path is found,
std::optional<Path> Pathfinder::FindPath(int startTileX, int startTileY, int goalTileX, int goalTileY) {
	// Hierarchical pathfinding: find chunk-level path then local A* per chunk
	EnsureChunksForPath(startTileX, startTileY, goalTileX, goalTileY);

	const int chunkW = m_chunkManager.GetChunkWidth();
	const int chunkH = m_chunkManager.GetChunkHeight();
	if (chunkW <= 0 || chunkH <= 0) return std::nullopt;

	auto floorDiv = [](int a, int b) {
		if (a >= 0) return a / b;
		return -(((-a) + b - 1) / b);
	};

	const int startChunkX = floorDiv(startTileX, chunkW);
	const int startChunkY = floorDiv(startTileY, chunkH);
	const int goalChunkX = floorDiv(goalTileX, chunkW);
	const int goalChunkY = floorDiv(goalTileY, chunkH);

	std::vector<std::pair<int,int>> chunkPath;
	if (!FindChunkPath(startTileX, startTileY, goalTileX, goalTileY, chunkPath)) {
		// fallback to simple rectangle A* if chunk pathing fails; then for simplicity reuse the earlier rectangle A* by calling this function with same bounds
		// (We'll just run the simple A* over bounding rectangle)
		const int minX = std::min(startTileX, goalTileX);
		const int minY = std::min(startTileY, goalTileY);
		const int maxX = std::max(startTileX, goalTileX);
		const int maxY = std::max(startTileY, goalTileY);

		// Compute the width and height of the bounding rectangle
		const int width = maxX - minX + 1;
		const int height = maxY - minY + 1;

		// Limit the maximum number of cells to prevent excessive memory usage
		const int maxCells = 200000;
		
		// If the bounding rectangle is too large, return std::nullopt to indicate failure
		if (width <= 0 || height <= 0 || (width * height) > maxCells) return std::nullopt;

		// A* search on the bounding rectangle, using a 1D array to represent the 2D grid
		// Helper to convert 2D coordinates to 1D index
		const auto index		= [&](int x, int y) { return (y - minY) * width + (x - minX); };
		const int startIndex	= index(startTileX, startTileY);
		const int goalIndex		= index(goalTileX, goalTileY);
		constexpr float INF		= std::numeric_limits<float>::infinity();

		// Initialize the gCosts, parent indices, and closed set
		std::vector<float> gCosts(width * height, INF);
		std::vector<int> parent(width * height, -1);
		std::vector<char> closed(width * height, 0);

		// Priority queue for the open set, ordered by f-cost (g + h)
		std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> open;
		// Heuristic function: Manhattan distance to the goal
		auto heuristic = [&](int tx, int ty) { return static_cast<float>(std::abs(tx - goalTileX) + std::abs(ty - goalTileY)); };
		
		
		// Check if the start or goal tiles are blocked (non-zero tile value)
		if (m_chunkManager.GetTileAt(startTileX, startTileY) != 0) return std::nullopt;
		if (m_chunkManager.GetTileAt(goalTileX, goalTileY) != 0) return std::nullopt;

		// We ok to continue so initialize the starting node
		gCosts[startIndex] = 0.0f; open.push({heuristic(startTileX, startTileY), startIndex});
		const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}; // 4 cardinal directions
		bool found = false; // Flag to indicate if a path to the goal has been found


		// Main A* loop: continue until the open set is empty or the goal is found
		while (!open.empty()) {
			auto [f, cur] = open.top(); open.pop();
			if (closed[cur]) continue; closed[cur] = 1;
			if (cur == goalIndex) { found = true; break; }
			int cx = (cur % width) + minX; int cy = (cur / width) + minY;
			for (auto &d : dirs) {
				int nx = cx + d[0]; int ny = cy + d[1];
				if (nx < minX || nx > maxX || ny < minY || ny > maxY) continue;
				int ni = index(nx, ny); if (closed[ni]) continue;
				int tv = m_chunkManager.GetTileAt(nx, ny); if (tv != 0) continue;
				float ng = gCosts[cur] + 1.0f;
				if (ng < gCosts[ni]) { gCosts[ni] = ng; parent[ni] = cur; open.push({ng + heuristic(nx, ny), ni}); }
			}
		}
		if (!found) return std::nullopt;
		std::vector<Vec2> out; int p = goalIndex; float tileSize = 32.0f;
		{ std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex()); auto &chunks = m_chunkManager.GetChunks(); if (!chunks.empty()) tileSize = chunks.begin()->second.tileSize; }
		while (p != -1) { int tx = (p % width) + minX; int ty = (p / width) + minY; float wx = tx * tileSize + tileSize * 0.5f; float wy = ty * tileSize + tileSize * 0.5f; out.emplace_back(wx, wy); p = parent[p]; }
		std::reverse(out.begin(), out.end()); return out;
	}

	// Stitch local paths across chunkPath
	std::vector<Vec2> finalPath;
	// helper to get preferred world point (tile center)
	auto tileCenter = [&](int tx, int ty) {
		float ts = 32.0f; {
			std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex()); auto &chunks = m_chunkManager.GetChunks(); if (!chunks.empty()) ts = chunks.begin()->second.tileSize;
		}
		return Vec2(tx * ts + ts * 0.5f, ty * ts + ts * 0.5f);
	};

	// current start tile coords
	int curTx = startTileX, curTy = startTileY;

	for (size_t i = 0; i < chunkPath.size(); ++i) {
		int cx = chunkPath[i].first;
		int cy = chunkPath[i].second;
		// compute local coords
		int localStartX = curTx - cx * chunkW;
		int localStartY = curTy - cy * chunkH;

		if (i + 1 == chunkPath.size()) {
			// last chunk: target is goal
			int localGoalX = goalTileX - cx * chunkW;
			int localGoalY = goalTileY - cy * chunkH;
			// fetch chunk copy
			Chunk chunkCopy;
			{
				std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex());
				auto &chunks = m_chunkManager.GetChunks();
				auto it = chunks.find((static_cast<long long>(cx) << 32) | static_cast<unsigned int>(cy));
				if (it == chunks.end()) return std::nullopt;
				chunkCopy = it->second;
			}
			auto seg = LocalAStar(chunkCopy, localStartX, localStartY, localGoalX, localGoalY);
			if (!seg.has_value()) return std::nullopt;
			// append, avoid duplicating first point
			if (!finalPath.empty() && !seg->empty()) seg->erase(seg->begin());
			finalPath.insert(finalPath.end(), seg->begin(), seg->end());
			curTx = goalTileX; curTy = goalTileY;
		} else {
			// find portal between this chunk and next
			int nx = chunkPath[i+1].first;
			int ny = chunkPath[i+1].second;
			// scan shared border for candidate crossing tiles
			int dx = nx - cx; int dy = ny - cy;
			float bestDist = std::numeric_limits<float>::infinity();
			int bestAx= -1, bestAy = -1, bestBx = -1, bestBy = -1;
			// preferred point is current position center
			Vec2 pref = tileCenter(curTx, curTy);
			if (dx != 0) {
				int edgeAx = cx * chunkW + (dx > 0 ? (chunkW - 1) : 0);
				int edgeBx = nx * chunkW + (dx > 0 ? 0 : (chunkW - 1));
				for (int irow = 0; irow < chunkH; ++irow) {
					int ay = cy * chunkH + irow;
					int by = ay;
					int va = m_chunkManager.GetTileAt(edgeAx, ay);
					int vb = m_chunkManager.GetTileAt(edgeBx, by);
					if (va == 0 && vb == 0) {
						Vec2 ca = tileCenter(edgeAx, ay);
						float dxp = ca.x - pref.x; float dyp = ca.y - pref.y;
						float dist = dxp*dxp + dyp*dyp;
						if (dist < bestDist) { bestDist = dist; bestAx = edgeAx; bestAy = ay; bestBx = edgeBx; bestBy = by; }
					}
				}
			} else {
				int edgeAy = cy * chunkH + (dy > 0 ? (chunkH - 1) : 0);
				int edgeBy = ny * chunkH + (dy > 0 ? 0 : (chunkH - 1));
				for (int icol = 0; icol < chunkW; ++icol) {
					int ax = cx * chunkW + icol;
					int bx = nx * chunkW + icol;
					int va = m_chunkManager.GetTileAt(ax, edgeAy);
					int vb = m_chunkManager.GetTileAt(bx, edgeBy);
					if (va == 0 && vb == 0) {
						Vec2 ca = tileCenter(ax, edgeAy);
						float dxp = ca.x - pref.x; float dyp = ca.y - pref.y;
						float dist = dxp*dxp + dyp*dyp;
						if (dist < bestDist) { bestDist = dist; bestAx = ax; bestAy = edgeAy; bestBx = bx; bestBy = edgeBy; }
					}
				}
			}
			if (bestAx == -1) return std::nullopt; // no portal found

			// compute local coords in current chunk
			int exitLocalX = bestAx - cx * chunkW;
			int exitLocalY = bestAy - cy * chunkH;

			// fetch chunk copy
			Chunk chunkCopy;
			{
				std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex());
				auto &chunks = m_chunkManager.GetChunks();
				auto it = chunks.find((static_cast<long long>(cx) << 32) | static_cast<unsigned int>(cy));
				if (it == chunks.end()) return std::nullopt;
				chunkCopy = it->second;
			}

			auto seg = LocalAStar(chunkCopy, localStartX, localStartY, exitLocalX, exitLocalY);
			if (!seg.has_value()) return std::nullopt;
			if (!finalPath.empty() && !seg->empty()) seg->erase(seg->begin());
			finalPath.insert(finalPath.end(), seg->begin(), seg->end());

			// set next chunk start as the corresponding tile in next chunk (bestBx,bestBy)
			curTx = bestBx; curTy = bestBy;
		}
	}

	return finalPath;
}
/////////////////////////////////



/////////////////////////////////
// FindChunkPath - High-level A* pathfinding on the chunk graph, returning a list of chunk coordinates that form the path from the
bool Pathfinder::FindChunkPath(int sx, int sy, int gx, int gy, std::vector<std::pair<int, int>>& outChunks) {
	outChunks.clear();
	const int cw = m_chunkManager.GetChunkWidth();
	const int ch = m_chunkManager.GetChunkHeight();
	if (cw <= 0 || ch <= 0) return false;

	auto floorDiv = [](int a, int b) {
		if (b <= 0) return 0;
		if (a >= 0) return a / b;
		return -(((-a) + b - 1) / b);
	};

	const int scx = floorDiv(sx, cw);
	const int scy = floorDiv(sy, ch);
	const int gcx = floorDiv(gx, cw);
	const int gcy = floorDiv(gy, ch);

	if (scx == gcx && scy == gcy) {
		outChunks.emplace_back(scx, scy);
		return true;
	}

	auto keyFor = [](int x, int y) {
		return (static_cast<long long>(x) << 32) | static_cast<unsigned int>(y);
	};

	struct Node { int x,y; float g; long long parent; };
	std::unordered_map<long long, Node> nodes;

	using PQItem = std::pair<float, long long>; // f, key
	std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> open;

	auto pushNode = [&](int x, int y, float g, long long parent) {
		long long k = keyFor(x,y);
		auto it = nodes.find(k);
		if (it == nodes.end() || g < it->second.g) {
			nodes[k] = {x,y,g,parent};
			float h = HeuristicChunk(x,y,gcx,gcy);
			open.push({g + h, k});
		}
	};

	pushNode(scx, scy, 0.0f, -1);

	const int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};

	auto chunkHasConnection = [&](int cx, int cy, int nx, int ny) {
		// check adjacency (orthogonal or diagonal)
		int dx = nx - cx;
		int dy = ny - cy;
		if (std::abs(dx) > 1 || std::abs(dy) > 1) return false;

		// orthogonal neighbor
		if (std::abs(dx) + std::abs(dy) == 1) {
			if (dx != 0) {
				// vertical strip along y
				int edgeX_A = cx * cw + (dx > 0 ? (cw - 1) : 0);
				int edgeX_B = nx * cw + (dx > 0 ? 0 : (cw - 1));
				for (int i = 0; i < ch; ++i) {
					int ay = cy * ch + i;
					int txA = edgeX_A;
					int txB = edgeX_B;
					int va = m_chunkManager.GetTileAt(txA, ay);
					int vb = m_chunkManager.GetTileAt(txB, ay);
					if (va == 0 && vb == 0) return true;
				}
			} else {
				// horizontal strip along x
				int edgeY_A = cy * ch + (dy > 0 ? (ch - 1) : 0);
				int edgeY_B = ny * ch + (dy > 0 ? 0 : (ch - 1));
				for (int i = 0; i < cw; ++i) {
					int ax = cx * cw + i;
					int bx = nx * cw + i;
					int va = m_chunkManager.GetTileAt(ax, edgeY_A);
					int vb = m_chunkManager.GetTileAt(bx, edgeY_B);
					if (va == 0 && vb == 0) return true;
				}
			}
			return false;
		}

		// diagonal neighbor: check corner-adjacent tile pair and avoid corner-cut across blocked orthogonals
		if (std::abs(dx) == 1 && std::abs(dy) == 1) {
			int ax = cx * cw + (dx > 0 ? (cw - 1) : 0);
			int ay = cy * ch + (dy > 0 ? (ch - 1) : 0);
			int bx = nx * cw + (dx > 0 ? 0 : (cw - 1));
			int by = ny * ch + (dy > 0 ? 0 : (ch - 1));
			int va = m_chunkManager.GetTileAt(ax, ay);
			int vb = m_chunkManager.GetTileAt(bx, by);
			if (va != 0 || vb != 0) return false;
			// prevent corner-cut across blocked orthogonals: require at least one adjacent orthogonal tile free
			int orth1x = ax + dx; int orth1y = ay; // tile across horizontal
			int orth2x = ax; int orth2y = ay + dy; // tile across vertical
			int o1 = m_chunkManager.GetTileAt(orth1x, orth1y);
			int o2 = m_chunkManager.GetTileAt(orth2x, orth2y);
			if (o1 == 0 || o2 == 0) return true;
			return false;
		}

		return false;
	};

	std::unordered_map<long long, bool> closed;
	bool found = false;
	long long goalKey = keyFor(gcx, gcy);

	const float DIAG = std::sqrt(2.0f);
	while (!open.empty()) {
		auto [f, k] = open.top(); open.pop();
		if (closed[k]) continue;
		closed[k] = true;
		const Node cur = nodes[k];
		if (k == goalKey) { found = true; break; }
		for (auto &d : dirs) {
			int nx = cur.x + d[0];
			int ny = cur.y + d[1];
			if (!chunkHasConnection(cur.x, cur.y, nx, ny)) continue;
			float moveCost = (std::abs(d[0]) + std::abs(d[1]) == 2) ? DIAG : 1.0f;
			pushNode(nx, ny, cur.g + moveCost, k);
		}
	}

	if (!found) return false;

	// reconstruct path
	long long curK = goalKey;
	while (curK != -1) {
		auto it = nodes.find(curK);
		if (it == nodes.end()) break;
		outChunks.emplace_back(it->second.x, it->second.y);
		curK = it->second.parent;
	}
	std::reverse(outChunks.begin(), outChunks.end());
	return true;
}
/////////////////////////////////



/////////////////////////////////
// LocalAStar - Low-level A* pathfinding inside a single chunk (tile coords relative to chunk)
std::optional<std::vector<Vec2>> Pathfinder::LocalAStar(const Chunk& chunk, int sx, int sy, int gx, int gy) {
	const int w = chunk.width;
	const int h = chunk.height;
	if (sx < 0 || sx >= w || sy < 0 || sy >= h) return std::nullopt;
	if (gx < 0 || gx >= w || gy < 0 || gy >= h) return std::nullopt;

	auto idx = [&](int x, int y) { return y * w + x; };
	const int start = idx(sx, sy);
	const int goal = idx(gx, gy);

	const float INF = std::numeric_limits<float>::infinity();
	std::vector<float> g(w * h, INF);
	std::vector<int> parent(w * h, -1);
	std::vector<char> closed(w * h, 0);

	using PQItem = std::pair<float, int>;
	std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> open;

	const float SQRT2 = std::sqrt(2.0f);
	auto heuristic = [&](int x, int y) {
		int dx = std::abs(x - gx);
		int dy = std::abs(y - gy);
		int mn = std::min(dx, dy);
		int mx = std::max(dx, dy);
		return static_cast<float>((mx - mn) + mn * SQRT2);
	};

	// walkable check using chunk's primary layer
	auto walkable = [&](int x, int y) {
		int v = chunk.GetTileSingleLayer(x, y);
		return v == 0;
	};

	if (!walkable(sx, sy) || !walkable(gx, gy)) return std::nullopt;

	g[start] = 0.0f;
	open.push({heuristic(sx, sy), start});

	// 8-directional moves (dx,dy)
	const int dirs8[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
	bool found = false;

	while (!open.empty()) {
		auto [f, cur] = open.top(); open.pop();
		if (closed[cur]) continue;
		closed[cur] = 1;
		if (cur == goal) { found = true; break; }
		int cx = cur % w;
		int cy = cur / w;
		for (auto &d : dirs8) {
			int nx = cx + d[0];
			int ny = cy + d[1];
			if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
			int ni = idx(nx, ny);
			if (closed[ni]) continue;
			if (!walkable(nx, ny)) continue;
			// corner cutting prevention for diagonal moves
			if (d[0] != 0 && d[1] != 0) {
				if (!walkable(cx + d[0], cy) || !walkable(cx, cy + d[1])) continue;
			}
			float moveCost = (d[0] != 0 && d[1] != 0) ? SQRT2 : 1.0f;
			float ng = g[cur] + moveCost;
			if (ng < g[ni]) {
				g[ni] = ng;
				parent[ni] = cur;
				open.push({ng + heuristic(nx, ny), ni});
			}
		}
	}

	if (!found) return std::nullopt;

	// reconstruct path as world-space points (tile centers)
	std::vector<Vec2> out;
	int p = goal;
	float ts = chunk.tileSize;
	while (p != -1) {
		int tx = p % w;
		int ty = p / w;
		int worldTx = chunk.chunkX * chunk.width + tx;
		int worldTy = chunk.chunkY * chunk.height + ty;
		float wx = worldTx * ts + ts * 0.5f;
		float wy = worldTy * ts + ts * 0.5f;
		out.emplace_back(wx, wy);
		p = parent[p];
	}
	std::reverse(out.begin(), out.end());
	return out;
}
/////////////////////////////////



/////////////////////////////////
// EnsureChunksForPath - Ensures that the necessary chunks are loaded for pathfinding between the start and goal tile coordinates.
void Pathfinder::EnsureChunksForPath(int sx, int sy, int gx, int gy) {
	const int cw = m_chunkManager.GetChunkWidth();
	const int ch = m_chunkManager.GetChunkHeight();
	if (cw <= 0 || ch <= 0) return;
	auto floorDiv = [](int a, int b) {
		if (a >= 0) return a / b;
		return -(((-a) + b - 1) / b);
	};
	int scx = floorDiv(sx, cw);
	int scy = floorDiv(sy, ch);
	int gcx = floorDiv(gx, cw);
	int gcy = floorDiv(gy, ch);
	int minChunkX = std::min(scx, gcx);
	int minChunkY = std::min(scy, gcy);
	int maxChunkX = std::max(scx, gcx);
	int maxChunkY = std::max(scy, gcy);

	int minTileX = minChunkX * cw;
	int minTileY = minChunkY * ch;
	int maxTileX = (maxChunkX + 1) * cw - 1;
	int maxTileY = (maxChunkY + 1) * ch - 1;
	// Request a margin of 1 chunk around path area
	m_chunkManager.EnsureChunksInTileRect(minTileX, minTileY, maxTileX, maxTileY, 1);
}
/////////////////////////////////



/////////////////////////////////
// HeuristicChunk - Calculates the heuristic cost between two chunks based on their coordinates. This function is used in the A* algorithm 
// to estimate the cost of reaching the goal chunk from the current chunk.
float Pathfinder::HeuristicChunk(int cx, int cy, int tx, int ty) const {
	const float SQRT2 = std::sqrt(2.0f);
	int dx = std::abs(cx - tx);
	int dy = std::abs(cy - ty);
	int mn = std::min(dx, dy);
	int mx = std::max(dx, dy);
	return static_cast<float>((mx - mn) + mn * SQRT2);
}
/////////////////////////////////
