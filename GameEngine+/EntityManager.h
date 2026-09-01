/////////////////////////////////
// EntityManager.h - EntityManager class definition.
#pragma once
/////////////////////////////////



/////////////////////////////////
// Forward Declarations, Note I have includes and forward declarations in an unusual order here to minimize coupling and reduce compile times.
class TileSystem; // Forward declaration of TileSystem.
class MusicSystem; // Forward declaration of MusicSystem
class SoundSystem; // Forward declaration of SoundSystem
/////////////////////////////////



/////////////////////////////////
// Includes for the EntityManager class.
#include <array>
#include "Entity.h"
#include "BVHSystem.h"
/////////////////////////////////



/////////////////////////////////
// More forward declarations; RenderWindow is used by EntityManager but we want to avoid including the full SFML header here if possible to reduce compile times. We can include it in the .cpp file instead since we only need a reference to it.
// We also forward declare Entity since we use pointers to it in our containers, and we want to avoid including the full Entity header here to reduce compile times. The full definition of Entity will be needed in the .cpp file where we implement the methods that manipulate Entity objects.
namespace sf {class RenderWindow;}
class Entity;
/////////////////////////////////



/////////////////////////////////
// Include necessary headers for SFML, standard library containers, and other components used by EntityManager
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <chrono>
#include <thread>

#include "SpatialHashGrid.h"
#include "Vec2.h"
#include "EntityType.h"
#include "Raycast.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/RenderSystem.h"
#include "CTileMap.h"
#include "SpatialLayerRegistry.h"
/////////////////////////////////



/////////////////////////////////
// Type aliases (for convenience and readability)
using EntityVector = std::vector<std::unique_ptr<Entity>>;
using EntityMap = std::map<EntityType, std::vector<Entity*>>;
/////////////////////////////////



/////////////////////////////////
// EntityManager class - responsible for managing all entities in the game, including creation, updating, rendering, and spatial organization. The EntityManager maintains a collection of active entities, 
// a spatial hash grid for efficient spatial queries, and manages the main systems that operate on entities such as physics, collision detection, and rendering. It also provides an interface for adding 
// and removing entities, as well as processing pending entities that have been added but not yet integrated into the main entity list.
//								|
//								|_______________________________________________________________________
class EntityManager {
	/////////////////////////////////
	// Public interface for the EntityManager class
public:
	/////////////////////////////////
	// Constructor and destructor for the EntityManager class.
	explicit EntityManager(sf::RenderWindow& window, float cellSize = 32.0f);
	~EntityManager();
	/////////////////////////////////



	/////////////////////////////////
	// Public methods for updating and rendering entities, as well as adding and removing entities from the manager. The Update method will handle 
	// updating all systems and processing pending entities, while the Render methods will handle drawing entities.
	void Update(float deltaTime = 1.0f / 60.0f);
	void RenderShapes(); // Direct rendering (not queued)
	void RenderText(); // Direct render (no queue)
	void RenderAll(RenderSystem::RenderMode mode = RenderSystem::RenderMode::ShapesThenText);
	/////////////////////////////////



	/////////////////////////////////
	// Methods for adding and removing entities, as well as accessing the list of entities and the spatial hash grid. The AddEntity method creates a 
	// new entity of the specified type and adds it to the manager, while the KillEntity method marks an entity for removal.
	Entity* AddEntity(EntityType type);
	void KillEntity(Entity* entity);
	// Safely kill an entity pointer only if it is currently managed by this EntityManager.
	// This checks internal containers by pointer identity before calling KillEntity so callers
	// can avoid dereferencing pointers that may have been freed elsewhere.
	void SafeKillEntity(Entity* entity);
	// Remove all entities immediately (call when switching scenes)
	void ClearAll();
	/////////////////////////////////



	/////////////////////////////////
	// Accessor methods for retrieving the list of entities and the spatial hash grid, as well as getting and setting the death count for the current frame. The GetEntities method returns a reference to the vector of 
	// active entities, while the overloaded GetEntities method returns a vector of pointers to entities of a specific type. The GetSpatialHash method returns a reference to the spatial hash grid used for spatial queries. 
	EntityVector& GetEntities();
	std::vector<Entity*>& GetEntities(EntityType type);

	SpatialHashGrid<Entity>& GetSpatialHash();
	void SetSpatialLayerRegistry(SpatialLayerRegistry* registry) { m_layerRegistry = registry; }
	SpatialLayerRegistry* GetSpatialLayerRegistry() const { return m_layerRegistry; }
	/////////////////////////////////



	/////////////////////////////////
	// Accessor methods GetDeathCountThisFrame and SetDeathCountThisFrame for tracking the number of entities that have been marked as dead during the current frame. This can be used for debugging, performance monitoring, or gameplay mechanics that depend on entity deaths.
	int GetDeathCountThisFrame() const { return m_deathCountThisFrame; }
	void SetDeathCountThisFrame(int count) { m_deathCountThisFrame = count; }
	/////////////////////////////////



	/////////////////////////////////
	const std::vector<std::unique_ptr<Entity>>& GetEntities() const { return m_entities; }
	/////////////////////////////////



