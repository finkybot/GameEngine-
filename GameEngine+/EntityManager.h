// ***** EntityManager.h - EntityManager class definition *****
#pragma once
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <chrono>

#include "SpatialHashGrid.h"
#include "QuadTree.h"  // For BoundingBox definition
#include "Vec2.h"
#include "EntityType.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/RenderSystem.h"

// Forward Declarations
namespace sf { class RenderWindow; }	// Forward declaration for SFML type used by reference in this header
class Entity;

typedef std::vector<std::unique_ptr<Entity>> EntityVector;		// Vector of unique pointers to entities, owned by EntityManager for automatic memory management
typedef std::map<EntityType, std::vector<Entity*>> EntityMap;	// Map of entity type to vector of raw pointers for quick access by type, not owned (raw pointers) since EntityManager owns the entities

// EntityManager is responsible for managing all entities in the game, including their creation, destruction, updates, and rendering. It maintains a spatial hash grid for efficient collision detection and organizes entities by type for quick access. The EntityManager also orchestrates the main game loop by updating physics, handling collisions, and rendering entities each frame. It tracks FPS and provides debug visualization of the spatial partitioning structure.
class EntityManager
{
// ****** Private Member Variables ******
private:
	SpatialHashGrid<Entity> m_spatialHash{ 100.0f };// 100-unit cells for collision queries
	EntityVector		m_entities;					// Active entities currently in the game, owned by EntityManager
	EntityVector		m_toAdd;					// Entities queued for addition next frame to avoid iterator invalidation during game logic, owned by EntityManager
	EntityMap			m_entityMap;				// Map of entity type to vector of raw pointers for quick access by type, not owned (raw pointers) since EntityManager owns the entities
	size_t				m_totalEntities = 0;		// Total number of entities ever created, used for unique ID assignment
	sf::RenderWindow& m_window;						// Reference to the SFML render window for drawing entities and updating the window title with FPS
	char m_fpsTitle[64] = "FPS: 0.0";				// Buffer for storing the FPS title string
	int					m_deathCountThisFrame = 0;	// Counter for tracking the number of deaths in the current frame
	
	
	std::map<size_t, std::chrono::high_resolution_clock::time_point> m_explosionTimes;	// Track explosion creation times: entity_id -> creation time
	std::map<size_t, Vec3> m_explosionColors;											// Track explosion colors: entity_id -> color

	// ***** Systems *****
	PhysicsSystem	m_physicsSystem;	// Handles entity movement and boundary collisions
	CollisionSystem	m_collisionSystem;	// Handles collision detection and resolution between entities, including spawning explosions on enemy collisions and bouncing allies apart
	RenderSystem	m_renderSystem;		// Handles rendering all entities to the SFML window, including shapes and text components

	// ****** Public Methods *****
public:
	EntityManager(sf::RenderWindow& window);	// Constructor - takes reference to SFML render window for drawing and FPS reporting
	
	void update(float deltaTime = 1.0f / 60.0f);																			// Main update method called each frame to update physics, handle collisions, render entities, and manage entity lifecycle. It takes the elapsed time since the last frame (deltaTime) as a parameter for time-based updates.
	void ReportFPS(int& fpsFrames, std::chrono::steady_clock::time_point& fpsLast, double& fpsSmooth, const double alpha);	// Updates the FPS counter and sets the window title to display the current FPS. It uses an exponential moving average to smooth the FPS value over time, with a configurable alpha parameter for smoothing factor.
	void DrawBoundingBox(const std::vector<BoundingBox>& bboxes);															// Debug method to draw bounding boxes for spatial partitioning visualization. It takes a vector of BoundingBox structures and draws them as rectangles on the SFML window for debugging purposes.
	Entity* addEntity(EntityType type, float radius, Vec3 color, Vec2 positon, Vec2 velocity, int alpha);					// Creates a new entity with the specified parameters and adds it to the pending addition queue. It takes the entity type, radius, color, initial position, velocity, and alpha value as parameters, creates a new Entity instance, and queues it for addition in the next update cycle to avoid iterator invalidation during game logic.
	EntityVector& getEntities();																							// Returns a reference to the vector of unique pointers for all active entities. This allows external systems (e.g. physics, collision, rendering) to access and iterate through the list of entities while maintaining ownership and memory management within EntityManager.
	std::vector<Entity*>& getEntities(EntityType type);																		// Returns a reference to the vector of raw pointers for entities of the specified type. This allows external systems to quickly access entities by type without needing to filter through the entire list. It takes an EntityType as a parameter and returns a reference to the corresponding vector in the entity map.
	int GetDeathCountThisFrame() const { return m_deathCountThisFrame; }													// Returns the number of deaths that occurred in the current frame, which is tracked by the EntityManager and can be used for game logic or UI display.
	void SpawnExplosion(const Vec2& position, float radius, const Vec2& velocity, const Vec3& color);						// Spawns an explosion entity at the specified position with the given radius, velocity, and color. It creates a new explosion entity, adds it to the pending addition queue, and tracks its creation time and color for fading out over time in the UpdateExplosions method.
	int GetExplosionCount() const { return static_cast<int>(m_explosionTimes.size()); }										// Returns the number of active explosions currently playing.

	// ***** Private Methods *****
private:
	void AddPendingEntities();							// Adds all entities from the pending addition queue to the active entity list and updates the spatial hash and entity map accordingly Called at the beginning of each update cycle to safely add new entities without invalidating iterators during game logic.
	void RemoveDeadEntities();							// Removes all entities that are marked as not alive from the active entity list, spatial hash, and entity map. Called at the beginning of each update cycle to safely remove destroyed entities without invalidating iterators during game logic.
	void UpdateSpatialHashAndRender();					// Updates the spatial hash grid with the current positions of all entities and renders them to the SFML window. It iterates through all active entities, updates their positions in the spatial hash if they have moved, and calls the RenderSystem to draw them on the window.
	void UpdateExplosions();							// Updates the state of all active explosions. It iterates through the tracked explosion entities, calculates their age based on their creation time, updates their color alpha for fading effect, and removes them if they have exceeded their lifespan.
	void DetectAndResolveCollisions(float deltaTime);	// Detects and resolves collisions between entities using the CollisionSystem. It queries the spatial hash for potential collisions, checks for actual collisions between entities, and applies the appropriate collision response (e.g. bouncing allies apart, spawning explosions on enemy collisions). It takes deltaTime as a parameter for time-based collision resolution if needed.
};
