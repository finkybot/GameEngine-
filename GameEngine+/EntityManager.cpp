/////////////////////////////////
// EntityManager.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "EntityManager.h"
#include "Entity.h"
#include "EntityType.h"
#include "CCircle.h"
#include "CExplosion.h"
#include <algorithm>
#include <execution>
#include <chrono>
#include <cmath>
#include <sstream>
#include <unordered_set>
#include <iomanip>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include "CRectangle.h"
#include "CStatic.h"
#include "Systems/TileSystem.h"
#include "MusicSystem.h"
/////////////////////////////////



/////////////////////////////////
// EntityManager implementation - takes reference to SFML render window for drawing and FPS reporting
EntityManager::EntityManager(sf::RenderWindow& window, float cellSize): m_window(window), m_spatialHash(cellSize), m_collisionSystem(this) {
	// Initialize main systems with reference to this EntityManager
	m_tileSystem = std::make_unique<TileSystem>(this);
	m_musicSystem = std::make_unique<MusicSystem>(*this);

	// Store the ID of the thread that created this EntityManager instance for debugging purposes. This allows us to assert that certain methods are only called from the owning thread, which can help catch threading issues during development.
	m_ownerThreadId = std::this_thread::get_id();
}
/////////////////////////////////



/////////////////////////////////
// Destructor for the EntityManager class. Ensures proper destruction order for forward-declared types by resetting the unique pointers to the main systems (TileSystem and MusicSystem) before the EntityManager itself is destroyed. 
// This prevents potential issues with dangling pointers or incomplete types during destruction.
EntityManager::~EntityManager() {
	// ensure proper destruction order for forward-declared types
	m_musicSystem.reset();
	m_tileSystem.reset();
}
/////////////////////////////////


/////////////////////////////////
// ClearAll - immediately removes every entity from the EntityManager without going through the normal kill/process cycle.
// Call this when switching scenes so the previous scene's entities do not bleed into the next one.
void EntityManager::ClearAll() {
	m_toAdd.clear();
	m_entityMap.clear();
	for (auto& bucket : m_layerBuckets) bucket.clear();
	m_spatialHash.Clear();
	m_entities.clear();
	m_deathCountThisFrame = 0;
	m_hasPendingTileMaps = false;
}
/////////////////////////////////



/////////////////////////////////
// CreateTileMapEntity - creates a new entity of type TileMap and adds a CTileMap component with the provided tile map data. The method marks the tile map as dirty for processing by the TileSystem, which will generate colliders based on the tile data in the next update cycle.
Entity* EntityManager::CreateTileMapEntity(const TileMap& map) {
	Entity* e = AddEntity(EntityType::TileMap);
	if (!e)
		return nullptr;

	// Add CTileMap component with the provided tile map data and mark it as dirty for processing by the TileSystem then finally return the created entity
	e->AddComponent<CTileMap>(map);
	m_hasPendingTileMaps = true;
	return e;
}
/////////////////////////////////



/////////////////////////////////
// AddPendingEntities - processes all entities that were queued for addition to the EntityManager. This method is called during the update cycle to add new entities to the main entity list and update relevant data structures such as the entity map and layer buckets.
void EntityManager::AddPendingEntities() {
    // Debug: assert caller thread is owner
#ifdef _DEBUG
	if (std::this_thread::get_id() != m_ownerThreadId) {
		std::cerr << "EntityManager::AddPendingEntities called from non-owner thread" << std::endl;
		// fall through in release builds but help debugging in dev
	}
#endif
	
	// Move pending entities into the main entity list and update the entity map and layer buckets. We iterate over the m_toAdd vector, which contains 
	// unique pointers to new entities that were created during the current frame. For each entity, we move it into the m_entities vector,
    for (auto& entity : m_toAdd) {
		Entity* entityPtr = entity.get();
		m_entities.push_back(std::move(entity));
		m_entityMap[entityPtr->GetType()].push_back(entityPtr);
		// Insert into layer bucket for incremental rendering
		int layerIdx = static_cast<int>(entityPtr->GetLayer());
		if (layerIdx < 0) layerIdx = 0;
		if (layerIdx > 3) layerIdx = 3;
        m_layerBuckets[layerIdx].push_back(entityPtr);
		entityPtr->SetBucketInfo(layerIdx, static_cast<int>(m_layerBuckets[layerIdx].size()) - 1); // Store bucket index and position for O(1) removal later
	}
	m_toAdd.clear();
}
/////////////////////////////////



