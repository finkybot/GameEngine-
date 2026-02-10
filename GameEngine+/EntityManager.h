#pragma once
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <chrono>

// Forward declaration for SFML type used by reference in this header
namespace sf { class RenderWindow; }

#include "QuadTree.h"
#include "Vec2.h"
#include "EntityType.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/RenderSystem.h"

// Forward Declarations
class Entity;

typedef std::vector<std::unique_ptr<Entity>> EntityVector;
typedef std::map<EntityType, std::vector<Entity*>> EntityMap;

class EntityManager
{
private:
	QuadTree<Entity>	m_quadTree;
	EntityVector		m_entities;
	EntityVector		m_toAdd;
	EntityMap			m_entityMap;
	size_t				m_totalEntities = 0;
	sf::RenderWindow&	m_window;
	char m_fpsTitle[64] = "FPS: 0.0";
	int					m_deathCountThisFrame = 0;
	
	// Track explosion creation times: entity_id -> creation_time
	std::map<size_t, std::chrono::high_resolution_clock::time_point> m_explosionTimes;
	// Track explosion colors: entity_id -> color
	std::map<size_t, Vec3> m_explosionColors;

	// Systems
	PhysicsSystem	m_physicsSystem;
	CollisionSystem	m_collisionSystem;
	RenderSystem	m_renderSystem;

public:
	/// <summary>
	/// Constructor - initializes the entity manager with a render window reference.
	/// Sets up the quadtree for spatial partitioning and initializes all systems.
	/// </summary>
	EntityManager(sf::RenderWindow& window);
	
	/// <summary>
	/// Main update loop for the entity manager.
	/// Orchestrates all game logic: entity addition, removal, quadtree updates, rendering, and collision detection.
	/// Also maintains FPS reporting to the window title.
	/// </summary>
	void update(float deltaTime = 1.0f / 60.0f);

	/// <summary>
	/// Calculates and reports the current FPS to the window title.
	/// Uses exponential moving average to smooth FPS values and avoid jitter.
	/// Updates the window title once per second.
	/// </summary>
	void ReportFPS(int& fpsFrames, std::chrono::steady_clock::time_point& fpsLast, double& fpsSmooth, const double alpha);

	/// <summary>
	/// Draws debug bounding boxes for the provided quadtree nodes.
	/// Used for visualizing the spatial partitioning structure.
	/// </summary>
	void DrawBoundingBox(const std::vector<BoundingBox>& bboxes);

	/// <summary>
	/// Creates a new entity with the given properties and adds it to the pending queue.
	/// The entity is not immediately added to the active list; instead, it's queued for addition next frame.
	/// This prevents iterator invalidation during game logic.
	/// </summary>
	/// <returns>Pointer to the created entity (valid immediately)</returns>
	Entity* addEntity(EntityType type, float radius, Vec3 color, Vec2 positon, Vec2 velocity, int alpha);
	
	/// <summary>
	/// Gets all active entities.
	/// </summary>
	/// <returns>Reference to the vector of unique pointers to all entities.</returns>
	EntityVector& getEntities();
	
	/// <summary>
	/// Gets all entities with a specific tag/team.
	/// Returns a reference to the vector of raw pointers for the requested tag.
	/// </summary>
	std::vector<Entity*>& getEntities(EntityType type);
	
	/// <summary>
	/// Gets the number of entities destroyed this frame.
	/// </summary>
	int GetDeathCountThisFrame() const { return m_deathCountThisFrame; }
	
	/// <summary>
	/// Spawns an explosion visual effect at the given position.
	/// Explosions are temporary entities that fade out over 300 milliseconds.
	/// Stores creation time and color for fade animation.
	/// </summary>
	void SpawnExplosion(const Vec2& position, float radius, const Vec2& velocity, const Vec3& color);
	
	/// <summary>
	/// Gets the number of active explosions currently playing.
	/// </summary>
	int GetExplosionCount() const { return static_cast<int>(m_explosionTimes.size()); }

private:
	/// <summary>
	/// Adds all pending entities from m_toAdd to the active entity list.
	/// Uses deferred addition pattern to prevent iterator invalidation during game logic.
	/// Also updates the tag-based entity map and inserts entities into the spatial quadtree.
	/// </summary>
	void AddPendingEntities();
	
	/// <summary>
	/// Removes all dead entities (IsAlive() returns false) from the entity list.
	/// Uses erase-remove idiom for efficient removal.
	/// Also removes dead entities from the tag map and quadtree for consistency.
	/// </summary>
	void RemoveDeadEntities();
	
	/// <summary>
	/// Updates the spatial quadtree structure and renders all entities.
	/// Rebuilds the quadtree if entity count changes, frame counter exceeds threshold, or query performance degrades.
	/// Incrementally updates entity positions in the quadtree if rebuild not needed.
	/// </summary>
	void UpdateQuadTreeAndRender();
	
	/// <summary>
	/// Updates all active explosions: applies fade animation and removes expired ones.
	/// Explosions fade out over 300 milliseconds by reducing alpha value.
	/// Automatically marks explosions for removal once they expire.
	/// </summary>
	void UpdateExplosions();
	
	/// <summary>
	/// Detects and resolves collisions between entities.
	/// Updates entity physics/movement first, then detects collisions using the quadtree.
	/// Handles both entity-entity collisions and boundary collisions with window edges.
	/// </summary>
	void DetectAndResolveCollisions(float deltaTime);
	
	bool AreEnemies(Entity* entity1, Entity* entity2);
};
