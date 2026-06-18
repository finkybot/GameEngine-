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
void PhysicsSystem::Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime, float windowWidth,
						   float windowHeight) {
	// Parallel execution: process each entity's physics independently
	std::for_each(std::execution::par, entities.begin(), entities.end(),
				  [this, deltaTime, windowWidth, windowHeight](const std::unique_ptr<Entity>& entity) {
					  if (!entity->IsAlive())
						  return;

					  SlowEntity(
						  entity.get(),
						  0.9999f); // Apply a global slow factor to simulate friction (can be adjusted or made dynamic)
					  MoveEntity(entity.get(), deltaTime, windowWidth, windowHeight);
				  });
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
		transform->m_velocity.x *= slowFactor;
		transform->m_velocity.y *= slowFactor;
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
		transform->m_position.x += transform->m_velocity.x * deltaTime;
		transform->m_position.y += transform->m_velocity.y * deltaTime;
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

	Vec2 position = transform->m_position;
	float radius = entity->GetRadius();

	// Despawn entities that go off the of the screen, allowing a 100-unit buffer for them to fully exit before despawning. This prevents entities from bouncing back and forth at the edges and allows for a more natural flow of entities across the screen.
	if (position.GetX() + radius < -101.0f || position.GetX() - radius > windowWidth + 101.0f ||
		position.GetY() + radius < -101.0f || position.GetY() - radius > windowHeight + 101.0f) {
		entity->Destroy();
		return;
	}
}
/////////////////////////////////



/////////////////////////////////
// UpdateExplosions - iterates through all explosion entities and updates their size and alpha based on their age. Explosions grow in size and fade out over a lifespan of 600 milliseconds, creating a visually appealing effect that reacts to the music. 
// Once an explosion exceeds its lifespan, it is destroyed to remove it from the scene. - Deprecated: this method is now handled within the MusicVisualizerScene to allow for more dynamic explosion effects that react to the music spectrum. 
// The explosion lifespan and visual properties can be adjusted based on the music level for a more immersive experience.
//void PhysicsSystem::UpdateExplosions()
//{
//	auto now = std::chrono::high_resolution_clock::now();
//	std::vector<size_t> expiredExplosions;
//
//	for (auto& [explosionId, creationTime] : m_explosionTimes)
//	{
//		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - creationTime);
//
//		if (elapsed.count() > 600)
//		{
//			for (auto& entity : m_entities)
//			{
//				if (entity->m_id == explosionId)
//				{
//					entity->Destroy();
//					break;
//				}
//			}
//			expiredExplosions.push_back(explosionId);
//		}
//		else
//		{
//			float fadeProgress = static_cast<float>(elapsed.count()) / 600.0f;
//			int newAlpha = static_cast<int>(200 * (1.0f - fadeProgress));
//
//			for (auto& entity : m_entities)
//			{
//				if (entity->m_id == explosionId)
//				{
//					auto shape = entity->GetComponent<CShape>();
//					if (shape)
//					{
//						if (auto* circle = dynamic_cast<CCircle*>(shape))
//						{
//							circle->SetRadius(circle->GetRadius() + 0.5f); // Expand the explosion radius over time
//							Vec2 explosionPosition = circle->GetPosition();
//							circle->SetPosition(explosionPosition.x + 0.4f, explosionPosition.y - 0.5f); // Keep the explosion centered as it expands, adding a little drift for visual interest
//							sf::Color currentColor = circle->GetColor();
//							circle->SetColor(
//								static_cast<float>(currentColor.r),
//								static_cast<float>(currentColor.g),
//								static_cast<float>(currentColor.b),
//								newAlpha
//							);
//						}
//					}
//				}
//			}
//		}
//	}
//
//	for (size_t explosionId : expiredExplosions)
//	{
//		m_explosionTimes.erase(explosionId);
//		m_explosionColors.erase(explosionId);
//	}
//}
/////////////////////////////////
