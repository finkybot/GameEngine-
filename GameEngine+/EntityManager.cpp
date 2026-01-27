#include "EntityManager.h"
#include "Entity.h"
#include "CCircle.h"
#include "imgui/imgui.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Color.hpp>

EntityManager::EntityManager(sf::RenderWindow& window)
	: m_quadTree(BoundingBox(Vec2(0, 0), Vec2(window.getSize().x, window.getSize().y)), 16, 0, 5),
	  m_window(window)
{
}

void EntityManager::AddPendingEntities()
{
	for (auto& entity : m_toAdd)
	{
		m_entities.push_back(entity);
		m_entityMap[entity->GetTag()].push_back(entity);
	}
	m_toAdd.clear();
}

void EntityManager::RemoveDeadEntities()
{
	std::vector<Entity*> deadEntities;
	
	auto end = std::remove_if(m_entities.begin(), m_entities.end(), 
		[&deadEntities](const auto e) { 
			if (!e->IsAlive())
			{
				deadEntities.push_back(e.get());
				return true;
			}
			return false;
		});
	m_entities.erase(end, m_entities.end());

	for (auto& itr : m_entityMap)
	{
		auto end = std::remove_if(itr.second.begin(), itr.second.end(),
			[](const auto e) { return !e->IsAlive(); });
		itr.second.erase(end, itr.second.end());
	}

	for (Entity* deadEntity : deadEntities)
	{
		m_quadTree.RemoveEntityFromTree(deadEntity);
	}
}

bool EntityManager::AreEnemies(Entity* entity1, Entity* entity2)
{
	return entity1->GetTag() != entity2->GetTag();
}

void EntityManager::UpdateQuadTreeAndRender()
{
	static size_t lastEntityCount = 0;
	static int framesSinceRebuild = 0;
	
	bool shouldRebuild = false;
	
	if (m_entities.size() != lastEntityCount)
	{
		shouldRebuild = true;
		lastEntityCount = m_entities.size();
		framesSinceRebuild = 0;
	}
	else if (++framesSinceRebuild >= 30)
	{
		shouldRebuild = true;
		framesSinceRebuild = 0;
	}
	else if (QuadTree<Entity>::GetAverageObjectsPerQuery() > 25.0)
	{
		shouldRebuild = true;
		framesSinceRebuild = 0;
	}

	if (shouldRebuild)
	{
		m_quadTree.ClearTree();
		for (auto entity = m_entities.begin(); entity != m_entities.end(); ++entity)
		{
			m_quadTree.Insert(entity->get());
			entity->get()->m_previousPosition = entity->get()->GetPosition();
		}
	}
	else
	{
		for (auto entity = m_entities.begin(); entity != m_entities.end(); ++entity)
		{
			Entity* e = entity->get();
			const Vec2& currentPos = e->GetPosition();
			if ((currentPos.GetX() != e->m_previousPosition.GetX()) || 
				(currentPos.GetY() != e->m_previousPosition.GetY()))
			{
				m_quadTree.UpdatePosition(e);
				e->m_previousPosition = currentPos;
			}
		}
	}

	for (auto entity = m_entities.begin(); entity != m_entities.end(); ++entity)
	{
		entity->get()->cShape->DrawShape(m_window);
	}
}

