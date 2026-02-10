#pragma once
#include <vector>
#include <memory>

namespace sf { class RenderWindow; }
class Entity;

/// <summary>
/// Render System - Draws all entities to the screen
/// Iterates through all alive entities and calls their DrawShape method
/// Also renders associated text if present
/// </summary>
class RenderSystem
{
public:
	/// <summary>Constructor</summary>
	RenderSystem() = default;
	
	/// <summary>Destructor</summary>
	~RenderSystem() = default;

	/// <summary>
	/// Render all alive entities to the render window
	/// </summary>
	/// <param name="entities">List of all entities in the game</param>
	/// <param name="window">SFML render window to draw to</param>
	void Render(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window);

private:
	/// <summary>
	/// Render a single entity (shape and text if present)
	/// </summary>
	void RenderEntity(Entity* entity, sf::RenderWindow& window) const;
};
