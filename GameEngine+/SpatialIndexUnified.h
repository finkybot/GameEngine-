#pragma once
#include "ISpatialIndex.h"
#include "SpatialHashGrid.h"
#include "BVHSystem.h"
#include "ChunkManager.h"

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

	void RebuildDynamic(const std::vector<std::unique_ptr<Entity>>& entities);
	void RebuildBVH(const std::vector<std::unique_ptr<Entity>>& entities);
	void RebuildWorldMask(ChunkManager* chunks);
};
