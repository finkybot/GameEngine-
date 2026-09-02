/////////////////////////////////
// ISpatialIndex Interface
/////////////////////////////////



/////////////////////////////////
// Include
#pragma once
#include <vector>
#include "Vec2.h"
/////////////////////////////////



/////////////////////////////////
// Forward declarations for classes used in the ISpatialIndex interface. These declarations allow the interface to reference these classes without 
// needing to include their full definitions, which can help reduce compilation dependencies and improve build times.
class Entity;
struct RaycastHit;
class ChunkManager;
/////////////////////////////////



/////////////////////////////////
// ISpatialIndex - Interface for spatial indexing and querying of entities in a 2D space. This interface defines the methods that any spatial index implementation must provide,
// including rebuilding the index, querying for entities within a radius, performing raycasts against entities and the world, and checking for solid tiles in the world.
//								|
//								|_______________________________________________________________________
class ISpatialIndex {
	/////////////////////////////////
	// Public methods for the ISpatialIndex interface. These methods must be implemented by any class that inherits from this interface, providing the necessary functionality for spatial indexing and querying of entities in a 2D space.
public:
	/////////////////////////////////
	// Virtual destructor to ensure proper cleanup of derived classes. This allows for safe deletion of objects through a pointer to the ISpatialIndex interface, ensuring that the destructor of the derived class is called.
	virtual ~ISpatialIndex() = default;
	/////////////////////////////////



	/////////////////////////////////
	// Rebuild - Rebuilds the spatial index based on the provided list of entities and the chunk manager. This method is responsible for updating the spatial index to reflect any changes in the positions or states of entities, ensuring that subsequent queries return accurate results.
	virtual void Rebuild(const std::vector<std::unique_ptr<Entity>>& entities, ChunkManager* chunks) = 0;
	/////////////////////////////////



	/////////////////////////////////
	// QueryEntities - Queries the spatial index for entities within a specified radius of a given position, excluding a specific entity if provided. The results are stored in the outFound vector, allowing for efficient retrieval of nearby entities for collision detection or other spatial queries.
	virtual void QueryEntities(std::vector<Entity*>& outFound, const Vec2& position, float radius, const Entity* exclude) const = 0;
	/////////////////////////////////



	/////////////////////////////////
	// RaycastEntities - Performs a raycast against entities in the spatial index, starting from a given origin and extending in a specified direction for a maximum distance. If an entity is hit, the method returns true and populates the outHit and outEntity parameters 
	// with information about the hit, allowing for efficient detection of line-of-sight or projectile collisions with entities.
	virtual bool RaycastEntities(const Vec2& origin, const Vec2& dirN, float maxDist, RaycastHit& outHit, Entity*& outEntity) const = 0;
	/////////////////////////////////



	/////////////////////////////////
	// RaycastWorld - Performs a raycast against the world (e.g., tiles or terrain) starting from a given origin and extending in a specified direction for a maximum distance. The method returns a RaycastHit structure containing information about the 
	// hit, allowing for efficient detection of line-of-sight or projectile collisions with the world.
	virtual RaycastHit RaycastWorld(const Vec2& origin, const Vec2& dir, float maxDist) const = 0;
	/////////////////////////////////



	/////////////////////////////////
	// IsWorldSolid - Checks if a specific tile in the world is solid (i.e., impassable or collidable) based on its tile coordinates. This method allows for efficient collision detection and pathfinding by determining whether an entity can move through a given tile.
	virtual bool IsWorldSolid(int tileX, int tileY) const = 0;
	/////////////////////////////////
};
/////////////////////////////////