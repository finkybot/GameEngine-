/////////////////////////////////
// MouseSystem.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "EntityManager.h"
/////////////////////////////////



/////////////////////////////////
//	|	MouseSystem class - ECS system that processes CMouseState components and updates their state based on the current mouse input. It iterates over all entities with a CMouseState component, reads the current mouse position and button states, and updates the component's data accordingly.
// 	|	This system allows for efficient mouse input handling in a 2D game engine by leveraging the EntityManager to access entities and their components, and the SFML RenderWindow to retrieve the current mouse state.
//	|_______________________________________________________________________
class MouseSystem {
public:
	MouseSystem(EntityManager* entityManager, sf::RenderWindow* window)
		: m_entityManager(entityManager), m_window(window) {}

	void Update(float dt);

private:
	EntityManager* m_entityManager = nullptr;
	sf::RenderWindow* m_window = nullptr;
};
/////////////////////////////////
