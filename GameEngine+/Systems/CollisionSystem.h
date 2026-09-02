/////////////////////////////////
// CollisionSystem.h - Responsible for detecting and resolving collisions between entities. It uses a SpatialHashGrid for efficient broad-phase collision detection, and then performs narrow-phase checks to determine if entities are colliding. When a collision is detected, 
// it resolves the collision based on the entity types: enemies (different tags) create explosions and are destroyed, while allies (same tag) bounce off each other elastically.
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <vector>
#include <memory>
#include "../Vec2.h"
#include "../SpatialHashGrid.h"
#include "SpatialIndexUnified.h"
/////////////////////////////////



/////////////////////////////////
// Forward declarations
class Entity;
class EntityManager;
class SoundSystem;
namespace Spawn { class SpawnSystem; }
/////////////////////////////////



/////////////////////////////////
// CollisionSystem class - Responsible for detecting and resolving collisions between entities. It uses a SpatialHashGrid for efficient broad-phase collision detection, and then performs narrow-phase checks to determine if entities are colliding. 
// When a collision is detected, it resolves the collision based on the entity types: enemies (different tags) create explosions and are destroyed, while allies (same tag) bounce off each other elastically.
//								|
//								|_______________________________________________________________________
class CollisionSystem {
	/////////////////////////////////
	// Public member variables
public:
	/////////////////////////////////
	// Configuration parameters for the collision system, at the moment we only have a parameter for the explosion radius, which determines how large the explosion effect will be when two enemy entities collide.
	// This can be adjusted to create larger or smaller explosions based on the desired visual effect and gameplay balance.
	int m_explosionCount = 0;
	/////////////////////////////////



	/////////////////////////////////
	// Public interface
public:
	/////////////////////////////////
	// Constructor and destructor for the CollisionSystem class. The constructor takes a pointer to the EntityManager for accessing entities and spawning explosions during collision resolution. The destructor is defaulted since we have no resources to clean up.
	explicit CollisionSystem(EntityManager* entityManager) : m_entityManager(entityManager) {}
	~CollisionSystem() = default;
	/////////////////////////////////



	/////////////////////////////////
	// SetSpawnSystem - Set the SpawnSystem reference for explosion spawning
	void SetSpawnSystem(Spawn::SpawnSystem* spawnSystem) { m_spawnSystem = spawnSystem; }
	/////////////////////////////////



	/////////////////////////////////
	// SetSoundSystem - Set the SoundSystem reference for checking concurrent sound limits
	void SetSoundSystem(SoundSystem* soundSystem) { m_soundSystem = soundSystem; }
	/////////////////////////////////



	/////////////////////////////////
	// DetectAndResolve method - Detects and resolves collisions between entities. It iterates through the list of entities, queries the spatial hash for nearby entities, checks for actual collisions, and applies the appropriate collision response based on entity types.
	// It returns the number of entities destroyed as a result of collisions (e.g. 2 for enemy collisions, 0 for ally bounces).
	void DetectAndResolve(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime);
	/////////////////////////////////



	/////////////////////////////////
	// SetSpatialIndex - Set the ISpatialIndex reference for spatial queries
	void SetSpatialIndex(ISpatialIndex* idx) { m_spatialIndex = idx; }
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods for collision detection and resolution. These methods include checking if two entities are colliding based on their positions and radii,
	// resolving a collision between two entities based on their types (enemies vs allies), applying an elastic collision response to bounce allied entities apart, and checking if two entities are enemies based on their types/tags.
private:
	/////////////////////////////////
	// Pointer to the EntityManager for accessing entities and spawning explosions during collision resolution. This allows the CollisionSystem to interact with the EntityManager to create new entities (e.g., explosions) and update the state of existing entities (e.g., marking them as destroyed) based on collision outcomes.
	EntityManager*
		m_entityManager; // Pointer to the EntityManager for accessing entities and spawning explosions during collision resolution
	/////////////////////////////////



	/////////////////////////////////
	// Pointer to the SpawnSystem for spawning explosions during collision resolution. Allows CollisionSystem to delegate explosion creation through SpawnSystem instead of directly creating entities.
	Spawn::SpawnSystem* m_spawnSystem = nullptr;
	/////////////////////////////////



	/////////////////////////////////
	// Pointer to the SoundSystem for checking concurrent sound limits before creating new explosion sounds.
	SoundSystem* m_soundSystem = nullptr;
	/////////////////////////////////



	/////////////////////////////////
	ISpatialIndex* m_spatialIndex = nullptr;
	/////////////////////////////////



	/////////////////////////////////
	// IsColliding method - Checks if two entities are colliding based on their positions and radii. It calculates the distance between the centers of the two entities and compares it to the sum of their radii to determine if a collision is occurring.
	bool IsColliding(const Entity* entity1, const Entity* entity2) const;
	/////////////////////////////////



	/////////////////////////////////
	// ResolveCollision method - Resolves a collision between two entities based on their types. If the entities are enemies (different tags), it spawns an explosion at the collision point and marks both entities as not alive (destroyed). If the entities are allies (same tag), it applies an elastic collision response to bounce them apart without destroying them.
	int ResolveCollision(Entity* entity1, Entity* entity2) const;
	/////////////////////////////////



	/////////////////////////////////
	// BounceEntities method - Applies an elastic collision response to bounce two allied entities apart. It calculates the normal vector between the two entities, computes the relative velocity, and updates the velocities of both entities based on the collision response formula for elastic collisions.
	void BounceEntities(Entity* entity1, Entity* entity2) const;
	/////////////////////////////////



	/////////////////////////////////
	// AreEnemies method - Checks if two entities are enemies based on their types/tags. It compares the EntityType of both entities and returns true if they are different (indicating they are enemies) or false if they are the same (indicating they are allies).
	bool AreEnemies(const Entity* entity1, const Entity* entity2) const;
	/////////////////////////////////
};
/////////////////////////////////