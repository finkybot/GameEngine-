#pragma once
#include <vector>
#include <memory>
#include "../Vec2.h"

class Entity;

/// <summary>
/// Physics System - Updates entity positions and handles boundary collisions
/// Applies velocity to position and bounces entities off window edges
/// </summary>
class PhysicsSystem
{
public:
	/// <summary>Constructor</summary>
	PhysicsSystem() = default;
	
	/// <summary>Destructor</summary>
	~PhysicsSystem() = default;

	/// <summary>
	/// Update all entity positions based on their velocities and handle boundary collisions
	/// </summary>
	/// <param name="entities">List of all active entities</param>
	/// <param name="deltaTime">Time elapsed since last frame (in seconds)</param>
	/// <param name="windowWidth">Width of the render window</param>
	/// <param name="windowHeight">Height of the render window</param>
	void Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime, float windowWidth, float windowHeight);

private:
	/// <summary>
	/// Move a single entity based on its velocity
	/// </summary>
	void MoveEntity(Entity* entity, float deltaTime, float windowWidth, float windowHeight) const;
	
	/// <summary>
	/// Handle collision with window boundaries (rebounding off edges)
	/// Reverses velocity component and applies damping when entity hits edge
	/// </summary>
	void HandleBoundaryCollision(Entity* entity, float windowWidth, float windowHeight) const;
};
