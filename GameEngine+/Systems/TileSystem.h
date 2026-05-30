/////////////////////////////////
// TileSystem.h: defines the TileSystem class, which processes entities with CTileMap components and creates static collider entities based on the tile map data. This system is responsible for generating colliders for solid tiles in the tile map, 
// allowing for collision detection and response in the game world.
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "../Entity.h"
#include "../CTileMap.h"
#include "../EntityManager.h"
/////////////////////////////////
 
 

/////////////////////////////////
// TileSystem: processes entities with CTileMap and creates static collider entities
class TileSystem {
	/////////////////////////////////
	// Public interface
public:
	/////////////////////////////////
	// Constructor and destructor for the TileSystem class. The constructor takes a pointer to the EntityManager, which is used to access entities and create new collider entities based on the tile map data. 
	// The destructor can be defaulted since we don't have any special cleanup logic, but we can implement it if needed in the future.
	explicit TileSystem(EntityManager* manager) : m_entityManager(manager) {}
	~TileSystem() = default;
	/////////////////////////////////



	/////////////////////////////////
	// Process method for the TileSystem class. This method iterates through all entities with a CTileMap component, checks for solid tiles, and creates static collider entities for those tiles. 
	// It serves as the main entry point for processing tile maps and generating colliders in the game loop.
	void Process();
	/////////////////////////////////



	/////////////////////////////////
	// Private members
private:
	/////////////////////////////////
	// Pointer to the EntityManager, which is used to access entities and create new collider entities based on the tile map data. This allows the TileSystem to interact with the overall entity management system 
	// of the game engine and ensure that colliders are properly integrated into the game world.
	EntityManager* m_entityManager;
	/////////////////////////////////
};
/////////////////////////////////
