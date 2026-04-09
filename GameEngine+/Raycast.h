// Raycast.h - DDA raycasting for tilemaps and ray vs AABB utilities
#pragma once

#include "Vec2.h"
#include <vector>
#include <cfloat>
#include <cmath>
#include <limits>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <optional>
#include <string>
#include <iomanip>
#include "Utils.h"
// TileMap is referenced here via TileMap.h included below by caller when needed

// Debug helpers to optionally collect visited cells when performing raycasts.
namespace RaycastDebug {
    inline bool collectVisited = false;
    inline std::vector<std::pair<int,int>> lastVisited;
    inline void EnableVisited(bool enable) {
        // Force debug collection off. Ignore requests to enable it at runtime.
        (void)enable;
        collectVisited = false;
        lastVisited.clear();
    }
    inline const std::vector<std::pair<int,int>>& GetLastVisited() { return lastVisited; }
}

// TileMap is defined in TileMap.h - include it for use in raycast utilities
#include "TileMap.h"

// JSON save/load for TileMap moved to Utils.* to centralize parsing utilities.

// Raycast result
struct RaycastHit
{
	bool hit = false;       // did we hit a solid tile?
	int tileX = -1;         // tile coordinates of hit tile
	int tileY = -1;         // tile coordinates of hit tile
    Vec2 position{0,0};     // world space hit position
    Vec2 normal{0,0};       // surface normal
    float distance = 0.0f;  // distance along ray (world units)
    int tileValue = 0;      // value of tile hit
};

// Ray vs Axis Aligned Bounding Box (AABB) using slab method. rectMin and rectMax are world coordinates of AABB.
// dir must be normalized. Returns true and fills out outT if hit at positive t <= maxDistance.
inline bool RayIntersectsAABB(const Vec2& origin, const Vec2& dir, const Vec2& rectMin, const Vec2& rectMax, float& outDistance, float maxDistance = FLT_MAX)
{
    // Set up a few things, first define a small epsilon for handling floating-point precision when checking for parallel rays.Next initialize tmin and tmax 
    // to manage intersection intervals along the ray. I'll set tmin to 0.0 as I only care about positive distances along the ray and tmax to the passed max 
    // distance, so we are measuring a line segment and not a infinite ray. 
    // We will update these values as we check against the slabs of the AABB, and if at any point tmin exceeds tmax, we can conclude there is no intersection.
	const float epsilon = 1e-9f;                   
	float tmin = 0.0f;                                 
	float tmax = maxDistance;                       

	// Lets start with the x slab of the AABB. We check if the ray is parallel to the x axis by comparing the absolute value of dir.x to a small epsilon.
	// If the ray is parallel to the X axis and the origin's X coordinate is outside the AABB's X bounds, there is no intersection. If the ray is not parallel 
    // to the X axis,then calculate the intersection t values for the two vertical slabs of the AABB, swap them if necessary to ensure t1 is the entry point and
    // t2 is the exit point for the X slab, and then update tmin and tmax to account for the X slab intersection interval. If at any point tmin exceeds tmax, 
    // then there is no intersection...
    // Are we parallel to the x axis?, that is, is dir.x smaller than our epsilon threshold?
	if (std::fabs(dir.x) < epsilon) 
    {
		if (origin.x < rectMin.x || origin.x > rectMax.x) return false; 
    }
    // Not parallel to x axis? then calculate intersection t values for the two vertical slabs of AABB and swap if necessary
    // Note we guard against division by zero (ood) to remove possible infinte t values when computing t1 and t2; next swap
	// t1 and t2 to ensure the entry and exit points are correct for the x slab; Finally we update tmin and tmax to account 
    // for the intersection interval of the x slab, and if at any point tmin exceeds tmax, there is no intersection so return
    // false...
	else 
    {
		float ood = 1.0f / dir.x;                   
		float t1 = (rectMin.x - origin.x) * ood;    
		float t2 = (rectMax.x - origin.x) * ood;    
		if (t1 > t2) std::swap(t1, t2);             
		tmin = std::max(tmin, t1);                  
		tmax = std::min(tmax, t2);                  
		if (tmin > tmax) return false;              
    }

    // Next is the y slab of the AABB. We check if the ray is parallel to the y axis by comparing the absolute value of dir.y to a small epsilon.
    // If the ray is parallel to the Y axis and the origin's Y coordinate is outside the AABB's Y bounds, there is no intersection. If the ray is not parallel 
    // to the Y axis,then calculate the intersection t values for the two horizontal slabs of the AABB, swap them if necessary to ensure t1 is the entry point and
    // t2 is the exit point for the Y slab, and then update tmin and tmax to account for the Y slab intersection interval. If at any point tmin exceeds tmax, 
    // then there is no intersection...
    // Are we parallel to the y axis?, that is, is dir.y smaller than our epsilon threshold?
    if (std::fabs(dir.y) < epsilon)                
    {
		if (origin.y < rectMin.y || origin.y > rectMax.y) return false;
    }
    // Not parallel to y axis? then calculate intersection t values for the two horizontal slabs of AABB and swap if necessary.
    // Like above, we guard against division by zero (ood) to remove possible infinte t values when computing t1 and t2; next swap
    // t1 and t2 to ensure the entry and exit points are correct for the y slab; Finally we update tmin and tmax to account 
    // for the intersection interval of the y slab, and if at any point tmin exceeds tmax, there is no intersection so return
    // false...
	else 
    {
		float ood = 1.0f / dir.y;                   
		float t1 = (rectMin.y - origin.y) * ood;    
		float t2 = (rectMax.y - origin.y) * ood;    
		if (t1 > t2) std::swap(t1, t2);             
		tmin = std::max(tmin, t1);                  
		tmax = std::min(tmax, t2);                  
		if (tmin > tmax) return false;              
    }

	// At this point, we have the intersection interval [tmin, tmax] along the ray where it intersects the AABB. We want to report the first 
    // hit at positive t within maxDistance. If tmin is negative, it means the ray starts inside the AABB, so we check if tmax is positive to 
	// report the exit hit; if both tmin and tmax are negative, it means the AABB is behind the ray origin and there is no valid hit, 
    // so we return false...
    // 
	if (tmin < 0.0f)
    {
		if (tmax < 0.0f) return false; 
		
        outDistance = tmax;
    }
    // Otherwise, tmin is positive and we can report the entry hit at tmin...
	else 
    {
		outDistance = tmin; 
    }

	// Final check to ensure the reported outDistance hit is within the maxDistance limit. If it is not then return false for no valid hit.
	// Note this manages the case where the ray starts inside the AABB and we report the exit hit at tmax,thus ensuring that we don't report 
    // hits beyond maxDistance in that case as well...
    if (outDistance > maxDistance) return false;
    return true;
}

