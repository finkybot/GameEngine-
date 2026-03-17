// RenderSystem.cpp
#include "RenderSystem.h"
#include "../Entity.h"
#include "../CShape.h"
#include <SFML/Graphics/RenderWindow.hpp>

void RenderSystem::RenderAliveEntities(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window)
{
	for (const auto& entity : entities)
	{
		if (!entity->IsAlive())
			continue;

		RenderEntity(entity.get(), window);
	}
}

void RenderSystem::RenderEntity(Entity* entity, sf::RenderWindow& window) const
{
	auto shape = entity->GetComponent<CShape>();
	if (shape)
	{
		window.draw(shape->GetShape());
	}
}

