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
// SpatialHashGrid is a spatial partitioning data structure that divides the 2D world into a grid of cells and hashes objects into those cells based on their positions.
// It allows for efficient querying of nearby objects within a specified radius, which is useful for collision detection and other spatial queries in a game.
// The grid is implemented using an unordered_map where the key is a hash representing the cell coordinates and the value is a vector of pointers to objects that occupy that cell.
// The class provides methods for inserting objects into the grid, clearing the grid, and querying for nearby objects while optionally excluding a specific object from the results (e.g. to avoid self-collision checks).
// It also includes instrumentation for tracking query performance metrics such as total queries and total objects queried.
//								|
//								|_______________________________________________________________________
template <typename T>
class SpatialHashGrid {
	/////////////////////////////////
	// Private member variables for the SpatialHashGrid class.
private:
	/////////////////////////////////
	// The size of each cell in the grid, which determines how objects are hashed into cells based on their positions. A smaller cell size will result in more cells and potentially more precise queries, but may also increase memory usage 
	// and reduce performance if there are many objects clustered in the same area.
	float m_cellSize;
	/////////////////////////////////



	/////////////////////////////////
	// The grid itself, implemented as an unordered_map where the key is a hash representing the cell coordinates and the value is a vector of pointers to objects that occupy that cell. This allows for efficient storage and retrieval of objects based on their spatial location.
	std::unordered_map<size_t, std::vector<T*>> m_grid;
	/////////////////////////////////



	/////////////////////////////////
	// Static variables for tracking query performance metrics. These variables are incremented during query operations to allow developers to analyze the efficiency of the spatial hash grid and optimize it if necessary. They can be reset at the beginning of each 
	// frame or query session to track metrics for specific time periods.
	inline static size_t s_totalQueriesThisFrame = 0;
	inline static size_t s_totalObjectsQueried = 0;
	inline static size_t s_queryCount = 0;
	/////////////////////////////////
	 
	 

	/////////////////////////////////
	// Private methods
private:
	/////////////////////////////////
	// GetCellHash function to convert 2D coordinates to a unique hash for the corresponding cell. It calculates the cell coordinates by dividing the position by the cell size and then uses a pairing function (Cantor pairing) 
	// to combine the cell coordinates into a single hash value. This ensures that objects in the same cell will have the same hash, allowing for efficient storage and retrieval in the grid.
	static size_t GetCellHash(float x, float y, float cellSize) noexcept {
		int cellX = static_cast<int>(x / cellSize);
		int cellY = static_cast<int>(y / cellSize);
		return GetCellHashFromCell(cellX, cellY);
	}

	// Signed-int-safe cell hash helper. Maps signed cell coordinates to unsigned space first,
	// then applies Cantor pairing so negative cell coords do not alias unexpectedly.
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
	/////////////////////////////////



	/////////////////////////////////
	// Public methods for the SpatialHashGrid class.
public:
	/////////////////////////////////
	// Constructors ~ Destructors.
	SpatialHashGrid(float cellSize = 100.0f) : m_cellSize(cellSize) {}
	~SpatialHashGrid() { Clear(); }
	/////////////////////////////////



	/////////////////////////////////
	// Clear - Clears all objects from the spatial grid.
	void Clear() noexcept { m_grid.clear(); }
	/////////////////////////////////



	/////////////////////////////////
	// Insert - Inserts an object into the spatial grid based on its center point position.
	void Insert(T* object) noexcept {
		const Vec2& pos = object->GetCentrePoint();
		size_t hash = GetCellHash(pos.GetX(), pos.GetY(), m_cellSize);
		m_grid[hash].push_back(object);
	}
	/////////////////////////////////






	/////////////////////////////////
	// Query - Overloaded version of the Query method. Performs a spatial query to find all objects within a specified radius of a position using a grid-based spatial hash. I'll store the results in the provided outFound vector, this version of the method
	// will include any objects found within the query radius, excluding the object passed to the query (e.g. the object performing the query so theres no self collision). The query works by checking all cells within a radius
	// of the given position. For each cell, we calculate the hash and look up any objects in that cell. We then check the distance from each object to the query position to determine if it falls within the query radius,
	// and if so, we add it to the outFound vector. We also increment our query performance counters for monitoring.
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
	/////////////////////////////////
	 
	 
	
	/////////////////////////////////
	// ResetQueryStats - Static query statistics method for performance monitoring. Resets the query statistics counters (this should be called at the start of each frame to track per-frame query performance).
	static void ResetQueryStats() noexcept {
		s_totalQueriesThisFrame = 0;
		s_totalObjectsQueried = 0;
		s_queryCount = 0;
	}
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// GetQueryCount - Static query statistics method for performance monitoring. Gets the total number of queries performed in the current frame.
	static size_t GetQueryCount() noexcept { return s_queryCount; }
	/////////////////////////////////



	/////////////////////////////////
	// GetTotalObjectsQueried - Static query statistics method for performance monitoring. Gets the total number of objects queried across all queries.
	static size_t GetTotalObjectsQueried() noexcept { return s_totalObjectsQueried; }
	/////////////////////////////////



	/////////////////////////////////
	// GetAverageObjectsPerQuery - Static query statistics method for performance monitoring. Gets the average number of objects returned per query, calculated as total objects queried divided by total queries, with a check to avoid division by zero.
	static double GetAverageObjectsPerQuery() noexcept {
		return s_queryCount > 0 ? static_cast<double>(s_totalObjectsQueried) / s_queryCount : 0.0;
	}
	/////////////////////////////////
	


	/////////////////////////////////
	// GetCellCount - Gets the total number of cells currently stored in the grid (debugging/monitoring method).
	size_t GetCellCount() const noexcept { return m_grid.size(); }
	/////////////////////////////////



	/////////////////////////////////
	// GetTotalObjectCount - Gets the total number of objects stored across all grid cells. (debugging/monitoring method).
	size_t GetTotalObjectCount() const noexcept {
		size_t total = 0;
		for (const auto& [hash, objects] : m_grid) {
			total += objects.size();
		}
		return total;
	}
	/////////////////////////////////
};
/////////////////////////////////