/////////////////////////////////
// RemoveDeadEntities - removes entities that have been marked as dead from the EntityManager. This method is called during the update cycle to clean up entities that are no longer alive, 
// ensuring that they are properly removed from all relevant data structures and deleted from memory.
void EntityManager::RemoveDeadEntities() {
    // Debug: assert caller thread is owner
#ifdef _DEBUG
	if (std::this_thread::get_id() != m_ownerThreadId) {
		std::cerr << "EntityManager::RemoveDeadEntities called from non-owner thread" << std::endl;
	}
#endif

	// First, we build a list of raw pointers to the entities that are marked as dead. This allows us to identify which entities need to be removed without modifying the main entity list while iterating over it.
	std::vector<Entity*> deadEntities;
	deadEntities.reserve(m_entities.size() / 10);

	// Iterate over the main entity list and collect pointers to entities that are not alive. We use the IsAlive() method of each entity to check its status, and if it returns false, we add the raw pointer to the deadEntities vector for later processing.
	for (const auto& up : m_entities) {
		if (!up->IsAlive())
			deadEntities.push_back(up.get());
	}

	// If there are no dead entities, we can return early without modifying any data structures.
	if (deadEntities.empty())
		return;

	// Build a fast lookup set of dead pointers so we can remove references without dereferencing them.
	std::unordered_set<Entity*> deadSet(deadEntities.begin(), deadEntities.end());

    // Remove references to dead entities from the entity map by pointer identity only
	for (auto& deadEnt : m_entityMap) {
		auto& vec = deadEnt.second;
		vec.erase(std::remove_if(vec.begin(), vec.end(), [&deadSet](Entity* e) { return deadSet.find(e) != deadSet.end(); }), vec.end());
	}

	// Remove references to dead entities from the layer buckets by pointer identity only, then rebuild bucket position metadata for remaining entries. We iterate over each layer bucket and use std::remove_if to remove any pointers that are found in the deadSet, 
	// which allows us to clean up the buckets without needing to dereference the pointers. After removing the dead entities, we rebuild the bucket position metadata for the remaining entries to ensure that any entities that were moved during removal have their 
	// bucket info updated correctly.
	for (size_t bucketIdx = 0; bucketIdx < m_layerBuckets.size(); ++bucketIdx) {
		auto& bucket = m_layerBuckets[bucketIdx];
		bucket.erase(std::remove_if(bucket.begin(), bucket.end(), [&deadSet](Entity* e) {
			return e == nullptr || deadSet.find(e) != deadSet.end();
		}), bucket.end());

		// Rebuild bucket position metadata for remaining entries
		for (size_t pos = 0; pos < bucket.size(); ++pos) {
			Entity* ent = bucket[pos];
			if (ent) {
				ent->SetBucketInfo(static_cast<int>(bucketIdx), static_cast<int>(pos));
			}
		}
	}

	// If we had a spatial tree that stored raw pointers,then we would need remove the dead ones now....but..... I'm using a SpatialHashGrid which is rebuilt each frame so explicit removal is not required (Yeeeeaaa Me!!!).
	// AAAANNNYYWAY!!!!! I have included the cleanup for a tree; cleanup iterate over the deadEntities and remove each pointer from it:
	// for (Entity* d : deadEntities) m_quadTree.RemoveEntityFromTree(d);

	// It's is now safe to erase the owning unique_ptrs from m_entities and thus delete the objects.
	auto end = std::remove_if(m_entities.begin(), m_entities.end(),
							  [](const std::unique_ptr<Entity>& e) { return !e->IsAlive(); });
    m_entities.erase(end, m_entities.end());
// ValidateIntegrity calls disabled by user request.
}
/////////////////////////////////



