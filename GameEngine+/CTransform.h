// ***** CTransform.h - Transform component for storing position and velocity data *****
#pragma once
#include "Vec2.h"
#include "Component.h"

// CTransform component - stores position and velocity data for an entity. This is a pure data component used by the PhysicsSystem to update entity positions based on their velocities. 
// It allows for separation of concerns by keeping movement-related data in a dedicated component, making it easier to manage and update entity transformations in the game.
class CTransform : public Component
{
	// ***** Public Member Variables (ECS pure data component) *****
public:
	Vec2 position = { 0, 0 };	// World position of the entity (units in pixels, with (0, 0) at the top-left corner of the window)
	Vec2 velocity = { 0, 0 };	// Velocity of the entity (units in pixels per second, used by PhysicsSystem for movement)

	CTransform() {}																	// Default constructor initializes position and velocity to zero vectors
	CTransform(const Vec2& pos, const Vec2& vel) : position(pos), velocity(vel) {};	// Constructor with initial position and velocity														
};



