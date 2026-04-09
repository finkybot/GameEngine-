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
#include "CRectangle.h"
#include "CStatic.h"
#include "Systems/TileSystem.h"

// Constructor - takes reference to SFML render window for drawing and FPS reporting
EntityManager::EntityManager(sf::RenderWindow& window, float cellSize)
	: m_window(window)
	, m_spatialHash(cellSize)
	, m_collisionSystem(this)
{
    // initialize systems that need the entity manager pointer
    m_tileSystem = std::make_unique<TileSystem>(this);

	// If the engine owns a FontManager, bind it to the render system later via caller.
}

EntityManager::~EntityManager()
{
	// ensure proper destruction order for forward-declared types
	m_tileSystem.reset();
}

Entity* EntityManager::CreateTileMapEntity(const TileMap& map)
{
	Entity* e = addEntity(EntityType::TileMap);
	if (!e) return nullptr;

	// Attach the tilemap component which TileSystem will process on next Update
    // copy entire TileMap (including tileset metadata) so TileSystem has full information
	e->AddComponent<CTileMap>(map);
    // mark manager flag so TileSystem will be processed on next update
	m_hasPendingTileMaps = true;
	return e;
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

void EntityManager::AddTileMapAsEntities(const TileMap& map, int tileValueToTreatAsSolid)
{
	if (map.width <= 0 || map.height <= 0) return;
    // Ensure spatial hash cell size matches tile size for optimal alignment and query accuracy
	// Recreate the spatial hash with the tile size so tiles map 1:1 to cells when possible
	m_spatialHash = SpatialHashGrid<Entity>(map.tileSize);
	
	// 2D greedy rectangle merging: create maximal rectangles of contiguous solid tiles	
	std::vector<char> used(map.width * map.height, 0);
	for (int y = 0; y < map.height; ++y)
	{
		for (int x = 0; x < map.width; ++x)
		{
			int idx = y * map.width + x;
			if (used[idx]) continue;
			if (!map.IsSolid(x, y)) continue;

			// determine maximal width
			int w = 1;
			while (x + w < map.width && map.IsSolid(x + w, y) && !used[y * map.width + (x + w)]) ++w;

			// determine maximal height we can extend where every row has the same solid run
			int h = 1;
			bool canExtend = true;
			while (y + h < map.height && canExtend)
			{
				for (int xi = 0; xi < w; ++xi)
				{
					if (!map.IsSolid(x + xi, y + h) || used[(y + h) * map.width + (x + xi)])
					{
						canExtend = false;
						break;
					}
				}
				if (canExtend) ++h;
			}

			// mark used
			for (int yy = 0; yy < h; ++yy)
				for (int xx = 0; xx < w; ++xx)
					used[(y + yy) * map.width + (x + xx)] = 1;

			float tileW = map.tileSize * w;
			float tileH = map.tileSize * h;
			float posX = x * map.tileSize;
			float posY = y * map.tileSize;

            Entity* e = addEntity(EntityType::TileMap);
            if (e)
			{
				// When creating tile entities through AddTileMapAsEntities we mark them as static colliders.
				e->AddComponent<CTransform>(Vec2(posX, posY), Vec2(0.0f, 0.0f));
				auto rect = std::make_unique<CRectangle>(tileW, tileH);
				rect->SetColor(160.0f, 160.0f, 160.0f, 200);
				e->AddComponentPtr<CShape>(std::move(rect));
				e->AddComponent<CStatic>();
			}
		}
	}
}

void EntityManager::UpdateSpatialHashAndRender()
{
	// Rebuild spatial hash every frame (very fast)
	m_spatialHash.Clear();
	for (auto& entity : m_entities)
	{
		m_spatialHash.Insert(entity.get());
	}

	// Rendering is now controlled explicitly by the engine's render pass.
}

// Render entities' shapes. The engine should call this during its render phase before scene overlays if desired.
void EntityManager::RenderShapes()
{
	m_renderSystem.RenderShapes(m_entities, m_window);
}

// Render entities' text. The engine should call this after scene overlays if desired.
void EntityManager::RenderText()
{
	m_renderSystem.RenderText(m_entities, m_window);
}

void EntityManager::RenderAll(RenderSystem::RenderMode mode)
{
	m_renderSystem.RenderAll(m_entities, m_window, mode);
}

void EntityManager::Update(float deltaTime)
{
	SpatialHashGrid<Entity>::ResetQueryStats();

	m_deathCountThisFrame = 0;

	AddPendingEntities();
	RemoveDeadEntities();

    // Process tilemaps into tile entities before rebuilding spatial hash
    if (m_tileSystem && m_hasPendingTileMaps)
	{
		m_tileSystem->Process(); // Process pending tilemaps
		m_hasPendingTileMaps = false;
		AddPendingEntities(); // Add any new tile entities created by TileSystem
	}

	UpdateSpatialHashAndRender();
}

Entity* EntityManager::addEntity(EntityType type)
{
	auto entity = std::unique_ptr<Entity>(new Entity(type, m_totalEntities++));
	entity->m_creationTime = std::chrono::high_resolution_clock::now(); // Track creation time for entity (currently used for explosions but could be useful for other time-based logic in the future)
    // Ensure new entities have a transform so systems can rely on it
	entity->AddComponent<CTransform>(Vec2::Zero, Vec2::Zero);
	Entity* entityPtr = entity.get();  // Capture pointer BEFORE moving
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

SpatialHashGrid<Entity>& EntityManager::GetSpatialHash()
{
	return m_spatialHash;
}


