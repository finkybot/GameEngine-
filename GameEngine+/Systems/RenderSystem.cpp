/////////////////////////////////
// RenderSystem.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "RenderSystem.h"
#include "../Entity.h"
#include "../CShape.h"
#include "../CTransform.h"
#include "../CText.h"
#include "../FontManager.h"
#include "../CTexture.h"
#include "../GameEngine.h"
#include "../TextureAtlas.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <unordered_map>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Font.hpp>
/////////////////////////////////



/////////////////////////////////
// RenderAliveEntities - Renders all alive entities to the provided SFML render window. This method iterates through the list of entities, 
// checks if they are alive, and draws their shapes and text components if present. It serves as the main entry point for rendering entities 
// in the game loop.
void RenderSystem::RenderAliveEntities(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window) {
	// Backwards-compatible: render shapes then text if configured.
	RenderShapes(entities, window);
	if (m_fontManager)
		RenderText(entities, window);
}
/////////////////////////////////



/////////////////////////////////
// RenderAll - A convenience method that renders all entities with a single call. The mode parameter controls whether to render only shapes, 
// render shapes followed by text, or render shapes now and defer text rendering until after overlays are rendered.
void RenderSystem::RenderAll(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window,
							 RenderSystem::RenderMode mode) {
	// Always render shapes
	RenderShapes(entities, window);
	if (mode == RenderMode::ShapesThenText) {
		if (m_fontManager)
			RenderText(entities, window);
	}
	// ShapesThenTextAfterOverlays intentionally does not render text here so caller can draw overlays first
}
/////////////////////////////////



/////////////////////////////////
// RenderEntity - Renders a single entity to the provided SFML render window. This method checks for the presence of a CTexture component 
// and attempts to render it using the associated texture atlas. If the entity has an area specified in the CTexture component, it will draw repeated 
// sprites to fill that area. If any of the requirements for textured rendering are not met, it falls back to rendering a shape component if present.
void RenderSystem::RenderEntity(Entity* entity, sf::RenderWindow& window) const {
	if (!entity)
		return;

	// Immediate textured fallback: reuse a single sprite to avoid per-tile allocations.
	if (auto tex = entity->GetComponent<CTexture>()) {
		if (tex->visible) {
			auto transform = entity->GetComponent<CTransform>();
			if (transform) {
				auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(tex->atlasKey);
				if (atlasOpt.has_value()) {
					auto atlasPtr = *atlasOpt;
					if (atlasPtr) {
						auto texPtr = atlasPtr->GetTexture();
						auto rectOpt = atlasPtr->GetSfFloatRectForTile((size_t)tex->tileIndex);
                        if (texPtr && rectOpt.has_value()) {
							static std::unique_ptr<sf::Sprite> sprite;
							static const sf::Texture* lastTex = nullptr;
							if (!sprite || lastTex != texPtr.get()) {
								sprite = std::make_unique<sf::Sprite>(*texPtr);
								lastTex = texPtr.get();
							}

							sf::FloatRect fr = rectOpt.value();

							// inset
							fr.position.x += 0.5f;
							fr.position.y += 0.5f;
							fr.size.x -= 1.0f;
							fr.size.y -= 1.0f;

							sprite->setTextureRect(sf::IntRect(sf::Vector2i((int)fr.position.x, (int)fr.position.y),
															   sf::Vector2i((int)fr.size.x, (int)fr.size.y)));
							sprite->setPosition(sf::Vector2f(transform->position.x, transform->position.y));

							// Tiled area: draw repeated sprites using the reused sprite
							if (tex->areaW > 0.0f && tex->areaH > 0.0f) {
								int atlasW = atlasPtr->TileWidth();
								int atlasH = atlasPtr->TileHeight();
								if (atlasW > 0 && atlasH > 0) {
									int tilesX = static_cast<int>(std::round(tex->areaW / static_cast<float>(atlasW)));
									int tilesY = static_cast<int>(std::round(tex->areaH / static_cast<float>(atlasH)));
									for (int ty = 0; ty < tilesY; ++ty) {
										for (int tx = 0; tx < tilesX; ++tx) {
											sprite->setPosition(sf::Vector2f(transform->position.x + tx * atlasW,
																			  transform->position.y + ty * atlasH));
											window.draw(*sprite);
										}
									}
									return;
								}
							}

							window.draw(*sprite);
							return;
						}
					}
				}
			}
		}
	}

	// Fallback: render shape component if present
	if (auto shape = entity->GetComponent<CShape>()) {
		auto transform = entity->GetComponent<CTransform>();
		if (transform) {
			shape->GetShape().setPosition(sf::Vector2f(transform->position.x, transform->position.y));
		}
		window.draw(shape->GetShape());
	}
}
/////////////////////////////////



