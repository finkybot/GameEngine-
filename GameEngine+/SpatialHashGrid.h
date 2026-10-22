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
#include <cmath>
#include <limits>
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
	struct BucketMetadata {
		size_t hash;
		size_t index;
		int cellX;
		int cellY;
	};
	std::unordered_map<T*, BucketMetadata> m_bucketMetadata;

	// Static variables for tracking query performance metrics. These variables are incremented during query operations to allow developers to analyze the efficiency of the spatial hash grid and optimize it if necessary. They can be reset at the beginning of each 
	// frame or query session to track metrics for specific time periods.
	inline static size_t s_totalQueriesThisFrame = 0;
	inline static size_t s_totalObjectsQueried = 0;
	inline static size_t s_queryCount = 0;



private:
	// GetCellHash - Convert 2D coordinates to a unique hash for the corresponding cell. It calculates the cell coordinates by dividing the position by the cell size and then uses a pairing function (Cantor pairing) to combine the cell coordinates into a single hash value. This ensures that objects in the 
	// same cell will have the same hash, allowing for efficient storage and retrieval in the grid.
	static size_t GetCellHash(float x, float y, float cellSize) noexcept {
		int cellX = static_cast<int>(x / cellSize);
		int cellY = static_cast<int>(y / cellSize);
		return GetCellHashFromCell(cellX, cellY);
	}



	// AppendToBucket - Add an object to a specific cell in the grid. It takes the object pointer, the hash of the cell, and the cell coordinates as parameters. The function retrieves the corresponding bucket (vector) for the cell from the grid, adds the object to that bucket, and updates the metadata for the object to track its current cell and index within the bucket.
	void AppendToBucket(T* object, size_t hash, int cellX, int cellY) noexcept {
		auto& bucket = m_grid[hash];
		m_bucketMetadata[object] = BucketMetadata{hash, bucket.size(), cellX, cellY};
		bucket.push_back(object);
		object->currentCellX = cellX;
		object->currentCellY = cellY;
	}



	// RemoveFromTrackedBucket - Remove an object from its current cell in the grid. It first checks if the object is valid and if it is currently tracked in the grid. If so, it retrieves the metadata for the object, finds the corresponding bucket in the grid, and removes the object from that bucket. 
	// It also updates the metadata for any moved object in the bucket to ensure that their indices are correct. Finally, it resets the object's current cell coordinates and removes its metadata from the tracking map.
	bool RemoveFromTrackedBucket(T* object) noexcept {
		if (!object)
			return false;

		auto metadataIt = m_bucketMetadata.find(object);
		if (metadataIt == m_bucketMetadata.end())
			return false;
		const BucketMetadata metadata = metadataIt->second;
		const size_t hash = metadata.hash;
		auto it = m_grid.find(hash);
		if (it == m_grid.end())
			return false;

		auto& bucket = it->second;
		const size_t index = metadata.index;
		if (index >= bucket.size() || bucket[index] != object)
			return false;

		T* moved = bucket.back();
		bucket[index] = moved;
		auto movedMetadata = m_bucketMetadata.find(moved);
		if (movedMetadata != m_bucketMetadata.end())
			movedMetadata->second.index = index;
		bucket.pop_back();
		if (bucket.empty())
			m_grid.erase(it);

		object->currentCellX = INT_MIN;
		object->currentCellY = INT_MIN;
		m_bucketMetadata.erase(metadataIt);
		return true;
	}



