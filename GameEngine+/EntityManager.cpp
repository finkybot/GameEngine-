// EntityManager.cpp
#include "EntityManager.h"
#include "Entity.h"
#include "EntityType.h"
#include "CCircle.h"
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
EntityManager::EntityManager(sf::RenderWindow& window)
	: m_window(window),
	  m_collisionSystem(this)
{
}

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

void EntityManager::DetectAndResolveCollisions(float deltaTime)
{
	float windowWidth = static_cast<float>(m_window.getSize().x);
	float windowHeight = static_cast<float>(m_window.getSize().y);
	m_physicsSystem.Update(m_entities, deltaTime, windowWidth, windowHeight);

	m_deathCountThisFrame += m_collisionSystem.DetectAndResolve(m_entities, m_spatialHash, deltaTime);

    // Boundary handling and off-screen despawn is performed in PhysicsSystem::HandleBoundaryCollision
}

void EntityManager::update(float deltaTime)
{
	static auto fpsLast = std::chrono::steady_clock::now();
	static int fpsFrames = 0;
	static double fpsSmooth = 0.0;
	static constexpr double alpha = 0.15;

	SpatialHashGrid<Entity>::ResetQueryStats();

	m_deathCountThisFrame = 0;

	AddPendingEntities();
	RemoveDeadEntities();

	UpdateSpatialHashAndRender();

	UpdateExplosions();

	DetectAndResolveCollisions(deltaTime);

	ReportFPS(fpsFrames, fpsLast, fpsSmooth, alpha);
}

void EntityManager::ReportFPS(int& fpsFrames, std::chrono::steady_clock::time_point& fpsLast, double& fpsSmooth, const double alpha)
{
	++fpsFrames;
	auto fpsNow = std::chrono::steady_clock::now();
	auto fpsElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(fpsNow - fpsLast);
	if (fpsElapsed.count() >= 1.0)
	{
		double currentFps = static_cast<double>(fpsFrames) / fpsElapsed.count();
		if (fpsSmooth <= 0.0)
		{
			fpsSmooth = currentFps;
		}
		else
		{
			fpsSmooth = (alpha * currentFps) + ((1.0 - alpha) * fpsSmooth);
		}

		std::stringstream ss;
		ss << std::fixed << std::setprecision(1) << fpsSmooth;
		std::string title = "FPS: " + ss.str();
		m_window.setTitle(title);

		fpsFrames = 0;
		fpsLast = fpsNow;
	}
}

void EntityManager::DrawBoundingBox(const std::vector<BoundingBox>& bboxes)
{
	for (const auto& bbox : bboxes)
	{
		sf::RectangleShape rect;
		rect.setPosition(sf::Vector2f(bbox.topLeft.GetX(), bbox.topLeft.GetY()));
		rect.setSize(sf::Vector2f(bbox.bottomRight.GetX() - bbox.topLeft.GetX(), bbox.bottomRight.GetY() - bbox.topLeft.GetY()));
		rect.setFillColor(sf::Color::Transparent);
		rect.setOutlineColor(sf::Color::Green);
		rect.setOutlineThickness(1.0f);
		m_window.draw(rect);
	}
}

Entity* EntityManager::addEntity(EntityType type, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha)
{
	auto entity = std::unique_ptr<Entity>(new Entity(type, m_totalEntities++));

	entity->AddComponent<CTransform>(position, velocity);
	entity->AddComponent<CName>();

	auto circle = std::make_unique<CCircle>();
	circle->SetRadius(radius);
	circle->SetColor(color.x, color.y, color.z, alpha);
	circle->SetPosition(position.x, position.y);
	circle->SetVelocity(velocity.x, velocity.y);
	circle->SetVelocity(velocity.x, velocity.y);
	entity->AddComponentPtr<CShape>(std::move(circle));

	Entity* entityPtr = entity.get();
	m_toAdd.push_back(std::move(entity));

	return entityPtr;
}

EntityVector& EntityManager::getEntities()
{
	return m_entities;
}

std::vector<Entity*>& EntityManager::getEntities(EntityType type)
{
	return m_entityMap[type];
}

void EntityManager::SpawnExplosion(const Vec2& position, float radius, const Vec2& velocity, const Vec3& color)
{
	auto explosionEntity = addEntity(
		EntityType::Explosion,
		radius,
		color,
		position,
		velocity,
		200
	);
	
	m_explosionTimes[explosionEntity->m_id] = std::chrono::high_resolution_clock::now();
	m_explosionColors[explosionEntity->m_id] = color;
}

void EntityManager::UpdateExplosions()
{
	auto now = std::chrono::high_resolution_clock::now();
	std::vector<size_t> expiredExplosions;
	
	for (auto& [explosionId, creationTime] : m_explosionTimes)
	{
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - creationTime);
		
		if (elapsed.count() > 600)
		{
			for (auto& entity : m_entities)
			{
				if (entity->m_id == explosionId)
				{
					entity->Destroy();
					break;
				}
			}
			expiredExplosions.push_back(explosionId);
		}
		else
		{
			float fadeProgress = static_cast<float>(elapsed.count()) / 600.0f;
			int newAlpha = static_cast<int>(200 * (1.0f - fadeProgress));
			
			for (auto& entity : m_entities)
			{
				if (entity->m_id == explosionId)
				{
					auto shape = entity->GetComponent<CShape>();
					if (shape)
					{
						if (auto* circle = dynamic_cast<CCircle*>(shape))
						{
							circle->SetRadius(circle->GetRadius() + 0.5f); // Expand the explosion radius over time
							Vec2 explosionPosition = circle->GetPosition();
							circle->SetPosition(explosionPosition.x + 0.4f, explosionPosition.y - 0.5f); // Keep the explosion centered as it expands, adding a little drift for visual interest
							sf::Color currentColor = circle->GetColor();
							circle->SetColor(
								static_cast<float>(currentColor.r),
								static_cast<float>(currentColor.g),
								static_cast<float>(currentColor.b),
								newAlpha
							);
						}
					}
				}
			}
		}
	}
	
	for (size_t explosionId : expiredExplosions)
	{
		m_explosionTimes.erase(explosionId);
		m_explosionColors.erase(explosionId);
	}
}