void EntityManager::DetectAndResolveCollisions()
{
	static std::vector<Entity*> foundRaw;
	static size_t lastEntityCount = 0;
	
	if (m_entities.size() != lastEntityCount && !m_entities.empty())
	{
		foundRaw.reserve(std::max(size_t(16), m_entities.size() / 100));
		lastEntityCount = m_entities.size();
	}
	
	for (auto iterator = m_entities.begin(); iterator != m_entities.end(); ++iterator)
	{
		Entity* currentEntity = iterator->get();
		
		if (!currentEntity->IsAlive())
			continue;
		
		// Skip collision detection for explosions - they're visual effects only
		if (currentEntity->GetTag() == "Explosion")
			continue;

		const Vec2& position = currentEntity->GetPosition();
		float width = currentEntity->GetWidth();
		float height = currentEntity->GetHeight();

		BoundingBox rect(
			Vec2(position - Vec2(width * 1.5f, height * 1.5f)),
			Vec2(position + Vec2(width * 1.5f, height * 1.5f))
		);

		foundRaw.clear();
		QuadTree<Entity>::IncrementQueryCount();
		m_quadTree.Query(foundRaw, rect, currentEntity);

		for (Entity* entityPtr : foundRaw)
		{
			if (!currentEntity->Intersects(entityPtr))
				continue;
			
			// Skip if other entity is an explosion
			if (entityPtr->GetTag() == "Explosion")
				continue;

			if (AreEnemies(currentEntity, entityPtr))
			{
			// Spawn explosion at collision point
			// Use the velocity of the faster moving entity for momentum-based drift
			Vec2 currentVel = Vec2(currentEntity->cShape->GetVelocity().x, currentEntity->cShape->GetVelocity().y);
			Vec2 otherVel = Vec2(entityPtr->cShape->GetVelocity().x, entityPtr->cShape->GetVelocity().y);
			
			float currentSpeed = std::sqrt(currentVel.x * currentVel.x + currentVel.y * currentVel.y);
			float otherSpeed = std::sqrt(otherVel.x * otherVel.x + otherVel.y * otherVel.y);
			
			Vec2 explosionVelocity = (currentSpeed >= otherSpeed) ? currentVel : otherVel;
			
			Vec2 collisionPoint = (currentEntity->GetPosition() + entityPtr->GetPosition()) * 0.5f;
			SpawnExplosion(collisionPoint, 20.0f, explosionVelocity);
				
				currentEntity->Destroy();
				entityPtr->Destroy();
				m_deathCountThisFrame += 2;
			}
			else
			{
				currentEntity->Bounce(entityPtr);
			}
		}

		currentEntity->cShape->Includer(m_window);
		currentEntity->cShape->MoveShape();
	}
}

void EntityManager::update()
{
	static auto fpsLast = std::chrono::steady_clock::now();
	static int fpsFrames = 0;
	static double fpsSmooth = 0.0;
	static constexpr double alpha = 0.15;

	m_window.clear();

	QuadTree<Entity>::ResetQueryStats();
	
	m_deathCountThisFrame = 0;

	AddPendingEntities();
	RemoveDeadEntities();

	UpdateQuadTreeAndRender();
	
	UpdateExplosions();

	DetectAndResolveCollisions();

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
			fpsSmooth = alpha * currentFps + (1.0 - alpha) * fpsSmooth;	
		}

		snprintf(m_fpsTitle, sizeof(m_fpsTitle), "FPS: %.1f", fpsSmooth);
		m_window.setTitle(m_fpsTitle);
		fpsFrames = 0;
		fpsLast = fpsNow;
	}
}

void EntityManager::DrawBoundingBox(const std::vector<BoundingBox>& bboxes)
{
	for (const auto& bbox : bboxes)
	{
		sf::RectangleShape line1(sf::Vector2f(bbox.GetWidth(), 1.f));
		line1.setPosition({ bbox.GetTopLeftPoint().GetX(), bbox.GetTopLeftPoint().GetY() });
		line1.setFillColor(sf::Color(120, 130, 130, 127));
		m_window.draw(line1);

		sf::RectangleShape line2(sf::Vector2f(1.f, bbox.GetHeight()));
		line2.setPosition({ bbox.GetBottomRightPoint().GetX(), bbox.GetTopLeftPoint().GetY() });
		line2.setFillColor(sf::Color(60, 190, 130, 127));
		m_window.draw(line2);

		sf::RectangleShape line3(sf::Vector2f(bbox.GetWidth(), 1.f));
		line3.setPosition({ bbox.GetTopLeftPoint().GetX(), bbox.GetBottomRightPoint().GetY() });
		line3.setFillColor(sf::Color(200, 190, 60, 127));
		m_window.draw(line3);

		sf::RectangleShape line4(sf::Vector2f(1.f, bbox.GetHeight()));
		line4.setPosition({ bbox.GetTopLeftPoint().GetX(), bbox.GetTopLeftPoint().GetY() });
		line4.setFillColor(sf::Color(60, 230, 160, 127));
		m_window.draw(line4);
	}
}

