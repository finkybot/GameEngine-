#include "PhysicsSystem.h"
#include "../Entity.h"
#include "../CShape.h"

void PhysicsSystem::Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime, float windowWidth, float windowHeight)
{
	for (const auto& entity : entities)
	{
		if (!entity->IsAlive())
			continue;
		
		MoveEntity(entity.get(), deltaTime, windowWidth, windowHeight);
	}
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
	Vec2 velocity = shape->GetVelocity();
	float radius = entity->GetRadius();
	
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
}
