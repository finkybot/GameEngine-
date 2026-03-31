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

// Simple tile map structure used by the raycaster
struct TileMap
{
    int width = 0;
    int height = 0;
    float tileSize = 32.0f;
    std::vector<int> tiles; // 0 = empty, non-zero = solid

    TileMap() = default;
    TileMap(int w, int h, float sz = 32.0f) : width(w), height(h), tileSize(sz), tiles(w*h, 0) {}

    inline bool InBounds(int x, int y) const { return x >= 0 && y >= 0 && x < width && y < height; }
    inline int GetTile(int x, int y) const { return InBounds(x,y) ? tiles[y*width + x] : 0; }
    inline void SetTile(int x, int y, int v) { if (InBounds(x,y)) tiles[y*width + x] = v; }
    inline bool IsSolid(int x, int y) const { return GetTile(x,y) != 0; }
};

// JSON save/load for TileMap moved to Utils.* to centralize parsing utilities.

// Raycast result
struct RaycastHit
{
    bool hit = false;
    int tileX = -1;
    int tileY = -1;
    Vec2 position{0,0}; // world space hit position
    Vec2 normal{0,0}; // surface normal
    float distance = 0.0f; // distance along ray (world units)
    int tileValue = 0; // value of tile hit
};

// Ray vs AABB using slab method. rectMin and rectMax are world coordinates of AABB.
// dir must be normalized. Returns true and fills out outT if hit at positive t <= maxDistance.
inline bool RayIntersectsAABB(const Vec2& origin, const Vec2& dir, const Vec2& rectMin, const Vec2& rectMax, float& outT, float maxDistance = FLT_MAX)
{
	const float epsilon = 1e-9f;                    // Small epsilon to handle floating-point precision issues when checking for parallel rays
	float tmin = 0.0f;                              // Initialize tmin to 0 to report hits at the ray origin (t=0) and beyond, but not behind the ray. This allows us to handle rays that start inside the AABB correctly.    
	float tmax = maxDistance;                       // Initialize tmax to the provided maxDistance to limit hits to that range. This also allows us to handle rays that start inside the AABB and exit it, as we will report the exit hit if it is within maxDistance.

	// Dealing with the X slab ***********
	if (std::fabs(dir.x) < epsilon) // Ray is parallel to X axis
    {
		if (origin.x < rectMin.x || origin.x > rectMax.x) return false; // If the ray is parallel to the X axis and the origin's X coordinate is outside the AABB's X bounds, there is no intersection.
    }
	else // Ray is not parallel to X axis
    {
		float ood = 1.0f / dir.x;                   // Make sure we get no ooopies with division by zero by checking dir.x against a small epsilon first
		float t1 = (rectMin.x - origin.x) * ood;    // Compute intersection t values for the two vertical slabs of the AABB
		float t2 = (rectMax.x - origin.x) * ood;    // Swap t1 and t2 if necessary to ensure t1 is the entry point and t2 is the exit point for the X slab
		if (t1 > t2) std::swap(t1, t2);             // Update tmin and tmax to account for the X slab intersection interval
		tmin = std::max(tmin, t1);                  // If the intersection interval is empty, there is no hit
		tmax = std::min(tmax, t2);                  // If the ray misses the AABB or the hit is beyond maxDistance, return false
		if (tmin > tmax) return false;              // If the ray origin is inside the AABB, tmin will be negative. We want to report hits at positive t only, so if tmax is also negative, there is no hit.
    }

    // Dealing with the Y slab ***********
    if (std::fabs(dir.y) < epsilon)                 // Ray is parallel to Y axis
    {
		if (origin.y < rectMin.y || origin.y > rectMax.y) return false; // If the ray is parallel to the Y axis and the origin's Y coordinate is outside the AABB's Y bounds, there is no intersection.
    }
	else // Ray is not parallel to Y axis
    {
		float ood = 1.0f / dir.y;                   // Make sure we get no ooopies with division by zero by checking dir.y against a small epsilon first
		float t1 = (rectMin.y - origin.y) * ood;    // Compute intersection t values for the two horizontal slabs of the AABB
		float t2 = (rectMax.y - origin.y) * ood;    // Swap t1 and t2 if necessary to ensure t1 is the entry point and t2 is the exit point for the Y slab
		if (t1 > t2) std::swap(t1, t2);             // Update tmin and tmax to account for the Y slab intersection interval
		tmin = std::max(tmin, t1);                  // If the intersection interval is empty, there is no hit
		tmax = std::min(tmax, t2);                  // If the ray misses the AABB or the hit is beyond maxDistance, return false
		if (tmin > tmax) return false;              // If the ray origin is inside the AABB, tmin will be negative. We want to report hits at positive t only, so if tmax is also negative, there is no hit.
    }

	// At this point, we have the intersection interval [tmin, tmax] along the ray where it intersects the AABB. We want to report the first hit at positive t within maxDistance. 
    // If tmin is negative, it means the ray starts inside the AABB, so we check if tmax is positive to report the exit hit. 
    // If tmin is positive, we report that as the hit.
	if (tmin < 0.0f) // Ray starts inside the AABB, so we check if it exits at a positive t value
    {
		if (tmax < 0.0f) return false; // Ray starts inside the AABB but also exits behind the ray origin, so we consider that no hit.
		outT = tmax; // Report the exit hit if it is positive. This allows us to handle rays that start inside the AABB and exit it, as we will report the exit hit if it is within maxDistance.
    }
	else // Ray starts outside the AABB, so we report the entry hit at tmin if it is positive and within maxDistance
    {
		outT = tmin; // Report the entry hit if it is positive and within maxDistance. If tmin is negative, it means the ray starts inside the AABB, and we will report the exit hit at tmax instead.
    }

	// Final check to ensure the reported hit is within the maxDistance limit. This also handles the case where the ray starts inside the AABB and we report the exit hit at tmax, ensuring that it is also within maxDistance.
    if (outT > maxDistance) return false;
    return true;
}

