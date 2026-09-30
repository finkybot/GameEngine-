/////////////////////////////////
// SpatialHashGrid.h - Spatial Hash Grid for efficient collision detection
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Vec2.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
/////////////////////////////////



/////////////////////////////////
//	|	SpatialHashGrid is a spatial partitioning data structure that divides the 2D world into a grid of cells and hashes objects into those cells based on their positions. It allows for efficient querying of nearby objects within a specified radius, which is useful for collision detection and other spatial queries in a game.
//	|	The grid is implemented using an unordered_map where the key is a hash representing the cell coordinates and the value is a vector of pointers to objects that occupy that cell.The class provides methods for inserting objects into the grid, clearing the grid, and querying for nearby objects while optionally excluding a 
//	|	specific object from the results (e.g. to avoid self-collision checks). It also includes instrumentation for tracking query performance metrics such as total queries and total objects queried.
//	|_______________________________________________________________________
template <typename T>
class SpatialHashGrid {
private:
	// The size of each cell in the grid, which determines how objects are hashed into cells based on their positions. A smaller cell size will result in more cells and potentially more precise queries, but may also increase memory usage 
	// and reduce performance if there are many objects clustered in the same area.
	float m_cellSize;

	// The grid itself, implemented as an unordered_map where the key is a hash representing the cell coordinates and the value is a vector of pointers to objects that occupy that cell. This allows for efficient storage and retrieval of objects based on their spatial location.
	std::unordered_map<size_t, std::vector<T*>> m_grid;

	// Static variables for tracking query performance metrics. These variables are incremented during query operations to allow developers to analyze the efficiency of the spatial hash grid and optimize it if necessary. They can be reset at the beginning of each 
	// frame or query session to track metrics for specific time periods.
	inline static size_t s_totalQueriesThisFrame = 0;
	inline static size_t s_totalObjectsQueried = 0;
	inline static size_t s_queryCount = 0;



private:
	// GetCellHash function to convert 2D coordinates to a unique hash for the corresponding cell. It calculates the cell coordinates by dividing the position by the cell size and then uses a pairing function (Cantor pairing) to combine the cell coordinates into a single hash value. This ensures that objects in the 
	// same cell will have the same hash, allowing for efficient storage and retrieval in the grid.
	static size_t GetCellHash(float x, float y, float cellSize) noexcept {
		int cellX = static_cast<int>(x / cellSize);
		int cellY = static_cast<int>(y / cellSize);
		return GetCellHashFromCell(cellX, cellY);
	}

	// Signed-int-safe cell hash helper. Maps signed cell coordinates to unsigned space first, then applies Cantor pairing so negative cell coords do not alias unexpectedly.
	static size_t GetCellHashFromCell(int cellX, int cellY) noexcept {
		auto toUnsigned = [](int v) -> unsigned long long {
			long long lv = static_cast<long long>(v);
			return (lv >= 0) ? static_cast<unsigned long long>(lv) * 2ULL
								  : static_cast<unsigned long long>((-lv * 2LL) - 1LL);
		};

		const unsigned long long ux = toUnsigned(cellX);
		const unsigned long long uy = toUnsigned(cellY);
		const unsigned long long sum = ux + uy;
		const unsigned long long hash = (sum * (sum + 1ULL)) / 2ULL + uy;
		return static_cast<size_t>(hash);
	}



public:
	SpatialHashGrid(float cellSize = 100.0f) : m_cellSize(cellSize) {}
	~SpatialHashGrid() { Clear(); }

	// Remove - Removes an object from the spatial grid based on pointer identity.
	void Remove(T* object) noexcept {
		if (!object)
			return;

		bool removed = false;

		// Fast path: remove from tracked cell if available.
		if (object->currentCellX != INT_MIN && object->currentCellY != INT_MIN) {
			size_t hash = GetCellHashFromCell(object->currentCellX, object->currentCellY);
			auto it = m_grid.find(hash);
			if (it != m_grid.end()) {
				auto& vec = it->second;
				auto before = vec.size();
				vec.erase(std::remove(vec.begin(), vec.end(), object), vec.end());
				removed = (vec.size() != before);
				if (vec.empty())
					m_grid.erase(it);
			}
		}

		// Fallback: if cell tracking drifted, scan all buckets and remove by pointer identity.
		if (!removed) {
			for (auto it = m_grid.begin(); it != m_grid.end();) {
				auto& vec = it->second;
				vec.erase(std::remove(vec.begin(), vec.end(), object), vec.end());
				if (vec.empty()) {
					it = m_grid.erase(it);
				} else {
					++it;
				}
			}
		}

		// Mark as no longer in any tracked cell regardless of where we found it.
		object->currentCellX = INT_MIN;
		object->currentCellY = INT_MIN;
	}

