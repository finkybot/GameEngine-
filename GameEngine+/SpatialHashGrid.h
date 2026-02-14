#pragma once
#include "Vec2.h"
#include <vector>
#include <unordered_map>
#include <algorithm>

/// <summary>
/// Spatial Hash Grid for efficient collision detection.
/// Divides space into uniform cells and uses hash table for O(1) cell lookup.
/// </summary>
template<typename T>
class SpatialHashGrid
{
private:
	float m_cellSize;
	std::unordered_map<size_t, std::vector<T*>> m_grid;
	
	// Query instrumentation
	inline static size_t s_totalQueriesThisFrame = 0;
	inline static size_t s_totalObjectsQueried = 0;
	inline static size_t s_queryCount = 0;

	// Convert world position to cell coordinates
	static size_t GetCellHash(float x, float y, float cellSize) noexcept
	{
		int cellX = static_cast<int>(x / cellSize);
		int cellY = static_cast<int>(y / cellSize);
		
		// Cantor pairing function for 2D -> 1D hash
		size_t hash = ((cellX + cellY) * (cellX + cellY + 1)) / 2 + cellY;
		return hash;
	}

public:
	SpatialHashGrid(float cellSize = 100.0f) : m_cellSize(cellSize) {}

	~SpatialHashGrid() { Clear(); }

	void Clear() noexcept
	{
		m_grid.clear();
	}

	void Insert(T* object) noexcept
	{
		const Vec2& pos = object->GetCentrePoint();
		size_t hash = GetCellHash(pos.GetX(), pos.GetY(), m_cellSize);
		m_grid[hash].push_back(object);
	}

	void Query(std::vector<T*>& found, const Vec2& position, float queryRadius) noexcept
	{
		++s_queryCount;
		
		// Query all cells within radius
		int cellX = static_cast<int>(position.GetX() / m_cellSize);
		int cellY = static_cast<int>(position.GetY() / m_cellSize);
		int cellRadius = static_cast<int>(queryRadius / m_cellSize) + 1;

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
							found.push_back(obj);
							++s_totalObjectsQueried;
						}
					}
				}
			}
		}

		++s_totalQueriesThisFrame;
	}

	void Query(std::vector<T*>& found, const Vec2& position, float queryRadius, const T* excludeObject) noexcept
	{
		++s_queryCount;
		
		int cellX = static_cast<int>(position.GetX() / m_cellSize);
		int cellY = static_cast<int>(position.GetY() / m_cellSize);
		int cellRadius = static_cast<int>(queryRadius / m_cellSize) + 1;

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
						if (obj == excludeObject) continue;

						const Vec2& objPos = obj->GetCentrePoint();
						float dx = objPos.GetX() - position.GetX();
						float dy = objPos.GetY() - position.GetY();
						float distSq = dx * dx + dy * dy;
						float radiusSq = queryRadius * queryRadius;

						if (distSq <= radiusSq)
						{
							found.push_back(obj);
							++s_totalObjectsQueried;
						}
					}
				}
			}
		}

		++s_totalQueriesThisFrame;
	}

	// Statistics
	static void ResetQueryStats() noexcept
	{
		s_totalQueriesThisFrame = 0;
		s_totalObjectsQueried = 0;
		s_queryCount = 0;
	}

	static size_t GetQueryCount() noexcept { return s_queryCount; }
	static size_t GetTotalObjectsQueried() noexcept { return s_totalObjectsQueried; }
	static double GetAverageObjectsPerQuery() noexcept
	{
		return s_queryCount > 0 ? static_cast<double>(s_totalObjectsQueried) / s_queryCount : 0.0;
	}

	size_t GetCellCount() const noexcept { return m_grid.size(); }
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