// Helper functions
// Normalize a direction vector and return its length. Returns false if the vector is too small to normalize.
static inline bool NormalizeDir(const Vec2& d, double& outDx, double& outDy, double& outLen)
{
    const double EPS_SMALL = 1e-12;
    outDx = d.x; outDy = d.y; outLen = std::sqrt(outDx * outDx + outDy * outDy);
    if (outLen < EPS_SMALL) return false;
    outDx /= outLen; outDy /= outLen;
    return true;
}

// Calculate the initial t value for the ray to reach the next vertical or horizontal grid line based on the ray direction and origin.
static inline double CalcInitialT(double dd, int mapC, double originC, double tileSize)
{
    const double EPS_SMALL = 1e-12;
    if (std::fabs(dd) < EPS_SMALL) return std::numeric_limits<double>::infinity();
    if (dd > 0.0) return (((mapC + 1) * tileSize) - originC) / dd;
    return (originC - (mapC * tileSize)) / -dd;
}

// Calculate the t delta for stepping from one grid line to the next in the DDA algorithm, which is tileSize divided by the absolute value of the ray direction component.
static inline double CalcTDelta(double dd, double tileSize)
{
    const double EPS_SMALL = 1e-12;
    return (std::fabs(dd) < EPS_SMALL) ? std::numeric_limits<double>::infinity() : (tileSize / std::fabs(dd));
}

// Check if a ray intersects the AABB of a tile at (hx, hy) in the tilemap. Returns true and fills outHitDist if hit within maxDist.
static inline bool IntersectAABB_Ray(const Vec2& origin, double ndx, double ndy, const TileMap& map, int hx, int hy, float maxDist, float& outHitDist)
{
    return RayIntersectsAABB(origin, Vec2(static_cast<float>(ndx), static_cast<float>(ndy)), Vec2(hx * map.tileSize, hy * map.tileSize), Vec2((hx + 1) * map.tileSize, (hy + 1) * map.tileSize), outHitDist, maxDist);
}

