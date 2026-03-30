// ***** PhysicsSystem.h - Physics system for handling entity movement and collisions *****
#pragma once
#include <vector>
#include <memory>
#include "../Vec2.h"

class Entity;

// PhysicsSystem is responsible for all physics requirements, I also handle boundary collisions (for now, I will at some time remove or depreciate it).
class PhysicsSystem
{
	// ****** Public Methods ******
public:
	PhysicsSystem() = default;	// Constructor - default is fine since we have no member variables to initialize
	~PhysicsSystem() = default;	// Destructor - default is fine since we have no resources to clean up

	// Updates the positions of all entities based on their velocities and handles boundary collisions. It iterates through the list of entities, moves each entity according to its velocity and the elapsed time (deltaTime), and checks for collisions with the window boundaries to apply rebounding effects.
	void Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime, float windowWidth, float windowHeight);

	void SlowEntity(Entity* entity, float slowFactor) const; // Applies a slowing effect to the entity by multiplying its velocity by the specified slow factor (a value between 0 and 1). This method reduces the entity's speed, simulating effects like friction or slowing zones in the game.

	// ****** Private Methods *****
private:
	// Moves a single entity based on its velocity and the elapsed time (deltaTime). It updates the entity's position by adding the product of its velocity and deltaTime to its current position.
	void MoveEntity(Entity* entity, float deltaTime, float windowWidth, float windowHeight) const;
	
	// Checks for collisions between the entity and the window boundaries. If a collision is detected, it inverts the corresponding velocity component (x or y) to create a rebounding effect and ensures the entity stays within the window bounds.
	void HandleBoundaryCollision(Entity* entity, float windowWidth, float windowHeight) const;
};
