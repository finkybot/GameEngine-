#pragma once
#include <vector>
#include <memory>
#include "../Vec2.h"
#include "../QuadTree.h"

// Forward declarations
class Entity;
class EntityManager;

/// <summary>
/// Collision System - Detects and resolves collisions between entities
/// Uses QuadTree spatial partitioning for efficient broad-phase detection
/// Supports both elastic collisions (bounce) and destruction collisions (enemy teams)
/// </summary>
class CollisionSystem
{
public:
	/// <summary>
	/// Constructor - takes reference to EntityManager for spawning explosions
	/// </summary>
	explicit CollisionSystem(EntityManager* entityManager) : m_entityManager(entityManager) {}
	
	/// <summary>Destructor</summary>
	~CollisionSystem() = default;

	/// <summary>
	/// Detect and resolve collisions between entities using QuadTree spatial partitioning
	/// Enemies (different tags) collide and create explosions
	/// Allies (same tag) bounce off each other elastically
	/// </summary>
	/// <param name="entities">List of all active entities</param>
	/// <param name="quadTree">Spatial partitioning structure for broad-phase detection</param>
	/// <param name="deltaTime">Time elapsed since last frame (unused but kept for consistency)</param>
	/// <returns>Number of entities destroyed this frame</returns>
	int DetectAndResolve(const std::vector<std::unique_ptr<Entity>>& entities, QuadTree<Entity>& quadTree, float deltaTime);

private:
	EntityManager* m_entityManager;

	/// <summary>
	/// Check if two entities are colliding (circle-circle intersection)
	/// Uses squared distance to avoid expensive square root calculation
	/// </summary>
	bool IsColliding(const Entity* entity1, const Entity* entity2) const;

	/// <summary>
	/// Resolve collision between two entities
	/// Enemies: Creates explosion and marks both for destruction
	/// Allies: Applies elastic collision and bounces them apart
	/// </summary>
	/// <returns>Number of entities destroyed (0 for bounce, 2 for explosion)</returns>
	int ResolveCollision(Entity* entity1, Entity* entity2) const;

	/// <summary>
	/// Bounce two allied entities apart using elastic collision physics
	/// Applies impulse-based collision response with position correction
	/// </summary>
	void BounceEntities(Entity* entity1, Entity* entity2) const;

	/// <summary>
	/// Check if two entities are enemies (belong to different teams/tags)
	/// </summary>
	bool AreEnemies(const Entity* entity1, const Entity* entity2) const;
};


