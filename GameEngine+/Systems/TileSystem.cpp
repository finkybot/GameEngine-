#include "TileSystem.h"
#include "../CRectangle.h"
#include "../CStatic.h"
#include "../CTexture.h"
#include "../GameEngine.h"

void TileSystem::Process() {
	// Iterate all entities and find ones with CTileMap that haven't been processed yet
	for (auto& up : m_entityManager->getEntities()) {
		Entity* e = up.get();
		if (!e->IsAlive())
			continue;
		auto tileComp = e->GetComponent<CTileMap>();
		if (!tileComp)
			continue; // Skip entities that don't have a CTileMap component
		// Skip tilemaps that have already been processed into tile entities unless marked dirty
		if (tileComp->m_processed && !tileComp->m_dirty)
			continue;

		// Remove any existing generated tile entities so TileSystem can recreate them from the updated map
		for (Entity* te : m_entityManager->getEntities(EntityType::Tile)) {
			if (te)
				m_entityManager->KillEntity(te);
		}

		// Ensure spatial hash aligns with tile size
		m_entityManager->GetSpatialHash() = SpatialHashGrid<Entity>(tileComp->GetTileSize());

		// Convert tilemap to merged rectangles (greedy 2D merge) - similar logic to existing AddTileMapAsEntities
		TileMap& map = tileComp->map;
		std::vector<char> used(map.width * map.height, 0);
		for (int y = 0; y < map.height; ++y) {
			for (int x = 0; x < map.width; ++x) {
				int idx = y * map.width + x;
				if (used[idx])
					continue; // Skip tiles that have already been merged into a rectangle
				int baseVal = map.GetTile(x, y);
				if (baseVal == 0)
					continue; // Skip non-solid/empty tiles

				int w = 1;
				while (x + w < map.width && map.GetTile(x + w, y) == baseVal && !used[y * map.width + (x + w)])
					++w;

				int h = 1;
				bool canExtend = true;
				while (y + h < map.height && canExtend) {
					for (int xi = 0; xi < w; ++xi) {
						if (map.GetTile(x + xi, y + h) != baseVal || used[(y + h) * map.width + (x + xi)]) {
							canExtend = false;
							break;
						}
					}
					if (canExtend)
						++h;
				}

				for (int yy = 0; yy < h; ++yy)
					for (int xx = 0; xx < w; ++xx)
						used[(y + yy) * map.width + (x + xx)] = 1;

				float tileW = map.tileSize * w;
				float tileH = map.tileSize * h;
				float posX = x * map.tileSize;
				float posY = y * map.tileSize;

				Entity* tileEntity = m_entityManager->addEntity(EntityType::Tile);
				if (tileEntity) {
					tileEntity->AddComponent<CTransform>(Vec2(posX, posY), Vec2(0.0f, 0.0f));
					bool textureAttached = false;
					// Attach texture component if tileset metadata is present and atlas contains the index
					if (!map.tilesetKey.empty() && map.tilesetTileW > 0 && map.tilesetTileH > 0) {
						// Tile indices in map.tiles are assumed to be 1-based (0 == empty). Convert to 0-based atlas index.
						int tileVal = map.GetTile(x, y);
						int atlasIndex = tileVal > 0 ? (tileVal - 1) : 0;
						// Verify atlas exists and index is valid
						auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(map.tilesetKey);
						if (atlasOpt.has_value()) {
							auto atlasPtr = *atlasOpt;
							if (atlasPtr && atlasIndex >= 0 && (size_t)atlasIndex < atlasPtr->TileCount()) {
								// Attach texture component and set area to merged rectangle size so RenderSystem
								// knows not to scale a single sprite over the merged area.
								tileEntity->AddComponent<CTexture>(map.tilesetKey, atlasIndex, tileW, tileH);
								textureAttached = true;
								// Debug log
								std::cout << "TileSystem: attached texture atlas='" << map.tilesetKey
										  << "' index=" << atlasIndex << " at (" << posX << "," << posY
										  << ") size=" << tileW << "x" << tileH << "\n";
							} else {
								std::cout << "TileSystem: atlas index out of range or atlas missing for key='"
										  << map.tilesetKey << "' index=" << atlasIndex << "\n";
							}
						} else {
							std::cout << "TileSystem: atlas not found for key='" << map.tilesetKey << "'\n";
						}
					}
					// Fallback shape for collision visualization if no texture is available
					// If texture is attached, still add a CShape but mark it invisible so it doesn't draw over the sprite.
					auto rect = std::make_unique<CRectangle>(tileW, tileH);
					rect->SetColor(160.0f, 160.0f, 160.0f, 200);
					tileEntity->AddComponentPtr<CShape>(std::move(rect));
					if (textureAttached)
						tileEntity->GetComponent<CShape>()->GetShape().setFillColor(sf::Color::Transparent);
					tileEntity->AddComponent<CStatic>();
				}
			}
		}

		tileComp->m_processed = true;
		tileComp->m_dirty = false;
	}
}
