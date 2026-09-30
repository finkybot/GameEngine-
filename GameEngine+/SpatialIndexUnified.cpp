//////////////////////////////////
#include "SpatialIndexUnified.h"
#include "Raycast.h"
#include "Entity.h"
#include "CStatic.h"
#include <unordered_set>
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::Rebuild(const std::vector<std::unique_ptr<Entity>>& entities, ChunkManager* chunks) {
	m_chunks = chunks;

	std::unordered_set<Entity*> activeDynamic;
	activeDynamic.reserve(entities.size());

	for (const auto& up : entities) {
		Entity* e = up.get();
		if (!e || !e->IsAlive())
			continue;
		if (!e->GetShape())
			continue;
		if (e->HasComponent<CStatic>())
			continue;
		activeDynamic.insert(e);
	}

	if (!m_dynamicInitialized) {
		m_dynamicGrid.Clear();
		for (Entity* e : activeDynamic) {
			m_dynamicGrid.Insert(e);
		}
		m_dynamicInitialized = true;
	} else {
		// Remove stale pointers without dereferencing potential freed entities.
		m_dynamicGrid.PruneToActiveSet(activeDynamic);

		// Ensure each active dynamic entity is present in the grid.
		for (Entity* e : activeDynamic) {
			if (!m_dynamicGrid.ContainsPointer(e)) {
				m_dynamicGrid.Insert(e);
			}
		}
	}

	// Keep dynamic cell membership current even when callers do not issue explicit Update() calls.
	for (Entity* e : activeDynamic) {
		m_dynamicGrid.Update(e);
	}

	if (chunks) {
		const uint64_t revision = chunks->GetWorldRevision();
		if (m_worldMaskDirty || revision != m_lastWorldRevision) {
			RebuildWorldMask(chunks);
			m_worldMaskDirty = false;
		}
	}
}
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::Insert(Entity* e) {
	if (!e || !e->IsAlive())
		return;

	// Only dynamic, non-static entities go into the dynamic grid
	if (e->GetShape() && !e->HasComponent<CStatic>())
		m_dynamicGrid.Insert(e);

	// BVH only stores dynamic non-tile entities
	if (e->GetShape() && e->GetType() != EntityType::Tile && e->GetType() != EntityType::TileMap &&
		e->GetType() != EntityType::Chunk) {
		m_bvh.Insert(e);
	}
}
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::Remove(Entity* e) {
	if (!e)
		return;

	// Remove from dynamic grid
	m_dynamicGrid.Remove(e);

	// Remove from BVH
	m_bvh.Remove(e);
}

//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::Update(Entity* e) {
	if (!e || !e->IsAlive())
		return;

	// Update dynamic grid position
	if (e->GetShape() && !e->HasComponent<CStatic>())
		m_dynamicGrid.Update(e);

	// Update BVH node
	m_bvh.Update(e);
}
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::Reset() {
	m_dynamicGrid.Clear();
	m_bvh = BVHSystem{};
	m_dynamicInitialized = false;
	m_worldMask.clear();
	m_worldWidth = 0;
	m_worldHeight = 0;
	m_worldOffsetX = 0;
	m_worldOffsetY = 0;
	m_worldMaskDirty = false;
	m_lastWorldRevision = 0;
}
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::InitialBuildDynamic(const std::vector<std::unique_ptr<Entity>>& entities) {
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
	m_dynamicInitialized = true;
}
//////////////////////////////////



//////////////////////////////////
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
	m_dynamicInitialized = true;
}
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::RebuildBVH(const std::vector<std::unique_ptr<Entity>>& entities) {
	// Build BVH once at scene load or when world is reset
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

	// Build initial BVH tree
	m_bvh.Rebuild(dynamic);
}
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::RebuildWorldMask(ChunkManager* chunks) {
	if (!chunks)
		return;

	m_tileSize = chunks->GetTileSize();
	chunks->GetWorldMaskSnapshot(m_worldMask, m_worldWidth, m_worldHeight, m_worldOffsetX, m_worldOffsetY, m_lastWorldRevision);
}
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::QueryEntities(std::vector<Entity*>& outFound, const Vec2& position, float radius,
										const Entity* exclude) const {
	m_dynamicGrid.Query(outFound, position, radius, static_cast<Entity*>(const_cast<Entity*>(exclude)));
}
//////////////////////////////////



//////////////////////////////////
bool SpatialIndexUnified::RaycastEntities(const Vec2& origin, const Vec2& dirN, float maxDist, RaycastHit& outHit,
										  Entity*& outEntity) const {
	BVHDebugTraversal dbg;
	return m_bvh.Raycast(origin, dirN, maxDist, outHit, outEntity, &dbg);
}
//////////////////////////////////



//////////////////////////////////
RaycastHit SpatialIndexUnified::RaycastWorld(const Vec2& origin, const Vec2& dir, float maxDist) const {
	return RaycastWorldMaskDDA(origin, dir, m_worldMask, m_worldWidth, m_worldHeight, m_worldOffsetX, m_worldOffsetY,
							   m_tileSize, maxDist, false, nullptr);
}
//////////////////////////////////



//////////////////////////////////
bool SpatialIndexUnified::IsWorldSolid(int tx, int ty) const {
	int lx = tx - m_worldOffsetX;
	int ly = ty - m_worldOffsetY;

	if (lx < 0 || ly < 0 || lx >= m_worldWidth || ly >= m_worldHeight)
		return false;

	size_t idx = ly * m_worldWidth + lx;
	return idx < m_worldMask.size() && m_worldMask[idx] != 0;
}
//////////////////////////////////