#include "SpatialIndexUnified.h"
#include "Raycast.h"
#include "Entity.h"
#include "CStatic.h"

void SpatialIndexUnified::Rebuild(const std::vector<std::unique_ptr<Entity>>& entities, ChunkManager* chunks) {
	m_chunks = chunks;
	RebuildDynamic(entities);
	RebuildBVH(entities);
	RebuildWorldMask(chunks);
}

void SpatialIndexUnified::RebuildDynamic(const std::vector<std::unique_ptr<Entity>>& entities) {
	m_dynamicGrid.Clear();
	for (auto& u : entities) {
		Entity* e = u.get();
		if (!e->IsAlive())
			continue;
		if (!e->GetShape())
			continue;
		if (e->HasComponent<CStatic>())
			continue;
		m_dynamicGrid.Insert(e);
	}
}

void SpatialIndexUnified::RebuildBVH(const std::vector<std::unique_ptr<Entity>>& entities) {
	std::vector<Entity*> dynamic;
	dynamic.reserve(entities.size());

	for (auto& u : entities) {
		Entity* e = u.get();
		if (!e->IsAlive())
			continue;
		if (!e->GetShape())
			continue;

		if (e->GetType() == EntityType::Tile || e->GetType() == EntityType::TileMap ||
			e->GetType() == EntityType::Chunk)
			continue;

		dynamic.push_back(e);
	}

	m_bvh.Rebuild(dynamic);
}

void SpatialIndexUnified::RebuildWorldMask(ChunkManager* chunks) {
	if (!chunks)
		return;

	m_tileSize = chunks->GetTileSize();
	m_worldOffsetX = chunks->worldOffsetX;
	m_worldOffsetY = chunks->worldOffsetY;

	chunks->BuildWorldMask(m_worldMask, m_worldWidth, m_worldHeight);
}

void SpatialIndexUnified::QueryEntities(std::vector<Entity*>& outFound, const Vec2& position, float radius,
										const Entity* exclude) const {
	m_dynamicGrid.Query(outFound, position, radius, static_cast<Entity*>(const_cast<Entity*>(exclude)));
}


bool SpatialIndexUnified::RaycastEntities(const Vec2& origin, const Vec2& dirN, float maxDist, RaycastHit& outHit,
										  Entity*& outEntity) const {
	BVHDebugTraversal dbg;
	return m_bvh.Raycast(origin, dirN, maxDist, outHit, outEntity, &dbg);
}

RaycastHit SpatialIndexUnified::RaycastWorld(const Vec2& origin, const Vec2& dir, float maxDist) const {
	return RaycastWorldMaskDDA(origin, dir, m_worldMask, m_worldWidth, m_worldHeight, m_worldOffsetX, m_worldOffsetY,
							   m_tileSize, maxDist, false, nullptr);
}

bool SpatialIndexUnified::IsWorldSolid(int tx, int ty) const {
	int lx = tx - m_worldOffsetX;
	int ly = ty - m_worldOffsetY;

	if (lx < 0 || ly < 0 || lx >= m_worldWidth || ly >= m_worldHeight)
		return false;

	size_t idx = ly * m_worldWidth + lx;
	return idx < m_worldMask.size() && m_worldMask[idx] != 0;
}
