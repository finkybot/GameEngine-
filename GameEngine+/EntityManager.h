// ***** EntityManager.h - EntityManager class definition *****
#pragma once

// Forward Declarations
class TileSystem;   // Forward declaration of TileSystem to avoid circular dependency with EntityManager, since EntityManager will have a unique_ptr to TileSystem and TileSystem will need to access EntityManager for managing tile entities. This allows us to use pointers to TileSystem in EntityManager without needing the full definition of TileSystem at this point, which helps to reduce compilation dependencies and improve build times.
class MusicSystem;  // forward declare MusicSystem
namespace sf { class RenderWindow; }
class Entity;

// Include necessary headers for SFML, standard library containers, and other components used by EntityManager
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

// Type aliases (for convenience and readability)
using EntityVector = std::vector<std::unique_ptr<Entity>>;
using EntityMap = std::map<EntityType, std::vector<Entity*>>;


class EntityManager
{
public:
    explicit EntityManager(sf::RenderWindow& window, float cellSize = 32.0f);
    ~EntityManager();

    void Update(float deltaTime = 1.0f / 60.0f);
    void RenderShapes();
    void RenderText();
    void RenderAll(RenderSystem::RenderMode mode = RenderSystem::RenderMode::ShapesThenText);

    Entity* addEntity(EntityType type);
    void KillEntity(Entity* entity);

    EntityVector& getEntities();
    std::vector<Entity*>& getEntities(EntityType type);

    SpatialHashGrid<Entity>& GetSpatialHash();

    int GetDeathCountThisFrame() const { return m_deathCountThisFrame; }
    void SetDeathCountThisFrame(int count) { m_deathCountThisFrame = count; }

    PhysicsSystem GetPhysicsSystem() { return m_physicsSystem; }
    CollisionSystem GetCollisionSystem() { return m_collisionSystem; }
    RenderSystem& GetRenderSystem() { return m_renderSystem; }

    void AddTileMapAsEntities(const TileMap& map, int tileValueToTreatAsSolid = 1);
    Entity* CreateTileMapEntity(const TileMap& map);                                                // Preferred: create a CTileMap entity which will be processed by TileSystem
    
    void SetHasPendingTileMaps(bool v) { m_hasPendingTileMaps = v; }                                // When creating/updating CTileMap components set this flag so TileSystem knows work is pending
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
    std::unique_ptr<MusicSystem> m_musicSystem; // system owning runtime sf::Music objects
    bool m_hasPendingTileMaps = true;
};