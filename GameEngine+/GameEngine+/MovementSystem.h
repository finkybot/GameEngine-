/////////////////////////////////
// MovementSystem.h - System for moving entities along computed paths
/////////////////////////////////




/////////////////////////////////
// Includes
#pragma once
#include <vector>
#include <memory>
#include "Vec2.h"
/////////////////////////////////




/////////////////////////////////
// Forward declarations
class Entity;
/////////////////////////////////




/////////////////////////////////
// MovementSystem - Moves entities with CPathFollower + CPath components along waypoints each frame. Entities with both components and isActive=true will move from waypoint to waypoint at their configured 
// speed. When the last waypoint is reached, isActive is set to false automatically (components remain for reuse).
//
// Usage:
//   1. Entity must have CPath component with waypoints
//   2. Add CPathFollower component with desired speed
//   3. Set CPathFollower::isActive = true
//   4. MovementSystem::Update() handles the rest each frame
//								|
//								|_______________________________________________________________________
class MovementSystem {
	/////////////////////////////////
	// Public interface
public:
	/////////////////////////////////
	// Constructor and destructor
	MovementSystem() = default;
	~MovementSystem() = default;
	/////////////////////////////////



	/////////////////////////////////
	// Update - Moves all entities with CPathFollower + CPath components along their paths based on deltaTime
	void Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime);
	/////////////////////////////////



	/////////////////////////////////
	// Private constants
private:
	/////////////////////////////////
	// Distance threshold to consider a waypoint reached
	static constexpr float WAYPOINT_ARRIVAL_THRESHOLD = 5.0f;
	/////////////////////////////////
};
/////////////////////////////////