	// Update - Updates the spatial grid with the new position of an object. If the object has moved to a different cell, it is removed from its old cell and inserted into the new cell. If the object has skipped cells (moved more than one cell away), it is also removed from 
	// any intermediate cells to ensure accurate spatial partitioning.
	void Update(T* object) noexcept {
		const Vec2& pos = object->GetCentrePoint();

		int newX = static_cast<int>(pos.x / m_cellSize);
		int newY = static_cast<int>(pos.y / m_cellSize);

		int oldX = object->currentCellX;
		int oldY = object->currentCellY;

		// First-time insert
		if (oldX == INT_MIN || oldY == INT_MIN) {
			size_t hash = GetCellHashFromCell(newX, newY);
			m_grid[hash].push_back(object);
			object->currentCellX = newX;
			object->currentCellY = newY;
			return;
		}

		// Same cell → nothing to do
		if (newX == oldX && newY == oldY)
			return;

		// Remove from old cell
		size_t oldHash = GetCellHashFromCell(oldX, oldY);
		auto it = m_grid.find(oldHash);
		if (it != m_grid.end()) {
			auto& vec = it->second;
			vec.erase(std::remove(vec.begin(), vec.end(), object), vec.end());
			if (vec.empty())
				m_grid.erase(it);
		}

		// If movement skipped cells, remove from all intermediate cells
		int dx = newX - oldX;
		int dy = newY - oldY;

		if (std::abs(dx) > 1 || std::abs(dy) > 1) {
			int stepX = (dx > 0 ? 1 : -1);
			int stepY = (dy > 0 ? 1 : -1);

			int cx = oldX;
			int cy = oldY;

			while (cx != newX || cy != newY) {
				cx += (cx != newX ? stepX : 0);
				cy += (cy != newY ? stepY : 0);

				size_t skipHash = GetCellHashFromCell(cx, cy);
				auto it2 = m_grid.find(skipHash);
				if (it2 != m_grid.end()) {
					auto& vec2 = it2->second;
					vec2.erase(std::remove(vec2.begin(), vec2.end(), object), vec2.end());
					if (vec2.empty())
						m_grid.erase(it2);
				}
			}
		}

		// Insert into new cell
		size_t newHash = GetCellHashFromCell(newX, newY);
		m_grid[newHash].push_back(object);

		object->currentCellX = newX;
		object->currentCellY = newY;
	}


	// Clear - Clears all objects from the spatial grid.
	void Clear() noexcept { m_grid.clear(); }


	// ContainsPointer - checks if a pointer is currently present in any grid bucket using pointer identity only.
	bool ContainsPointer(const T* object) const noexcept {
		if (!object)
			return false;

		for (const auto& [hash, vec] : m_grid) {
			(void)hash;
			if (std::find(vec.begin(), vec.end(), object) != vec.end())
				return true;
		}
		return false;
	}


	// PruneToActiveSet - removes any pointer not present in the provided active set. Uses pointer identity only (never dereferences pointed objects).
	void PruneToActiveSet(const std::unordered_set<T*>& activeSet) noexcept {
		for (auto it = m_grid.begin(); it != m_grid.end();) {
			auto& vec = it->second;
			vec.erase(std::remove_if(vec.begin(), vec.end(), [&activeSet](T* ptr) {
				return ptr == nullptr || activeSet.find(ptr) == activeSet.end();
			}), vec.end());

			if (vec.empty()) {
				it = m_grid.erase(it);
			} else {
				++it;
			}
		}
	}


