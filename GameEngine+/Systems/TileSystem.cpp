#include "TileSystem.h"
#include "../CRectangle.h"
#include "../CStatic.h"

void TileSystem::Process()
{
    // Iterate all entities and find ones with CTileMap that haven't been processed yet
    for (auto& up : m_entityManager->getEntities())
    {
        Entity* e = up.get();
        if (!e->IsAlive()) continue;
        auto tileComp = e->GetComponent<CTileMap>();
        if (!tileComp) continue;
        if (tileComp->m_processed) continue;

        // Ensure spatial hash aligns with tile size
        m_entityManager->GetSpatialHash() = SpatialHashGrid<Entity>(tileComp->GetTileSize());

        // Convert tilemap to merged rectangles (greedy 2D merge) - similar logic to existing AddTileMapAsEntities
        TileMap& map = tileComp->map;
        std::vector<char> used(map.width * map.height, 0);
        for (int y = 0; y < map.height; ++y)
        {
            for (int x = 0; x < map.width; ++x)
            {
                int idx = y * map.width + x;
                if (used[idx]) continue;
                if (!map.IsSolid(x, y)) continue;

                int w = 1;
                while (x + w < map.width && map.IsSolid(x + w, y) && !used[y * map.width + (x + w)]) ++w;

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

                for (int yy = 0; yy < h; ++yy)
                    for (int xx = 0; xx < w; ++xx)
                        used[(y + yy) * map.width + (x + xx)] = 1;

                float tileW = map.tileSize * w;
                float tileH = map.tileSize * h;
                float posX = x * map.tileSize;
                float posY = y * map.tileSize;

                Entity* tileEntity = m_entityManager->addEntity(EntityType::Tile);
                if (tileEntity)
                {
                    tileEntity->AddComponent<CTransform>(Vec2(posX, posY), Vec2(0.0f, 0.0f));
                    auto rect = std::make_unique<CRectangle>(tileW, tileH);
                    rect->SetColor(160.0f, 160.0f, 160.0f, 200);
                    tileEntity->AddComponentPtr<CShape>(std::move(rect));
                    tileEntity->AddComponent<CStatic>();
                }
            }
        }

        tileComp->m_processed = true;
    }
}
