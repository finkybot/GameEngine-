// ***** PhysicsSystem class *****
#include "PhysicsSystem.h"
#include "../Entity.h"
#include "../CShape.h"
#include <execution>
#include <algorithm>
#include "../Vec2.h"

void PhysicsSystem::Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime, float windowWidth, float windowHeight)
{
	// Parallel execution: process each entity's physics independently
	std::for_each(std::execution::par, entities.begin(), entities.end(),
		[this, deltaTime, windowWidth, windowHeight](const std::unique_ptr<Entity>& entity)
		{
			if (!entity->IsAlive())
				return;

			MoveEntity(entity.get(), deltaTime, windowWidth, windowHeight);
		});
}

void PhysicsSystem::MoveEntity(Entity* entity, float deltaTime, float windowWidth, float windowHeight) const
{
	auto shape = entity->GetComponent<CShape>();
	if (!shape) return;

	const Vec2& velocity = shape->GetVelocity();
	const Vec2& position = shape->GetPosition();
	
	// Update position based on velocity
	shape->SetPosition(
		position.GetX() + velocity.GetX() * deltaTime,
		position.GetY() + velocity.GetY() * deltaTime
	);
	
	// Handle boundary collisions
	HandleBoundaryCollision(entity, windowWidth, windowHeight);
}

void PhysicsSystem::HandleBoundaryCollision(Entity* entity, float windowWidth, float windowHeight) const
{
	auto shape = entity->GetComponent<CShape>();
	if (!shape) return;

	Vec2 position = shape->GetPosition();
	float radius = entity->GetRadius();

	// Despawn entities that go off the left edge of screen
	if (position.GetX() + radius < 0.0f)
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