	// Insert - Inserts an object into the spatial grid based on its center point position.
	void Insert(T* object) noexcept {
		if (!object)
			return;

		const Vec2& pos = object->GetCentrePoint();

		int cellX = static_cast<int>(pos.x / m_cellSize);
		int cellY = static_cast<int>(pos.y / m_cellSize);

		// If already tracked in a different cell, remove stale membership first.
		if (object->currentCellX != INT_MIN && object->currentCellY != INT_MIN &&
			(object->currentCellX != cellX || object->currentCellY != cellY)) {
			Remove(object);
		}

		size_t hash = GetCellHashFromCell(cellX, cellY);
		auto& vec = m_grid[hash];
		if (std::find(vec.begin(), vec.end(), object) == vec.end()) {
			vec.push_back(object);
		}

		object->currentCellX = cellX;
		object->currentCellY = cellY;
	}


	// Query - Queries the spatial grid for objects within a specified radius of a given position, excluding a specific object if provided. The results are stored in the outFound vector, which is cleared at the start of the query. The method uses a set to track seen objects to avoid duplicates 
	// and calculates the squared distance to determine if an object is within the query radius.
	void Query(std::vector<T*>& outFound, const Vec2& position, float queryRadius, const T* excludeObject) const noexcept {
		++s_queryCount; // Increment query count for performance monitoring.
		outFound.clear();
		std::unordered_set<T*> seen;

		// Calculate the cell coordinates and radius in terms of cells to determine which cells to query.
		int cellX = static_cast<int>(position.GetX() / m_cellSize);
		int cellY = static_cast<int>(position.GetY() / m_cellSize);
		int cellRadius =
			static_cast<int>(queryRadius / m_cellSize) +
			1; // Get the radius in terms of cells but add 1 to ensure we cover the entire query radius even if it extends slightly beyond the last cell boundary.
		const float radiusSq = queryRadius * queryRadius;

		// Loop through all cells within the calculated cell radius and check for objects in those cells.
		for (int x = cellX - cellRadius; x <= cellX + cellRadius; ++x) {
			for (int y = cellY - cellRadius; y <= cellY + cellRadius; ++y) {
				size_t hash = GetCellHashFromCell(x, y); // Calculate the hash for the current cell coordinates using the same method as GetCellHash to ensure consistency.

				auto it = m_grid.find(hash);
				if (it != m_grid.end()) {
					for (T* obj : it->second) {
						if (obj == excludeObject)
							continue;
						if (!seen.insert(obj).second)
							continue;

						const Vec2& objPos = obj->GetCentrePoint();
						float dx = objPos.GetX() - position.GetX();
						float dy = objPos.GetY() - position.GetY();
						float distSq = dx * dx + dy * dy;

						if (distSq <= radiusSq) {
							outFound.push_back(obj);
							++s_totalObjectsQueried;
						}
					}
				}
			}
		}

		++s_totalQueriesThisFrame; // Increment total queries for performance monitoring.
	}
	 
	 
	// ResetQueryStats - Static query statistics method for performance monitoring. Resets the query statistics counters (this should be called at the start of each frame to track per-frame query performance).
	static void ResetQueryStats() noexcept {
		s_totalQueriesThisFrame = 0;
		s_totalObjectsQueried = 0;
		s_queryCount = 0;
	}


	// GetQueryCount - Static query statistics method for performance monitoring. Gets the total number of queries performed in the current frame.
	static size_t GetQueryCount() noexcept { return s_queryCount; }


	// GetTotalObjectsQueried - Static query statistics method for performance monitoring. Gets the total number of objects queried across all queries.
	static size_t GetTotalObjectsQueried() noexcept { return s_totalObjectsQueried; }


	// GetAverageObjectsPerQuery - Static query statistics method for performance monitoring. Gets the average number of objects returned per query, calculated as total objects queried divided by total queries, with a check to avoid division by zero.
	static double GetAverageObjectsPerQuery() noexcept {
		return s_queryCount > 0 ? static_cast<double>(s_totalObjectsQueried) / s_queryCount : 0.0;
	}


	// GetCellCount - Gets the total number of cells currently stored in the grid (debugging/monitoring method).
	size_t GetCellCount() const noexcept { return m_grid.size(); }


	// GetTotalObjectCount - Gets the total number of objects stored across all grid cells. (debugging/monitoring method).
	size_t GetTotalObjectCount() const noexcept {
		size_t total = 0;
		for (const auto& [hash, objects] : m_grid) {
			total += objects.size();
		}
		return total;
	}
};
/////////////////////////////////
