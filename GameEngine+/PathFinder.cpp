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
// FindPath - Finds a path from the start tile to the goal tile using A* algorithm. Returns an optional Path containing world positions if a path is found,
std::optional<Path> Pathfinder::FindPath(int startTileX, int startTileY, int goalTileX, int goalTileY) {
	// Hierarchical pathfinding: find chunk-level path then local A* per chunk
	EnsureChunksForPath(startTileX, startTileY, goalTileX, goalTileY);

	const int chunkW = m_chunkManager.GetChunkWidth();
	const int chunkH = m_chunkManager.GetChunkHeight();
	if (chunkW <= 0 || chunkH <= 0) return std::nullopt;

	// Get the chunk coordinate offset (minimum chunk coordinates)
	int minChunkX = 0, minChunkY = 0;
	m_chunkManager.GetMinChunkCoords(minChunkX, minChunkY);

	auto floorDiv = [](int a, int b) {
		if (a >= 0) return a / b;
		return -(((-a) + b - 1) / b);
	};

	// Tile coordinates are relative to world minimum and in tile-space.
	// Convert to world chunk coordinates by accounting for the map origin.
	// Chunk coords = floorDiv(tile, chunkSize) + minChunk
	// But we also need to account for the world origin offset in the tile calculation.
	// Since tiles are calculated as (worldPos - minWorldPos) / tileSize,
	// and chunks are calculated as worldPos / (chunkSize * tileSize),
	// we need to use: chunkCoord = minChunkCoord + floorDiv(tileCoord, chunkSize)

	const int startChunkX = minChunkX + floorDiv(startTileX, chunkW);
	const int startChunkY = minChunkY + floorDiv(startTileY, chunkH);
	const int goalChunkX = minChunkX + floorDiv(goalTileX, chunkW);
	const int goalChunkY = minChunkY + floorDiv(goalTileY, chunkH);

	std::vector<std::pair<int,int>> chunkPath;
	if (!FindChunkPath(startTileX, startTileY, goalTileX, goalTileY, chunkPath)) {
		std::cout << "[PathFinder] Chunk pathfinding failed from (" << startTileX << "," << startTileY 
				  << ") to (" << goalTileX << "," << goalTileY << "). Using fallback bounding-box A*.\n";

		// Fallback: create a virtual chunk covering the bounding rectangle and use LocalAStar
		// This ensures 8-directional diagonal movement with optimal heuristics
		const int minX = std::min(startTileX, goalTileX);
		const int minY = std::min(startTileY, goalTileY);
		const int maxX = std::max(startTileX, goalTileX);
		const int maxY = std::max(startTileY, goalTileY);

		const int width = maxX - minX + 1;
		const int height = maxY - minY + 1;

		// Limit to prevent excessive memory allocation
		const int maxCells = 1000000; // Increased from 200000 to support longer paths
		if (width <= 0 || height <= 0 || (width * height) > maxCells) {
			std::cout << "[PathFinder] Bounding box too large: " << width << "x" << height << " = " << (width * height) << " cells\n";
			return std::nullopt;
		}

		// Check if start/goal are walkable (check layer 1 only - obstacles)
		int startTile = m_chunkManager.GetTileAt(startTileX, startTileY, 1);
		int goalTile = m_chunkManager.GetTileAt(goalTileX, goalTileY, 1);
		//std::cout << "[PathFinder] Checking start (" << startTileX << "," << startTileY << ") layer 1 tile=" << startTile << "\n";
		//std::cout << "[PathFinder] Checking goal (" << goalTileX << "," << goalTileY << ") layer 1 tile=" << goalTile << "\n";
		if (startTile != 0) {
			//std::cout << "[PathFinder] Start tile is blocked by obstacle (value=" << startTile << ")\n";
			return std::nullopt;
		}
		if (goalTile != 0) {
			//std::cout << "[PathFinder] Goal tile is blocked by obstacle (value=" << goalTile << ")\n";
			return std::nullopt;
		}

		// Create a virtual chunk covering the bounding rectangle
		float tileSize = 32.0f;
		{
			std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex());
			auto &chunks = m_chunkManager.GetChunks();
			if (!chunks.empty()) tileSize = chunks.begin()->second.tileSize;
		}

		// Create virtual chunk with correct number of layers
		int numLayers = m_chunkManager.GetNumLayers();
		Chunk virtualChunk(0, 0, width, height, tileSize, numLayers);

		// Populate tile data from the chunk manager - copy ALL layers
		for (int y = minY; y <= maxY; ++y) {
			for (int x = minX; x <= maxX; ++x) {
				int localX = x - minX;
				int localY = y - minY;
				// Copy all layers from the chunk manager to the virtual chunk
				for (int layer = 0; layer < numLayers; ++layer) {
					int tileValue = m_chunkManager.GetTileAt(x, y, layer);
					virtualChunk.tilesPerLayer[layer][localY * width + localX] = tileValue;
				}
			}
		}

		// DEBUG: Check virtual chunk layer 1 (obstacle layer)
		{
			int layer1NonZero = 0;
			if (virtualChunk.tilesPerLayer.size() > 1) {
				for (int t : virtualChunk.tilesPerLayer[1]) {
					if (t != 0) layer1NonZero++;
				}
			}
			//std::cout << "[PathFinder] Virtual chunk layer 1 has " << layer1NonZero << " non-zero tiles\n";
		}

		// Call LocalAStar with local coordinates (relative to the virtual chunk)
		int localStartX = startTileX - minX;
		int localStartY = startTileY - minY;
		int localGoalX = goalTileX - minX;
		int localGoalY = goalTileY - minY;

		auto result = LocalAStar(virtualChunk, localStartX, localStartY, localGoalX, localGoalY);
		if (result.has_value() && !result->empty()) {
			//std::cout << "[PathFinder] Fallback LocalAStar succeeded with " << result->size() << " waypoints\n";

			// Note: LocalAStar will have computed positions based on chunkX=0, chunkY=0, so positions are already correct world-space coordinates since local (tx,ty)
			// gets converted as:	worldTx = 0*width + tx = tx,	which is what we want (minX+tx converted to world space). We need to verify this is correct...
			// LocalAStar does:		wx = worldTx * tileSize + tileSize*0.5 with	chunkX=0: worldTx = 0 * width + tx = tx (ranges 0 to width-1)
			// But we want:			worldTx = minX + tx (absolute world tile)
			// So we need to offset all returned positions!
			
			float offsetX = minX * tileSize;
			float offsetY = minY * tileSize;
			for (auto& pos : *result) {
				pos.x += offsetX;
				pos.y += offsetY;
			}
		} else if (!result.has_value()) {
			// Debug: LocalAStar failed due to invalid input or other error
			std::cout << "[PathFinder] Fallback LocalAStar also failed!\n";
		}
		return result;
	}

	// Debug : print chunk path
	//std::cout << "[PathFinder] Chunk pathfinding succeeded, found " << chunkPath.size() << " chunks\n";

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

	//std::cout << "[PathFinder] Starting path stitching with chunkW=" << chunkW << " chunkH=" << chunkH << "\n";
	//std::cout << "[PathFinder] Start world tile: (" << startTileX << "," << startTileY << ")\n";
	//std::cout << "[PathFinder] Chunk path: ";
	//for (auto& c : chunkPath) std::cout << "(" << c.first << "," << c.second << ") ";
	//std::cout << "\n";

	for (size_t i = 0; i < chunkPath.size(); ++i) {
		int cx = chunkPath[i].first;
		int cy = chunkPath[i].second;
		// compute local coords
		int localStartX = curTx - cx * chunkW;
		int localStartY = curTy - cy * chunkH;

		//std::cout << "[PathFinder] Iteration " << i << ": chunk (" << cx << "," << cy << ") "
		//		  << "curTx,curTy=(" << curTx << "," << curTy << ") "
		//		  << "localStart=(" << localStartX << "," << localStartY << ")\n";

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
				//std::cout << "[PathFinder] Chunk (" << cx << "," << cy << ") Layer 0: " << chunkCopy.tilesPerLayer[0].size() 
				//		  << ", Layer 1: " << (chunkCopy.tilesPerLayer.size() > 1 ? chunkCopy.tilesPerLayer[1].size() : 0) << "\n";
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
			//std::cout << "[PathFinder] Looking for portal: dx=" << dx << " dy=" << dy << "\n";
			float bestDist = std::numeric_limits<float>::infinity();
			int bestAx= INT_MIN, bestAy = INT_MIN, bestBx = INT_MIN, bestBy = INT_MIN;
			// preferred point is current position center
			Vec2 pref = tileCenter(curTx, curTy);

			// For diagonal moves, we need to find a corner crossing point
			if (dx != 0 && dy != 0) {
				// Diagonal move: check the corner tiles where both chunks meet
				int cornerAx = cx * chunkW + (dx > 0 ? (chunkW - 1) : 0);
				int cornerAy = cy * chunkH + (dy > 0 ? (chunkH - 1) : 0);
				int cornerBx = nx * chunkW + (dx > 0 ? 0 : (chunkW - 1));
				int cornerBy = ny * chunkH + (dy > 0 ? 0 : (chunkH - 1));

				// Check both layer 0 (surface) and layer 1 (obstacles) for corners
				int l0a = m_chunkManager.GetTileAt(cornerAx, cornerAy, 0);  // Layer 0
				int l1a = m_chunkManager.GetTileAt(cornerAx, cornerAy, 1);  // Layer 1
				int l0b = m_chunkManager.GetTileAt(cornerBx, cornerBy, 0);  // Layer 0
				int l1b = m_chunkManager.GetTileAt(cornerBx, cornerBy, 1);  // Layer 1

				// Both corners must have surface and no obstacles
				if ((l0a != 0 && l1a == 0) && (l0b != 0 && l1b == 0)) {
					// Also check that we can actually cross diagonally (not blocked by orthogonal neighbors)
					int orth1x = cornerAx + dx; int orth1y = cornerAy;
					int orth2x = cornerAx; int orth2y = cornerAy + dy;
					int o1_l0 = m_chunkManager.GetTileAt(orth1x, orth1y, 0);
					int o1_l1 = m_chunkManager.GetTileAt(orth1x, orth1y, 1);
					int o2_l0 = m_chunkManager.GetTileAt(orth2x, orth2y, 0);
					int o2_l1 = m_chunkManager.GetTileAt(orth2x, orth2y, 1);
					// At least one orthogonal neighbor must be walkable
					bool orth1Walkable = (o1_l0 != 0 && o1_l1 == 0);
					bool orth2Walkable = (o2_l0 != 0 && o2_l1 == 0);
					if (orth1Walkable || orth2Walkable) {
						bestAx = cornerAx; bestAy = cornerAy;
						bestBx = cornerBx; bestBy = cornerBy;
						bestDist = 0; // Perfect corner match
					}
				}

				// If diagonal corner crossing not possible, fall back to orthogonal edges
				if (bestAx == INT_MIN) {
					// Try horizontal edge
					int edgeAx = cx * chunkW + (dx > 0 ? (chunkW - 1) : 0);
					int edgeBx = nx * chunkW + (dx > 0 ? 0 : (chunkW - 1));
					for (int irow = 0; irow < chunkH; ++irow) {
						int ay = cy * chunkH + irow;
						int by = ny * chunkH + irow; // FIXED: by should be in chunk ny, not cy!
						int va = m_chunkManager.GetTileAt(edgeAx, ay, 1);  // Layer 1
						int vb = m_chunkManager.GetTileAt(edgeBx, by, 1);  // Layer 1
						if (va == 0 && vb == 0) {
							Vec2 ca = tileCenter(edgeAx, ay);
							float dxp = ca.x - pref.x; float dyp = ca.y - pref.y;
							float dist = dxp*dxp + dyp*dyp;
							if (dist < bestDist) { bestDist = dist; bestAx = edgeAx; bestAy = ay; bestBx = edgeBx; bestBy = by; }
						}
					}

					// Try vertical edge  
					if (bestAx == INT_MIN || bestDist > 1000000) {
						int edgeAy = cy * chunkH + (dy > 0 ? (chunkH - 1) : 0);
						int edgeBy = ny * chunkH + (dy > 0 ? 0 : (chunkH - 1));
						for (int icol = 0; icol < chunkW; ++icol) {
							int ax = cx * chunkW + icol;
							int bx = nx * chunkW + icol; // FIXED: bx should be in chunk nx, not cx!
							int va = m_chunkManager.GetTileAt(ax, edgeAy, 1);  // Layer 1
							int vb = m_chunkManager.GetTileAt(bx, edgeBy, 1);  // Layer 1
							if (va == 0 && vb == 0) {
								Vec2 ca = tileCenter(ax, edgeAy);
								float dxp = ca.x - pref.x; float dyp = ca.y - pref.y;
								float dist = dxp*dxp + dyp*dyp;
								if (dist < bestDist) { bestDist = dist; bestAx = ax; bestAy = edgeAy; bestBx = bx; bestBy = edgeBy; }
							}
						}
					}
				}
			} else if (dx != 0) {
				// Horizontal move only
				int edgeAx = cx * chunkW + (dx > 0 ? (chunkW - 1) : 0);
				int edgeBx = nx * chunkW + (dx > 0 ? 0 : (chunkW - 1));
				int portalCount = 0;
				for (int irow = 0; irow < chunkH; ++irow) {
					int ay = cy * chunkH + irow;
					int by = ay;
					// Check both layer 0 (surface) and layer 1 (obstacles)
					int l0a = m_chunkManager.GetTileAt(edgeAx, ay, 0);  // Layer 0
					int l1a = m_chunkManager.GetTileAt(edgeAx, ay, 1);  // Layer 1
					int l0b = m_chunkManager.GetTileAt(edgeBx, by, 0);  // Layer 0
					int l1b = m_chunkManager.GetTileAt(edgeBx, by, 1);  // Layer 1
					// Valid crossing if: both have surface (layer 0) AND neither has obstacle (layer 1)
					if ((l0a != 0 && l1a == 0) && (l0b != 0 && l1b == 0)) {
						portalCount++;
						Vec2 ca = tileCenter(edgeAx, ay);
						float dxp = ca.x - pref.x; float dyp = ca.y - pref.y;
						float dist = dxp*dxp + dyp*dyp;
						if (dist < bestDist) { bestDist = dist; bestAx = edgeAx; bestAy = ay; bestBx = edgeBx; bestBy = by; }
					}
				}
				//if (bestDist > 1000000 && portalCount > 0) {
				//	std::cout << "[PathFinder] Horizontal edge search: found " << portalCount << " possible crossings, selected none (too far)\n";
				//} else if (portalCount == 0) {
				//	std::cout << "[PathFinder] Horizontal edge search: no crossing points available (all blocked)\n";
				//} else {
				//	std::cout << "[PathFinder] Horizontal edge search: found " << portalCount << " crossings, selected best at (" << bestAx << "," << bestAy << ")\n";
				//}
			} else if (dy != 0) {
				// Vertical move only
				int edgeAy = cy * chunkH + (dy > 0 ? (chunkH - 1) : 0);
				int edgeBy = ny * chunkH + (dy > 0 ? 0 : (chunkH - 1));
				int portalCount = 0;
				for (int icol = 0; icol < chunkW; ++icol) {
					int ax = cx * chunkW + icol;
					int bx = nx * chunkW + icol;
					// Check both layer 0 (surface) and layer 1 (obstacles)
					int l0a = m_chunkManager.GetTileAt(ax, edgeAy, 0);  // Layer 0
					int l1a = m_chunkManager.GetTileAt(ax, edgeAy, 1);  // Layer 1
					int l0b = m_chunkManager.GetTileAt(bx, edgeBy, 0);  // Layer 0
					int l1b = m_chunkManager.GetTileAt(bx, edgeBy, 1);  // Layer 1
					// Valid crossing if: both have surface (layer 0) AND neither has obstacle (layer 1)
					if ((l0a != 0 && l1a == 0) && (l0b != 0 && l1b == 0)) {
						portalCount++;
						Vec2 ca = tileCenter(ax, edgeAy);
						float dxp = ca.x - pref.x; float dyp = ca.y - pref.y;
						float dist = dxp*dxp + dyp*dyp;
						if (dist < bestDist) { bestDist = dist; bestAx = ax; bestAy = edgeAy; bestBx = bx; bestBy = edgeBy; }
					}
				}
				//if (bestDist > 1000000) {
				//	std::cout << "[PathFinder] Vertical edge search: found " << portalCount << " possible crossings, selected none\n";
				//}
			}
			if (bestAx == INT_MIN) return std::nullopt; // no portal found

			// compute local coords in current chunk
			int exitLocalX = bestAx - cx * chunkW;
			int exitLocalY = bestAy - cy * chunkH;

			//std::cout << "[PathFinder] Chunk (" << cx << "," << cy << ") -> (" << (nx) << "," << (ny) 
			//		  << ") portal at world (" << bestAx << "," << bestAy << ") local exit (" 
			//		  << exitLocalX << "," << exitLocalY << ") start (" << localStartX << "," 
			//		  << localStartY << ")\n";

			// Entry point in next chunk for debugging
			//std::cout << "[PathFinder]   Entry in next chunk: bestBx=" << bestBx << " bestBy=" << bestBy 
			//		  << " (next chunk " << nx << "," << ny << " range X=" << (nx*chunkW) << ".." 
			//		  << (nx*chunkW + chunkW - 1) << " Y=" << (ny*chunkH) << ".." << (ny*chunkH + chunkH - 1) << ")\n";

			// fetch chunk copy
			Chunk chunkCopy;
			{
				std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex());
				auto &chunks = m_chunkManager.GetChunks();
				auto it = chunks.find((static_cast<long long>(cx) << 32) | static_cast<unsigned int>(cy));
				if (it == chunks.end()) {
					//std::cout << "[PathFinder] ERROR: Chunk (" << cx << "," << cy << ") not loaded!\n";
					return std::nullopt;
				}
				chunkCopy = it->second;
			}

			// TEMP DEBUG: check layer 0 and 1 data
			{
				int layer0NonZero = 0, layer1NonZero = 0;
				if (chunkCopy.tilesPerLayer.size() > 0) {
					for (int t : chunkCopy.tilesPerLayer[0]) {
						if (t != 0) layer0NonZero++;
					}
				}
				if (chunkCopy.tilesPerLayer.size() > 1) {
					for (int t : chunkCopy.tilesPerLayer[1]) {
						if (t != 0) layer1NonZero++;
					}
				}
				//std::cout << "[PathFinder] Chunk (" << cx << "," << cy << ") Layer 0: " << layer0NonZero << ", Layer 1: " << layer1NonZero << " non-zero tiles\n";
			}
			//std::cout << "[PathFinder] Calling LocalAStar with localStart=(" << localStartX << "," << localStartY 
			//		  << ") localExit=(" << exitLocalX << "," << exitLocalY << ")\n";

			auto seg = LocalAStar(chunkCopy, localStartX, localStartY, exitLocalX, exitLocalY);
			if (!seg.has_value()) {
				//std::cout << "[PathFinder] ERROR: LocalAStar failed in chunk (" << cx << "," << cy << ")\n";
				return std::nullopt;
			}
			if (!finalPath.empty() && !seg->empty()) seg->erase(seg->begin());
			finalPath.insert(finalPath.end(), seg->begin(), seg->end());

			// set next chunk start as the corresponding tile in next chunk (bestBx,bestBy)
			curTx = bestBx; curTy = bestBy;
			//std::cout << "[PathFinder]   Updated curTx,curTy to (" << curTx << "," << curTy << ")\n";
		}
	}

	return finalPath;
}
/////////////////////////////////