	/////////////////////////////////
	// Accessor methods for the main systems managed by the EntityManager, including the PhysicsSystem, CollisionSystem, RenderSystem, MusicSystem, and SoundSystem. These methods provide access to the systems for updating and rendering entities, as well as managing music and sound playback.
	PhysicsSystem& GetPhysicsSystem() { return m_physicsSystem; }
	CollisionSystem& GetCollisionSystem() { return m_collisionSystem; }
	RenderSystem& GetRenderSystem() { return m_renderSystem; }	
	MusicSystem* GetMusicSystem() { return m_musicSystem.get(); } // Accessor for MusicSystem (may be nullptr)
	SoundSystem* GetSoundSystem() { return m_soundSystem.get(); } // Accessor for SoundSystem (may be nullptr)
	/////////////////////////////////



	/////////////////////////////////
	// Methods for adding a tile map to the entity manager. The AddTileMapAsEntities method takes a TileMap and creates individual entities for each solid tile, while the CreateTileMapEntity method 
	// creates a single CTileMap entity that can be processed by the TileSystem. These methods provide flexibility in how tile maps are represented and managed within the entity system.
	void AddTileMapAsEntities(const TileMap& map, int tileValueToTreatAsSolid = 1);
	Entity* CreateTileMapEntity(const TileMap& map); // Preferred: create a CTileMap entity which will be processed by TileSystem
	/////////////////////////////////



	/////////////////////////////////
	// Methods for managing pending tile maps. The SetHasPendingTileMaps method sets a flag indicating that there are pending tile maps that need to be processed, while the HasPendingTileMaps method checks the status of this flag.
	void SetHasPendingTileMaps(bool v) { m_hasPendingTileMaps = v; }
	bool HasPendingTileMaps() const { return m_hasPendingTileMaps; }
	/////////////////////////////////
	 
	 
	
	/////////////////////////////////
	// Commit pending entities without running full Update(). This moves entities from the add-queue into the active list so systems can see them.
	void ProcessPending();
	/////////////////////////////////



	/////////////////////////////////
	// Debug: validate internal container integrity (only enabled in debug builds)
	void ValidateIntegrity() const;
	/////////////////////////////////
	


	/////////////////////////////////
	// Layer buckets access for RenderSystem (incremental maintenance)
	std::array<std::vector<Entity*>, 4>& GetLayerBuckets() { return m_layerBuckets; }
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// Helper to set an entity's layer (moves between buckets)
	void SetEntityLayer(Entity* e, Entity::Layer layer);
	/////////////////////////////////



	/////////////////////////////////
	BVHSystem& GetBVH() { return m_bvh; } // BVH system for efficient raycasting and spatial queries
	/////////////////////////////////



	/////////////////////////////////
	void UpdateBVH(); // Rebuild the BVH tree based on current entities
	/////////////////////////////////



	/////////////////////////////////
	void SetSpatialHashCellSize(float cellSize) { m_spatialHash = SpatialHashGrid<Entity>(cellSize); }
	/////////////////////////////////


	/////////////////////////////////
	bool MatchesFilter(Entity* e, const SpatialLayerFilter& filter);
	/////////////////////////////////



	//////////////////////////////////
	// Private helper methods
private:
	/////////////////////////////////
	// AddPendingEntities - move entities from the pending add queue into the main entity list and spatial hash, and update the entity map by type. This method is called during the Update process to integrate newly added entities into the main systems.
	void AddPendingEntities();
	/////////////////////////////////



	/////////////////////////////////
	// RemoveDeadEntities - remove entities that have been marked as dead from the main entity list and spatial hash, and update the entity map by type. This method is called during the Update process to clean up entities that are no longer active.
	void RemoveDeadEntities();
	/////////////////////////////////



	/////////////////////////////////
	// UpdateSpatialHashAndRender - update the spatial hash grid with the current positions of all entities, and prepare the layer buckets for rendering. This method is called during the 
	// Update process to ensure that the spatial hash is accurate for collision detection and that entities are organized into their respective rendering layers.
	void UpdateSpatialHashAndRender();
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables.
private:
	SpatialHashGrid<Entity> m_spatialHash;
	SpatialLayerRegistry* m_layerRegistry =	nullptr; // Optional: pointer to a SpatialLayerRegistry for managing spatial layers (may be nullptr)`
	EntityVector m_entities;
	EntityVector m_toAdd;
	EntityMap m_entityMap;
	size_t m_totalEntities = 0;
	sf::RenderWindow& m_window;
	int m_deathCountThisFrame = 0;

	PhysicsSystem m_physicsSystem;
	CollisionSystem m_collisionSystem;
	RenderSystem m_renderSystem;
	std::unique_ptr<TileSystem> m_tileSystem;
	std::unique_ptr<MusicSystem> m_musicSystem; // system owning runtime sf::Music objects
	std::unique_ptr<SoundSystem> m_soundSystem; // system managing sound effects with 3D spatial audio
	bool m_hasPendingTileMaps = true;
	BVHSystem m_bvh;
	/////////////////////////////////



	/////////////////////////////////
	// Incremental layer buckets for fast rendering
	std::array<std::vector<Entity*>, 4> m_layerBuckets;
	/////////////////////////////////



	/////////////////////////////////
	// Thread id that owns this EntityManager (captured at construction). Used to detect cross-thread access in debug builds.
	std::thread::id m_ownerThreadId;
	/////////////////////////////////
};
/////////////////////////////////