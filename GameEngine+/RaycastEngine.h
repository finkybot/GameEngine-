/////////////////////////////////
// RaycastEngine.h : Provides static methods for performing raycasting against dynamic entities and tile grids in a 2D game engine. It supports raycasting against dynamic entities using a dynamic BVH, raycasting against tile grids using DDA or tile BVH, and unified raycasting that combines both approaches to find 
// the nearest hit of either type. 
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Vec2.h"
#include "RayCast.h"			 // for RaycastResult + CRaycast
#include "SpatialIndexUnified.h" // for dynamic BVH access
#include "ChunkManager.h"		 // for tile grid access
#include "CRayCast.h"
/////////////////////////////////



/////////////////////////////////
//	|	RaycastEngine class - Provides static methods for performing raycasting against dynamic entities and tile grids in a 2D game engine. It supports raycasting against dynamic entities using a dynamic BVH, raycasting against tile grids using DDA or tile BVH, and unified raycasting that combines both 
//	|	approaches to find the nearest hit of either type.
//	|_______________________________________________________________________
class RaycastEngine {
public:
	//
	// Perform a raycast against dynamic entities using the dynamic BVH.
	// Returns the nearest dynamic entity hit (if any).
	//
	static RaycastResult RaycastDynamic(const Vec2& origin, const Vec2& direction, float maxDistance);

	//
	// Perform a raycast against the tile grid using DDA or tile BVH.
	// Returns the nearest tile hit (if any).
	//
	static RaycastResult RaycastTiles(const Vec2& origin, const Vec2& direction, float maxDistance);

	//
	// Unified raycast: dynamic BVH + tile DDA.
	// Returns the nearest hit of either type.
	//
	static RaycastResult RaycastUnified(const Vec2& origin, const Vec2& direction, float maxDistance);
};
