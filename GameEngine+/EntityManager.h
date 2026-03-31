// ***** EntityManager.h - EntityManager class definition *****
#pragma once

#include <vector>
#include <memory>
#include <map>
#include <string>
#include <chrono>

#include "SpatialHashGrid.h"
#include "Vec2.h"
#include "EntityType.h"
#include "Raycast.h"
#include "Systems/PhysicsSystem.h"
#include "Systems/CollisionSystem.h"
#include "Systems/RenderSystem.h"
#include "CTileMap.h"
class TileSystem;
#include <memory>

// Forward Declarations
namespace sf { class RenderWindow; }
class Entity;

using EntityVector = std::vector<std::unique_ptr<Entity>>;
using EntityMap = std::map<EntityType, std::vector<Entity*>>;

class EntityManager
{
public:
    explicit EntityManager(sf::RenderWindow& window, float cellSize = 32.0f);
    ~EntityManager() = default;

    void Update(float deltaTime = 1.0f / 60.0f);

    Entity* addEntity(EntityType type);
    void KillEntity(Entity* entity);

    EntityVector& getEntities();
    std::vector<Entity*>& getEntities(EntityType type);

    SpatialHashGrid<Entity>& GetSpatialHash();

    int GetDeathCountThisFrame() const { return m_deathCountThisFrame; }
    void SetDeathCountThisFrame(int count) { m_deathCountThisFrame = count; }

    PhysicsSystem GetPhysicsSystem() { return m_physicsSystem; }
    CollisionSystem GetCollisionSystem() { return m_collisionSystem; }
    RenderSystem GetRenderSystem() { return m_renderSystem; }

    void AddTileMapAsEntities(const TileMap& map, int tileValueToTreatAsSolid = 1);
    // Preferred: create a CTileMap entity which will be processed by TileSystem
    Entity* CreateTileMapEntity(const TileMap& map);
    // When creating/updating CTileMap components set this flag so TileSystem knows work is pending
    void SetHasPendingTileMaps(bool v) { m_hasPendingTileMaps = v; }
    bool HasPendingTileMaps() const { return m_hasPendingTileMaps; }

private:
    void AddPendingEntities();
    void RemoveDeadEntities();
    void UpdateSpatialHashAndRender();

private:
    SpatialHashGrid<Entity> m_spatialHash;
    EntityVector m_entities;
    EntityVector m_toAdd;
    EntityMap m_entityMap;
    size_t m_totalEntities = 0;
    sf::RenderWindow& m_window;
    int m_deathCountThisFrame = 0;

    PhysicsSystem m_physicsSystem;
    CollisionSystem m_collisionSystem;
    RenderSystem m_renderSystem;
    std::unique_ptr<TileSystem> m_tileSystem;
    bool m_hasPendingTileMaps = true;
};
      