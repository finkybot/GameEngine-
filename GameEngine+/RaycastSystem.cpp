////////////////////////////////
// RaycastSystem.cpp
////////////////////////////////



////////////////////////////////
// Includes
#include "RaycastSystem.h"
#include "Entity.h"
////////////////////////////////



////////////////////////////////
// RaycastSystem implementation - processes CRaycast components and fills their hit data using RaycastEngine. It iterates over all entities with a CRaycast component, performs raycasting based on the component's input data, and updates the component's output data with the results of the raycast.
void RaycastSystem::Update(float /*dt*/) {
	// Early exit if no entity manager is available
	if (!m_entityManager)	return;

	// GetEntities() returns std::vector<std::unique_ptr<Entity>>
	auto& entities = m_entityManager->GetEntities();

	// Iterate over all entities and process those with a CRaycast component
	for (auto& ptr : entities) {
		// Extract raw pointer
		Entity* e = ptr.get();
		
		// Skip dead entities
		if (!e || !e->IsAlive()) continue;

		// Get the CRaycast component from the entity
		CRaycast* rc = e->GetComponent<CRaycast>();
		if (!rc) continue;

		// Build ray parameters
		Vec2 origin = rc->worldPos;
		Vec2 direction = rc->direction; // assumed normalized
		float maxDist = rc->maxDistance;

		// Perform unified raycast
		RaycastResult hit = RaycastEngine::RaycastUnified(origin, direction, maxDist);

		// Write back into component
		rc->lastHit = hit;

		// Optional multi-hit logging
		rc->hits.clear();
		rc->hits.push_back(hit);
	}
}
////////////////////////////////