/////////////////////////////////
// SetEntityLayer - updates the rendering layer of an entity and moves it to the appropriate layer bucket for incremental rendering. This method ensures that the entity is removed from its old bucket and added to the new bucket based 
// on the specified layer, allowing for efficient rendering based on layers.
void EntityManager::SetEntityLayer(Entity* e, Entity::Layer layer) {
    // Debug: assert caller thread is owner
#ifdef _DEBUG
	if (std::this_thread::get_id() != m_ownerThreadId) {
		std::cerr << "EntityManager::SetEntityLayer called from non-owner thread" << std::endl;
	}
#endif

	// Update the entity's layer and move it to the appropriate layer bucket for incremental rendering. We first check if the entity pointer is valid, and if it is, we retrieve its current bucket information 
	// (old bucket and position) to determine where it is currently stored in the layer buckets.
	if (!e) return;
	int oldBucket = e->GetBucketId();
	int oldPos = e->GetBucketPos();
	int newBucket = static_cast<int>(layer);
	if (oldBucket == newBucket) {
		e->SetLayer(layer);
		return;
	}
	// Remove from old bucket if present
	if (oldBucket >= 0 && oldBucket < (int)m_layerBuckets.size() && oldPos >= 0) {
		auto& bucket = m_layerBuckets[oldBucket];
		int lastIdx = static_cast<int>(bucket.size()) - 1;
		if (oldPos <= lastIdx) {
			Entity* lastEnt = bucket[lastIdx];
			bucket[oldPos] = lastEnt;
			lastEnt->SetBucketInfo(oldBucket, oldPos);
			bucket.pop_back();
		}
	}
	// Add to new bucket
	auto& nb = m_layerBuckets[newBucket];
	nb.push_back(e);
	e->SetBucketInfo(newBucket, static_cast<int>(nb.size()) - 1);
	e->SetLayer(layer);
}
/////////////////////////////////



