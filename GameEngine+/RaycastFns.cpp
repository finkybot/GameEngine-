/////////////////////////////////
// RaycastFns.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "RaycastFns.h"
#include "Entity.h"
#include "CTransform.h"
#include "CCircle.h"
#include "CRectangle.h"
#include "Raycast.h"   // for RayIntersectsAABB
#include <cmath>
/////////////////////////////////



/////////////////////////////////
// RaycastCircle - performs raycasting against a circle entity. This function is a placeholder and currently returns false, indicating no hit. In a complete implementation, it would calculate the intersection of the ray with the circle and return true if a hit occurred, 
// along with the hit time and normal vector at the hit point.
bool RaycastCircle(const Vec2& origin, const Vec2& direction, float maxDistance, const Entity& entity, float& outT, Vec2& outNormal) {
	// Retrieve the CTransform and CCircle components from the entity
	auto* t = entity.GetComponent<CTransform>();
	auto* c = entity.GetComponent<CCircle>();

	// Check if both components are present
	if (!t || !c) {	return false; /* If either component is missing, return false (no hit) */ }


	// Get the centre and radius of the circle 
	Vec2 centre = t->position; // transform position is the centre of the circle
	float radius = c->GetRadius(); // CCircle contains the radius of the circle

	// Compute the vector from the ray origin to the circle centre
	Vec2 oc = origin - centre;

	// Compute the closest approach of the ray to the circle centre using the dot product
	float tClosest = oc.Dot(direction); 

	// Check if the closest approach is behind the ray origin
	if (tClosest < 0.0f) { return false; /* If the closest approach is behind the ray origin, return false (no hit) */ }

	// Compute the squared distance from the ray to the circle centre
	float distSq = oc.Mag2() - tClosest * tClosest; 
	float radiusSq = radius * radius;

	// Check if the squared distance is greater than the squared radius
	if (distSq > radiusSq) { return false; /* If the ray misses the circle, return false (no hit) */ }

	// Compute the distance from the closest approach to the intersection point then calculate the hit time along the ray
	float thc =	std::sqrt(radiusSq - distSq); 
	float tHit = tClosest + thc;

	// Check if the hit time is within the valid range of the ray
	if (tHit < 0.0f || tHit > maxDistance) { return false; /* If the hit time is out of bounds, return false (no hit) */ }

	// Compute the hit point and normal vector at the hit point
	outT = tHit;

	Vec2 hitPos = origin + direction * tHit;
	outNormal = (hitPos - centre).Normalise(); // Normal vector points from the circle centre to the hit point
	return true;
}
/////////////////////////////////



/////////////////////////////////
// RaycastRectangleShape - performs raycasting against a rectangle entity. This function retrieves the CTransform and CRectangle components from the entity, computes the rectangle's AABB bounds, and checks for intersection with the ray using a slab intersection method. 
// If a hit occurs, it calculates the hit time and normal vector at the hit point.
bool RaycastRectangleShape(const Vec2& origin, const Vec2& dir, float maxDist, const Entity& e, float& outT, Vec2& outNormal) {
	auto* t = e.GetComponent<CTransform>();
	auto* r = e.GetComponent<CRectangle>();
	if (!t || !r)
		return false;

	// Rectangle size
	float w = r->GetWidth();
	float h = r->GetHeight();

	// Compute half extents
	Vec2 half(w * 0.5f, h * 0.5f);

	// Rectangle centre
	Vec2 center = r->GetCentrePoint();

	// Compute AABB bounds
	Vec2 min = center - half;
	Vec2 max = center + half;

	// Use your existing slab intersection
	if (!RayIntersectsAABB(origin, dir, min, max, outT, maxDist)) return false;

	// Compute hit normal
	Vec2 hitPos = origin + dir * outT;

	const float eps = 0.001f;
	if (std::fabs(hitPos.x - min.x) < eps)
		outNormal = Vec2(-1, 0);
	else if (std::fabs(hitPos.x - max.x) < eps)
		outNormal = Vec2(1, 0);
	else if (std::fabs(hitPos.y - min.y) < eps)
		outNormal = Vec2(0, -1);
	else if (std::fabs(hitPos.y - max.y) < eps)
		outNormal = Vec2(0, 1);

	return true;
}

/////////////////////////////////