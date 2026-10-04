/////////////////////////////////
// RaycastEngine.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "RaycastEngine.h"
#include "BVHSystem.h"
#include "SpatialIndexUnified.h"
#include "BVHSystem.h"
#include "Raycast.h"
#include <limits>
/////////////////////////////////



/////////////////////////////////
// RaycastDynamic - Perform a raycast against dynamic entities using the dynamic BVH. Returns the nearest dynamic entity hit (if any).
RaycastResult RaycastEngine::RaycastDynamic(const Vec2& origin, const Vec2& direction, float maxDistance) {
	RaycastResult result;

	SpatialIndexUnified& spatial = SpatialIndexUnified::Get();
	BVHSystem& bvh = spatial.GetDynamicBVH();

	if (!bvh.GetRoot())
		return result;

	RaycastHit hit; // tile-style hit struct
	Entity* hitEntity = nullptr;

	bool didHit = bvh.Raycast(origin, direction, maxDistance, hit, hitEntity);

	if (!didHit || !hitEntity)
		return result;

	// Convert BVH hit → unified RaycastResult
	result.hit = true;
	result.distance = hit.distance;
	result.position = hit.position;
	result.normal = hit.normal;
	result.entity = hitEntity;

	// dynamic hit → no tile data
	result.tileX = -1;
	result.tileY = -1;
	result.tileValue = -1;

	return result;
}
/////////////////////////////////



/////////////////////////////////
// RaycastTiles - Perform a raycast against the tile grid using DDA or tile BVH. Returns the nearest tile hit (if any).
RaycastResult RaycastEngine::RaycastTiles(const Vec2& origin, const Vec2& direction, float maxDistance) {
	RaycastResult result;

	SpatialIndexUnified& spatial = SpatialIndexUnified::Get();

	const int tileSize = spatial.GetTileSize(); // you may need to expose this
	const int worldW = spatial.GetWorldWidth();
	const int worldH = spatial.GetWorldHeight();
	const int offX = spatial.GetWorldOffsetX();
	const int offY = spatial.GetWorldOffsetY();

	// Convert origin to tile coordinates
	float ox = (origin.x - offX) / tileSize;
	float oy = (origin.y - offY) / tileSize;

	int tileX = static_cast<int>(std::floor(ox));
	int tileY = static_cast<int>(std::floor(oy));

	float dx = direction.x;
	float dy = direction.y;

	// DDA setup
	float stepX = (dx > 0 ? 1.f : -1.f);
	float stepY = (dy > 0 ? 1.f : -1.f);

	float tDeltaX = (dx != 0.f) ? std::abs(1.f / dx) : std::numeric_limits<float>::infinity();
	float tDeltaY = (dy != 0.f) ? std::abs(1.f / dy) : std::numeric_limits<float>::infinity();

	float nextTileBoundaryX = (stepX > 0 ? (tileX + 1) : tileX);
	float nextTileBoundaryY = (stepY > 0 ? (tileY + 1) : tileY);

	float tMaxX = (dx != 0.f) ? (nextTileBoundaryX - ox) / dx : std::numeric_limits<float>::infinity();
	float tMaxY = (dy != 0.f) ? (nextTileBoundaryY - oy) / dy : std::numeric_limits<float>::infinity();

	float t = 0.f;

	while (t <= maxDistance) {
		// Bounds check
		if (tileX >= 0 && tileX < worldW && tileY >= 0 && tileY < worldH) {
			if (spatial.IsWorldSolid(tileX, tileY)) {
				// Compute hit position
				Vec2 hitPos = origin + direction * t;

				// Compute normal
				Vec2 normal{0.f, 0.f};
				if (tMaxX < tMaxY)
					normal.x = -stepX;
				else
					normal.y = -stepY;

				result.hit = true;
				result.distance = t;
				result.position = hitPos;
				result.normal = normal;
				result.tileX = tileX;
				result.tileY = tileY;
				result.tileValue = spatial.GetWorldTileValue(tileX, tileY); // expose this if needed

				return result;
			}
		} else {
			// Out of world bounds → no hit
			return result;
		}

		// Step to next tile
		if (tMaxX < tMaxY) {
			tileX += static_cast<int>(stepX);
			t = tMaxX;
			tMaxX += tDeltaX;
		} else {
			tileY += static_cast<int>(stepY);
			t = tMaxY;
			tMaxY += tDeltaY;
		}
	}

	return result;
}
/////////////////////////////////



/////////////////////////////////
RaycastResult RaycastEngine::RaycastUnified(const Vec2& origin, const Vec2& direction, float maxDistance) {
	RaycastResult dynHit = RaycastDynamic(origin, direction, maxDistance);
	RaycastResult tileHit = RaycastTiles(origin, direction, maxDistance);

	// If neither hit, return empty
	if (!dynHit.hit && !tileHit.hit)
		return RaycastResult{};

	// If only dynamic hit
	if (dynHit.hit && !tileHit.hit)
		return dynHit;

	// If only tile hit
	if (!dynHit.hit && tileHit.hit)
		return tileHit;

	// Both hit → choose nearest
	if (dynHit.distance < tileHit.distance)
		return dynHit;

	return tileHit;
}
/////////////////////////////////