/////////////////////////////////
// RenderShapes - Render only shapes for all alive entities. This method iterates through the entities, checks if they are alive, and renders their shape components if present. It organizes rendering by layers to ensure correct draw order and attempts to batch textured 
// entities by their atlas to reduce draw calls.
void RenderSystem::RenderShapes(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window) {
	// DEBUG: Log entity count (every frame)
	static int frameCount = 0;
	frameCount++;
	if (frameCount == 60 || frameCount % 60 == 0) {  // First time and every 60 frames
		//std::cout << "[RenderSystem::RenderShapes] Processing " << entities.size() << " entities (frame " << frameCount << ")\n";
		int shapeCount = 0;
		int aliveCount = 0;
		for (const auto& entity : entities) {
			if (!entity) {
				std::cout << "  WARNING: null entity in vector!\n";
				continue;
			}
			if (entity->IsAlive()) {
				aliveCount++;
				if (entity->GetComponent<CShape>()) {
					shapeCount++;
					//if (auto tf = entity->GetComponent<CTransform>()) {
					//	std::cout << "  - Alive entity with shape at (" << tf->m_position.x << ", " << tf->m_position.y << ")\n";
					//}
				}
			}
		}
		//std::cout << "  Alive: " << aliveCount << ", With shapes: " << shapeCount << "\n";
	}
	// Render in logical layers: Background -> Mid -> Foreground -> Overlay
	// Build per-layer buckets to avoid querying GetLayer() frequently in tight loop
	std::array<std::vector<Entity*>, 4> buckets;
	for (const auto& entity : entities) {
		if (!entity->IsAlive())
			continue;
		// Skip tile entities - they are collision/debug entities, not gameplay entities
		// Skip both TileMap (high-level tilemap data) and Tile (individual collision rectangles)
		if (entity->GetType() == EntityType::TileMap || entity->GetType() == EntityType::Tile) {
			continue;
		}
		int layerIdx = static_cast<int>(entity->GetLayer());
		if (layerIdx < 0) layerIdx = 0;
		if (layerIdx > 3) layerIdx = 3;
		buckets[layerIdx].push_back(entity.get());

		//if (auto shape = entity->GetComponent<CShape>()) {
		//	std::cout << "[RenderSystem::RenderShapes] Adding entity with CShape to bucket (layer " << layerIdx << ")\n";
		//}
	}

	for (int layer = 0; layer <= 3; ++layer) {
      // Batch textured entities by their atlas to reduce draw calls.
		// Map key: raw TextureAtlas* pointer. Value: pair(shared_ptr<TextureAtlas>, vector<Entity*>)
		std::unordered_map<void*, std::pair<std::shared_ptr<TextureAtlas>, std::vector<Entity*>>> batches;
		for (Entity* e : buckets[layer]) {
			// Attempt to categorize textured entities for batching. If any requirement fails, fall back to immediate render.
			auto tex = e->GetComponent<CTexture>();
			if (tex && tex->visible) {
				auto transform = e->GetComponent<CTransform>();
				if (!transform) {
					// Can't render textured entity without transform - skip
					continue;
				}

				// Attempt to get the atlas for this entity's texture. If successful, add to batch; otherwise, fall back to immediate render.
				auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(tex->atlasKey);
				if (atlasOpt.has_value()) {
					auto atlasPtr = *atlasOpt;
					if (atlasPtr) {
						auto texPtr = atlasPtr->GetTexture();
						auto rectOpt = atlasPtr->GetSfFloatRectForTile((size_t)tex->tileIndex);
						if (texPtr && rectOpt.has_value()) {
							// Good candidate for batching
							auto key = static_cast<void*>(atlasPtr.get());
							auto &entry = batches[key];
							
							// Store the shared_ptr<TextureAtlas> in the first element of the pair if not already set
							if (!entry.first) entry.first = atlasPtr;
							entry.second.push_back(e);
							continue; // handled by batching later
						}
					}
				}
					}
					// Not eligible for batching - render immediately (shapes or missing/invalid atlas)
					RenderEntity(e, window);
				}

		// For each atlas batch, create a vertex array (triangles) and draw once using the atlas texture
		for (auto &kv : batches) {
			auto atlasPtr = kv.second.first;
			auto &entitiesForAtlas = kv.second.second;
			if (!atlasPtr) continue;
			auto texPtr = atlasPtr->GetTexture();
			if (!texPtr) continue;

			sf::VertexArray va(sf::PrimitiveType::TriangleStrip);
			va.clear();


			// Code re-write completely to use a single vertex array for all entities using the same atlas, instead of creating a new vertex array for each entity. This reduces draw calls and improves performance.
			// Pre-cache atlas UVs once
			std::vector<sf::FloatRect> uvCache;
			uvCache.resize(atlasPtr->TileCount()); // resize to number of tiles in atlas

			// Precompute UVs for all tiles in the atlas
			for (size_t i = 0; i < uvCache.size(); ++i) {
				auto rectOpt = atlasPtr->GetSfFloatRectForTile(i);
				if (rectOpt.has_value()) {
					sf::FloatRect fr = rectOpt.value();

					// Pixel-perfect inset to prevent shimmering / bleeding
					fr.position.x += 0.5f;
					fr.position.y += 0.5f;
					fr.size.x -= 1.0f;
					fr.size.y -= 1.0f;

					uvCache[i] = fr;
				} else {
					uvCache[i] = sf::FloatRect();
				}
			}


			// Helper to append a single quad (two triangles)
			auto appendQuad = [&](float x, float y, const sf::FloatRect& fr) {
				// UVs are already inset in uvCache
				const float u0 = fr.position.x;
				const float v0 = fr.position.y;
				const float u1 = fr.position.x + fr.size.x;
				const float v1 = fr.position.y + fr.size.y;

				const float w = fr.size.x;
				const float h = fr.size.y;

				const float sx = std::round(x);
				const float sy = std::round(y);

				const float x0 = sx;
				const float y0 = sy;
				const float x1 = sx + w;
				const float y1 = sy + h;

				va.append(sf::Vertex({x0, y0}, sf::Color::White, {u0, v0}));
				va.append(sf::Vertex({x1, y0}, sf::Color::White, {u1, v0}));
				va.append(sf::Vertex({x0, y1}, sf::Color::White, {u0, v1}));
				va.append(sf::Vertex({x1, y1}, sf::Color::White, {u1, v1}));
			};

			// Main loop
			for (Entity* e : entitiesForAtlas) {
				auto tex = e->GetComponent<CTexture>();
				auto xf = e->GetComponent<CTransform>();
				if (!tex || !xf)
					continue;

				const size_t tileIndex = (size_t)tex->tileIndex;

				if (tileIndex >= uvCache.size()) {
					// This entity is using a tile index that does not exist in this atlas.
					// Skip it to avoid corrupting the vertex array.
					continue;
				}

				const sf::FloatRect& fr = uvCache[tileIndex];

				const float baseX = xf->position.x;
				const float baseY = xf->position.y;

				// Multi‑tile area
				if (tex->areaW > 0.f && tex->areaH > 0.f) {
					const int atlasW = atlasPtr->TileWidth();
					const int atlasH = atlasPtr->TileHeight();

					if (atlasW <= 0 || atlasH <= 0) {
						appendQuad(baseX, baseY, fr);
						continue;
					}

					const int tilesX = (int)std::round(tex->areaW / (float)atlasW);
					const int tilesY = (int)std::round(tex->areaH / (float)atlasH);

					for (int ty = 0; ty < tilesY; ++ty) {
						for (int tx = 0; tx < tilesX; ++tx) {
							appendQuad(baseX + tx * atlasW, baseY + ty * atlasH, fr);
						}
					}
				} else {
					// Single tile
					appendQuad(baseX, baseY, fr);
				}
			}

			if (va.getVertexCount() > 0) {
				sf::RenderStates states;
				states.texture = texPtr.get();
				window.draw(va, states);
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// RenderText - Render only text for all alive entities. This method iterates through the entities, checks if they are alive, and renders their text components if present and visible. It relies on the presence of a FontManager to resolve fonts for rendering text.
void RenderSystem::RenderText(const std::vector<std::unique_ptr<Entity>>& entities, sf::RenderWindow& window) const {
	if (!m_fontManager)
		return;
	for (const auto& entity : entities) {
		if (!entity->IsAlive())
			continue;
		// Skip tile entities - they are collision/debug entities, not gameplay entities
		if (entity->GetType() == EntityType::TileMap || entity->GetType() == EntityType::Tile)
			continue;
		RenderTextEntity(entity.get(), window);
	}
}
/////////////////////////////////



/////////////////////////////////
// RenderTextEntity - Renders the text component of a single entity if present and visible. This method checks for the presence of a CText component, resolves the font using the FontManager, and constructs an sf::Text object to render the text. 
// It also handles text alignment and positioning based on the entity's transform and text offset.
void RenderSystem::RenderTextEntity(Entity* entity, sf::RenderWindow& window) const {
	//Guard clause: if no CText component or if text is not visible, skip rendering.
	if (!entity)
		return;

	// Get CText component and check visibility; if no text component or if text is not visible, skip rendering, and get out of here.
	auto txt = entity->GetComponent<CText>();
	if (!txt || !txt->visible)
		return;

	// Resolve font via the configured FontManager
	if (!m_fontManager)
		return;
	auto fontOpt = m_fontManager->GetFont(txt->fontKey);
	if (!fontOpt.has_value() || !(*fontOpt)) {
		// Font not found - nothing to draw or fallback behavior could be added here.
		return;
	}
	std::shared_ptr<sf::Font> font = *fontOpt;

	// Build sf::Text
	sf::Text sfTxt(*font);
	sfTxt.setString(txt->text);
	sfTxt.setCharacterSize(txt->charSize);
	sfTxt.setFillColor(txt->color);

	// Determine world position: prefer transform position, fall back to entity centre
	Vec2 worldPos = entity->GetCentrePoint();
	if (auto transform = entity->GetComponent<CTransform>(); transform) {
		worldPos = transform->position;
	}
	// Apply offset from CText
	sf::Vector2f pos(worldPos.x + txt->offset.x, worldPos.y + txt->offset.y);
	sfTxt.setPosition(pos);

	// Horizontal alignment: measure local bounds and set origin appropriately
	sf::FloatRect bounds = sfTxt.getLocalBounds();

	// Set origin based on alignment. SFML 3.0 changed setOrigin to take a Vector2f for the origin point, so we calculate the appropriate origin based on the desired alignment and the text bounds.
	if (txt->align == CText::Align::Center) {
		sfTxt.setOrigin(
			sf::Vector2f(bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f));
	} else if (txt->align == CText::Align::Right) {
		sfTxt.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x, bounds.position.y));
	} else {
		sfTxt.setOrigin(sf::Vector2f(bounds.position.x, bounds.position.y + bounds.size.y * 0.0f));
	}

	window.draw(sfTxt);
}
/////////////////////////////////
