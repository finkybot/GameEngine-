// ***** SpatialHashGrid.h - Spatial Hash Grid for efficient collision detection *****
#pragma once
#include "Vec2.h"
#include <vector>
#include <unordered_map>
#include <algorithm>

// SpatialHashGrid is a spatial partitioning data structure that divides the 2D world into a grid of cells and hashes objects into those cells based on their positions. 
// It allows for efficient querying of nearby objects within a specified radius, which is useful for collision detection and other spatial queries in a game. 
// The grid is implemented using an unordered_map where the key is a hash representing the cell coordinates and the value is a vector of pointers to objects that occupy that cell. 
// The class provides methods for inserting objects into the grid, clearing the grid, and querying for nearby objects while optionally excluding a specific object from the results (e.g. to avoid self-collision checks). 
// It also includes instrumentation for tracking query performance metrics such as total queries and total objects queried.
template<typename T>
class SpatialHashGrid
{

	// ***** Private Member Variables *****
private:
	float m_cellSize;
	std::unordered_map<size_t, std::vector<T*>> m_grid;
	
	// Query instrumentation
	inline static size_t s_totalQueriesThisFrame = 0;
	inline static size_t s_totalObjectsQueried = 0;
	inline static size_t s_queryCount = 0;

	// ***** Private Methods *****

	// Hash function to convert 2D coordinates to a unique hash for the corresponding cell. It calculates the cell coordinates by dividing the position by the cell size and then uses a pairing function (Cantor pairing) to combine the cell coordinates into a single hash value. This ensures that objects in the same cell will have the same hash, allowing for efficient storage and retrieval in the grid.
	static size_t GetCellHash(float x, float y, float cellSize) noexcept
	{
		int cellX = static_cast<int>(x / cellSize);
		int cellY = static_cast<int>(y / cellSize);
		
		// Cantor pairing function for 2D -> 1D reversable hash
		size_t hash = ((cellX + cellY) * (cellX + cellY + 1)) / 2 + cellY;
		return hash;
	}

	// ***** Public Methods *****
public:
	// Constructors ~ Destructors.
	SpatialHashGrid(float cellSize = 100.0f) : m_cellSize(cellSize) {}
	~SpatialHashGrid() { Clear(); }

	// Methods.

	// Clears all objects from the spatial grid.
	void Clear() noexcept
	{
		m_grid.clear();
	}
	
	// Insert an object into the spatial grid based on its center point position.
	void Insert(T* object) noexcept
	{
		const Vec2& pos = object->GetCentrePoint();
		size_t hash = GetCellHash(pos.GetX(), pos.GetY(), m_cellSize);
		m_grid[hash].push_back(object);
	}

	// Performs a spatial query to find all objects within a specified radius of a position using a grid-based spatial hash. I'll store the results in the provided outFound vector, this version of the method
	// will include all objects within the query radius, including the object performing the query if it is within the radius. Our query works by checking all cells within a radius of the given position.
	// For each cell, we calculate the hash and look up any objects in that cell. We then check the distance from each object to the query position to determine if it falls within the query radius, 
	// and if so, we add it to the outFound vector. We also increment our query performance counters for monitoring.
	void Query(std::vector<T*>& outFound, const Vec2& position, float queryRadius) noexcept
	{
		++s_queryCount; // performance monitoring: Increment the query count each time this method is called.
		
		// Query all cells within radius
		int cellX = static_cast<int>(position.GetX() / m_cellSize);
		int cellY = static_cast<int>(position.GetY() / m_cellSize);
		int cellRadius = static_cast<int>(queryRadius / m_cellSize) + 1; // Get the radius in terms of cells but add 1 to ensure we cover the entire query radius even if it extends slightly beyond the last cell boundary.

		// Loop through all cells within the calculated cell radius and check for objects in those cells.
		for (int x = cellX - cellRadius; x <= cellX + cellRadius; ++x)
		{
			for (int y = cellY - cellRadius; y <= cellY + cellRadius; ++y)
			{
				size_t hash = ((x + y) * (x + y + 1)) / 2 + y;
				
				auto it = m_grid.find(hash);
				if (it != m_grid.end())
				{
					for (T* obj : it->second)
					{
						const Vec2& objPos = obj->GetCentrePoint();
						float dx = objPos.GetX() - position.GetX();
						float dy = objPos.GetY() - position.GetY();
						float distSq = dx * dx + dy * dy;
						float radiusSq = queryRadius * queryRadius;

						if (distSq <= radiusSq)
						{
							outFound.push_back(obj);
							++s_totalObjectsQueried;
						}
					}
				}
			}
		}

		++s_totalQueriesThisFrame;
	}

