/////////////////////////////////
// RaycastResult and CRaycast structures for raycasting in a 2D game engine
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Vec2.h"
#include "Entity.h"
/////////////////////////////////



/////////////////////////////////
//	|	RaycastResult structure - stores the result of a raycast operation, including whether a hit occurred, the distance to the hit, the position and normal of the hit, and references to the entity and tile that were hit.
//	|_______________________________________________________________________
struct RaycastResult {
	bool hit = false;
	float distance = 0.f;

	Vec2 position;
	Vec2 normal;

	Entity* entity = nullptr; // dynamic entity hit

	int tileX = -1; // tile hit
	int tileY = -1;
	int tileValue = -1;
};
/////////////////////////////////



/////////////////////////////////
//	|	CRaycast structure - stores the input and output data for a raycast operation, including the screen-space and world-space positions, the direction of the ray, the maximum distance, and the results of the raycast.
//	|_______________________________________________________________________
struct CRaycast : public Component {
	// Input
	Vec2 screenPos; // raw screen-space (mouse)
	Vec2 worldPos;	// converted world-space origin
	Vec2 direction; // world-space direction
	float maxDistance = 1000.f;

	// Output
	RaycastResult lastHit;			 // nearest hit
	std::vector<RaycastResult> hits; // optional multi-hit

	bool debugDraw = false;
};
/////////////////////////////////