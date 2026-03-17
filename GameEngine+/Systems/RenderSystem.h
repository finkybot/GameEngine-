// ***** RenderSystem.h - Render system for drawing entities *****
#pragma once
#include <vector>
#include <memory>

namespace sf { class RenderWindow; }
class Entity;

// RenderSystem is responsible for rendering all alive entities to the SFML render window. It iterates through the list of entities, checks if they are alive, and draws their shapes and text components if present.
class RenderSystem
{
	// ****** Public Methods *****
public:
	RenderSystem() = default;	// Constructor - default is fine since we have no member variables to initialize
	~RenderSystem() = default;	// Destructor - default is fine since we have no resources to clean up

	void RenderAliveEntities(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window); 	// Renders all alive entities to the provided SFML render window (why was you missing????)
	
	// ****** Private Methods *****
private:
	void RenderEntity(Entity* entity, sf::RenderWindow& window) const;	// Helper function that renders a single entity (shape and text if present), called by the public Render method.
};
