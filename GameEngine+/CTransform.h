/////////////////////////////////
// Transform.h - Transform component for storing position and velocity data. This is a pure data component used by the PhysicsSystem to update entity positions based on their velocities. 
// It allows for separation of concerns by keeping movement-related data in a dedicated component, making it easier to manage and update entity transformations in the game.
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the CTransform component. We include the base Component class for ECS architecture and a custom Vec2 class for 2D vector operations, which will be used to represent position and velocity in the transform component.
#pragma once
#include "Vec2.h"
#include "Component.h"
/////////////////////////////////



/////////////////////////////////
// CTransform component - stores position and velocity data for an entity. This is a pure data component used by the PhysicsSystem to update entity positions based on their velocities.
// It allows for separation of concerns by keeping movement-related data in a dedicated component, making it easier to manage and update entity transformations in the game.
class CTransform : public Component {
	/////////////////////////////////
	// Public member variables for CTransform.
public:
	/////////////////////////////////
	// Member variables for position and velocity.
	Vec2 position = {0, 0}; // World position of the entity (units in pixels, with (0, 0) at the top-left corner of the window)
	Vec2 velocity = {0, 0}; // Velocity of the entity (units in pixels per second, used by PhysicsSystem for movement)
	/////////////////////////////////



	/////////////////////////////////
	// Constructors for the CTransform component. The default constructor initializes position and velocity to zero vectors, while the constructor with parameters allows for initializing the transform with specific position and velocity values.
	CTransform() {}
	CTransform(const Vec2& pos, const Vec2& vel): position(pos), velocity(vel) {};
	/////////////////////////////////
};
/////////////////////////////////
