/////////////////////////////////
// ChunkManagerV2.cpp - Implementation of the ChunkManagerV2 class for managing tile-based chunks in a game world. This class handles loading, saving, and managing chunks of tiles,
// This is new, cleaner version of ChunkManager with a more modular design and improved performance. It provides methods for accessing and modifying tile data, managing chunk streaming, 
// and handling rendering properties.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "ChunkManagerV2.h"
#include "EntityManager.h"
#include "json.hpp"
#include "Entity.h"

using json = nlohmann::json;
/////////////////////////////////



/////////////////////////////////
// Constructor - initializes the ChunkManagerV2 with specified chunk dimensions and tile size. Default values are provided for convenience.
ChunkManagerV2::ChunkManagerV2(EntityManager& em, int chunkTilesWide, int chunkTilesHigh, int tileSize)
	: m_em(em), m_chunkTilesWide(chunkTilesWide), m_chunkTilesHigh(chunkTilesHigh), m_tileSize(tileSize) {}
/////////////////////////////////



/////////////////////////////////
// CreateChunk - Creates a new chunk at the specified chunk coordinates (cx, cy). Returns a pointer to the newly created chunk entity.
Entity* ChunkManagerV2::CreateChunk(int cx, int cy) {
	// Create entity via EntityManager
	Entity* chunk = m_em.AddEntity(EntityType::Chunk);

	// Attach ChunkComponent
	chunk->AddComponent<CChunkComponent>();
	auto* cc = chunk->GetComponent<CChunkComponent>();
	
	cc->ChunkX = cx;
	cc->ChunkY = cy;
	cc->TilesWide = m_chunkTilesWide;
	cc->TilesHigh = m_chunkTilesHigh;
	cc->TileSize = m_tileSize;
	cc->IsLoaded = false;
	cc->IsActive = false;	

	// Register in chunk map
	m_chunkMap[PackChunkID(cx, cy)] = chunk;

	return chunk;
}
/////////////////////////////////



/////////////////////////////////
// DestroyChunk - Destroys the specified chunk entity, freeing any associated resources and removing it from the chunk map.
void ChunkManagerV2::DestroyChunk(Entity* chunk) {
	if (!chunk)
		return;

	// Remove from map
	auto* cc = chunk->GetComponent<CChunkComponent>();
	uint64_t id = PackChunkID(cc->ChunkX, cc->ChunkY);
	m_chunkMap.erase(id);

	// Kill via EntityManager
	m_em.SafeKillEntity(chunk);
}
/////////////////////////////////



