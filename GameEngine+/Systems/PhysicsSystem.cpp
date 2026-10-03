/////////////////////////////////
// PhysicsSystem.cpp - Implementation of the PhysicsSystem class, responsible for updating entity positions based on their velocities and handling boundary collisions with the window edges. This system should be called every frame to ensure that entities move according to 
// their velocities and interact properly with the window boundaries.
/////////////////////////////////



/////////////////////////////////
#include "PhysicsSystem.h"
#include "EntityManager.h"
#include "../Entity.h"
#include "../CShape.h"

#include "../Vec2.h"
#include <vector>
#include "../CStatic.h"
/////////////////////////////////



/////////////////////////////////
// Update - Handles updating the positions of entities based on their velocities and the elapsed time (deltaTime), as well as handling boundary collisions with the window edges. This method should be called every frame to ensure that entities are moved according to their velocities and that they bounce off the window
void PhysicsSystem::Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime, float worldW,
						   float worldH) {
	auto& T = m_entityManager->GetTransformSoA();
	float slowFactor = std::pow(0.999f, deltaTime * 60.0f);

	for (auto& up : entities) {
		Entity* e = up.get();
		if (!e->IsAlive())
			continue;
		if (e->HasComponent<CStatic>())
			continue;

		size_t idx = e->transformIndex;
		if (idx == SIZE_MAX)
			continue;

		// Apply friction
		T.velX[idx] *= slowFactor;
		T.velY[idx] *= slowFactor;

		// Integrate velocity → new position
		float newX = T.posX[idx] + T.velX[idx] * deltaTime;
		float newY = T.posY[idx] + T.velY[idx] * deltaTime;

		// Write through SoA (marks dirty)
		T.SetPosition(idx, newX, newY);

		// Boundary handling
		HandleBoundaryCollisionSoA(e, idx, worldW, worldH);
	}
}
/////////////////////////////////



/////////////////////////////////
// UpdateSingle - Updates the position of a single entity based on its velocity and the elapsed time (dt), as well as handling boundary collisions with the window edges. This method should be called for individual entities that need to be updated, allowing for more granular control over entity movement and 
// collision handling.
void PhysicsSystem::UpdateSingle(Entity* entity, float dt, float worldW, float worldH) {
	if (!entity->IsAlive())
		return;
	if (entity->HasComponent<CStatic>())
		return;

	auto& T = m_entityManager->GetTransformSoA();
	size_t idx = entity->transformIndex;
	if (idx == SIZE_MAX)
		return;

	float slowFactor = std::pow(0.999f, dt * 60.0f);

	// Apply friction
	T.velX[idx] *= slowFactor;
	T.velY[idx] *= slowFactor;

	// Integrate velocity → new position
	float newX = T.posX[idx] + T.velX[idx] * dt;
	float newY = T.posY[idx] + T.velY[idx] * dt;

	// Write through SoA (marks dirty)
	T.SetPosition(idx, newX, newY);

	// Boundary handling
	HandleBoundaryCollisionSoA(entity, idx, worldW, worldH);
}
/////////////////////////////////



/////////////////////////////////
// HandleBoundaryCollisionSoA - Checks for collisions between the entity and the window boundaries. If a collision is detected, it inverts the corresponding velocity component (x or y) to create a rebounding effect and ensures the entity stays within the window bounds. This method is designed for use with 
// Structure of Arrays (SoA) data layout, allowing for efficient access to position and velocity data.
void PhysicsSystem::HandleBoundaryCollisionSoA(Entity* entity, size_t idx, float windowWidth, float windowHeight) const {
	auto shape = entity->GetComponent<CShape>();
	// If the entity does not have a shape component, we cannot perform boundary collision checks, so we return early.
	if (!shape)	return;

	auto& T = m_entityManager->GetTransformSoA();

	float x = T.posX[idx];
	float y = T.posY[idx];
	float radius = entity->GetRadius();

	// Despawn when fully off‑screen with buffer
	if (x + radius < -101.0f || x - radius > windowWidth + 101.0f || y + radius < -101.0f ||
		y - radius > windowHeight + 101.0f) {
		entity->Destroy();
		return;
	}

	// Clamp X
	float clampedX = x;
	if (x - radius < 0.0f)
		clampedX = radius;
	else if (x + radius > windowWidth)
		clampedX = windowWidth - radius;

	// Clamp Y
	float clampedY = y;
	if (y - radius < 0.0f)
		clampedY = radius;
	else if (y + radius > windowHeight)
		clampedY = windowHeight - radius;

	// If clamped, write through SoA (marks dirty)
	if (clampedX != x || clampedY != y)
		T.SetPosition(idx, clampedX, clampedY);
}
/////////////////////////////////