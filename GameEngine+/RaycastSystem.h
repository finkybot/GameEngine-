////////////////////////////////
// RaycastSystem - ECS system that processes CRaycast
// components and fills their hit data using RaycastEngine.
////////////////////////////////



////////////////////////////////
#pragma once
#include "EntityManager.h"
#include "CRaycast.h"
#include "RaycastEngine.h"
////////////////////////////////



////////////////////////////////
//	|	RaycastSystem class - ECS system that processes CRaycast components and fills their hit data using RaycastEngine. It iterates over all entities with a CRaycast component, performs raycasting based on the component's input data, and updates the component's output data with the results of the raycast. 
//	|	This system allows for efficient raycasting in a 2D game engine by leveraging the RaycastEngine and the spatial index for dynamic entities and tile grids.
//	|_______________________________________________________________________
class RaycastSystem {
public:
	RaycastSystem(EntityManager* entityManager) : m_entityManager(entityManager) {}

	void Update(float dt);

private:
	EntityManager* m_entityManager = nullptr;
};
////////////////////////////////