/////////////////////////////////
// FindChunkPath - High-level A* pathfinding on the chunk graph, returning a list of chunk coordinates that form the path from the
// start to goal chunks. Tiles sx,sy,gx,gy are absolute world-space tile coordinates.
bool Pathfinder::FindChunkPath(int startX, int startY, int goalX, int goalY, std::vector<std::pair<int, int>>& outChunks) {

	// Ensure the output chunks vector is empty before starting
	outChunks.clear();

	// Lets store a local cache of chunk dimensions from the chunk manager
	const int chunkW = m_chunkManager.GetChunkWidth();
	const int chunkH = m_chunkManager.GetChunkHeight();

	// Guard against invalid chunk dimensions
	if (chunkW <= 0 || chunkH <= 0) return false;

	// Internal helper to perform floor division for negative coordinates, ensuring correct chunk mapping 
	auto floorDiv = [](int a, int b) {
		if (b <= 0) return 0;
		if (a >= 0) return a / b;
		return -(((-a) + b - 1) / b);
	};

	// Convert absolute world tiles directly to chunk coordinates
	const int startChunkX = floorDiv(startX, chunkW);
	const int startChunkY = floorDiv(startY, chunkH);
	const int goalChunkX = floorDiv(goalX, chunkW);
	const int goalChunkY = floorDiv(goalY, chunkH);

	// If the start and goal chunks are the same, we can return immediately with that chunk
	if (startChunkX == goalChunkX && startChunkY == goalChunkY) {
		outChunks.emplace_back(startChunkX, startChunkY);
		return true;
	}

	// Compute a unique key for coordinates (x,y) to use in hash maps.
	auto keyFor = [](int x, int y) {
		// Use a 64-bit integer to combine x and y into a unique key; 
		// Shift x to the upper 32 bits and y to the lower 32 bits.
		return (static_cast<long long>(x) << 32) | static_cast<unsigned int>(y); 
	};

	// Heuristic function for A* search: Manhattan distance between two chunk coordinates.
	struct Node { int x,y; float g; long long parent; };
	std::unordered_map<long long, Node> nodes;
	
	using PQItem = std::pair<float, long long>; // PQItem is a pair consisting of the f-cost (float) and a unique key (long long)
												// representing a node in the search space. The f-cost is used to prioritize nodes
												// in the open set, with lower f-costs being processed first.

	// Priority queue for the open set in A* search, ordered by f-cost (g + h). The std::greater comparator ensures that the 
	// lowest f-cost is processed first.
	std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> open;

	// **** LAMBDA START: pushNode ****
	// Lambda function to push a node into the open set if it is either new or has a lower g-cost than previously recorded.
	auto pushNode = [&](int x, int y, float g, long long parent) {
		long long k = keyFor(x,y);
		auto it = nodes.find(k);
		if (it == nodes.end() || g < it->second.g) {
			nodes[k] = {x,y,g,parent};
			float h = HeuristicChunk(x,y,goalChunkX,goalChunkY);
			open.push({g + h, k});
		}
	};
	// **** LAMBDA END: pushNode ****

	pushNode(startChunkX, startChunkY, 0.0f, -1);

	// **** LAMBDA START: chunkHasConnection ****
	// Lambda function to determine if two chunks (cx,cy) and (nx,ny) are connected, meaning there is a valid path between them.
	auto chunkHasConnection = [&](int cx, int cy, int nx, int ny) {
		// check adjacency (orthogonal or diagonal)
		int dx = nx - cx;
		int dy = ny - cy;
		if (std::abs(dx) > 1 || std::abs(dy) > 1) return false;


		// *** LAMBDA START: isBlocked ***
		// Helper: check if a tile is blocked (layer 1 only - obstacle layer); we're doing lambda inception here
		auto isBlocked = [this](int tx, int ty) {
			// Only check layer 1 (main/obstacle layer)
			int tileValue = m_chunkManager.GetTileAt(tx, ty, 1);
			return tileValue != 0;  // Blocked if layer 1 has a tile
		};
		// *** LAMBDA END: isBlocked ***

		// Check orthogonal neighbor connection first
		bool retFlag;
		bool retVal = CheckOrthogonalNeighbour(dx, dy, cx, chunkW, nx, chunkH, cy, isBlocked, ny, retFlag);
		if (retFlag) return retVal;

		retVal = CheckDiagonalNeighbour(dx, dy, cx, chunkW, cy, chunkH, nx, ny, isBlocked, retFlag);
		if (retFlag) return retVal;

		// If neither orthogonal nor diagonal checks were applicable, return false (no connection).
		return false;
	};
	// **** LAMBDA END: chunkHasConnection ****

	// A* search setup
	std::unordered_map<long long, bool> closed;
	bool found = false;
	long long goalKey = keyFor(goalChunkX, goalChunkY);

	const float DIAG = std::sqrt(2.0f);
	
	// Main A* Search loop, processing nodes in the open set until either the goal is found or the open set is exhausted.
	while (!open.empty()) {
		auto [f, k] = open.top(); 
		open.pop();

		auto& node = nodes[k];
		float h = HeuristicChunk(node.x, node.y, goalChunkX, goalChunkY);

		if (f != node.g + h)
			continue; // Skip if f-cost is outdated

		if (closed[k]) continue;
		closed[k] = true;

		const Node cur = nodes[k];
		if (k == goalKey) { found = true; break; }
		for (auto &d : m_dirs) {
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
// CheckDiagonalNeighbour - Check if two diagonal chunks have a valid crossing point (at least one pair of adjacent walkable tiles)
bool Pathfinder::CheckDiagonalNeighbour(int dx, int dy, int cx, const int chunkW, int cy, const int chunkH, int nx,
										int ny, std::function<bool(int, int)> isBlocked, bool& retFlag) {
	retFlag = true;
	// diagonal neighbor: check corner-adjacent tile pair and avoid corner-cut across blocked orthogonals
	if (std::abs(dx) == 1 && std::abs(dy) == 1) {
		int ax = cx * chunkW + (dx > 0 ? (chunkW - 1) : 0);
		int ay = cy * chunkH + (dy > 0 ? (chunkH - 1) : 0);
		int bx = nx * chunkW + (dx > 0 ? 0 : (chunkW - 1));
		int by = ny * chunkH + (dy > 0 ? 0 : (chunkH - 1));

		if (isBlocked(ax, ay) || isBlocked(bx, by))
			return false;
		// prevent corner-cut across blocked orthogonals: require at least one adjacent orthogonal tile free
		int orth1x = ax + dx;
		int orth1y = ay; // tile across horizontal
		int orth2x = ax;
		int orth2y = ay + dy; // tile across vertical
		if (!isBlocked(orth1x, orth1y) || !isBlocked(orth2x, orth2y))
			return true;
		return false;
	}
	retFlag = false;
	return {};
}
/////////////////////////////////



/////////////////////////////////
// CheckOrthogonalNeighbour - Check if two orthogonal chunks have at least one pair of adjacent walkable tiles along their shared edge
bool Pathfinder::CheckOrthogonalNeighbour(int dx, int dy, int cx, const int chunkW, int nx, const int chunkH, int cy,
										  std::function<bool(int, int)> isBlocked, int ny, bool& retFlag) {
	retFlag = true;
	// orthogonal neighbor, check the entire edge between the two chunks for at least one pair of adjacent tiles
	// that are both walkable (layer 1 empty).
	if (std::abs(dx) + std::abs(dy) == 1) {
		if (dx != 0) {
			// vertical strip along y
			int edgeX_A = cx * chunkW + (dx > 0 ? (chunkW - 1) : 0);
			int edgeX_B = nx * chunkW + (dx > 0 ? 0 : (chunkW - 1));

			for (int i = 0; i < chunkH; ++i) {
				int ay = cy * chunkH + i;
				int txA = edgeX_A;
				int txB = edgeX_B;
				if (!isBlocked(txA, ay) && !isBlocked(txB, ay))
					return true;
			}
		} else {
			// horizontal strip along x
			int edgeY_A = cy * chunkH + (dy > 0 ? (chunkH - 1) : 0);
			int edgeY_B = ny * chunkH + (dy > 0 ? 0 : (chunkH - 1));
			for (int i = 0; i < chunkW; ++i) {
				int ax = cx * chunkW + i;
				int bx = nx * chunkW + i;
				if (!isBlocked(ax, edgeY_A) && !isBlocked(bx, edgeY_B))
					return true;
			}
		}
		return false;
	}
	retFlag = false;
	return {};
}
/////////////////////////////////



/////////////////////////////////
// LocalAStar - Low-level A* pathfinding inside a single chunk (tile coords relative to chunk)
std::optional<std::vector<Vec2>> Pathfinder::LocalAStar(const Chunk& chunk, int sx, int sy, int gx, int gy) {
	const int w = chunk.width;
	const int h = chunk.height;
	if (sx < 0 || sx >= w || sy < 0 || sy >= h) {
		//std::cout << "[LocalAStar] Start out of bounds: (" << sx << "," << sy << ") in " << w << "x" << h << " chunk\n";
		return std::nullopt;
	}
	if (gx < 0 || gx >= w || gy < 0 || gy >= h) {
		//std::cout << "[LocalAStar] Goal out of bounds: (" << gx << "," << gy << ") in " << w << "x" << h << " chunk\n";
		return std::nullopt;
	}

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

	// walkable check for collision detection
	// A tile is walkable if: layer 0 has a tile (surface exists) AND layer 1 is empty (no obstacle)
	auto walkable = [&](int x, int y) {
		// Check if coordinates are in bounds
		if (x < 0 || x >= chunk.width || y < 0 || y >= chunk.height) return false;

		// Check layer 0 (background/surface) - must have a tile to walk on
		bool hasLayer0 = false;
		if (!chunk.tilesPerLayer.empty()) {
			int v0 = chunk.tilesPerLayer[0][y * chunk.width + x];
			hasLayer0 = (v0 != 0);
		}

		// Check layer 1 (obstacles) - must NOT have a tile
		bool layer1Clear = true;
		if (chunk.tilesPerLayer.size() > 1) {
			int v1 = chunk.tilesPerLayer[1][y * chunk.width + x];
			layer1Clear = (v1 == 0);
		}

		bool result = hasLayer0 && layer1Clear;
		//static bool printed = false;
		//if (!printed && (x == sx || x == gx) && (y == sy || y == gy)) {
		//	std::cout << "[LocalAStar] At (" << x << "," << y << ") layer0=" << (hasLayer0 ? 1 : 0) 
		//			  << " layer1=" << (!layer1Clear ? 1 : 0) << " walkable=" << result << "\n";
		//	printed = true;
		//}
		return result;  // Walkable only if layer 0 exists AND layer 1 is empty
	};

	if (!walkable(sx, sy) || !walkable(gx, gy)) {
		//std::cout << "[LocalAStar] Start or goal is not walkable\n";
		return std::nullopt;
	}

	g[start] = 0.0f;
	open.push({heuristic(sx, sy), start});

	// 8-directional moves (dx,dy)
	const int dirs8[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
	bool found = false;
	int nodesExpanded = 0;

	while (!open.empty()) {
		auto [f, cur] = open.top(); open.pop();
		if (closed[cur]) continue;
		closed[cur] = 1;
		nodesExpanded++;
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

	if (!found) {
		//std::cout << "[LocalAStar] Failed to find path. Expanded " << nodesExpanded << " nodes in " << w << "x" << h << " chunk\n";
		return std::nullopt;
	}

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
// EnsureChunksForPath - Ensures that the necessary chunks are loaded for pathfinding between the start and goal tiles.
// sx, sy, gx, gy are absolute world-space tile coordinates.
void Pathfinder::EnsureChunksForPath(int sx, int sy, int gx, int gy) {
	const int cw = m_chunkManager.GetChunkWidth();
	const int ch = m_chunkManager.GetChunkHeight();
	if (cw <= 0 || ch <= 0) return;

	auto floorDiv = [](int a, int b) {
		if (a >= 0) return a / b;
		return -(((-a) + b - 1) / b);
	};

	// Set the chunk coords for start and goal tiles
	int scx = floorDiv(sx, cw);
	int scy = floorDiv(sy, ch);
	int gcx = floorDiv(gx, cw);
	int gcy = floorDiv(gy, ch);

	// Set a bounding box of chunks that covers both start and goal chunks
	int minChunkX = std::min(scx, gcx);
	int minChunkY = std::min(scy, gcy);
	int maxChunkX = std::max(scx, gcx);
	int maxChunkY = std::max(scy, gcy);

	// Convert chunk coordinates back to tile coordinates for the bounding box
	int minTileX = minChunkX * cw;
	int minTileY = minChunkY * ch;
	int maxTileX = (maxChunkX + 1) * cw - 1; // Inclusive max tile coordinate
	int maxTileY = (maxChunkY + 1) * ch - 1; // Inclusive max tile coordinate
	// Request a margin of 1 chunk around path area
	m_chunkManager.EnsureChunksInTileRect(minTileX, minTileY, maxTileX, maxTileY, 1);
}
/////////////////////////////////



/////////////////////////////////
// HeuristicChunk - Calculates the heuristic cost between two chunks based on their coordinates.
float Pathfinder::HeuristicChunk(int cx, int cy, int tx, int ty) const {
	// Use diagonal distance heuristic for chunk-to-chunk movement	
	const float SQRT2 = std::sqrt(2.0f);

	// Calculate the differences in x and y coordinates between the current chunk and the target chunk
	int dx = std::abs(cx - tx);
	int dy = std::abs(cy - ty);
	
	// Calculate the minimum and maximum of the differences to apply the diagonal distance heuristic
	int mn = std::min(dx, dy);
	int mx = std::max(dx, dy);

	// Return the heuristic cost as a float, combining the straight and diagonal distances
	return static_cast<float>((mx - mn) + mn * SQRT2);
}
/////////////////////////////////
