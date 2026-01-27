#pragma once
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <chrono>

// Forward declaration for SFML type used by reference in this header
namespace sf { class RenderWindow; }

#include "QuadTree.h"
#include "Vec2.h"

// Forward Declarations
class Entity;

typedef std::vector<std::shared_ptr<Entity>> EntityVector;
typedef std::map<std::string, EntityVector> EntityMap;

class EntityManager
{
private:
	QuadTree<Entity>	m_quadTree;
	EntityVector		m_entities;
	EntityVector		m_toAdd;
	EntityMap			m_entityMap;
	size_t				m_totalEntities = 0;
	sf::RenderWindow&	m_window;
	char m_fpsTitle[64] = "FPS: 0.0";
	int					m_deathCountThisFrame = 0;
	
	// Track explosion creation times: entity_id -> creation_time
	std::map<size_t, std::chrono::high_resolution_clock::time_point> m_explosionTimes;


public:
	EntityManager(sf::RenderWindow& window);
	
	void update();

	void ReportFPS(int& fpsFrames, std::chrono::steady_clock::time_point& fpsLast, double& fpsSmooth, const double alpha);

	void DrawBoundingBox(const std::vector<BoundingBox>& bboxes);

	std::shared_ptr<Entity> addEntity(const std::string& tag, float radius, Vec3 color, Vec2 positon, Vec2 velocity, int alpha);
	EntityVector& getEntities();
	EntityVector& getEntities(const std::string& tag);
	int GetDeathCountThisFrame() const { return m_deathCountThisFrame; }
	
	// Spawn an explosion effect at a given position
	void SpawnExplosion(const Vec2& position, float radius = 15.0f, const Vec2& velocity = Vec2(0, 0));
	int GetExplosionCount() const { return static_cast<int>(m_explosionTimes.size()); }

private:
void AddPendingEntities();
void RemoveDeadEntities();
void UpdateQuadTreeAndRender();
void UpdateExplosions();
void DetectAndResolveCollisions();
bool AreEnemies(Entity* entity1, Entity* entity2);
};