/////////////////////////////////
// UpdateStreaming - Updates the streaming of chunks based on the camera position and view dimensions. This method determines 
// which chunks should be active or inactive based on their proximity to the camera.
void ChunkManagerV2::UpdateStreaming(const Vec2& cameraPos, float viewWidth, float viewHeight) {
	// Compute center chunk from camera
	const float chunkWorldWidth = float(m_chunkTilesWide * m_tileSize);
	const float chunkWorldHeight = float(m_chunkTilesHigh * m_tileSize);

	int centerCx = int(std::floor(cameraPos.x / chunkWorldWidth));
	int centerCy = int(std::floor(cameraPos.y / chunkWorldHeight));

	int minCx = centerCx - m_activeRadius;
	int maxCx = centerCx + m_activeRadius;
	int minCy = centerCy - m_activeRadius;
	int maxCy = centerCy + m_activeRadius;

	// Activate / create chunks in range
	for (int cy = minCy; cy <= maxCy; ++cy) {
		for (int cx = minCx; cx <= maxCx; ++cx) {
			Entity* chunk = FindChunk(cx, cy);
			if (!chunk) {
				chunk = CreateChunk(cx, cy);
				LoadChunkFromDisk(chunk); // or static map init
			}
			ActivateChunk(chunk);
		}
	}

	// Deactivate chunks outside range
	for (auto& pair : m_chunkMap) {
		Entity* chunk = pair.second;
		auto* cc = chunk->GetComponent<CChunkComponent>();

		if (cc->ChunkX < minCx || cc->ChunkX > maxCx || cc->ChunkY < minCy || cc->ChunkY > maxCy) {
			DeactivateChunk(chunk);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// ActivateChunk - Activates the specified chunk entity, making it ready for rendering and interaction in the game world.
void ChunkManagerV2::ActivateChunk(Entity* chunk) {
	if (!chunk)
		return;
	auto* cc = chunk->GetComponent<CChunkComponent>();
	cc->IsActive = true;
}
/////////////////////////////////



/////////////////////////////////
// DeactivateChunk - Deactivates the specified chunk entity, making it inactive and potentially unloading it from memory if not needed.
void ChunkManagerV2::DeactivateChunk(Entity* chunk) {
	if (!chunk)
		return;
	auto* cc = chunk->GetComponent<CChunkComponent>();
	cc->IsActive = false;
}
/////////////////////////////////



/////////////////////////////////
// GetTile - Retrieves a reference to the tile at the specified local coordinates (localX, localY) within the given chunk and layer.
Tile& ChunkManagerV2::GetTile(Entity* chunk, int layerID, int localX, int localY) {
	// TODO: insert return statement here
	auto* cc = chunk->GetComponent<CChunkComponent>();
	TileLayer& layer = cc->Layers[layerID];
	return layer.Tiles[localY * cc->TilesWide + localX];
}
/////////////////////////////////



/////////////////////////////////
// SetTile - Sets the tile at the specified local coordinates (localX, localY) within the given chunk and layer to the provided tile value.
void ChunkManagerV2::SetTile(Entity* chunk, int layerID, int localX, int localY, const Tile& tile) {
	auto* cc = chunk->GetComponent<CChunkComponent>();
	TileLayer& layer = cc->Layers[layerID];
	layer.Tiles[localY * cc->TilesWide + localX] = tile;
	layer.NeedsRebuild = true; // Mark layer for mesh rebuild
}
/////////////////////////////////



/////////////////////////////////
// IsTileSolid - Checks if the tile at the specified local coordinates (localX, localY) within the given chunk and layer is solid (collidable).
bool ChunkManagerV2::IsTileSolid(Entity* chunk, int layerID, int localX, int localY) {
	auto* cc = chunk->GetComponent<CChunkComponent>();
	TileLayer& layer = cc->Layers[layerID];

	if (localX < 0 || localX >= cc->TilesWide || localY < 0 || localY >= cc->TilesHigh)
		return false; // Out of bounds
	
	uint8_t type = layer.CollisionMask.Cells[localY * cc->TilesWide + localX];
	return type != 0;
}
/////////////////////////////////



/////////////////////////////////
// BuildCollisionGrid - Builds a collision grid for the specified chunk entity, generating collision data based on the tile properties.
void ChunkManagerV2::BuildCollisionGrid(Entity* chunk) {
	CChunkComponent* cc = chunk->GetComponent<CChunkComponent>();

	int w = cc->TilesWide;
	int h = cc->TilesHigh;

	for (TileLayer& layer : cc->Layers) {
		if (!layer.HasCollision)
			continue;
		
		layer.CollisionMask.Width = w;
		layer.CollisionMask.Height = h;
		layer.CollisionMask.Cells.resize(w * h);

		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				Tile& tile = layer.Tiles[y * w + x];
				layer.CollisionMask.Cells[y * w + x] = tile.CollisionType;
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// LoadChunkFromDisk - Loads the specified chunk entity from disk, populating its tile data and other properties as needed.
void ChunkManagerV2::LoadChunkFromDisk(Entity* chunk) {
	auto* cc = chunk->GetComponent<CChunkComponent>();

	std::string filename = "Chunks/chunk_" + std::to_string(cc->ChunkX) + "_" + std::to_string(cc->ChunkY) + ".json";

	std::ifstream file(filename);
	if (!file.is_open()) {
		// No file? Create empty chunk.
		cc->IsLoaded = true;
		return;
	}

	nlohmann::json j;
	file >> j;

	cc->TilesWide = j["tilesWide"];
	cc->TilesHigh = j["tilesHigh"];

	cc->Layers.clear();

	for (auto& jLayer : j["layers"]) {
		TileLayer layer;
		layer.LayerID = jLayer["layerID"];

		// Load tiles
		auto& tiles = jLayer["tiles"];
		auto& flags = jLayer["flags"];

		layer.Tiles.resize(cc->TilesWide * cc->TilesHigh);

		for (int i = 0; i < tiles.size(); ++i) {
			layer.Tiles[i].tileID = tiles[i];
			layer.Tiles[i].Flags = flags[i];
		}

		// Build collision mask
		layer.CollisionMask.Width = cc->TilesWide;
		layer.CollisionMask.Height = cc->TilesHigh;
		layer.CollisionMask.Cells.resize(cc->TilesWide * cc->TilesHigh);

		for (int i = 0; i < tiles.size(); ++i) {
			uint16_t flagsVal = flags[i];
			layer.CollisionMask.Cells[i] = (flagsVal & 0x1) ? 1 : 0; // Example: bit 0 = solid
		}

		cc->Layers.push_back(layer);
	}

	cc->IsLoaded = true;
}
	/////////////////////////////////



/////////////////////////////////
// SaveChunk - Saves the specified chunk entity to disk, persisting its tile data and other properties for future loading.
void ChunkManagerV2::SaveChunk(Entity* chunk) {}
/////////////////////////////////



/////////////////////////////////
// FindChunk - Finds and returns a pointer to the chunk entity at the specified chunk coordinates (cx, cy). Returns nullptr 
// if the chunk does not exist.
Entity* ChunkManagerV2::FindChunk(int cx, int cy) {
	auto it = m_chunkMap.find(PackChunkID(cx, cy));
	return (it != m_chunkMap.end()) ? it->second : nullptr;
}
/////////////////////////////////



/////////////////////////////////
// ChunkExists - Checks if a chunk exists at the specified chunk coordinates (cx, cy). Returns true if the chunk exists, false otherwise.
bool ChunkManagerV2::ChunkExists(int cx, int cy) const {
	return m_chunkMap.find(PackChunkID(cx, cy)) != m_chunkMap.end();
}
/////////////////////////////////



/////////////////////////////////
// PackChunkID - Packs the chunk coordinates (cx, cy) into a single 64-bit integer for use as a key in the chunk map.
uint64_t ChunkManagerV2::PackChunkID(int cx, int cy) const {
	return (uint64_t(uint32_t(cx)) << 32) | uint64_t(uint32_t(cy));
}
/////////////////////////////////