// EntityManager.cpp
#include "EntityManager.h"
#include "Entity.h"
#include "EntityType.h"
#include "CCircle.h"
#include "CExplosion.h"
#include <algorithm>
#include <execution>
#include <chrono>
#include <cmath>
#include <sstream>
#include <unordered_set>
#include <iomanip>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Color.hpp>

// Constructor - takes reference to SFML render window for drawing and FPS reporting
EntityManager::EntityManager(sf::RenderWindow& window): m_window(window),  m_collisionSystem(this) {}


void EntityManager::AddPendingEntities()
{
	// Process all entities that were queued for addition
	// Using a deferred addition pattern prevents invalidating iterators during game logic
	for (auto& entity : m_toAdd)
	{
		Entity* entityPtr = entity.get();
		m_entities.push_back(std::move(entity));
		m_entityMap[entityPtr->GetType()].push_back(entityPtr);
		// Spatial hash will be rebuilt each frame
	}
	m_toAdd.clear();
}

void EntityManager::RemoveDeadEntities()
{
	// First collect pointers to dead entities WITHOUT deleting them yet.
	std::vector<Entity*> deadEntities;
	deadEntities.reserve(m_entities.size() / 10);

	for (const auto& up : m_entities)
	{
		if (!up->IsAlive())
			deadEntities.push_back(up.get());
	}

	if (deadEntities.empty())
		return;

	// Build a fast lookup set of dead pointers so we can remove references without dereferencing them.
	std::unordered_set<Entity*> deadSet(deadEntities.begin(), deadEntities.end());

	// Remove references to dead entities from the entity map by pointer identity only
	for (auto& deadEnt : m_entityMap)
	{
		auto& vec = deadEnt.second;
		vec.erase(std::remove_if(vec.begin(), vec.end(), [&deadSet](Entity* e) { return deadSet.find(e) != deadSet.end(); }), vec.end());
	}

	// If we had a spatial tree that stored raw pointers,then we would need remove the dead ones now....but..... I'm using a SpatialHashGrid which is rebuilt each frame so explicit removal is not required (Yeeeeaaa Me!!!).
	// AAAANNNYYWAY!!!!! I have included the cleanup for a tree; cleanup iterate over the deadEntities and remove each pointer from it:
	// for (Entity* d : deadEntities) m_quadTree.RemoveEntityFromTree(d);

	// It's is now safe to erase the owning unique_ptrs from m_entities and thus delete the objects.
	auto end = std::remove_if(m_entities.begin(), m_entities.end(), [](const std::unique_ptr<Entity>& e) { return !e->IsAlive(); });
	m_entities.erase(end, m_entities.end());
}

void EntityManager::UpdateSpatialHashAndRender()
{
	// Rebuild spatial hash every frame (very fast)
	m_spatialHash.Clear();
	for (auto& entity : m_entities)
	{
		m_spatialHash.Insert(entity.get());
	}

	m_renderSystem.RenderAliveEntities(m_entities, m_window);
}

void EntityManager::Update(float deltaTime)
{
	SpatialHashGrid<Entity>::ResetQueryStats();

	m_deathCountThisFrame = 0;

	AddPendingEntities();
	RemoveDeadEntities();

	UpdateSpatialHashAndRender();
}

Entity* EntityManager::addEntity(EntityType type)
{
	auto entity = std::unique_ptr<Entity>(new Entity(type, m_totalEntities++));
	Entity* entityPtr = entity.get();  // Capture pointer BEFORE moving
	m_toAdd.push_back(std::move(entity));
	
	return entityPtr;
}

Entity* EntityManager::addEntity(EntityType type, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha)
{
	auto entity = std::unique_ptr<Entity>(new Entity(type, m_totalEntities++));
	entity->m_creationTime = std::chrono::high_resolution_clock::now(); // Track creation time for entity (currently used for explosions but could be useful for other time-based logic in the future)

	entity->AddComponent<CTransform>(position, velocity);
	entity->AddComponent<CName>();

	if (EntityType::Explosion == type)
	{
		auto explosion = std::make_unique<CExplosion>();
		explosion->SetRadius(radius);
		explosion->SetColor(color.x, color.y, color.z, alpha);
		explosion->SetPosition(position.x, position.y);
		explosion->SetVelocity(velocity.x, velocity.y);
		entity->AddComponentPtr<CShape>(std::move(explosion));
	}
	else
	{
		auto circle = std::make_unique<CCircle>();
		circle->SetRadius(radius);
		circle->SetColor(color.x, color.y, color.z, alpha);
		circle->SetPosition(position.x, position.y);
		circle->SetVelocity(velocity.x, velocity.y);
		circle->SetVelocity(velocity.x, velocity.y);
		entity->AddComponentPtr<CShape>(std::move(circle));
	}


	Entity* entityPtr = entity.get();
	m_toAdd.push_back(std::move(entity));

	return entityPtr;
}

void EntityManager::KillEntity(Entity* entity)
{
	entity->Destroy();
	SetDeathCountThisFrame(GetDeathCountThisFrame() + 1);
}

EntityVector& EntityManager::getEntities()
{
	return m_entities;
}

std::vector<Entity*>& EntityManager::getEntities(EntityType type)
{
	return m_entityMap[type];
}


