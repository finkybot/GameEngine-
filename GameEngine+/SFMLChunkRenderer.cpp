/////////////////////////////////
// SFMLChunkRenderer.cpp
/////////////////////////////////



/////////////////////////////////
#include "SFMLChunkRenderer.h"
#include "GameEngine.h"
/////////////////////////////////



/////////////////////////////////
void SFMLChunkRenderer::RenderChunk(const ChunkRenderData& data, sf::RenderWindow& window) {
	sf::VertexArray va(sf::PrimitiveType::Triangles);

	// --- GET TILESET ATLAS ---
	auto& engine = GameEngine::GetInstance();
	auto& texMan = engine.GetTextureManager();

	auto atlasOpt = texMan.GetAtlas(engine.GetChunkManager().GetTilesetKey());
	if (!atlasOpt.has_value() || !*atlasOpt)
		return;

	TextureAtlas* atlas = atlasOpt->get();
	sf::Texture* texture = atlas->GetTexture().get();
	if (!texture)
		return;

	const auto& layers = data.tilesPerLayer;

	for (int layer = 0; layer < data.numLayers; ++layer) {
		const auto& tiles = layers[layer];
		float alphaMul = 1.0f;
		if (data.activeLayer >= 0 && layer != data.activeLayer)
			alphaMul = data.unselectedLayerAlpha;

		for (int y = 0; y < data.height; ++y) {
			for (int x = 0; x < data.width; ++x) {

				int tile = tiles[y * data.width + x];
				if (tile == 0)
					continue;

				float worldX = (data.chunkX * data.width + x) * data.tileSize;
				float worldY = (data.chunkY * data.height + y) * data.tileSize;

				sf::Color c(255, 255, 255, static_cast<uint8_t>(255.0f * alphaMul));

				// --- GET UV RECT (SFML 3.x API) ---
				auto uvOpt = atlas->GetSfFloatRectForTile(tile - 1);
				if (!uvOpt.has_value())
					continue;

				const sf::Rect<float>& fr = *uvOpt;

				// --- CORRECT: USE INTERIOR TILE REGION ---
				const int pad = atlas->applyPadding ? 1 : 0;

				float u0 = fr.position.x + pad;
				float v0 = fr.position.y + pad;
				float u1 = fr.position.x + pad + atlas->TileWidth();
				float v1 = fr.position.y + pad + atlas->TileHeight();

				sf::Vertex v0v({worldX, worldY}, c, {u0, v0});
				sf::Vertex v1v({worldX + data.tileSize, worldY}, c, {u1, v0});
				sf::Vertex v2v({worldX + data.tileSize, worldY + data.tileSize}, c, {u1, v1});
				sf::Vertex v3v({worldX, worldY + data.tileSize}, c, {u0, v1});

				va.append(v0v);
				va.append(v1v);
				va.append(v2v);

				va.append(v0v);
				va.append(v2v);
				va.append(v3v);
			}
		}
	}

	window.draw(va, texture);
}
/////////////////////////////////
