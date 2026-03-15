// ****** CollisionSystem.h - Collision detection and resolution system ******
#pragma once
#include <vector>
#include <memory>
#include "../Vec2.h"
#include "../SpatialHashGrid.h"

// Forward declarations
class Entity;
class EntityManager;

// CollisionSystem is responsible for detecting and resolving collisions between entities. 
// It uses a SpatialHashGrid for efficient broad-phase collision detection, and then performs narrow-phase checks to determine if entities are colliding. 
// When a collision is detected, it resolves the collision based on the entity types: enemies (different tags) create explosions and are destroyed, while allies (same tag) bounce off each other elastically.
class CollisionSystem
{
	// ***** Public Methods *****
public:
	explicit CollisionSystem(EntityManager* entityManager) : m_entityManager(entityManager) {}	// Constructor - takes a pointer to the EntityManager for accessing entities and spawning explosions. It initializes the CollisionSystem with a reference to the EntityManager, which allows it to access the list of entities and manage their states during collision detection and resolution.
	~CollisionSystem() = default;																// Destructor - default is fine since we have no resources to clean up

	int DetectAndResolve(const std::vector<std::unique_ptr<Entity>>& entities, SpatialHashGrid<Entity>& spatialHash, float deltaTime);	// Detects and resolves collisions between entities. It iterates through the list of entities, queries the spatial hash for nearby entities, checks for actual collisions, and applies the appropriate collision response based on entity types. It returns the number of entities destroyed as a result of collisions (e.g. 2 for enemy collisions, 0 for ally bounces).

	// ***** Private Methods *****
private:
	EntityManager* m_entityManager; // Pointer to the EntityManager for accessing entities and spawning explosions during collision resolution

	bool IsColliding(const Entity* entity1, const Entity* entity2) const;	// Checks if two entities are colliding based on their positions and radii. It calculates the distance between the centers of the two entities and compares it to the sum of their radii to determine if a collision is occurring.
	int ResolveCollision(Entity* entity1, Entity* entity2) const;			// Resolves a collision between two entities based on their types. If the entities are enemies (different tags), it spawns an explosion at the collision point and marks both entities as not alive (destroyed). If the entities are allies (same tag), it applies an elastic collision response to bounce them apart without destroying them. It returns the number of entities destroyed as a result of the collision (e.g. 2 for enemy collisions, 0 for ally bounces).
	void BounceEntities(Entity* entity1, Entity* entity2) const;			// Applies an elastic collision response to bounce two allied entities apart. It calculates the normal vector between the two entities, computes the relative velocity, and updates the velocities of both entities based on the collision response formula for elastic collisions.
	bool AreEnemies(const Entity* entity1, const Entity* entity2) const;	// Checks if two entities are enemies based on their types/tags. It compares the EntityType of both entities and returns true if they are different (indicating they are enemies) or false if they are the same (indicating they are allies).
};


