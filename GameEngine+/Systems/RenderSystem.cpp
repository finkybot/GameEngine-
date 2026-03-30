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
	if (auto shape = entity->GetComponent<CShape>())
	{
		auto transform = entity->GetComponent<CTransform>();
		{
			if (transform)
			{
				shape->GetShape().setPosition(sf::Vector2f(transform->m_position.x, transform->m_position.y));
			}
		}
		window.draw(shape->GetShape());
	}
}

