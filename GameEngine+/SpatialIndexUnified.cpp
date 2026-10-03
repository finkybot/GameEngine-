//////////////////////////////////
#include "SpatialIndexUnified.h"
#include "Raycast.h"
#include "Entity.h"
#include "EntityManager.h"
#include "CStatic.h"
#include <unordered_set>
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::Insert(Entity* e) {
	if (!m_entityManager || !e || !e->IsAlive())
		return;

	size_t idx = e->transformIndex;
	if (idx == SIZE_MAX)
		return;

	auto& T = m_entityManager->GetTransformSoA();
	float x = T.posX[idx];
	float y = T.posY[idx];

	m_dynamicGrid.InsertFromSoA(e, x, y);
	m_bvh.Insert(e);
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
void SpatialIndexUnified::Reset() {
	m_dynamicEntities.clear(); // <-- important: drop all entity pointers

	m_dynamicGrid.Clear();
	m_bvh = BVHSystem{};
	m_worldMask.clear();
	m_worldWidth = 0;
	m_worldHeight = 0;
	m_worldOffsetX = 0;
	m_worldOffsetY = 0;
	m_worldMaskDirty = false;
	m_lastWorldRevision = 0;
	m_topologyDirty = false;
}
//////////////////////////////////



//////////////////////////////////
// Incremental build of the dynamic spatial index from a list of dynamic entities. This method clears the existing dynamic grid and inserts all alive dynamic entities into it based on their current positions. It uses the TransformSoA structure to efficiently access the position data of each entity.
// This method is intended to be called when the dynamic entities in the scene have changed significantly (topology), such as when loading a new level or resetting the scene. NOT EVERY FRAME. For per-frame updates, use Refit() or UpdateEntity() instead.
void SpatialIndexUnified::Build(const std::vector<Entity*>& dynamicEntities) {
	// Store the dynamic entities list internally
	m_dynamicEntities = dynamicEntities;

	// Clear the grid entirely
	m_dynamicGrid.Clear();

	// Get a reference to the TransformSoA for efficient access to entity positions
	auto& T = m_entityManager->GetTransformSoA();

	// Insert all dynamic entities into the grid
	for (Entity* e : m_dynamicEntities) {
		// Skip dead entities
		if (!e || !e->IsAlive())
			continue;

		// Get the index of the entity's transform in the SoA structure
		size_t idx = e->transformIndex;
		if (idx == SIZE_MAX)
			continue; // no valid SoA slot, skip

		// Bounds check: ensure idx is within the TransformSoA arrays
		if (idx >= T.posX.size() || idx >= T.posY.size())
			continue;

		// Collect the position of the entity from the SoA structure
		float x = T.posX[idx];
		float y = T.posY[idx];

		// Insert the entity into the dynamic grid based on its position
		m_dynamicGrid.InsertFromSoA(e, x, y);
	}

	m_bvh.Rebuild(dynamicEntities); // Rebuild the BVH tree based on the current dynamic entities
}
//////////////////////////////////



//////////////////////////////////
// Incremental refit of the dynamic spatial index for entities that have moved. This method iterates over all dynamic entities and checks if their transform is marked as dirty (indicating that they have moved). If an entity is dirty, it updates its position in the dynamic grid and BVH based on its current 
// position from the TransformSoA structure. After updating, it clears the dirty flag for that entity's transform.
void SpatialIndexUnified::Refit() {
	// Skip if the entity manager is not set
	if (!m_entityManager) return;

	// Skip if there are no dynamic entities to process
	if (m_dynamicEntities.empty()) return;

	// Get a reference to the TransformSoA for efficient access to entity positions
	auto& T = m_entityManager->GetTransformSoA();

	// Iterate over all dynamic entities and update their positions in the dynamic grid and BVH if they are marked as dirty
	for (Entity* e : m_dynamicEntities) {
		if (!e || !e->IsAlive())
			continue;

		size_t idx = e->transformIndex;
		if (idx == SIZE_MAX)
			continue;

		// Bounds check: ensure idx is within the TransformSoA arrays
		if (idx >= T.posX.size() || idx >= T.dirty.size())
			continue;

		if (!T.IsDirty(idx))
			continue; // NEW: skip clean transforms

		float x = T.posX[idx];
		float y = T.posY[idx];

		m_dynamicGrid.UpdateFromSoA(e, x, y);
		m_bvh.Update(e);

		T.ClearDirty(idx); // NEW: clear dirty flag
	}
}
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::UpdateEntity(Entity* e) {
	if (!e || !e->IsAlive())
		return;

	// Static or shapeless entities only live in BVH (or nowhere)
	if (!e->GetShape() || e->HasComponent<CStatic>()) {
		m_bvh.Update(e);
		return;
	}

	if (!m_entityManager)
		return;

	size_t idx = e->transformIndex;
	if (idx == SIZE_MAX)
		return;

	auto& T = m_entityManager->GetTransformSoA();
	float x = T.posX[idx];
	float y = T.posY[idx];

	// Dynamic grid update from SoA
	m_dynamicGrid.UpdateFromSoA(e, x, y);

	// BVH refit for this single entity
	m_bvh.Update(e);
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