public:
	// Constructor and destructor for the SpatialHashGrid class. The constructor initializes the cell size, which determines how objects are hashed into cells based on their positions. The destructor clears the grid to free up memory when the object is destroyed.
	SpatialHashGrid(float cellSize = 100.0f) : m_cellSize(cellSize) {}
	~SpatialHashGrid() { Clear(); }



	// GetGrid - Returns a const reference to the internal grid data structure, allowing external code to access the grid's contents without modifying it. This can be useful for debugging or analyzing the state of the grid at a specific point in time.
	const std::unordered_map<size_t, std::vector<T*>>& GetGrid() const noexcept { return m_grid; }



	// GetCellHashFromCell - Signed-int-safe cell hash helper. Maps signed cell coordinates to unsigned space first, then applies Cantor pairing so negative cell coords do not alias unexpectedly.
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



	// GetCellSize - Returns the size of each cell in the grid. This value is used to determine how objects are hashed into cells based on their positions and can be useful for understanding the spatial partitioning of the grid.
	float GetCellSize() const noexcept { return m_cellSize; }



	// QueryRay - Queries the spatial grid for objects along a ray defined by an origin point and a direction vector, up to a specified maximum distance. The method uses a DDA-like algorithm to traverse the grid cells intersected by the ray, collecting objects from each cell while avoiding duplicates using a set. 
	// The results are stored in the outFound vector, which is cleared at the start of the query. The padding parameter allows for expanding the search area around each cell to account for object sizes or other considerations.
	void QueryRay(std::vector<T*>& outFound, const Vec2& origin, const Vec2& direction, float maxDistance, float padding) const noexcept {
		// Clear the output vector to ensure it starts empty for this query.
		outFound.clear();

		// Early exit if the maximum distance is non-positive or the direction vector is effectively zero (to avoid division by zero and unnecessary processing).
		if (maxDistance <= 0.0f || direction.Mag2() <= 1e-12f)	return;

		// Use an unordered_set to track seen objects and avoid duplicates in the results.
		std::unordered_set<T*> seen;

		// Calculate the number of cells to pad around the ray's path based on the provided padding value and the cell size. This ensures that objects near the ray's path are also considered in the query.
		const int cellPadding = static_cast<int>(padding / m_cellSize) + 1;

		// Calculate the starting cell coordinates based on the origin point and the cell size. This determines which cell the ray starts in.
		int cellX = static_cast<int>(origin.x / m_cellSize);
		int cellY = static_cast<int>(origin.y / m_cellSize);


		// Calculate the step direction for traversing the grid cells based on the direction vector. The stepX and stepY values will be either 1 or -1, indicating whether to move to the next cell in the positive or negative direction along each axis.
		const int stepX = direction.x >= 0.0f ? 1 : -1;
		const int stepY = direction.y >= 0.0f ? 1 : -1;

		// Calculate the distance to the next cell boundary along each axis. If the direction component is zero, set the distance to infinity to avoid division by zero. This allows for proper traversal of the grid cells along the ray's path.
		const float inf = std::numeric_limits<float>::infinity();

		// Calculate the distance to the next cell boundary along each axis. If the direction component is zero, set the distance to infinity to avoid division by zero. This allows for proper traversal of the grid cells along the ray's path.
		const float nextX = direction.x == 0.0f ? inf :	((direction.x > 0.0f ? (cellX + 1) : cellX) * m_cellSize - origin.x) / direction.x;
		const float nextY = direction.y == 0.0f ? inf :	((direction.y > 0.0f ? (cellY + 1) : cellY) * m_cellSize - origin.y) / direction.y;

		// Initialize the maximum distances along each axis to the calculated next cell boundary distances. These values will be used to determine when to step to the next cell during the traversal of the grid.
		float tMaxX = nextX;
		float tMaxY = nextY;

		// Calculate the distance to traverse one cell along each axis based on the direction vector. If the direction component is zero, set the distance to infinity to avoid division by zero. This allows for proper traversal of the grid cells along the ray's path.
		const float tDeltaX = direction.x == 0.0f ? inf : m_cellSize / std::abs(direction.x);
		const float tDeltaY = direction.y == 0.0f ? inf : m_cellSize / std::abs(direction.y);

		// Loop through the grid cells along the ray's path until the maximum distance is exceeded. For each cell, check for objects in the cell and its padded neighbors, adding them to the output vector if they haven't been seen before. The traversal continues until the ray has traveled beyond the specified maximum distance.
		while (true) {
			for (int x = cellX - cellPadding; x <= cellX + cellPadding; ++x) {
				for (int y = cellY - cellPadding; y <= cellY + cellPadding; ++y) {
					auto bucket = m_grid.find(GetCellHashFromCell(x, y));
					if (bucket == m_grid.end())
						continue;
					for (T* object : bucket->second)
						if (object && seen.insert(object).second)
							outFound.push_back(object);
				}
			}
			
			// Check if the minimum of the maximum distances along each axis exceeds the maximum distance. If so, break the loop as the ray has traveled beyond the specified maximum distance.
			if (std::min(tMaxX, tMaxY) > maxDistance)
				break;
			if (tMaxX < tMaxY) {
				cellX += stepX;
				tMaxX += tDeltaX;
			} else {
				cellY += stepY;
				tMaxY += tDeltaY;
			}
		}
	}



	// InsertFromSoA - Inserts an object into the spatial grid based on its position (x, y). If the object is already tracked in a different cell, it removes the stale membership first. The method calculates the cell coordinates based on the position and cell size, computes the corresponding hash, and adds the 
	// object to the appropriate cell in the grid. It also updates the object's current cell coordinates for tracking.
	void InsertFromSoA(T* object, float x, float y) noexcept {
		if (!object)
			return;

		int cellX = static_cast<int>(x / m_cellSize);
		int cellY = static_cast<int>(y / m_cellSize);

		auto metadataIt = m_bucketMetadata.find(object);
		if (metadataIt != m_bucketMetadata.end() && metadataIt->second.cellX == cellX && metadataIt->second.cellY == cellY)
			return;
		RemoveFromTrackedBucket(object);

		size_t hash = GetCellHashFromCell(cellX, cellY);
		AppendToBucket(object, hash, cellX, cellY);
	}



	// UpdateFromSoA - Updates the spatial grid with the new position of an object based on its (x, y) coordinates. If the object has moved to a different cell, it is removed from its old cell and inserted into the new cell. If the object has skipped cells (moved more than one cell away), it is also removed from 
	// any intermediate cells to ensure accurate spatial partitioning.
	void UpdateFromSoA(T* object, float x, float y) noexcept {
		int newX = static_cast<int>(x / m_cellSize);
		int newY = static_cast<int>(y / m_cellSize);

		auto metadataIt = m_bucketMetadata.find(object);

		if (metadataIt == m_bucketMetadata.end()) {
			AppendToBucket(object, GetCellHashFromCell(newX, newY), newX, newY);
			return;
		}

		// Same cell → nothing to do
		if (newX == metadataIt->second.cellX && newY == metadataIt->second.cellY)
			return;

		RemoveFromTrackedBucket(object);

		// Insert into new cell
		size_t newHash = GetCellHashFromCell(newX, newY);
		AppendToBucket(object, newHash, newX, newY);
	}



	// Remove - Removes an object from the spatial grid based on pointer identity.
	void Remove(T* object) noexcept {
		if (!object)
			return;
		RemoveFromTrackedBucket(object);
		object->currentCellX = INT_MIN;
		object->currentCellY = INT_MIN;
	}



	// Update - Updates the spatial grid with the new position of an object. If the object has moved to a different cell, it is removed from its old cell and inserted into the new cell. If the object has skipped cells (moved more than one cell away), it is also removed from 
	// any intermediate cells to ensure accurate spatial partitioning.
	void Update(T* object) noexcept {
		const Vec2& pos = object->GetCentrePoint();
		UpdateFromSoA(object, pos.x, pos.y);
	}



	// Clear - Clears all objects from the spatial grid.
	void Clear() noexcept {
		m_grid.clear();
		m_bucketMetadata.clear();
	}



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
			vec.erase(std::remove_if(vec.begin(), vec.end(), [&activeSet, this](T* ptr) {
				const bool remove = ptr == nullptr || activeSet.find(ptr) == activeSet.end();
				if (remove && ptr)
					m_bucketMetadata.erase(ptr);
				return remove;
			}), vec.end());

			if (vec.empty()) {
				it = m_grid.erase(it);
			} else {
				for (size_t index = 0; index < vec.size(); ++index) {
					auto metadata = m_bucketMetadata.find(vec[index]);
					if (metadata != m_bucketMetadata.end())
						metadata->second.index = index;
				}
				++it;
			}
		}
	}



	// Insert - Inserts an object into the spatial grid based on its center point position.
	void Insert(T* object) noexcept {
		if (!object)
			return;

		const Vec2& pos = object->GetCentrePoint();
		InsertFromSoA(object, pos.x, pos.y);
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
