/////////////////////////////////
// CPathRequest and CPath are data-only components used for pathfinding in a game engine. CPathRequest holds information about a pathfinding request, 
// including the target position, whether partial paths are allowed, and an optional request ID. CPath holds the resulting path points, the echoed 
// request ID, and a flag indicating if the path is complete or partial.
/////////////////////////////////



/////////////////////////////////
// includes and forward declarations for the CPathRequest and CPath components. We include necessary headers for component handling, 2D vector math,
#pragma once
#include "Component.h"
#include "Vec2.h"
#include <vector>
#include <optional>
#include <cstdint>
/////////////////////////////////



/////////////////////////////////
// CPathRequest component - Represents a pathfinding request in the game world, with properties for the target position, partial path allowance, immediate search preference, and an optional request ID.
//						|
// 						|___________________________________________________________________________________
// Minimal path request component (data-only)
struct CPathRequest : public Component {
	/////////////////////////////////
	// Public member variables for the CPathRequest component
	Vec2 targetWorld; // world-space target position (or tile center)
	bool allowPartial = true;
	bool immediate = true;	// if true, system may perform synchronous search or prioritize job
	uint32_t requestId = 0; // client-provided id for matching results
	CPathRequest() = default;
	explicit CPathRequest(const Vec2& t) : targetWorld(t) {}
	/////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// CPath component - Represents the result of a pathfinding request, with properties for the path points, echoed request ID, and completion status.
// 						|
//						|___________________________________________________________________________________
// Minimal path result component (data-only)
struct CPath : public Component {
	/////////////////////////////////
	// Public member variables for the CPath component
	std::vector<Vec2> points; // world-space path polyline
	uint32_t requestId = 0;	  // echoed request id
	bool complete = true;	  // full path or partial
	CPath() = default;
	/////////////////////////////////
};
/////////////////////////////////




/////////////////////////////////
// CPathFollower component - Tracks movement state for entities following paths.
// Add this component to enable path following. Set isActive=true to start movement.
// The MovementSystem will advance the entity along CPath::points each frame.
// When the last waypoint is reached, isActive is set to false automatically.
struct CPathFollower : public Component {
	/////////////////////////////////
	// Public member variables for the CPathFollower component
	float speed = 50.0f;						// movement speed in pixels per second
	int currentWaypointIndex = 0;				// index of current target waypoint
	bool isActive = false;						// controls whether movement is active
	float distanceSinceLastFrame = 0.0f;		// accumulates sub-frame movement for precision

	CPathFollower() = default;
	explicit CPathFollower(float spd) : speed(spd) {}
	/////////////////////////////////
};
/////////////////////////////////