	// Overloaded Query; Performs a spatial query to find all objects within a specified radius of a position using a grid-based spatial hash. I'll store the results in the provided outFound vector, this version of the method
	// will include any objects found within the query radius, excluding the object passed to the query (e.g. the object performing the query so theres no self collision). The query works by checking all cells within a radius 
	// of the given position. For each cell, we calculate the hash and look up any objects in that cell. We then check the distance from each object to the query position to determine if it falls within the query radius, 
	// and if so, we add it to the outFound vector. We also increment our query performance counters for monitoring.
	void Query(std::vector<T*>& outFound, const Vec2& position, float queryRadius, const T* excludeObject) noexcept
	{
		++s_queryCount; // Increment query count for performance monitoring.
		
		// Calculate the cell coordinates and radius in terms of cells to determine which cells to query.
		int cellX = static_cast<int>(position.GetX() / m_cellSize);
		int cellY = static_cast<int>(position.GetY() / m_cellSize);
		int cellRadius = static_cast<int>(queryRadius / m_cellSize) + 1; // Get the radius in terms of cells but add 1 to ensure we cover the entire query radius even if it extends slightly beyond the last cell boundary.

		// Loop through all cells within the calculated cell radius and check for objects in those cells.
		for (int x = cellX - cellRadius; x <= cellX + cellRadius; ++x)
		{
			for (int y = cellY - cellRadius; y <= cellY + cellRadius; ++y)
			{
				size_t hash = ((x + y) * (x + y + 1)) / 2 + y; // Calculate the hash for the current cell coordinates using the same method as GetCellHash to ensure consistency.
				
				auto it = m_grid.find(hash);
				if (it != m_grid.end())
				{
					for (T* obj : it->second)
					{
						if (obj == excludeObject) continue;

						const Vec2& objPos = obj->GetCentrePoint();
						float dx = objPos.GetX() - position.GetX();
						float dy = objPos.GetY() - position.GetY();
						float distSq = dx * dx + dy * dy;
						float radiusSq = queryRadius * queryRadius;

						if (distSq <= radiusSq)
						{
							outFound.push_back(obj);
							++s_totalObjectsQueried;
						}
					}
				}
			}
		}

		++s_totalQueriesThisFrame; // Increment total queries for performance monitoring.
	}

	// Static query statistics method for performance monitoring. Resets the query statistics counters (this should be called at the start of each frame to track per-frame query performance).
	static void ResetQueryStats() noexcept
	{
		s_totalQueriesThisFrame = 0;
		s_totalObjectsQueried = 0;
		s_queryCount = 0;
	}

	// Static query statistics method for performance monitoring. Gets the total number of queries performed in the current frame.
	static size_t GetQueryCount() noexcept { return s_queryCount; }

	// Static query statistics method for performance monitoring. Gets the total number of objects queried across all queries.
	static size_t GetTotalObjectsQueried() noexcept { return s_totalObjectsQueried; }
	
	// Static query statistics method for performance monitoring. Gets the average number of objects returned per query, calculated as total objects queried divided by total queries, with a check to avoid division by zero.
	static double GetAverageObjectsPerQuery() noexcept
	{
		return s_queryCount > 0 ? static_cast<double>(s_totalObjectsQueried) / s_queryCount : 0.0;
	}

	// Gets the total number of cells currently stored in the grid (debugging/monitoring method).
	size_t GetCellCount() const noexcept { return m_grid.size(); }

	// Gets the total number of objects stored across all grid cells. (debugging/monitoring method).
	size_t GetTotalObjectCount() const noexcept
	{
		size_t total = 0;
		for (const auto& [hash, objects] : m_grid)
		{
			total += objects.size();
		}
		return total;
	}
};
