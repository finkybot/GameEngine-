/////////////////////////////////
// ISpatialIndex Interface
/////////////////////////////////



/////////////////////////////////
// Include
#pragma once
#include <vector>
#include <memory>
#include "Vec2.h"
/////////////////////////////////



/////////////////////////////////
// Forward declarations for classes used in the ISpatialIndex interface. These declarations allow the interface to reference these classes without 
// needing to include their full definitions, which can help reduce compilation dependencies and improve build times.
class EntityManager;
class Entity;
struct RaycastHit;
class ChunkManager;
/////////////////////////////////



/////////////////////////////////
//	|	ISpatialIndex - Interface for spatial indexing and querying of entities in a 2D space. This interface defines the methods that any spatial index implementation must provide, including rebuilding the index, querying for entities within a radius, performing raycasts against 
//	|	entities and the world, and checking for solid tiles in the world.
//	|_______________________________________________________________________
class ISpatialIndex {
public:
	virtual ~ISpatialIndex() = default;

	// Topology + movement
	virtual void Build(const std::vector<Entity*>& dynamicEntities) = 0;
	virtual void Refit() = 0;
	virtual void UpdateEntity(Entity* e) = 0;

	// Entity lifecycle
	virtual void Insert(Entity* e) = 0;
	virtual void Remove(Entity* e) = 0;
	virtual void Reset() = 0;

	// Queries
	virtual void QueryEntities(std::vector<Entity*>& outFound, const Vec2& position, float radius,
							   const Entity* exclude) const = 0;

	virtual bool RaycastEntities(const Vec2& origin, const Vec2& dirN, float maxDist, RaycastHit& outHit,
								 Entity*& outEntity) const = 0;

	virtual RaycastHit RaycastWorld(const Vec2& origin, const Vec2& dir, float maxDist) const = 0;

	virtual bool IsWorldSolid(int tileX, int tileY) const = 0;

	// Engine wiring
	virtual void SetEntityManager(EntityManager* entityManager) = 0;

	virtual void MarkTopologyDirty() = 0;
	virtual bool IsTopologyDirty() const = 0;
	virtual void ClearTopologyDirty() = 0;
};
/////////////////////////////////