/////////////////////////////////
// AddTileMapAsEntities - processes a tile map and creates entities for solid tiles, using a greedy rectangle merging algorithm to combine contiguous solid tiles into larger rectangles for efficient collision handling. This method ensures that the spatial hash grid is updated 
// to match the tile size for optimal performance when querying tile entities.
void EntityManager::AddTileMapAsEntities(const TileMap& map, int tileValueToTreatAsSolid) {
	if (map.width <= 0 || map.height <= 0)
		return;
	// Ensure spatial hash cell size matches tile size for optimal alignment and query accuracy
	// Recreate the spatial hash with the tile size so tiles map 1:1 to cells when possible
	m_spatialHash = SpatialHashGrid<Entity>(map.tileSize);

	// 2D greedy rectangle merging: create maximal rectangles of contiguous solid tiles
	std::vector<char> used(map.width * map.height, 0);
	for (int y = 0; y < map.height; ++y) {
		for (int x = 0; x < map.width; ++x) {
			int idx = y * map.width + x;
			if (used[idx])
				continue;
			if (!map.IsSolid(x, y))
				continue;

			// determine maximal width
			int w = 1;
			while (x + w < map.width && map.IsSolid(x + w, y) && !used[y * map.width + (x + w)])
				++w;

			// determine maximal height we can extend where every row has the same solid run
			int h = 1;
			bool canExtend = true;
			while (y + h < map.height && canExtend) {
				for (int xi = 0; xi < w; ++xi) {
					if (!map.IsSolid(x + xi, y + h) || used[(y + h) * map.width + (x + xi)]) {
						canExtend = false;
						break;
					}
				}
				if (canExtend)
					++h;
			}

			// mark used
			for (int yy = 0; yy < h; ++yy)
				for (int xx = 0; xx < w; ++xx)
					used[(y + yy) * map.width + (x + xx)] = 1;

			float tileW = map.tileSize * w;
			float tileH = map.tileSize * h;
			float posX = x * map.tileSize;
			float posY = y * map.tileSize;

			Entity* e = AddEntity(EntityType::TileMap);
			if (e) {
				// When creating tile entities through AddTileMapAsEntities we mark them as static colliders.
				e->AddComponent<CTransform>(Vec2(posX, posY), Vec2(0.0f, 0.0f));
				auto rect = std::make_unique<CRectangle>(tileW, tileH);
				rect->SetColor(160.0f, 160.0f, 160.0f, 200);
				e->AddComponentPtr<CShape>(std::move(rect));
				e->AddComponent<CStatic>();
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// UpdateSpatialHashAndRender - updates the spatial hash grid with the current positions of all entities and prepares them for rendering. This method is called during the update cycle to ensure that the spatial hash is accurate for collision detection and 
// that entities are rendered in the correct order based on their layers.
/////////////////////////////////
void EntityManager::UpdateSpatialHashAndRender() {
	// Rebuild spatial hash every frame (very fast)
	m_spatialHash.Clear();
	for (auto& entity : m_entities) {
		m_spatialHash.Insert(entity.get());
	}

	// Rendering is now controlled explicitly by the engine's render pass.
}
/////////////////////////////////



/////////////////////////////////
// RenderShapes - renders the shapes of all entities in the EntityManager using the RenderSystem. This method iterates through the layer buckets in order (Background, Mid, Foreground, Overlay) and renders each entity's shape if it is alive, 
// ensuring that entities are drawn in the correct order based on their layers.
/////////////////////////////////
void EntityManager::RenderShapes() {
    // Use incremental layer buckets for rendering: iterate Background->Mid->Foreground->Overlay
	for (size_t layer = 0; layer < m_layerBuckets.size(); ++layer) {
		for (Entity* e : m_layerBuckets[layer]) {
			if (!e || !e->IsAlive()) continue;
			m_renderSystem.RenderEntity(e, m_window);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// ValidateIntegrity - a debug method that checks the internal consistency of the EntityManager's data structures. This method can be called during development to ensure that entities are properly added and removed, and that all references are valid.
/////////////////////////////////
void EntityManager::ValidateIntegrity() const {
    // ValidateIntegrity disabled per user request. Keep function as no-op to avoid
	// impacting performance in production runs while preserving call sites.
	(void)0;
}
/////////////////////////////////



/////////////////////////////////
// RenderText - renders the text components of all entities in the EntityManager using the RenderSystem. This method iterates through the list of entities and renders any text components for alive entities, allowing for text to be drawn separately from shapes if desired.
void EntityManager::RenderText() {
	m_renderSystem.RenderText(m_entities, m_window);
}
/////////////////////////////////



/////////////////////////////////
// RenderAll - a convenience method that renders all entities in the EntityManager using the RenderSystem. The mode parameter allows the caller to specify whether to render only shapes, render shapes followed by text, or render shapes now and defer text 
// rendering until after overlays are rendered.
void EntityManager::RenderAll(RenderSystem::RenderMode mode) {
	m_renderSystem.RenderAll(m_entities, m_window, mode);
}
/////////////////////////////////



/////////////////////////////////
// Update - the main update method for the EntityManager, called once per frame to update all systems, process pending entities, and manage the lifecycle of entities. This method handles adding new entities, removing dead entities, updating the spatial hash grid, 
// and allowing systems like the MusicSystem and TileSystem to process their logic.
void EntityManager::Update(float deltaTime) {
	SpatialHashGrid<Entity>::ResetQueryStats();

	m_deathCountThisFrame = 0;

// ValidateIntegrity calls disabled by user request.

	AddPendingEntities();
	RemoveDeadEntities();

// ValidateIntegrity calls disabled by user request.

	// Let MusicSystem reconcile component data with runtime sf::Music instances.
	if (m_musicSystem)
		m_musicSystem->Process();

	// Process tilemaps into tile entities before rebuilding spatial hash
	if (m_tileSystem && m_hasPendingTileMaps) {
		m_tileSystem->Process(); // Process pending tilemaps
		m_hasPendingTileMaps = false;
		AddPendingEntities(); // Add any new tile entities created by TileSystem
		// allow music system to pick up any newly-created entities with CMusic
		if (m_musicSystem)
			m_musicSystem->Process();
	}

	UpdateSpatialHashAndRender();
}
/////////////////////////////////



/////////////////////////////////
// ProcessPending - a method that can be called to process pending entities without performing a full update cycle. This allows the caller to add new entities and have them integrated into the EntityManager's data structures without immediately running all 
// systems or rebuilding the spatial hash,
void EntityManager::ProcessPending() {
	AddPendingEntities();
	// Do not run systems or rebuild spatial hash - caller may request full Update later in the frame.
}
/////////////////////////////////



/////////////////////////////////
// AddEntity - creates a new entity of the specified type and adds it to the EntityManager. This method initializes the entity with a unique ID, sets its creation time for potential time-based logic, and ensures that it has a CTransform component for systems 
// that rely on it. The new entity is added to the m_toAdd vector for processing during the next update cycle.
Entity* EntityManager::AddEntity(EntityType type) {
	auto entity = std::unique_ptr<Entity>(new Entity(type, m_totalEntities++));
	entity->m_creationTime = std::chrono::high_resolution_clock::
		now(); // Track creation time for entity (currently used for explosions but could be useful for other time-based logic in the future)
			   // Ensure new entities have a transform so systems can rely on it
	entity->AddComponent<CTransform>(Vec2::Zero, Vec2::Zero);
	Entity* entityPtr = entity.get(); // Capture pointer BEFORE moving
	m_toAdd.push_back(std::move(entity));

	return entityPtr;
}
/////////////////////////////////



/////////////////////////////////
// KillEntity - marks an entity for removal by calling its Destroy method and increments the death count for the current frame. This method allows systems and gameplay logic to track how many entities have been marked as dead during the frame, which can be useful 
// for debugging, performance monitoring, or gameplay mechanics that depend on entity deaths.
void EntityManager::KillEntity(Entity* entity) {
	entity->Destroy();
	SetDeathCountThisFrame(GetDeathCountThisFrame() + 1);
}



/////////////////////////////////
// Kill the entity only if it is present in our managed collections. Avoids calling Destroy
// on pointers that the manager doesn't own which can lead to use-after-free from external callers.
void EntityManager::SafeKillEntity(Entity* entity) {
	if (!entity) return;
	// check if pointer exists in m_entities or m_toAdd by pointer identity
	for (const auto& up : m_entities) if (up.get() == entity) { KillEntity(entity); return; }
	for (const auto& up : m_toAdd) if (up.get() == entity) { KillEntity(entity); return; }
	// Also check type-map lists
	for (auto &pr : m_entityMap) {
		for (Entity* e : pr.second) if (e == entity) { KillEntity(entity); return; }
	}
	// If not found, ignore — this entity is not owned by us.
}
/////////////////////////////////



/////////////////////////////////
// GetEntities - returns a reference to the vector of active entities in the EntityManager. This allows systems and other parts of the code to access the list of entities for processing, rendering, or other operations.
EntityVector& EntityManager::GetEntities() {
	return m_entities;
}
/////////////////////////////////



/////////////////////////////////
// GetEntities (overloaded) - returns a reference to the vector of pointers to entities of a specific type. This allows systems and other parts of the code to access entities of a particular type for processing, rendering, 
// or other operations without needing to filter the main entity list.
std::vector<Entity*>& EntityManager::GetEntities(EntityType type) {
	return m_entityMap[type];
}
/////////////////////////////////



/////////////////////////////////
// GetSpatialHash - returns a reference to the spatial hash grid used for spatial queries. This allows systems like the CollisionSystem to perform efficient spatial queries for nearby entities based on their positions, 
// which can improve performance for collision detection and other spatial operations.
SpatialHashGrid<Entity>& EntityManager::GetSpatialHash() {
	return m_spatialHash;
}
/////////////////////////////////