// DDA raycasting for tilemaps. Returns RaycastHit with hit info if we hit a solid tile, or default RaycastHit with hit=false if no hit.
// The ray is defined by an origin and a direction vector. The direction vector should ideally be normalized for consistent results, 
// but the function can handle non-normalized directions as well.
inline RaycastHit RaycastTilemapDDA(const Vec2& origin, const Vec2& dir, const TileMap& map, float maxDistance, bool ignoreStartingCell = false, std::vector<std::pair<int,int>>* outVisited = nullptr)
{
    // Use small helper functions declared above instead of local lambdas.
    RaycastHit result;
    if (map.width <= 0 || map.height <= 0) return result;

	// Normalize the direction vector and get its length. If the direction is too small to normalize, return no hit.
    double dxN, dyN, len;
    if (!NormalizeDir(dir, dxN, dyN, len)) return result;

	// Calculate the starting cell coordinates in the tilemap based on the ray origin. This is done by dividing the world coordinates of the origin by the tile size and flooring the result to get integer cell indices.
    int mapX = static_cast<int>(std::floor(origin.x / map.tileSize));
    int mapY = static_cast<int>(std::floor(origin.y / map.tileSize));

	// If the caller provided an outVisited vector, use it to store visited cells. If not, but debug collection is enabled, use the debug vector to store visited cells.
    std::vector<std::pair<int,int>>* usedVisited = outVisited;
    if (!usedVisited && RaycastDebug::collectVisited) { RaycastDebug::lastVisited.clear(); usedVisited = &RaycastDebug::lastVisited; }
    if (usedVisited) usedVisited->push_back({mapX, mapY});

	// Check the starting cell for a hit if we are not ignoring it. This allows for rays that start inside a solid tile to report an immediate hit.
    if (map.InBounds(mapX, mapY) && map.IsSolid(mapX, mapY) && !ignoreStartingCell)
    {
        result.hit = true;
        result.tileX = mapX; result.tileY = mapY; result.tileValue = map.GetTile(mapX, mapY);
        result.position = origin; result.distance = 0.0f;
        return result;
    }

	// Calculate the initial t values for the ray to reach the next vertical and horizontal grid lines, as well as the t deltas for stepping through the grid.
    double tMaxX = CalcInitialT(dxN, mapX, origin.x, map.tileSize);
    double tMaxY = CalcInitialT(dyN, mapY, origin.y, map.tileSize);
    double tDeltaX = CalcTDelta(dxN, map.tileSize);
    double tDeltaY = CalcTDelta(dyN, map.tileSize);
    double maxDistanceT = static_cast<double>(maxDistance);

	// Main DDA loop: at each step, we compare tMaxX and tMaxY to determine whether we step to the next vertical or horizontal grid line. We also check for corner cases where tMaxX and tMaxY 
    // are very close, which can happen when the ray passes near a grid corner. In that case, we need to check all three potentially hit tiles (the one we step into and the two adjacent ones) 
    // to ensure we don't miss any hits due to floating-point precision issues.
    const double CORNER_EPS = 1e-12;
    while (true)
    {
		// Handle corner case where tMaxX and tMaxY are very close, which can happen when the ray passes near a grid corner. In that case, we need to check all three potentially hit tiles 
        // (the one we step into and the two adjacent ones) to ensure we don't miss any hits due to floating-point precision issues.
        if (std::fabs(tMaxX - tMaxY) < CORNER_EPS)
        {
            double cornerT = tMaxX;

			// If the corner t value exceeds maxDistance, we can break out of the loop early since we won't find any valid hits beyond that point.
            if (cornerT > maxDistanceT) break;
            int prevX = mapX, prevY = mapY;
            mapX += (dxN > 0.0) ? 1 : -1;
            mapY += (dyN > 0.0) ? 1 : -1;
            tMaxX += tDeltaX; tMaxY += tDeltaY;

			// For debugging, we can optionally collect the visited cells. In this corner case, we are effectively stepping into three cells at once (the new cell at (mapX, mapY) 
            // and the two adjacent cells at (prevX, mapY) and (mapX, prevY)), so we add all three to the visited list if it's enabled.
            if (outVisited) { outVisited->push_back({mapX, prevY}); outVisited->push_back({prevX, mapY}); outVisited->push_back({mapX, mapY}); }
            
			// Check the new cell at (mapX, mapY) for a hit. If it's solid, we perform a ray vs AABB intersection test to find the exact hit position and distance. 
            // If the intersection test fails due to precision issues,
            if (!map.InBounds(mapX, mapY)) break;

			// Check the new cell at (mapX, mapY) for a hit. If it's solid, we perform a ray vs AABB intersection test to find the exact hit position and distance.
            if (map.IsSolid(mapX, mapY))
            {
                float outHitDistance = 0.0f;

				// In this corner case, we want to ensure we report the correct hit position and distance even if the ray passes very close to the corner of the tile.
                if (IntersectAABB_Ray(origin, dxN, dyN, map, mapX, mapY, maxDistance, outHitDistance))
                {
                    result.distance = outHitDistance;
                    result.position = Vec2(origin.x + dxN * outHitDistance, origin.y + dyN * outHitDistance);
                }
				// If the intersection test fails due to precision issues, we can still report a hit at the corner t value, which is the best estimate we have for the hit distance in this case.
                else
                {
                    result.distance = static_cast<float>(cornerT);
                    result.position = Vec2(origin.x + static_cast<float>(dxN * cornerT), origin.y + static_cast<float>(dyN * cornerT));
                }
                result.hit = true; result.tileX = mapX; result.tileY = mapY; result.tileValue = map.GetTile(mapX, mapY);
                return result;
            }

			// Next we need to check the two adjacent cells at (prevX, mapY) and (mapX, prevY) for hits, since in this corner case we are effectively stepping into all three cells at once.
            if (map.IsSolid(mapX, prevY))
            {
                int hx = mapX, hy = prevY; float outHitDistance = 0.0f;

				// Perform a ray vs AABB intersection test for the adjacent cell at (mapX, prevY). If it hits, we report the hit information. 
                // Note that the normal will be different for this cell compared to the main cell at (mapX, mapY),
                if (IntersectAABB_Ray(origin, dxN, dyN, map, hx, hy, maxDistance, outHitDistance))
                {
                    result.hit = true; result.tileX = hx; result.tileY = hy; result.tileValue = map.GetTile(hx, hy);
                    result.distance = outHitDistance; result.position = Vec2(origin.x + dxN * outHitDistance, origin.y + dyN * outHitDistance);
                    result.normal = Vec2((dxN > 0) ? -1.0f : 1.0f, 0.0f);
                    return result;
                }
            }

			// Perform a ray vs AABB intersection test for the adjacent cell at (prevX, mapY). If it hits, we report the hit information.
            if (map.IsSolid(prevX, mapY))
            {
                int hx = prevX, hy = mapY; float outHitDistance = 0.0f;

				// Perform a ray vs AABB intersection test for the adjacent cell at (prevX, mapY). If it hits, we report the hit information.
                if (IntersectAABB_Ray(origin, dxN, dyN, map, hx, hy, maxDistance, outHitDistance))
                {
                    result.hit = true; result.tileX = hx; result.tileY = hy; result.tileValue = map.GetTile(hx, hy);
                    result.distance = outHitDistance; result.position = Vec2(origin.x + dxN * outHitDistance, origin.y + dyN * outHitDistance);
                    result.normal = Vec2(0.0f, (dyN > 0) ? -1.0f : 1.0f);
                    return result;
                }
            }
            continue;
        }

		// Determine whether to step to the next vertical or horizontal grid line based on which tMax is smaller. 
        // This is the core of the DDA algorithm, where we are effectively "walking" along the ray through the grid.
        bool stepXNext = (tMaxX < tMaxY);
        double t = stepXNext ? tMaxX : tMaxY;
        
		// If the next t value exceeds maxDistance, we can break out of the loop early since we won't find any valid hits beyond that point.
        if (t > maxDistanceT) break;

		// Step to the next cell in the grid based on the ray direction. If we are stepping to the next vertical grid line, we update mapX and tMaxX; 
        // if we are stepping to the next horizontal grid line, we update mapY and tMaxY.
        if (stepXNext)
        {
            mapX += (dxN > 0.0) ? 1 : -1;
            tMaxX += tDeltaX;
        }

		// Step to the next cell in the grid based on the ray direction. If we are stepping to the next vertical grid line, we update mapX and tMaxX;
        else
        {
            mapY += (dyN > 0.0) ? 1 : -1;
            tMaxY += tDeltaY;
        }

		// For debugging, we can optionally collect the visited cells. We add the new cell at (mapX, mapY) to the visited list if it's enabled.
        if (outVisited) outVisited->push_back({ mapX, mapY });
        
		// Check if the new cell at (mapX, mapY) is within the bounds of the tilemap. If it's out of bounds, we can break out of the loop since we won't find any valid hits beyond that point.
        if (!map.InBounds(mapX, mapY)) break;

		// Check if the new cell at (mapX, mapY) is solid. If it is, we perform a ray vs AABB intersection test to find the exact hit position and distance.
        if (map.IsSolid(mapX, mapY))
        {
            float outHitDistance = 0.0f;

			// Perform a ray vs AABB intersection test for the cell at (mapX, mapY). If it hits, we report the hit information.
            if (IntersectAABB_Ray(origin, dxN, dyN, map, mapX, mapY, maxDistance, outHitDistance))
            {
                result.distance = outHitDistance;
                result.position = Vec2(origin.x + dxN * outHitDistance, origin.y + dyN * outHitDistance);

                Vec2 hitP = result.position;
                const double EPS_FACE = 1e-3;

                result.hit = true; result.tileX = mapX; result.tileY = mapY; result.tileValue = map.GetTile(mapX, mapY);

				// Determine the normal based on which face of the tile was hit. We check if the hit position is within a small epsilon distance from any of the tile's edges, and if so, we set the normal accordingly.
                if (std::fabs(hitP.x - mapX * map.tileSize) < EPS_FACE) result.normal = Vec2(-1.0f, 0.0f);
                else if (std::fabs(hitP.x - (mapX + 1) * map.tileSize) < EPS_FACE) result.normal = Vec2(1.0f, 0.0f);
                else if (std::fabs(hitP.y - mapY * map.tileSize) < EPS_FACE) result.normal = Vec2(0.0f, -1.0f);
                else if (std::fabs(hitP.y - (mapY + 1) * map.tileSize) < EPS_FACE) result.normal = Vec2(0.0f, 1.0f);
                else result.normal = (stepXNext ? Vec2((dxN > 0) ? -1.0f : 1.0f, 0.0f) : Vec2(0.0f, (dyN > 0) ? -1.0f : 1.0f));

                return result;
            }

			// If the intersection test fails due to precision issues, we can still report a hit at the current t value, which is the best estimate we have for the hit distance in this case.
            else
            {
                result.hit = true; result.tileX = mapX; result.tileY = mapY; result.tileValue = map.GetTile(mapX, mapY);
                result.distance = static_cast<float>(t); result.position = Vec2(origin.x + static_cast<float>(dxN * t), origin.y + static_cast<float>(dyN * t));
                result.normal = (stepXNext ? Vec2((dxN > 0) ? -1.0f : 1.0f, 0.0f) : Vec2(0.0f, (dyN > 0) ? -1.0f : 1.0f));
                return result;
            }
        }
    }

    const double endWorldX = origin.x + dxN * maxDistance;
    const double endWorldY = origin.y + dyN * maxDistance;
    const int endMapX = static_cast<int>(std::floor(endWorldX / map.tileSize));
    const int endMapY = static_cast<int>(std::floor(endWorldY / map.tileSize));

	// After the DDA loop, we can optionally check the end cell at the maximum distance to see if it is solid and report a hit there if it is. 
    // This can help ensure we report hits at the max distance limit even if the ray passes very close to a tile boundary at that point.
    if (map.InBounds(endMapX, endMapY) && map.IsSolid(endMapX, endMapY))
    {
        float outHitDistance = 0.0f;

		// Perform a ray vs AABB intersection test for the cell at (endMapX, endMapY). If it hits, we report the hit information.
        if (IntersectAABB_Ray(origin, dxN, dyN, map, endMapX, endMapY, maxDistance, outHitDistance))
        {
            result.hit = true; result.tileX = endMapX; result.tileY = endMapY; result.tileValue = map.GetTile(endMapX, endMapY);
            result.distance = outHitDistance; result.position = Vec2(origin.x + static_cast<float>(dxN * outHitDistance), origin.y + static_cast<float>(dyN * outHitDistance));
            return result;
        }

        result.hit = true; result.tileX = endMapX; result.tileY = endMapY; result.tileValue = map.GetTile(endMapX, endMapY);
        result.distance = maxDistance; result.position = Vec2(static_cast<float>(endWorldX), static_cast<float>(endWorldY));
        return result;
    }
    
	return result;
}
