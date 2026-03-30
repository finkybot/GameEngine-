#pragma once
#include "../Entity.h"
#include "../CTileMap.h"
#include "../EntityManager.h"

// TileSystem: processes entities with CTileMap and creates static collider entities
class TileSystem
{
public:
    explicit TileSystem(EntityManager* manager) : m_entityManager(manager) {}
    void Process();

private:
    EntityManager* m_entityManager;
};