std::shared_ptr<Entity> EntityManager::addEntity(const std::string& tag, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha)
{
	auto e = std::shared_ptr<Entity>(new Entity(tag, m_totalEntities++));
	auto circle = std::make_unique<CCircle>();

	circle->SetColor(color.x, color.y, color.z, alpha);
	circle->SetVelocity(velocity.x, velocity.y);
	circle->SetPosition(position.x, position.y);
	circle->SetRadius(radius);

	e->cShape = std::move(circle);
	e->SetMidLength(radius + 1);

	m_toAdd.push_back(e);
	m_quadTree.Insert(e.get());
	return e;
}

EntityVector& EntityManager::getEntities()
{
	return m_entities;
}

EntityVector& EntityManager::getEntities(const std::string& tag)
{
	return m_entityMap[tag];
}

void EntityManager::SpawnExplosion(const Vec2& position, float radius, const Vec2& velocity)
{
	// Spawn a bright white circle explosion
	// 20% bigger than max ball radius (3.0 * 1.2 = 3.6)
	float explosionRadius = 3.6f;
	// Dampen the velocity so explosions drift gently (30% of collision momentum)
	Vec2 driftVelocity = velocity * 0.3f;
	
	auto explosionEntity = addEntity("Explosion", explosionRadius, Vec3(255, 255, 255), position, driftVelocity, 255);
	
	// Track this explosion's creation time for fade effect
	m_explosionTimes[m_totalEntities - 1] = std::chrono::high_resolution_clock::now();
}

void EntityManager::UpdateExplosions()
{
	// Update explosion fading and growing/shrinking
	auto now = std::chrono::high_resolution_clock::now();
	const float FADE_DURATION = 1.0f;  // 1 second for punchy effect
	const float MIN_RADIUS = 3.6f;     // Starting radius
	const float MAX_RADIUS = 7.0f;     // Max radius at midpoint
	const float MIN_ALPHA = 100.0f;    // Don't fade completely to black - stay more visible
	const float MAX_ALPHA = 255.0f;    // Start fully opaque
	
	// Update all explosions
	for (auto& entity : m_entities)
	{
		if (entity->GetTag() == "Explosion" && entity->IsAlive())
		{
			// Find creation time for this explosion
			auto it = m_explosionTimes.find(entity->m_id);
			if (it == m_explosionTimes.end())
				continue;
			
			float elapsed = std::chrono::duration<float>(now - it->second).count();
			
			// Check if fade is complete
			if (elapsed >= FADE_DURATION)
			{
				entity->Destroy();
				continue;
			}
			
			// Calculate progress (0 to 1)
			float progress = elapsed / FADE_DURATION;
			
		// Alpha: fade from MAX_ALPHA to MIN_ALPHA (stays brighter)
		int currentAlpha = static_cast<int>(MAX_ALPHA * (1.0f - progress) + MIN_ALPHA * progress);
		currentAlpha = std::max(static_cast<int>(MIN_ALPHA), std::min(static_cast<int>(MAX_ALPHA), currentAlpha));
			
			// Radius: grow in first half, shrink in second half
			float radiusProgress;
			if (progress < 0.5f)
			{
				// Growing phase (0 to 0.5)
				radiusProgress = progress * 2.0f;  // 0 to 1
			}
			else
			{
				// Shrinking phase (0.5 to 1.0)
				radiusProgress = 1.0f - ((progress - 0.5f) * 2.0f);  // 1 to 0
			}
			
			float currentRadius = MIN_RADIUS + (MAX_RADIUS - MIN_RADIUS) * radiusProgress;
			
			// Apply changes to the explosion circle
			CCircle* circle = dynamic_cast<CCircle*>(entity->cShape.get());
			if (circle)
			{
				circle->SetColor(255, 255, 255, currentAlpha);
				circle->SetRadius(currentRadius);
				entity->SetMidLength(currentRadius + 1);
			}
		}
	}
	
	// Clean up expired timers
	std::vector<size_t> toRemove;
	for (auto& explosionEntry : m_explosionTimes)
	{
		float elapsed = std::chrono::duration<float>(now - explosionEntry.second).count();
		if (elapsed >= FADE_DURATION)
		{
			toRemove.push_back(explosionEntry.first);
		}
	}
	
	for (auto id : toRemove)
	{
		m_explosionTimes.erase(id);
	}
}
