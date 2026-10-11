////////////////////////////////
// RaycastSystem.cpp
////////////////////////////////



////////////////////////////////
// Includes
#include "RaycastSystem.h"
#include "Entity.h"
#include "RaycastDispatcher.h" 
#include "RaycastFns.h"		  
////////////////////////////////



////////////////////////////////
// Constructor - register shape raycasters
RaycastSystem::RaycastSystem(EntityManager* em) : m_entityManager(em) {
	// Register all shape raycast functions here
	g_RaycastDispatcher.Register<CCircle>(RaycastCircle);
	g_RaycastDispatcher.Register<CRectangle>(RaycastRectangleShape);

	// We can add more later:
	// g_RaycastDispatcher.Register<CCapsule>(RaycastCapsule);
	// g_RaycastDispatcher.Register<CPolygon>(RaycastPolygon);
}
////////////////////////////////



////////////////////////////////
// Update - performs raycasting for all CRaycast components
void RaycastSystem::Update(float /*dt*/) {
	if (!m_entityManager)
		return;

	auto& entities = m_entityManager->GetEntities();

	for (auto& ptr : entities) {
		Entity* e = ptr.get();
		if (!e || !e->IsAlive())
			continue;

		CRaycast* rc = e->GetComponent<CRaycast>();
		if (!rc)
			continue;

		Vec2 origin = rc->worldPos;
		Vec2 direction = rc->direction; // assumed normalized
		float maxDist = rc->maxDistance;

		RaycastResult hit = RaycastEngine::RaycastUnified(origin, direction, maxDist);

		rc->lastHit = hit;

		rc->hits.clear();
		rc->hits.push_back(hit);
	}
}
////////////////////////////////
