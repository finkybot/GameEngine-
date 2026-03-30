// ***** PhysicsSystem class *****
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

void PhysicsSystem::Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime, float windowWidth, float windowHeight)
{
	// Parallel execution: process each entity's physics independently
	std::for_each(std::execution::par, entities.begin(), entities.end(),
		[this, deltaTime, windowWidth, windowHeight](const std::unique_ptr<Entity>& entity)
		{
			if (!entity->IsAlive())
				return;

			SlowEntity(entity.get(), 0.9992f); // Apply a global slow factor to simulate friction (can be adjusted or made dynamic)
			MoveEntity(entity.get(), deltaTime, windowWidth, windowHeight);
		});
}

void PhysicsSystem::SlowEntity(Entity* entity, float slowFactor) const
{
	// If entity is marked static, skip slowing
	if (entity->HasComponent<CStatic>())
		return;
	// Prefer transform component as authoritative velocity source
	auto transform = entity->GetComponent<CTransform>();
	auto shape = entity->GetComponent<CShape>();
	if (transform)
	{
		transform->m_velocity.x *= slowFactor;
		transform->m_velocity.y *= slowFactor;
	}
}

void PhysicsSystem::MoveEntity(Entity* entity, float deltaTime, float windowWidth, float windowHeight) const
{
    // If entity is marked static, skip movement
	if (entity->HasComponent<CStatic>())
		return;

	// Prefer transform component as authoritative position/velocity source
	auto transform = entity->GetComponent<CTransform>();
	auto shape = entity->GetComponent<CShape>();

	if (transform)
	{
		// Update transform position
		transform->m_position.x += transform->m_velocity.x * deltaTime;
		transform->m_position.y += transform->m_velocity.y * deltaTime;
	}

	// Handle boundary collisions
	HandleBoundaryCollision(entity, windowWidth, windowHeight);
}

void PhysicsSystem::HandleBoundaryCollision(Entity* entity, float windowWidth, float windowHeight) const
{
	auto shape = entity->GetComponent<CShape>();
	if (!shape) return;

	auto transform = entity->GetComponent<CTransform>();
	if (!transform) return;

	Vec2 position = transform->m_position;
	float radius = entity->GetRadius();

	// Despawn entities that go off the of the screen, allowing a 100-unit buffer for them to fully exit before despawning. This prevents entities from bouncing back and forth at the edges and allows for a more natural flow of entities across the screen.
	if (position.GetX() + radius < -101.0f || position.GetX() - radius > windowWidth + 101.0f || position.GetY() + radius < -101.0f || position.GetY() - radius > windowHeight + 101.0f)
	{
		entity->Destroy();
		return;
	}






	// Consider the entity's bounding circle
	//float left = position.GetX() - radius;
	//float right = position.GetX() + radius;
	//float top = position.GetY() - radius;
	//float bottom = position.GetY() + radius;

	//// If the entity has moved fully off any side of the window, despawn it
	//if (right < 0.0f || left > windowWidth || bottom < 0.0f || top > windowHeight)
	//{
	//	entity->Destroy();
	//	return;
	//}

	/* LEGACY: Boundary bouncing code (kept for future use)
	Vec2 velocity = shape->GetVelocity();
	bool bounced = false;

	// Left boundary
	if (position.GetX() - radius < 0.0f)
	{
		position.x = radius;
		velocity.x = -velocity.x * 0.9f; // Reverse and dampen slightly
		bounced = true;
	}

	// Right boundary
	else if (position.GetX() + radius > windowWidth)
	{
		position.x = windowWidth - radius;
		velocity.x = -velocity.x * 0.9f; // Reverse and dampen slightly
		bounced = true;
	}

	// Top boundary
	if (position.GetY() - radius < 0.0f)
	{
		position.y = radius;
		velocity.y = -velocity.y * 0.9f; // Reverse and dampen slightly
		bounced = true;
	}

	// Bottom boundary
	else if (position.GetY() + radius > windowHeight)
	{
		position.y = windowHeight - radius;
		velocity.y = -velocity.y * 0.9f; // Reverse and dampen slightly
		bounced = true;
	}

	if (bounced)
	{
		shape->SetPosition(position.GetX(), position.GetY());
		shape->SetInitialVelocity(velocity.GetX(), velocity.GetY());
	}
	*/
}

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
