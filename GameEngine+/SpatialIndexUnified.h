////////////////////////////////
// SpatialIndexUnified.h
////////////////////////////////


////////////////////////////////
// Includes
#pragma once
#include "ISpatialIndex.h"
#include "SpatialHashGrid.h"
#include "BVHSystem.h"
////////////////////////////////



////////////////////////////////
// Forward declarations
class EntityManager;
class Entity;
////////////////////////////////



////////////////////////////////
//	|	SpatialIndexUnified class - Implements a Scene-agnostic unified spatial index combining, dynamic spatial hash grid and BVH for efficent raycasting. Provides incremental update paths, Build(), Refit(), and UpdateEntity() to support dynamic entity movement and topology changes. Queries are supported for both 
//	|	entities and world tiles. This class is designed to be used in conjunction with an EntityManager, which manages the lifecycle of entities in the scene.
//	|_______________________________________________________________________
class SpatialIndexUnified : public ISpatialIndex {
public:
	SpatialIndexUnified(float dynamicCellSize = 100.0f) : m_dynamicGrid(dynamicCellSize) {}

	// Topology + movement
	void Build(const std::vector<Entity*>& dynamicEntities) override;
	void Refit() override;
	void UpdateEntity(Entity* e) override;

	// Entity lifecycle
	void Insert(Entity* e) override;
	void Remove(Entity* e) override;
	void Reset() override;

	// Queries
	void QueryEntities(std::vector<Entity*>& outFound, const Vec2& position, float radius, const Entity* exclude) const override;

	bool RaycastEntities(const Vec2& origin, const Vec2& dirN, float maxDist, RaycastHit& outHit, Entity*& outEntity) const override;

	RaycastHit RaycastWorld(const Vec2& origin, const Vec2& dir, float maxDist) const override;

	bool IsWorldSolid(int tileX, int tileY) const override;

	// Engine wiring
	void SetEntityManager(EntityManager* entityManager) override { m_entityManager = entityManager;	} // Set the EntityManager reference for this spatial index.

	void MarkTopologyDirty() { m_topologyDirty = true; } // Mark the topology as dirty.
	bool IsTopologyDirty() const { return m_topologyDirty; } // Check if the topology is dirty.
	void ClearTopologyDirty() { m_topologyDirty = false; }	 // Clear the topology dirty flag.

	BVHSystem& GetDynamicBVH() { return m_bvh; } // Get a reference to the BVH system for this spatial index.

	static SpatialIndexUnified& Get() {
		static SpatialIndexUnified instance;
		return instance;	
	}

	// World mask accessors
	int GetTileSize() const { return m_tileSize; }
	int GetWorldWidth() const { return m_worldWidth; }
	int GetWorldHeight() const { return m_worldHeight; }
	int GetWorldOffsetX() const { return m_worldOffsetX; }
	int GetWorldOffsetY() const { return m_worldOffsetY; }

	int GetWorldTileValue(int x, int y) const { return m_worldMask[y * m_worldWidth + x]; }



private:
	// Dynamic entities
	std::vector<Entity*> m_dynamicEntities;

	bool m_topologyDirty = false;

	// Spatial structures
	SpatialHashGrid<Entity> m_dynamicGrid;
	BVHSystem m_bvh;

	// Engine reference
	EntityManager* m_entityManager = nullptr;

	// World mask data (needed by RaycastWorld / IsWorldSolid)
	std::vector<uint8_t> m_worldMask;
	int m_worldWidth = 0;
	int m_worldHeight = 0;
	int m_worldOffsetX = 0;
	int m_worldOffsetY = 0;
	int m_tileSize = 0;
	bool m_worldMaskDirty = false;
	uint64_t m_lastWorldRevision = 0;
};
////////////////////////////////