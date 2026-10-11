/////////////////////////////////
// RaycastFns.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Vec2.h"
/////////////////////////////////



/////////////////////////////////
// Forward declarations
class Entity;
/////////////////////////////////



/////////////////////////////////
// Raycast function declarations for different entity types. These functions perform raycasting against specific shapes (AABB, Circle) and return whether a hit occurred, along with the hit time and normal vector at the hit point.
bool RaycastCircle(const Vec2& origin, const Vec2& direction, float maxDistance, const Entity& entity, float& outT, Vec2& outNormal);
bool RaycastRectangleShape(const Vec2& origin, const Vec2& dir, float maxDist, const Entity& e, float& outT, Vec2& outNormal);
	/////////////////////////////////