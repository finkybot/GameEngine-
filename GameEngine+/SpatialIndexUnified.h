////////////////////////////////
// SpatialIndexUnified.h
////////////////////////////////


////////////////////////////////
// Includes
#pragma once
#include "ISpatialIndex.h"
#include "SpatialHashGrid.h"
#include "BVHSystem.h"
#include "ChunkManager.h"
////////////////////////////////



////////////////////////////////
//	|	SpatialIndexUnified class - Implements a unified spatial index that combines a dynamic spatial hash grid for entities and a BVH system for efficient raycasting. It also maintains a world mask for collision detection with the game world. This class provides methods for rebuilding the spatial index,
//	|	querying entities, performing raycasts, and checking world solidity.
//	|_______________________________________________________________________
class SpatialIndexUnified : public ISpatialIndex {
public:
	SpatialIndexUnified(float dynamicCellSize = 100.0f) : m_dynamicGrid(dynamicCellSize) {}

	void Rebuild(const std::vector<std::unique_ptr<Entity>>& entities, ChunkManager* chunks) override;

	void QueryEntities(std::vector<Entity*>& outFound, const Vec2& position, float radius,
					   const Entity* exclude) const override;

	bool RaycastEntities(const Vec2& origin, const Vec2& dirN, float maxDist, RaycastHit& outHit,
						 Entity*& outEntity) const override;

	RaycastHit RaycastWorld(const Vec2& origin, const Vec2& dir, float maxDist) const override;

	bool IsWorldSolid(int tileX, int tileY) const override;

	
	void Insert(Entity* e) override;
	void Remove(Entity* e) override;
	void Update(Entity* e) override;
	void Reset() override;

		// Call this when chunks/tilemaps change
	void MarkWorldMaskDirty() { m_worldMaskDirty = true; }


	void InitialBuildDynamic(const std::vector<std::unique_ptr<Entity>>& entities);
	void RebuildDynamic(const std::vector<std::unique_ptr<Entity>>& entities);
	void RebuildBVH(const std::vector<std::unique_ptr<Entity>>& entities);

private:
	SpatialHashGrid<Entity> m_dynamicGrid;
	BVHSystem m_bvh;

	ChunkManager* m_chunks = nullptr;

	std::vector<uint8_t> m_worldMask;
	int m_worldWidth = 0;
	int m_worldHeight = 0;
	int m_worldOffsetX = 0;
	int m_worldOffsetY = 0;
	float m_tileSize = 32.0f;

	bool m_worldMaskDirty = false;
	bool m_dynamicInitialized = false;
	uint64_t m_lastWorldRevision = 0;

	void RebuildWorldMask(ChunkManager* chunks);
};
////////////////////////////////