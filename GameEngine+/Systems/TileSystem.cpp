/////////////////////////////////
// TileSystem.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "TileSystem.h"
#include "../CRectangle.h"
#include "../CStatic.h"
#include "../CTexture.h"
#include "../GameEngine.h"
#include "CColliderRect.h"
/////////////////////////////////



/////////////////////////////////
// Process - iterates through all entities with a CTileMap component, checks for solid tiles, and creates static collider entities for those 
// tiles. This method serves as the main entry point for processing tile maps and generating colliders in the game loop. It uses a greedy 
// rectangle merging algorithm to combine adjacent solid tiles into larger rectangles for more efficient collision handling.
void TileSystem::Process() {
	// Iterate all entities and find ones with CTileMap that haven't been processed yet
	for (auto& up : m_entityManager->GetEntities()) {
		Entity* entity = up.get();
		
		// Skip dead entities
		if (!entity->IsAlive())
			continue;
		
		// Get the CTileMap component from the entity
		auto tileComp = entity->GetComponent<CTileMap>();
		
		// Skip entities that don't have a CTileMap component
		if (!tileComp)
			continue; 
		
		// Skip tilemaps that have already been processed into tile entities unless marked dirty
		if (tileComp->m_processed && !tileComp->m_dirty)
			continue;

		// Remove any existing generated tile entities so TileSystem can recreate them from the updated map
		auto& tileEntities = m_entityManager->GetEntities(EntityType::Tile);		
		for (Entity* tileEntity : tileEntities) {
			
			// Only remove tile entities that were created by this tilemap entity (using owner ID)
			if (tileEntity->GetOwnerId() == entity->GetId()) {
				m_entityManager->KillEntity(tileEntity);
			}
		}

		// Get a reference to the TileMap data from the CTileMap component
		TileMap& map = tileComp->map;

		// Gonna do some local caching here, this is so we don't have to keep dereferencing the map object in the loops below
		int width = map.width;
		int height = map.height;
		float tileSize = map.tileSize;

		// Get the texture atlas for the tileset key from the TextureManager. If the atlas is not found, we will handle it gracefully by using a nullptr.
		auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(map.tilesetKey);
		auto atlasPtr = atlasOpt.has_value() ? atlasOpt.value().get() : nullptr;

		// Define a struct to represent a run of solid tiles in a row
		struct Run {
			int x;
			int width;
			int value;
		};

		// Run length encoding (replacing an O(n^2) algorithm with a more efficient O(n) algorithm for merging solid tiles into rectangles)
		// Lets start by creating a 2D vector to store runs of solid tiles for each row in the tilemap
		std::vector<std::vector<Run>> runs(height);

		// Build horizontal runs of solid tiles for each row in the tilemap. Each run represents a contiguous sequence of solid tiles with the same non-zero value.
		for (int y = 0; y < height; ++y) {
			int x = 0;
			while (x < width) {
				int idx = y * width + x;
				int val = map.tiles[idx];
				
				if (val == 0) { // Skip empty tiles
					++x;
					continue;
				}
				int runStart = x;

				while (x < width && map.tiles[y * width + x] == val) {
					++x;
				}
				runs[y].push_back({runStart, x - runStart, val});
			} 
			
		}

		// Step 2: merge vertical runs of solid tiles into rectangles
		for (int y = 0; y < height; ++y) {
			auto& rowRuns = runs[y];

			int maxHeightUsed = 1; // Initialize the maximum height of the rectangle to 1 (the current row)

			for (auto& run : rowRuns) {
				int h = 1;

				// try and extend the run downward as long as the next row has a matching run with the same value and width
				int ny = y + 1;
				while (ny < height) {
					bool foundMatchingRun = false;

					// Check if the next row has a run that matches the current run's x position, width, and value
					for (auto& nextRun : runs[ny]) {
						if (nextRun.x == run.x && nextRun.width == run.width && nextRun.value == run.value) {
							foundMatchingRun = true;
							break;
						}
					}

					// If no matching run is found in the next row, we can't extend the rectangle downward anymore
					if (!foundMatchingRun)	break;
					
					// If a matching run is found, we can extend the rectangle downward
					h++;
					ny++;
				}

				// Rectangle emit: create a new entity for the merged rectangle of solid tiles
				float posX = run.x * tileSize;
				float posY = y * tileSize;
				float tileW = run.width * tileSize;
				float tileH = h * tileSize;

				// Create a new entity of type Tile to represent the merged rectangle of solid tiles
				Entity* tileEntity = m_entityManager->AddEntity(EntityType::Tile);
				tileEntity->SetOwnerId(entity->GetId()); // Set owner ID to the tilemap entity for reference

				tileEntity->AddComponent<CTransform>(Vec2(posX, posY), Vec2(0.0f, 0.0f));
				bool textureAttached = false;
					
					// Attach texture component if tileset metadata is present and atlas contains the index
					if (!map.tilesetKey.empty() && map.tilesetTileW > 0 && map.tilesetTileH > 0) {
						// Tile indices in map.tiles are assumed to be 1-based (0 == empty). Convert to 0-based atlas index.
						
						// Calculate the atlas index based on the tile value. The atlas index is derived from the tile value, which is assumed to be 1-based (0 indicates an empty tile). 
						// Therefore, we subtract 1 from the tile value to get the corresponding 0-based index for the texture atlas.
						int atlasIndex = run.value - 1;
							
						// Check if the atlas pointer is valid and the index is within bounds
						if (atlasPtr && atlasIndex >= 0 && (size_t)atlasIndex < atlasPtr->TileCount()) {
							// Attach the texture component to the tile entity with the appropriate atlas key and index
							tileEntity->AddComponent<CTexture>(map.tilesetKey, atlasIndex, tileW, tileH);
							textureAttached = true;
								
							// Debug log: // SPAM SPAM SPAM
							std::cout << "TileSystem: attached texture atlas='" << map.tilesetKey << "' index=" << atlasIndex << " at (" << posX << "," << posY
																												<< ") size=" << tileW << "x" << tileH << "\n";
							} else { // SPAM SPAM SPAM
							std::cout << "TileSystem: atlas index out of range or atlas missing for key='" << map.tilesetKey << "' index=" << atlasIndex << "\n";
							} // SPAM SPAM SPAM
					} 			

					// If no texture was attached then add a collider rectangle component to the tile entity to represent the solid area for collision detection. 
					// This ensures that even if a texture is not available, the tile entity will still have a physical presence in the game world for collision purposes.
					if (!textureAttached) {
						tileEntity->AddComponent<CColliderRect>(tileW, tileH);
						//auto rect = std::make_unique<CRectangle>(tileW, tileH);
						//rect->SetColor(160.0f, 160.0f, 160.0f, 200);
						//tileEntity->AddComponentPtr<CShape>(std::move(rect));
					}

					tileEntity->AddComponent<CStatic>(); // Add a static component to indicate that this entity is static (not moving)	
					maxHeightUsed = std::max(maxHeightUsed,	h); // Update the maximum height used for this rectangle, in case we need to skip rows that were merged into it
			} // <-- end of run loop

			y += maxHeightUsed - 1; // Skip the rows that were merged into the rectangle, fuuuuuuuu AI, I was right
		} // <-- end of row loop

		tileComp->m_processed = true;
		tileComp->m_dirty = false;
	}

}
/////////////////////////////////
