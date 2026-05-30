/////////////////////////////////
// PhysicsSystem is responsible for all physics requirements, I also handle boundary collisions (for now, I will at some time remove or depreciate it).
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <vector>
#include <memory>
#include "../Vec2.h"
/////////////////////////////////



/////////////////////////////////
// Forward declaration of the Entity class, which is used by the PhysicsSystem to access entities and their components for updating positions and handling collisions. This allows us to avoid circular dependencies between the PhysicsSystem and Entity classes, 
// since we only need a pointer to Entity in the PhysicsSystem header.
class Entity;
/////////////////////////////////



/////////////////////////////////
// PhysicsSystem class - Responsible for updating the positions of entities based on their velocities and handling boundary collisions. It provides methods for applying a slowing effect to entities and for moving entities according to their velocity and elapsed time. 
// The PhysicsSystem interacts with the EntityManager to access entities and their components, allowing it to update their positions and handle collisions with the window boundaries.
class PhysicsSystem {
	/////////////////////////////////
	// Public interface for the PhysicsSystem class
public:
	/////////////////////////////////
	// Constructor and destructor for the PhysicsSystem class. The default constructor is sufficient since we have no member variables to initialize, and the default destructor is also sufficient since we have no resources to clean up.
	PhysicsSystem() = default;
	~PhysicsSystem() = default; 
	/////////////////////////////////



	/////////////////////////////////
	// Update - Handles updating the positions of entities based on their velocities and the elapsed time (deltaTime), as well as handling boundary collisions with the window edges. This method should be called every frame to ensure that entities are moved according to their 
	// velocities and that they bounce off the window boundaries when they collide with them.
	void Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime, float windowWidth, float windowHeight);
	/////////////////////////////////



	/////////////////////////////////
	// SlowEntity - Applies a slowing effect to the entity by multiplying its velocity by the specified slow factor (a value between 0 and 1). This method reduces the entity's speed, simulating effects like friction or slowing zones in the game. 
	// It should be called whenever you want to apply a slowing effect to an entity, such as when it enters a slowing zone or is affected by a debuff.
	void SlowEntity(Entity* entity, float slowFactor) const; 
	/////////////////////////////////
	


	/////////////////////////////////
	// Private helper methods
private:
	/////////////////////////////////
	// MoveEntity - Updates the position of the entity based on its velocity and the elapsed time (deltaTime). This method calculates the new position by adding the product of velocity and deltaTime to the current position, 
	// allowing entities to move smoothly across the screen according to their velocities.
	void MoveEntity(Entity* entity, float deltaTime, float windowWidth, float windowHeight) const;
	/////////////////////////////////



	/////////////////////////////////
	// HandleBoundaryCollision - Checks for collisions between the entity and the window boundaries. If a collision is detected, it inverts the corresponding velocity component (x or y) to create a rebounding effect and ensures the entity stays within the window bounds.
	void HandleBoundaryCollision(Entity* entity, float windowWidth, float windowHeight) const;
	/////////////////////////////////
};
/////////////////////////////////