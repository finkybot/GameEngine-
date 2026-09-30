/////////////////////////////////
// PhysicsSystem.cpp - Implementation of the PhysicsSystem class, responsible for updating entity positions based on their velocities and handling boundary collisions with the window edges. This system should be called every frame to ensure that entities move according to 
// their velocities and interact properly with the window boundaries.
/////////////////////////////////



/////////////////////////////////
#include "PhysicsSystem.h"
#include "../Entity.h"
#include "../CShape.h"
#include <execution>
#include <algorithm>
#include "../Vec2.h"
#include <SFML/Graphics/Color.hpp>
#include <chrono>
#include <memory>
#include <vector>
#include "../CStatic.h"
/////////////////////////////////



/////////////////////////////////
// Update - Handles updating the positions of entities based on their velocities and the elapsed time (deltaTime), as well as handling boundary collisions with the window edges. This method should be called every frame to ensure that entities are moved according to their velocities 
// and that they bounce off the window boundaries when they collide with them.
void PhysicsSystem::Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime, float worldW,
						   float worldH) {
	float slowFactor = std::pow(0.999f, deltaTime * 60.0f);

	for (auto& entity : entities) {
		if (!entity->IsAlive())
			continue;

		SlowEntity(entity.get(), slowFactor);
		MoveEntity(entity.get(), deltaTime, worldW, worldH);
	}
}
/////////////////////////////////



/////////////////////////////////
void PhysicsSystem::UpdateSingle(Entity* entity, float dt, float worldW, float worldH) {
	if (!entity->IsAlive())
		return;

	float slowFactor = std::pow(0.999f, dt * 60.0f);

	// Slow
	SlowEntity(entity, slowFactor);

	// Move + boundary
	MoveEntity(entity, dt, worldW, worldH);
}
/////////////////////////////////



/////////////////////////////////
// SlowEntity - Applies a slowing effect to the entity by multiplying its velocity by the specified slow factor (a value between 0 and 1). This method reduces the entity's speed, simulating effects like friction or slowing zones in the game. 
// It should be called whenever you want to apply a slowing effect to an entity,
void PhysicsSystem::SlowEntity(Entity* entity, float slowFactor) const {
	// If entity is marked static, skip slowing
	if (entity->HasComponent<CStatic>())
		return;
	// Prefer transform component as authoritative velocity source
	auto transform = entity->GetComponent<CTransform>();
	auto shape = entity->GetComponent<CShape>();
	if (transform) {
		transform->velocity.x *= slowFactor;
		transform->velocity.y *= slowFactor;
	}
}
/////////////////////////////////



/////////////////////////////////
// MoveEntity - Updates the position of the entity based on its velocity and the elapsed time (deltaTime). This method calculates the new position by adding the product of velocity and deltaTime to the current position, allowing entities to move smoothly
// across the screen according to their velocities.
void PhysicsSystem::MoveEntity(Entity* entity, float deltaTime, float windowWidth, float windowHeight) const {
	// If entity is marked static, skip movement
	if (entity->HasComponent<CStatic>())
		return;

	// Prefer transform component as authoritative position/velocity source
	auto transform = entity->GetComponent<CTransform>();
	auto shape = entity->GetComponent<CShape>();

	if (transform) {
		// Update transform position
		transform->position.x += transform->velocity.x * deltaTime;
		transform->position.y += transform->velocity.y * deltaTime;
	}

	// Handle boundary collisions
	HandleBoundaryCollision(entity, windowWidth, windowHeight);
}
/////////////////////////////////



/////////////////////////////////
// HandleBoundaryCollision - Checks for collisions between the entity and the window boundaries. If a collision is detected, it inverts the corresponding velocity component (x or y) to create a rebounding effect and ensures the entity stays within the window bounds.
void PhysicsSystem::HandleBoundaryCollision(Entity* entity, float windowWidth, float windowHeight) const {
	auto shape = entity->GetComponent<CShape>();
	if (!shape)
		return;

	auto transform = entity->GetComponent<CTransform>();
	if (!transform)
		return;

	Vec2 position = transform->position;
	float radius = entity->GetRadius();

	// Despawn entities that go off the of the screen, allowing a 100-unit buffer for them to fully exit before despawning. This prevents entities from bouncing back and forth at the edges and allows for a more natural flow of entities across the screen.
	if (position.GetX() + radius < -101.0f || position.GetX() - radius > windowWidth + 101.0f ||
		position.GetY() + radius < -101.0f || position.GetY() - radius > windowHeight + 101.0f) {
		entity->Destroy();
		return;
	}
}
/////////////////////////////////

