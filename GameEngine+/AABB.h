/////////////////////////////////
// AABB (Axis-Aligned Bounding Box) structure for representing rectangular bounding boxes in 2D space. It contains minimum and maximum points (Vec2) and provides methods for expanding the box and checking intersection with a sphere.
/////////////////////////////////



/////////////////////////////////
// Includes 
#pragma once
#include "Vec2.h"
/////////////////////////////////



/////////////////////////////////
// AABB (Axis-Aligned Bounding Box) structure definition. This struct represents a rectangular bounding box in 2D space, defined by its minimum and maximum points (Vec2). It provides methods for expanding the box to include another box and checking for intersection with a sphere.
struct AABB {
	/////////////////////////////////
	// Public member variables
	Vec2 min;
	Vec2 max;
	/////////////////////////////////



	/////////////////////////////////
	// Constructors for the AABB struct. The default constructor initializes the box to a zero-sized box at the origin, while the parameterized constructor allows for specifying the minimum and maximum points.
	AABB() : min(0, 0), max(0, 0) {}
	AABB(const Vec2& mn, const Vec2& mx) : min(mn), max(mx) {}
	/////////////////////////////////



	/////////////////////////////////
	// Expand this box to include another box
	void expand(const AABB& other) {
		min.x = std::min(min.x, other.min.x);
		min.y = std::min(min.y, other.min.y);
		max.x = std::max(max.x, other.max.x);
		max.y = std::max(max.y, other.max.y);
	}
	/////////////////////////////////
};
/////////////////////////////////



/////////////////////////////////
// AABBIntersectsSphere - Checks if an axis-aligned bounding box (AABB) intersects with a sphere defined by its center (Vec2) and radius (float). This function calculates the closest point on the AABB to the sphere's center and
// checks if the distance from that point to the center is less than or equal to the radius squared.
inline bool AABBIntersectsSphere(const AABB& b, const Vec2& c, float r) {
	float cx = std::max(b.min.x, std::min(c.x, b.max.x));
	float cy = std::max(b.min.y, std::min(c.y, b.max.y));
	float dx = cx - c.x;
	float dy = cy - c.y;
	return (dx * dx + dy * dy) <= r * r;
}
/////////////////////////////////
