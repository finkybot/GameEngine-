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
	SpatialHashGrid<Entity>	m_spatialHash{ 100.0f };		// 100-unit cells for collision queries
	EntityVector			m_entities;						// Active entities currently in the game, owned by EntityManager
	EntityVector			m_toAdd;						// Entities queued for addition next frame to avoid iterator invalidation during game logic, owned by EntityManager
	EntityMap				m_entityMap;					// Map of entity type to vector of raw pointers for quick access by type, not owned (raw pointers) since EntityManager owns the entities
	size_t					m_totalEntities = 0;			// Total number of entities ever created, used for unique ID assignment
	sf::RenderWindow&		m_window;						// Reference to the SFML render window for drawing and updating the window title with FPS
	int						m_deathCountThisFrame = 0;		// Counter for tracking the number of deaths in the current frame
	
	
	// ***** Systems *****
	PhysicsSystem	m_physicsSystem;	// Handles entity movement and boundary collisions
	CollisionSystem	m_collisionSystem;	// Handles collision detection and resolution between entities, including spawning explosions on enemy collisions and bouncing allies apart
	RenderSystem	m_renderSystem;		// Handles rendering all entities to the SFML window, including shapes and text components

	// ****** Public Methods *****
public:
	EntityManager(sf::RenderWindow& window);	// Constructor - takes reference to SFML render window for drawing and FPS reporting
	
	PhysicsSystem GetPhysicsSystem() { return m_physicsSystem; }	// Getter for the PhysicsSystem, allowing external systems (e.g. TestScene) to access physics functionality for updating entity positions and handling explosion updates.
	CollisionSystem GetCollisionSystem() { return m_collisionSystem; }	// Getter for the CollisionSystem, allowing external systems (e.g. TestScene) to access collision detection and resolution functionality for handling interactions between entities.
	RenderSystem GetRenderSystem() { return m_renderSystem; }	// Getter for the RenderSystem, allowing external systems (e.g. TestScene) to access rendering functionality for drawing entities to the SFML window.


	void Update(float deltaTime = 1.0f / 60.0f);																				// Main update method, handles adding and removing entities along with updating physics, collisions, and rendering. It takes the delta time since the last update.
	Entity* addEntity(EntityType type);																							// Adds an existing entity (passed as a unique_ptr) to the pending addition queue. This is a flexible entity creation (e.g. creating an entity with specific components before adding it to the manager). It takes ownership of the provided entity and queues it for addition in the next update cycle to avoid iterator invalidation during game logic.
	Entity* addEntity(EntityType type, float radius, Vec3 color, Vec2 positon, Vec2 velocity, int alpha);						// Creates a new entity with the specified parameters and adds it to the pending addition queue. It takes the entity type, radius, color, initial position, velocity, and alpha value as parameters, creates a new Entity instance, and queues it for addition in the next update cycle to avoid iterator invalidation during game logic.
	
	void KillEntity(Entity* entity);																								// Marks an entity as not alive, which will be removed in the next update cycle. It takes a pointer to the	
	
	
	EntityVector& getEntities();																								// Returns a reference to the vector of unique pointers for all active entities. This allows external systems (e.g. physics, collision, rendering) to access and iterate through the list of entities while maintaining ownership and memory management within EntityManager.
	std::vector<Entity*>& getEntities(EntityType type);																			// Returns a reference to the vector of raw pointers for entities of the specified type. This allows external systems to quickly access entities by type without needing to filter through the entire list. It takes an EntityType as a parameter and returns a reference to the corresponding vector in the entity map.
	int GetDeathCountThisFrame() const { return m_deathCountThisFrame; }														// Returns the number of deaths that occurred in the current frame, which is tracked by the EntityManager and can be used for game logic or UI display.
	SpatialHashGrid<Entity>& GetSpatialHash() { return m_spatialHash; }															// Getter for the spatial hash grid, allowing external systems (e.g. CollisionSystem) 

	void SetDeathCountThisFrame(int count) { m_deathCountThisFrame = count; }													// Setter for the death count this frame, allowing external systems (e.g. TestScene) to reset or update the death count as needed for game logic or UI display.

	// ***** Private Methods *****
private:
	void AddPendingEntities();							// Adds all entities from the pending addition queue to the active entity list and update the spatial hash & entity map; should be called at the beginning of each update cycle.
	void RemoveDeadEntities();							// Removes all entities that are marked as not alive from the active entity list, spatial hash, and entity map; Call this at the beginning of each update cycle to safely remove destroyed entities without invalidating iterators during gam... yada yada yada.
	void UpdateSpatialHashAndRender();					// Updates the spatial hash grid with the current positions of all entities and renders them to the SFML window.
};