// DDA raycast against a TileMap. origin and dir are world coordinates; dir should be normalized.
// maxDistance is world units. Optionally collects visited cells in outVisited.
inline RaycastHit RaycastTilemapDDA(const Vec2& origin, const Vec2& dir, const TileMap& map, float maxDistance, bool ignoreStartingCell = false, std::vector<std::pair<int,int>>* outVisited = nullptr)
{
	RaycastHit result;                                      // default is no hit
	if (map.width <= 0 || map.height <= 0) return result;   // invalid map, return no hit
	
    const double epsilon = 1e-12;               // small epsilon to handle floating-point precision issues when normalizing the direction and comparing t values
	double dx = dir.x;                          // Normalize direction to ensure consistent step sizes and distance calculations. If the direction is not normalized, the ray may skip cells or report incorrect distances due to varying step sizes in x and y.
	double dy = dir.y;                          // Normalize direction to ensure consistent step sizes and distance calculations. If the direction is not normalized, the ray may skip cells or report incorrect distances due to varying step sizes in x and y.
	double len = std::sqrt(dx * dx + dy * dy);  // Vector cross product magnitude for 2D vectors is just the determinant (dx*0 - dy*0), but we need the length to normalize. We check against a small epsilon to avoid division by zero and handle very short rays gracefully (see below).
    
	// Check for zero-length direction vector to avoid division by zero. If the length is very small, we can consider it as no movement and return no hit immediately. This also prevents issues with normalizing a zero-length vector.
    if (len < epsilon) return result;
    dx /= len; dy /= len;

	// Values are setup to step through the grid cell by cell in the direction of the ray, starting from the origin. 
    // Now we will compute how far we can go along the ray before we cross a cell boundary in x or y, 
    // and step to the next cell accordingly, checking for solid tiles until we hit one or exceed maxDistance.
    
	// Starting cell coordinates in the tilemap grid. I'll floor the value to get cell origin, which allows me to handle rays that start inside a cell correctly. 
    // The mapX and mapY will be used as our current cell coordinates as we traverse the grid.
    int mapX = static_cast<int>(std::floor(origin.x / map.tileSize));
    int mapY = static_cast<int>(std::floor(origin.y / map.tileSize));


	// Debug & Diagnostics ***********
	// If outVisited is provided, we use it for collecting visited cells
	std::vector<std::pair<int, int>>* usedVisited = outVisited; // Use the provided outVisited vector if given.
    
	// If outVisited is not provided but RaycastDebug::collectVisited is true then use it to collect visited cells for debugging purposes.
	if (!usedVisited && RaycastDebug::collectVisited) // If no outVisited was provided but debug collection is enabled, use the debug visited cells vector to collect visited cells.
    {
		RaycastDebug::lastVisited.clear();          // Clear previous debug visited cells before starting a new raycast.
		usedVisited = &RaycastDebug::lastVisited;   // Use the debug visited cells vector to collect visited cells for diagnostics/visualization.
    }

    if (usedVisited) usedVisited->push_back({mapX, mapY}); // if outVisited is provided, record starting cell.

    if (map.InBounds(mapX, mapY) && map.IsSolid(mapX, mapY) && !ignoreStartingCell) // If starting cell is solid and not ignored then report immediate hit at origin.
    {
        result.hit = true;
        result.tileX = mapX; result.tileY = mapY; result.tileValue = map.GetTile(mapX, mapY);
        result.position = origin; result.distance = 0.0f;
        return result;
    }

	// Compute initial tMaxX and tMaxY, which are the distances along the ray to the first vertical and horizontal grid lines respectively.
	double tMaxX, tMaxY;    // tMaxX is the distance along the ray to the first vertical grid line (in x) we will cross. If dx is zero, we will never cross a vertical line, so we set it to infinity. Otherwise, we calculate how far we can go along the ray before we hit the next vertical grid line in the direction of the ray.                                                            

	// Dealing with x axis first ***********
	if (std::fabs(dx) < epsilon) tMaxX = std::numeric_limits<double>::infinity();   // If the ray is almost parallel to the Y axis (dx is very small), we will never cross a vertical grid line, so we set tMaxX to infinity. This allows us to handle rays that are nearly vertical without running into floating-point precision issues.
	else if (dx > 0.0) tMaxX = (((mapX + 1) * map.tileSize) - origin.x) / dx;       // If the ray is pointing to the right (positive x direction), we calculate the distance to the next vertical grid line to the right of the current cell.
	else tMaxX = (origin.x - (mapX * map.tileSize)) / -dx;                          // If the ray is pointing to the left (negative x direction), we calculate the distance to the next vertical grid line to the left of the current cell.

	// Dealing with y axis next ***********
    if (std::fabs(dy) < epsilon) tMaxY = std::numeric_limits<double>::infinity();   // If the ray is almost parallel to the X axis (dy is very small), we will never cross a horizontal grid line, so we set tMaxY to infinity. This allows us to handle rays that are nearly horizontal without running into floating-point precision issues.
    else if (dy > 0.0) tMaxY = (((mapY + 1) * map.tileSize) - origin.y) / dy;       // If the ray is pointing downwards (positive y direction), we calculate the distance to the next horizontal grid line below the current cell.
    else tMaxY = (origin.y - (mapY * map.tileSize)) / -dy;                          // If the ray is pointing upwards (negative y direction), we calculate the distance to the next horizontal grid line above the current cell.
    
	// Check our x and y values are not below zero, if they are, set them to infinity so we don't have issues with negative tMax values which would cause us to incorrectly report hits behind the ray origin.
    double tDeltaX = (std::fabs(dx) < epsilon) ? std::numeric_limits<double>::infinity() : (map.tileSize / std::fabs(dx));
    double tDeltaY = (std::fabs(dy) < epsilon) ? std::numeric_limits<double>::infinity() : (map.tileSize / std::fabs(dy));

	double maxT = static_cast<double>(maxDistance); // Convert maxDistance to a double for precise comparisons with tMaxX and tMaxY.

	// Traveral Loop ***********
	// We will step through the grid cell by cell in the direction of the ray, checking for solid tiles until we hit one or exceed maxDistance.
    while (true)
    {
        // Corner Crossings ***********
        // Handle corner crossing where both tMaxX and tMaxY are equal (within epsilon)
        if (std::fabs(tMaxX - tMaxY) < epsilon)
        {
            double t = tMaxX;
			if (t > maxT) break;                    // If the next step would exceed maxDistance, we stop the traversal and return no hit.
			int prevX = mapX; int prevY = mapY;     // We are crossing a corner, so we step in both x and y directions.
			mapX += (dx > 0.0) ? 1 : -1;            // Step in x direction based on the sign of dx. If dx is positive, we move to the next cell to the right; if dx is negative, we move to the next cell to the left.
            mapY += (dy > 0.0) ? 1 : -1;            // Step in y direction based on the sign of dy. If dy is positive, we move to the next cell upwards; if dy is negative, we move to the next cell downwards.
			tMaxX += tDeltaX; tMaxY += tDeltaY;     // After stepping in both directions, we update tMaxX and tMaxY to the next vertical and horizontal grid lines respectively.
            
            if (outVisited) { outVisited->push_back({mapX, prevY}); outVisited->push_back({prevX, mapY}); outVisited->push_back({mapX, mapY}); } // Debug: record visited cells if outVisited was provided.
			if (!map.InBounds(mapX, mapY)) break;   // Out of bounds? Stop the traversal and return no hit.

			// For corner crossings We need to check all three cells (the new cell and the two adjacent cells) for a hit, 
            // and report the closest hit if there are multiple. This is because when crossing a corner, we could  hit any of the three cells first depending on the exact ray direction.
            // Start with the new cell we stepped into is solid, if there is a hit, lets find the exact hit position if possible.
            if (map.IsSolid(mapX, mapY))
            {
				float outTf = 0.0f; // Set up a variable to receive the hit distance from the ray-AABB intersection test, used in determining the exact hit position and distance along the ray.
				if (RayIntersectsAABB(origin, Vec2(static_cast<float>(dx), static_cast<float>(dy)), Vec2(mapX * map.tileSize, mapY * map.tileSize), Vec2((mapX + 1) * map.tileSize, (mapY + 1) * map.tileSize), outTf, maxDistance)) // Perform a ray-AABB intersection test to determine the exact hit position and distance along the ray for the cell we just stepped into.
                {
                    result.distance = outTf;                                                // Calculate the exact hit distance along the ray.  
					result.position = Vec2(origin.x + dx * outTf, origin.y + dy * outTf);   // Calculate the exact hit position in world coordinates using the hit distance along the ray.
                }
				else // if ray does not hit the cell's AABB, then fallback and report a hit at the cell boundary based on the t value of the corner crossing (remember we are crossing a corner of a solid cell, so we know we are hitting the cell boundary at least, even if we miss the AABB due to numerical issues, so we report a hit at the corner crossing position using the t value of the corner crossing).
                { 
                    result.distance = static_cast<float>(t); 
                    result.position = Vec2(origin.x + static_cast<float>(dx*t), origin.y + static_cast<float>(dy*t)); 
                }

                result.hit = true;                          // Record a hit
                result.tileX = mapX; result.tileY = mapY;   // Record the tile coordinates of the hit cell                                                   
                result.tileValue = map.GetTile(mapX, mapY); // Record the tile coordinates and value of the hit cell
				return result;                              // Return the hit result immediately since we know we are hitting the corner cell, which is the closest hit when crossing a corner, so we don't need to check the adjacent cells for hits in this case.
            }

			// Still going? Lets check the adjacent cells for hits as well, since we are crossing a corner and could hit either of the adjacent cells first depending on the exact ray direction. 
            // We check the adjacent cells in the order of their tMax values to ensure we report the closest hit if there are multiple.
            
            // Check x axis adjacent cell for hit
            if (map.IsSolid(mapX, prevY)) 
            {
				int hx = mapX, hy = prevY; float outTf = 0.0f; // Set up a variable to receive the hit distance from the ray-AABB intersection test, used in determining the exact hit position and distance along the ray.
                
				// Perform a ray-AABB intersection test to determine the exact hit position and distance along the ray for the x axis adjacent cell.
                if (RayIntersectsAABB(origin, Vec2(static_cast<float>(dx), static_cast<float>(dy)), Vec2(hx*map.tileSize, hy*map.tileSize), Vec2((hx+1)*map.tileSize,(hy+1)*map.tileSize), outTf, maxDistance))
                {
					// Record the hit result for the x axis adjacent cell.
                    result.hit=true; 
                    result.tileX=hx; result.tileY=hy; 
                    result.tileValue=map.GetTile(hx,hy); 
                    result.distance=outTf; 
                    result.position=Vec2(origin.x+dx*outTf, origin.y+dy*outTf); 
                    result.normal=Vec2((dx>0)?-1.0f:1.0f,0.0f);
					return result;  // Return the hit result immediately since we know this is the closest hit on the x axis when crossing the corner, so we don't need to check the y axis adjacent cell for a hit in this case.
                }
            }

			// Finally if we are still going we should check y axis adjacent cell for hit
            if (map.IsSolid(prevX, mapY))
            {
                int hx = prevX, hy = mapY; float outTf=0.0f; // Set up required variables for AABB intersection.
                
				// Perform a ray-AABB intersection test to determine the exact hit position and distance along the ray for the y axis adjacent cell.
                if (RayIntersectsAABB(origin, Vec2(static_cast<float>(dx), static_cast<float>(dy)), Vec2(hx*map.tileSize, hy*map.tileSize), Vec2((hx+1)*map.tileSize,(hy+1)*map.tileSize), outTf, maxDistance))
                {
					// Record the hit result for the y axis adjacent cell.
                    result.hit=true; 
                    result.tileX=hx; result.tileY=hy; 
                    result.tileValue=map.GetTile(hx,hy); 
                    result.distance=outTf; 
                    result.position=Vec2(origin.x+dx*outTf, origin.y+dy*outTf); 
                    result.normal=Vec2(0.0f,(dy>0)?-1.0f:1.0f);
					return result;  // Return the hit result immediately since we know this is the closest hit on the y axis when crossing the corner, so we don't need to check any further for hits in this case.
                }
            }

			continue; // If we are still going after checking the corner cell and both adjacent cells, we continue to the next iteration of the loop to step to the next cell.
        }


		// Regular Step ***********
        bool stepXNext = (tMaxX < tMaxY);       // Check if next step is in the x or y direction.
		double t = stepXNext ? tMaxX : tMaxY;   // Get the t value of the next step. This is the distance along the ray to the next vertical or horizontal grid line we will cross.
        
		// If the next step would exceed maxDistance, we stop the traversal and return no hit. We check this before stepping to the next cell to ensure we don't report hits beyond maxDistance.
        if (t > maxT) break;

		// Step to the next cell in the grid based on whether we are crossing a vertical or horizontal grid line next. We update mapX or mapY accordingly, and also update tMaxX or tMaxY to the next grid line after stepping.
        if (stepXNext) 
        { 
			mapX += (dx > 0.0) ? 1 : -1;    // if dx is positive, move to next cell to the right; if negative then left.
			tMaxX += tDeltaX;               // update tMaxX to next vertical grid line after stepping in x direction. 
        }
		else // We are crossing a horizontal grid line next, so we step in the y direction and update tMaxY.
        { 
            mapY += (dy > 0.0) ? 1 : -1;    // Move down a cell if dy is positive, or up if dy is negative.
            tMaxY += tDeltaY;               // update tMaxY to next horizontal grid line after stepping in y direction.
        }

		if (outVisited) outVisited->push_back({ mapX, mapY });  // Debug: record visited cell if outVisited was provided.
        
		if (!map.InBounds(mapX, mapY)) break;                   // Out of bounds? Stop the traversal and return no hit.
       
		// If we stepped into a solid cell, we have a hit. As above we can try to get a more accurate hit position using the ray-AABB intersection test, 
        // but if that fails for some reason (numerical issues, etc), we can fallback to reporting a hit at the cell boundary based on the t value of the step.
        if (map.IsSolid(mapX, mapY))
        {
			float outTf = 0.0f; // Set up a variable to receive hit distance from the AABB intersection test. This will be used in determining the exact hit position and distance along the ray.

			// Perform a ray-AABB intersection test to determine the exact hit position and distance along the ray for the cell we just stepped into. We'll pass in outTf to receive the hit distance.
            if (RayIntersectsAABB(origin, Vec2(static_cast<float>(dx), static_cast<float>(dy)), Vec2(mapX*map.tileSize, mapY*map.tileSize), Vec2((mapX+1)*map.tileSize,(mapY+1)*map.tileSize), outTf, maxDistance))
            {
				result.distance = outTf; // Store the hit distance calculated from the ray-AABB intersection test
				result.position = Vec2(origin.x + dx * outTf, origin.y + dy * outTf); // Calculate the exact hit position in world coordinates using the hit distance along the ray from the AABB intersection test.

				Vec2 hitP = result.position; // For the normal, we can determine which face of the cell we hit based on the hit position relative to the cell boundaries
                const double epsilon = 1e-3; // Small value to handle numerical precision issues
               
				result.hit = true;                          // Record a hit
				result.tileX = mapX; result.tileY = mapY;   // Record the tile coordinates of the hit cell
				result.tileValue = map.GetTile(mapX, mapY); // Record the tile value of the hit cell

				if (std::fabs(hitP.x - mapX * map.tileSize) < epsilon) result.normal = Vec2(-1.0f, 0.0f);                       // If the hit position is very close to the left vertical boundary of the cell, we consider it a hit on the left face and set the normal accordingly.
				else if (std::fabs(hitP.x - (mapX + 1) * map.tileSize) < epsilon) result.normal = Vec2(1.0f, 0.0f);             // If the hit position is very close to the right vertical boundary of the cell, we consider it a hit on the right face and set the normal accordingly.
				else if (std::fabs(hitP.y - mapY * map.tileSize) < epsilon) result.normal = Vec2(0.0f, -1.0f);                  // If the hit position is very close to the top horizontal boundary of the cell, we consider it a hit on the top face and set the normal accordingly.
                else if (std::fabs(hitP.y - (mapY+1)*map.tileSize) < epsilon) result.normal = Vec2(0.0f,1.0f);                  // If the hit position is very close to the bottom horizontal boundary of the cell, we consider it a hit on the bottom face and set the normal accordingly.
				else result.normal = (stepXNext ? Vec2((dx > 0) ? -1.0f : 1.0f, 0.0f) : Vec2(0.0f, (dy > 0) ? -1.0f : 1.0f));   // otherwise fallback to using the step direction to determine the normal.
                
				return result;                              // Return the hit result immediately since we know we are hitting the cell we just stepped into, so we don't need to check any further for hits in this case.
            }
			else // if the ray does not hit the cell's AABB for some reason (numerical issues, etc), then we fallback and report a hit at the cell boundary based on the t value of the step (which is the distance along the ray to the grid line we just crossed).
            {
				result.hit = true;                          // Record a hit
				result.tileX = mapX; result.tileY = mapY;   // Record the tile coordinates of the hit cell
				result.tileValue = map.GetTile(mapX, mapY); // Record the tile value of the hit cell

				result.distance = static_cast<float>(t); result.position = Vec2(origin.x + static_cast<float>(dx * t), origin.y + static_cast<float>(dy * t)); // We can determine the normal based on whether we stepped in the x or y direction. If we stepped in the x direction, the normal is horizontal; if we stepped in the y direction, the normal is vertical. The sign of the normal is determined by the direction of the ray.
				result.normal = (stepXNext ? Vec2((dx > 0) ? -1.0f : 1.0f, 0.0f) : Vec2(0.0f, (dy > 0) ? -1.0f : 1.0f)); // If we stepped in the x direction, the normal is horizontal; if we stepped in the y direction, the normal is vertical. The sign of the normal is determined by the direction of the ray.
               
				return result;                              // Return the hit result immediately since we know we are hitting the cell we just stepped into, so we don't need to check any further for hits in this case.
            }
        }
    }

	// Endpoint Check *********** (We have not hit a solid cell during the traversal, but we may still hit a solid cell at the endpoint if maxDistance ends inside a solid cell. 
    // This can happen if the ray starts outside the map and points inward, or if maxDistance is shorter than the distance to the first solid cell along the ray. 
    // To handle this case, we perform a final check at the endpoint of the ray to see if it hits a solid cell, and if so, we report that as a hit. 
    // We also try to get a more accurate hit position using the ray-AABB intersection test for the endpoint cell if it is solid.)
    // endpoint inclusive check - use the normalized direction (dx,dy) so maxDistance is in world units
    
	// Calculate the world coordinates of the endpoint of the ray based on the origin, direction, and maxDistance. We use the normalized direction (dx, dy) to ensure that maxDistance is in world units.
    const double endWorldX = origin.x + dx * maxDistance;
    const double endWorldY = origin.y + dy * maxDistance;

	// Convert the endpoint world coordinates to map cell coordinates. We use floor to get the cell that contains the endpoint, which allows us to handle endpoints that are exactly on a cell boundary correctly.
    const int endMapX = static_cast<int>(std::floor(endWorldX / map.tileSize));
    const int endMapY = static_cast<int>(std::floor(endWorldY / map.tileSize));
        
	// Finally, we check if the endpoint cell is solid. If it is, we have a hit at the endpoint. 
    // We can try to get a more accurate hit position using the ray-AABB intersection test for the endpoint cell, 
    // but if that fails for some reason (numerical issues, etc), we can fallback to reporting a hit at the endpoint position based on maxDistance.
    if (map.InBounds(endMapX, endMapY) && map.IsSolid(endMapX, endMapY))
    {
		float outTf = 0.0f;                         // Variable to receive hit distance from the AABB intersection test for the endpoint cell.
        
		// Perform a ray-AABB intersection test to determine the exact hit position and distance along the ray for the endpoint cell. We use the normalized direction (dx, dy) so that maxDistance is in world units for the intersection test.
        if (RayIntersectsAABB(origin, Vec2(static_cast<float>(dx), static_cast<float>(dy)), Vec2(endMapX * map.tileSize, endMapY * map.tileSize), Vec2((endMapX + 1) * map.tileSize, (endMapY + 1) * map.tileSize), outTf, maxDistance))
        {
			// We have a hit at the endpoint cell, and we were able to get an accurate hit position using the ray-AABB intersection test. We record the hit result using the information from the intersection test.
            result.hit = true; 
            result.tileX = endMapX; result.tileY = endMapY; 
            result.tileValue = map.GetTile(endMapX, endMapY);

            result.distance = outTf; 
            result.position = Vec2(origin.x + static_cast<float>(dx * outTf), origin.y + static_cast<float>(dy * outTf));

			return result; // Return the hit result immediately since we know we are hitting the endpoint cell, so we don't need to check any further for hits in this case.
        }

		// We have a hit at the endpoint cell, but for some reason we were not able to get an accurate hit position using the ray-AABB intersection test (numerical issues, etc). 
        // Lets store the hit result using the endpoint position based on maxDistance.
        result.hit = true; 
        result.tileX = endMapX; result.tileY = endMapY; 
        result.tileValue = map.GetTile(endMapX, endMapY);
        result.distance = maxDistance; 
        result.position = Vec2(static_cast<float>(endWorldX), static_cast<float>(endWorldY));
        
		return result; // Return the hit result immediately since we know we are hitting the endpoint cell, so we don't need to check any further for hits in this case.
    }
    
	return result; // No hit found within maxDistance, return the default no hit result.
}
