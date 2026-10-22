//////////////////////////////////
#include "SpatialIndexUnified.h"
#include "Raycast.h"
#include "Entity.h"
#include "EntityManager.h"
#include "CStatic.h"
#include "CShape.h"
#include "CTransform.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::Insert(Entity* e) {
	if (!m_entityManager || !e || !e->IsAlive())
		return;
	if (e->HasComponent<CStatic>() || !e->GetShape())
		return;
	const auto bounds = e->GetShape()->GetShape().getGlobalBounds();
	m_dynamicQueryPadding = std::max(m_dynamicQueryPadding, std::max(bounds.size.x, bounds.size.y));
	if (std::find(m_dynamicEntities.begin(), m_dynamicEntities.end(), e) == m_dynamicEntities.end()) {
		m_dynamicEntities.push_back(e);
		m_topologyDirty = true;
	}

	size_t idx = e->transformIndex;
	if (idx == SIZE_MAX)
		return;

	auto& T = m_entityManager->GetTransformSoA();
	float x = T.posX[idx];
	float y = T.posY[idx];

	m_dynamicGrid.InsertFromSoA(e, x, y);
}
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::Remove(Entity* e) {
	if (!e)
		return;

	const auto removedCount = std::erase(m_dynamicEntities, e);
	if (removedCount > 0)
		m_topologyDirty = true;

	// Remove from dynamic grid
	m_dynamicGrid.Remove(e);

}
//////////////////////////////////



//////////////////////////////////
void SpatialIndexUnified::Reset() {
	m_dynamicEntities.clear(); // <-- important: drop all entity pointers

	m_dynamicGrid.Clear();
	m_worldMask.clear();
	m_worldWidth = 0;
	m_worldHeight = 0;
	m_worldOffsetX = 0;
	m_worldOffsetY = 0;
	m_dynamicQueryPadding = 0.0f;
	m_worldMaskDirty = false;
	m_lastWorldRevision = 0;
	m_topologyDirty = false;
}
//////////////////////////////////



//////////////////////////////////
// Incremental build of the dynamic spatial index from a list of dynamic entities. This method clears the existing dynamic grid and inserts all alive dynamic entities into it based on their current positions. It uses the TransformSoA structure to efficiently access the position data of each entity.
// This method is intended to be called when the dynamic entities in the scene have changed significantly (topology), such as when loading a new level or resetting the scene. NOT EVERY FRAME. For per-frame updates, use Refit() or UpdateEntity() instead.
void SpatialIndexUnified::Build(const std::vector<Entity*>& dynamicEntities) {
	// Only shape-bearing dynamic entities can participate in grid or raycast queries.
	m_dynamicEntities.clear();
	m_dynamicEntities.reserve(dynamicEntities.size());
	m_dynamicQueryPadding = 0.0f;
	for (Entity* e : dynamicEntities) {
		if (e && e->IsAlive() && !e->HasComponent<CStatic>() && e->GetShape()) {
			m_dynamicEntities.push_back(e);
			const auto bounds = e->GetShape()->GetShape().getGlobalBounds();
			m_dynamicQueryPadding = std::max(m_dynamicQueryPadding, std::max(bounds.size.x, bounds.size.y));
		}
	}

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

	m_topologyDirty = false;
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
		T.ClearDirty(idx); // NEW: clear dirty flag
	}

}
//////////////////////////////////



//////////////////////////////////
// UpdateEntity - Update the position of a single entity in the dynamic spatial index. This method checks if the entity is alive and has a valid transform index, then retrieves its current position from the TransformSoA structure and updates its position in the dynamic grid. This is useful for
// updating individual entities that have moved without needing to refit the entire spatial index.
void SpatialIndexUnified::UpdateEntity(Entity* e) {
	if (!e || !e->IsAlive())
		return;

	// Static or shapeless entities only live in BVH (or nowhere)
	if (!e->GetShape() || e->HasComponent<CStatic>()) {
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

}
//////////////////////////////////



//////////////////////////////////
// QueryEntities - Perform a spatial query to find dynamic entities within a specified radius of a given position. This method queries the dynamic grid for potential candidates and populates the outFound vector with pointers to the found entities, excluding the specified entity if provided.
void SpatialIndexUnified::QueryEntities(std::vector<Entity*>& outFound, const Vec2& position, float radius,
										const Entity* exclude) const {
	m_dynamicGrid.Query(outFound, position, radius, static_cast<Entity*>(const_cast<Entity*>(exclude)));
}
//////////////////////////////////



//////////////////////////////////
// RaycastEntities - Perform a raycast against dynamic entities in the spatial index. This method queries the dynamic grid for potential candidates along the ray's path and checks for intersections with their bounding boxes. It returns the nearest hit entity and hit information if any intersection occurs.
bool SpatialIndexUnified::RaycastEntities(const Vec2& origin, const Vec2& dirN, float maxDist, RaycastHit& outHit,
										  Entity*& outEntity) const {
	std::vector<Entity*> candidates;
	m_dynamicGrid.QueryRay(candidates, origin, dirN, maxDist, m_dynamicQueryPadding);

	float nearest = maxDist;
	bool hitAny = false;
	outHit = RaycastHit{};
	outEntity = nullptr;

	for (Entity* entity : candidates) {
		if (!entity || !entity->IsAlive())
			continue;

		auto* transform = entity->GetComponent<CTransform>();
		auto* shape = entity->GetComponent<CShape>();
		
		if (!transform || !shape) continue;

		const sf::Vector2f originOffset = shape->GetShape().getOrigin();
		const float minX = transform->position.x - originOffset.x;
		const float minY = transform->position.y - originOffset.y;
		const float maxX = minX + shape->GetWidth();
		const float maxY = minY + shape->GetHeight();
		const float tx1 = (minX - origin.x) / dirN.x;
		const float tx2 = (maxX - origin.x) / dirN.x;
		const float ty1 = (minY - origin.y) / dirN.y;
		const float ty2 = (maxY - origin.y) / dirN.y;
		const float entry = std::max(std::min(tx1, tx2), std::min(ty1, ty2));
		const float exit = std::min(std::max(tx1, tx2), std::max(ty1, ty2));
		
		if (exit < 0.0f || entry > exit || entry > nearest)	continue;

		const float distance = std::max(0.0f, entry);
		nearest = distance;
		
		outEntity = entity;
		outHit.hit = true;
		outHit.distance = distance;
		outHit.position = origin + dirN * distance;
		
		const Vec2 center((minX + maxX) * 0.5f, (minY + maxY) * 0.5f);
		const Vec2 delta = outHit.position - center;
		const float halfW = (maxX - minX) * 0.5f;
		const float halfH = (maxY - minY) * 0.5f;
		const float nx = halfW > 0.0f ? delta.x / halfW : 0.0f;
		const float ny = halfH > 0.0f ? delta.y / halfH : 0.0f;
		
		outHit.normal = std::fabs(nx) > std::fabs(ny)
			? Vec2(nx > 0.0f ? 1.0f : -1.0f, 0.0f)
			: Vec2(0.0f, ny > 0.0f ? 1.0f : -1.0f);
		hitAny = true;
	}

	return hitAny;
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