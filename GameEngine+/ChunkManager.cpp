/////////////////////////////////
// ChunkManager.cpp - Implementation of the ChunkManager class for managing tile-based chunks in a game world. This class handles loading, saving, and managing chunks of tiles, 
// including background loading and eviction of least recently used chunks when memory limits are exceeded.

/////////////////////////////////
// Includes and namespace aliases for the ChunkManager implementation. We include necessary headers for file I/O, threading, synchronization, and SFML graphics.
#include "ChunkManager.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <thread>
#include <SFML/Graphics/RectangleShape.hpp>

// Namespace alias for filesystem
namespace fs = std::filesystem;

#include "GameEngine.h"
#include "CRectangle.h"
#include "CStatic.h"
#include "CTransform.h"
/////////////////////////////////



/////////////////////////////////
// Static members for background loading; pending chunks are stored in a thread-safe queue and processed in the main thread
static std::mutex s_pendingMutex; // Mutex to protect access to the pending chunks queue
static std::vector<std::tuple<int, int, std::vector<int>, uint32_t>> s_pendingChunks; // Queue of chunks pending loading (cx, cy, tiles, editVersion at enqueue)
/////////////////////////////////



/////////////////////////////////
// ****** ChunkManager Implementation ******
// Constructor - initializes the ChunkManager with specified chunk dimensions and tile size. Default values are provided for convenience.
ChunkManager::ChunkManager(int chunkWidth, int chunkHeight, float tileSize) : m_chunkWidth(chunkWidth), m_chunkHeight(chunkHeight), m_tileSize(tileSize) {
	// Ensure the base path exists for saving/loading chunks; if it doesn't exist, attempt to create it.
	if (!m_basePath.empty() && !fs::exists(m_basePath)) {
		try {
			fs::create_directories(m_basePath);
		} catch (const fs::filesystem_error& e) {
			std::cerr << "Error creating chunk base directory: " << e.what() << std::endl; // Log the error but continue with an empty base path, which will cause load/save operations to fail gracefully without crashing.
			m_basePath.clear(); // Clear the base path to indicate it's invalid
			}
		}

}
/////////////////////////////////



/////////////////////////////////
// LoadAllSavedChunks - Scans the base directory for saved chunk files and enqueues them for loading in the background thread. 
// Each chunk file is expected to be named in the format "chunk_X_Y.dat" where X and Y are the chunk coordinates.
void ChunkManager::LoadAllSavedChunks() {
	if (m_basePath.empty()) return;
	try {
		for (auto &p : fs::directory_iterator(m_basePath)) {
			if (!p.is_regular_file()) continue;
			std::string fname = p.path().filename().string();
			if (fname.rfind("chunk_", 0) != 0) continue;
			// parse chunk_X_Y.dat
			std::string body = fname.substr(6); // after "chunk_"
			size_t us = body.find('_');
			size_t dot = body.find('.');
			if (us == std::string::npos || dot == std::string::npos) continue;
			int cx = std::stoi(body.substr(0, us));
			int cy = std::stoi(body.substr(us+1, dot - (us+1)));
			EnqueueLoadChunk(cx, cy);
		}
	} catch(...) {}
}
/////////////////////////////////



/////////////////////////////////
// GetSavedChunkBounds - Scans the base directory for saved chunk files and computes the world-pixel bounding box
// of all saved chunks without loading any tile data. Returns false if no chunks are found.
bool ChunkManager::GetSavedChunkBounds(float& outMinX, float& outMinY, float& outMaxX, float& outMaxY) const {
	if (m_basePath.empty()) return false;
	bool found = false;
	outMinX = std::numeric_limits<float>::infinity();
	outMinY = std::numeric_limits<float>::infinity();
	outMaxX = -std::numeric_limits<float>::infinity();
	outMaxY = -std::numeric_limits<float>::infinity();
	try {
		for (auto& p : fs::directory_iterator(m_basePath)) {
			if (!p.is_regular_file()) continue;
			std::string fname = p.path().filename().string();
			if (fname.rfind("chunk_", 0) != 0) continue;
			std::string body = fname.substr(6);
			size_t us  = body.find('_');
			size_t dot = body.find('.');
			if (us == std::string::npos || dot == std::string::npos) continue;
			int cx = std::stoi(body.substr(0, us));
			int cy = std::stoi(body.substr(us + 1, dot - (us + 1)));
			float wx = (float)(cx * m_chunkWidth)  * m_tileSize;
			float wy = (float)(cy * m_chunkHeight) * m_tileSize;
			outMinX = std::min(outMinX, wx);
			outMinY = std::min(outMinY, wy);
			outMaxX = std::max(outMaxX, wx + m_chunkWidth  * m_tileSize);
			outMaxY = std::max(outMaxY, wy + m_chunkHeight * m_tileSize);
			found = true;
		}
	} catch (...) {}
	return found;
}
/////////////////////////////////



/////////////////////////////////
// SetBasePath
// The base path is used as a prefix for all chunk file operations, allowing for organized storage of chunk data on disk.
void ChunkManager::SetBasePath(const std::string& basePath) {
	m_basePath = basePath;
	if (!m_basePath.empty()) {
		try {
			if (!fs::exists(m_basePath)) fs::create_directories(m_basePath);
		} catch(...) { }
		// ensure trailing separator so simple concatenation works
		if (!m_basePath.empty()) {
			char last = m_basePath.back();
			if (last != '/' && last != '\\') m_basePath.push_back('/');
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// RebuildAllChunksFromTileset - Rebuilds the vertex arrays for all loaded chunks using the current tileset atlas. This is called when the tileset changes to update the visual representation of all chunks.
void ChunkManager::RebuildAllChunksFromTileset() {
	// Fetch atlas once outside the per-chunk loop
	std::shared_ptr<TextureAtlas> atlasPtr;
	if (!m_tilesetKey.empty()) {
		auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(m_tilesetKey);
		if (atlasOpt.has_value() && *atlasOpt) atlasPtr = *atlasOpt;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& pr : m_chunks) {
		BuildChunkVertexArray(pr.second, atlasPtr);
	}
}
/////////////////////////////////



/////////////////////////////////
// BuildChunkVertexArray - Builds the sf::VertexArray for a chunk from its tile data. Called from RebuildAllChunksFromTileset, FinalizeLoadedChunk, and SetTileAt.
// Pass a nullptr atlas to use the solid-colour fallback.
void ChunkManager::BuildChunkVertexArray(Chunk& chunk, const std::shared_ptr<TextureAtlas>& atlas) {
	chunk.vertexArray.clear();
	chunk.vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);
	chunk.vertexTexture.reset();
	if (atlas) chunk.vertexTexture = atlas->GetTexture();

	const float ts = chunk.tileSize;
	const int baseX = chunk.chunkX * chunk.width;
	const int baseY = chunk.chunkY * chunk.height;
	// pre-reserve: worst case every tile is solid (6 vertices per tile)
	chunk.vertexArray.resize(0); // clear without dealloc

	for (int y = 0; y < chunk.height; ++y) {
		for (int x = 0; x < chunk.width; ++x) {
			int v = chunk.tiles[y * chunk.width + x];
			if (v == 0) continue;
			const float px = (baseX + x) * ts;
			const float py = (baseY + y) * ts;

			bool usedTexture = false;
			sf::Vector2f uv00, uv11;
			if (atlas) {
				auto rectOpt = atlas->GetSfFloatRectForTile((size_t)(v - 1));
				if (rectOpt.has_value()) {
					const sf::FloatRect& fr = *rectOpt;
					uv00 = { fr.position.x, fr.position.y };
					uv11 = { fr.position.x + fr.size.x, fr.position.y + fr.size.y };
					usedTexture = true;
				}
			}

			if (usedTexture && chunk.vertexTexture) {
				chunk.vertexArray.append({ {px,      py      }, sf::Color::White, uv00 });
				chunk.vertexArray.append({ {px + ts, py      }, sf::Color::White, {uv11.x, uv00.y} });
				chunk.vertexArray.append({ {px + ts, py + ts }, sf::Color::White, uv11 });
				chunk.vertexArray.append({ {px,      py      }, sf::Color::White, uv00 });
				chunk.vertexArray.append({ {px + ts, py + ts }, sf::Color::White, uv11 });
				chunk.vertexArray.append({ {px,      py + ts }, sf::Color::White, {uv00.x, uv11.y} });
			} else {
				constexpr sf::Color fallback(120, 120, 120, 200);
				chunk.vertexArray.append({ {px,      py      }, fallback });
				chunk.vertexArray.append({ {px + ts, py      }, fallback });
				chunk.vertexArray.append({ {px + ts, py + ts }, fallback });
				chunk.vertexArray.append({ {px,      py      }, fallback });
				chunk.vertexArray.append({ {px + ts, py + ts }, fallback });
				chunk.vertexArray.append({ {px,      py + ts }, fallback });
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// DrawChunks - Renders visible chunks. Copies only a lightweight draw-info struct (no tiles vector) to minimise lock time.
void ChunkManager::DrawChunks(sf::RenderWindow& window, const sf::View& view) {
	const sf::Vector2f viewCenter = view.getCenter();
	const sf::Vector2f viewSize   = view.getSize();
	const float vleft   = viewCenter.x - viewSize.x * 0.5f;
	const float vtop    = viewCenter.y - viewSize.y * 0.5f;
	const float vright  = vleft + viewSize.x;
	const float vbottom = vtop  + viewSize.y;

	struct DrawInfo {
		sf::VertexArray              vertexArray;
		std::shared_ptr<sf::Texture> vertexTexture;
		bool readyForRendering = false;
		int  chunkX = 0, chunkY = 0, width = 0, height = 0;
		float tileSize = 32.f;
	};
	std::vector<DrawInfo> visible;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		visible.reserve(32);
		for (const auto& pr : m_chunks) {
			const Chunk& c = pr.second;
			if (c.width <= 0 || c.height <= 0) continue;
			const float cx      = (float)(c.chunkX * c.width)  * c.tileSize;
			const float cy      = (float)(c.chunkY * c.height) * c.tileSize;
			const float cright  = cx + (float)c.width  * c.tileSize;
			const float cbottom = cy + (float)c.height * c.tileSize;
			if (cright <= vleft || cx >= vright || cbottom <= vtop || cy >= vbottom) continue;
			DrawInfo di;
			di.vertexArray       = c.vertexArray;
			di.vertexTexture     = c.vertexTexture;
			// If the chunk is marked not ready but contains only zero tiles (cleared),
			// treat it as ready so we display empty space instead of the grey loading box.
			bool allZero = true;
			for (int t : c.tiles) { if (t != 0) { allZero = false; break; } }
			di.readyForRendering = c.readyForRendering || allZero;
			di.chunkX = c.chunkX; di.chunkY = c.chunkY;
			di.width  = c.width;  di.height = c.height;
			di.tileSize = c.tileSize;
			visible.push_back(std::move(di));
		}
	}

	for (const DrawInfo& d : visible) {
		const float wx = (float)(d.chunkX * d.width)  * d.tileSize;
		const float wy = (float)(d.chunkY * d.height) * d.tileSize;
		if (!d.readyForRendering) {
			sf::RectangleShape r(sf::Vector2f((float)d.width * d.tileSize, (float)d.height * d.tileSize));
			r.setPosition({wx, wy});
			r.setFillColor(sf::Color(60, 60, 60, 80));
			window.draw(r);
			continue;
		}
		if (d.vertexArray.getVertexCount() > 0) {
			sf::RenderStates states;
			if (d.vertexTexture) states.texture = d.vertexTexture.get();
			window.draw(d.vertexArray, states);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// Destructor - currently does not have any special cleanup logic, but we could add it if needed in the future (e.g., to save dirty chunks before exiting)
ChunkManager::~ChunkManager() { 
	SaveAllChunks(); // Ensure all dirty chunks are saved to disk when the ChunkManager is destroyed to prevent data loss.
	std::lock_guard<std::mutex> lock(s_pendingMutex); // Lock the mutex to safely clear the pending chunks queue
	s_pendingChunks.clear();						  // Clear the pending chunks queue to free memory	
}
/////////////////////////////////



/////////////////////////////////
// GetTileAt - Retrieves the tile value at the specified tile coordinates (tileX, tileY). If the corresponding chunk is not loaded, it will be enqueued for loading in the background thread. 
// Returns 0 if the chunk is not loaded or if the tile coordinates are out of bounds within the chunk.
int ChunkManager::GetTileAt(int tileX, int tileY) {
	const int chunkX = FloorDiv(tileX, m_chunkWidth);
	const int chunkY = FloorDiv(tileY, m_chunkHeight);
	const int localX = tileX - chunkX * m_chunkWidth;
	const int localY = tileY - chunkY * m_chunkHeight;
	const long long key = GetChunkKey(chunkX, chunkY);

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto itr = m_chunks.find(key);
		if (itr != m_chunks.end()) {
			const Chunk& chunk = itr->second;
			if (localX >= 0 && localX < chunk.width && localY >= 0 && localY < chunk.height)
				return chunk.tiles[localY * chunk.width + localX];
			return 0;
		}
	} // release lock before enqueue to avoid re-entrancy / deadlock
	EnqueueLoadChunk(chunkX, chunkY);
	return 0;
}
/////////////////////////////////



/////////////////////////////////
// SetTileAt - Sets the tile value at the specified tile coordinates (tileX, tileY) to the given tileValue. 
// If the corresponding chunk is not loaded, it will be enqueued for loading in the background thread.
int ChunkManager::SetTileAt(int tileX, int tileY, int tileValue) {
	const int chunkX = FloorDiv(tileX, m_chunkWidth);
	const int chunkY = FloorDiv(tileY, m_chunkHeight);
	const int localX = tileX - chunkX * m_chunkWidth;
	const int localY = tileY - chunkY * m_chunkHeight;
	const long long key = GetChunkKey(chunkX, chunkY);

	std::lock_guard<std::mutex> lock(m_mutex);

	// Create placeholder chunk on first paint
	auto [itr, inserted] = m_chunks.emplace(key, Chunk(chunkX, chunkY, m_chunkWidth, m_chunkHeight, m_tileSize));
	if (inserted) {
		m_lruList.push_front(key);
		m_lruIndex[key] = m_lruList.begin();
		EnqueueLoadChunk(chunkX, chunkY);
	}

	Chunk& chunk = itr->second;
	if (localX < 0 || localX >= chunk.width || localY < 0 || localY >= chunk.height) return 0;

	const int index     = localY * chunk.width + localX;
	const int prevValue = chunk.tiles[index];
	// If this chunk was just inserted as a placeholder (we created it because it wasn't loaded)
	// then still apply the requested change even if the placeholder value matches the requested value.
	// This handles erasing areas of maps saved on disk: the placeholder starts as zeros and an erase
	// should overwrite the on-disk non-zero tiles once the background load finalizes.
	// Always process clears (tileValue == 0) even if prevValue is already 0 — the chunk may be an
	// unfinalized placeholder whose real on-disk tiles are non-zero. Bumping editVersion here ensures
	// FinalizeLoadedChunk rejects the stale background load and keeps the cleared in-memory state.
	if (prevValue == tileValue && !inserted && tileValue != 0) return prevValue;

	chunk.tiles[index] = tileValue;
	chunk.dirty        = true;
	chunk.editVersion++;

	// O(1) LRU touch via iterator index
	auto lruIt = m_lruIndex.find(key);
	if (lruIt != m_lruIndex.end()) m_lruList.erase(lruIt->second);
	m_lruList.push_front(key);
	m_lruIndex[key] = m_lruList.begin();

	// Rebuild vertex array
	std::shared_ptr<TextureAtlas> atlasPtr;
	if (!m_tilesetKey.empty()) {
		auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(m_tilesetKey);
		if (atlasOpt.has_value() && *atlasOpt) atlasPtr = *atlasOpt;
	}
	BuildChunkVertexArray(chunk, atlasPtr);
	// Make chunk visible for rendering immediately after we rebuild its vertex array
	chunk.readyForRendering = true;
	// Rebuild collider entities so removed tiles don't leave grey rectangles behind
	RebuildChunkEntities(chunk);

	// Immediately persist to disk — delete the file if the chunk is entirely empty so
	// GetSavedChunkBounds only counts chunks that actually contain tiles.
	if (!m_basePath.empty()) {
		try {
			std::string filename = (fs::path(m_basePath) / ("chunk_" + std::to_string(chunk.chunkX) + "_" + std::to_string(chunk.chunkY) + ".dat")).string();
			bool allZero = true;
			for (int t : chunk.tiles) { if (t != 0) { allZero = false; break; } }
			if (allZero) {
				fs::remove(filename); // empty chunk — remove file so bounds shrink correctly
				chunk.dirty = false;
			} else {
				std::ofstream outFile(filename, std::ios::binary);
				if (outFile) {
					outFile.write(reinterpret_cast<const char*>(chunk.tiles.data()), chunk.tiles.size() * sizeof(int));
					chunk.dirty = false;
				} else {
					std::cerr << "ChunkManager: failed to save chunk " << filename << "\n";
				}
			}
		} catch (...) {}
	}
	return prevValue;
}
/////////////////////////////////



/////////////////////////////////
// EnsureChunksInTileRect - Ensures that all chunks that intersect the specified tile rectangle (tileX0, tileY0, tileX1, tileY1) are loaded and ready for rendering. 
// The marginChunks parameter specifies how many additional chunks to load around the edges of the rectangle to ensure smooth rendering when the player moves.
void ChunkManager::EnsureChunksInTileRect(int tileX0, int tileY0, int tileX1, int tileY1, int marginChunks) {
	if (tileX0 > tileX1) std::swap(tileX0, tileX1);
	if (tileY0 > tileY1) std::swap(tileY0, tileY1);

	int cX0 = FloorDiv(tileX0, m_chunkWidth)  - marginChunks;
	int cY0 = FloorDiv(tileY0, m_chunkHeight) - marginChunks;
	int cX1 = FloorDiv(tileX1, m_chunkWidth)  + marginChunks;
	int cY1 = FloorDiv(tileY1, m_chunkHeight) + marginChunks;

	// Safety clamp: avoid attempting to load an extremely large span of chunks which can freeze the app.
	if (cX1 - cX0 > (int)ChunkManager::kMaxChunkSpan) {
		int mid = (cX0 + cX1) / 2;
		cX0 = mid - ChunkManager::kMaxChunkSpan / 2;
		cX1 = mid + ChunkManager::kMaxChunkSpan / 2;
	}
	if (cY1 - cY0 > (int)ChunkManager::kMaxChunkSpan) {
		int mid = (cY0 + cY1) / 2;
		cY0 = mid - ChunkManager::kMaxChunkSpan / 2;
		cY1 = mid + ChunkManager::kMaxChunkSpan / 2;
	}

	for (int cy = cY0; cy <= cY1; ++cy) {
		for (int cx = cX0; cx <= cX1; ++cx) {
			const long long key = GetChunkKey(cx, cy);
			bool needLoad = false;
			{
				std::lock_guard<std::mutex> lock(m_mutex);
				if (m_chunks.find(key) != m_chunks.end()) {
					// O(1) LRU touch
					auto lruIt = m_lruIndex.find(key);
					if (lruIt != m_lruIndex.end()) m_lruList.erase(lruIt->second);
					m_lruList.push_front(key);
					m_lruIndex[key] = m_lruList.begin();
					continue;
				}
				// Insert placeholder chunk. Mark it ready for rendering immediately if it contains no tiles
				// so erasing/clearing operations show empty space instead of the "loading" grey box.
				auto insertRes = m_chunks.emplace(key, Chunk(cx, cy, m_chunkWidth, m_chunkHeight, m_tileSize));
				auto insertedItr = insertRes.first;
				bool insertedNow = insertRes.second;
				m_lruList.push_front(key);
				m_lruIndex[key] = m_lruList.begin();
					if (insertedNow) {
						Chunk& newChunk = insertedItr->second;
						// placeholder chunks are initialized with zero tiles; treat them as ready so they render empty immediately.
						bool allZero = true;
						for (int i = 0; i < newChunk.width * newChunk.height; ++i) { if (newChunk.tiles[i] != 0) { allZero = false; break; } }
					if (allZero) newChunk.readyForRendering = true;
				}
				needLoad = true;
			}
			if (needLoad) EnqueueLoadChunk(cx, cy);
		}
	}

	// After enqueuing loads, ensure we don't exceed allowed loaded chunks
	// Intentional no-op context anchor: keep eviction immediately after enqueuing.
	EvictIfNeeded();
}
/////////////////////////////////



/////////////////////////////////
// UpdateMainThread - This method should be called from the main thread to perform any necessary updates, such as processing dirty chunks or preparing vertex buffers for rendering.
void ChunkManager::UpdateMainThread() {
	// Process any chunks that have been loaded in the background thread and are pending finalization. We copy the pending chunks to a local variable while holding the mutex, 
	// then release the mutex before processing to minimize lock time and allow the background thread to continue loading new chunks without waiting for finalization to complete.
	std::vector<std::tuple<int, int, std::vector<int>, uint32_t>> pendingChunksCopy;
	{
		std::lock_guard<std::mutex> lock(s_pendingMutex); // Lock the mutex to safely access the pending chunks queue
		pendingChunksCopy = s_pendingChunks; // Copy the pending chunks to a local variable
		s_pendingChunks.clear(); // Clear the original pending chunks queue to free memory and allow new chunks to be added by the background thread
	}
	
	// Finalize each loaded chunk by setting its tile data and marking it as ready for rendering. 
	// This should be done in the main thread to ensure thread safety when modifying the chunks map and to prepare the chunk for rendering.
	for (const auto& [chunkX, chunkY, tileData, version] : pendingChunksCopy) { // Loop through the copied list of pending chunks
		FinalizeLoadedChunk(chunkX, chunkY, tileData, version);
	}
}
/////////////////////////////////



/////////////////////////////////
// SaveAllChunks - Saves all dirty chunks to disk in the specified directory. Each chunk will be saved as a separate file named "chunk_X_Y.dat" where X and Y are the chunk coordinates.
void ChunkManager::SaveAllChunks() {
	std::lock_guard<std::mutex> lock(m_mutex); // Lock the mutex to safely access the chunks map
	for (auto& pr : m_chunks) {
		Chunk& chunk = pr.second; // Get a reference to the current chunk in the loop
		if (chunk.dirty) {		  // Only save chunks that are marked as dirty to avoid unnecessary disk writes
		std::filesystem::path p(m_basePath);
		std::string name = std::string("chunk_") + std::to_string(chunk.chunkX) + "_" + std::to_string(chunk.chunkY) + ".dat";
		std::string filename = (p / name).string(); // Construct the filename for the chunk based on its coordinates
			std::ofstream outFile(filename, std::ios::binary); // Open a binary output file stream to save the chunk data
			
			if (outFile) {
				outFile.write(reinterpret_cast<const char*>(chunk.tiles.data()), chunk.tiles.size() * sizeof(int));
				chunk.dirty = false;
			} else {
				std::cerr << "Error saving chunk to file: " << filename << std::endl; // Log an error if the file could not be opened for writing
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// EnqueueLoadChunk - Enqueues a chunk to be loaded in the background thread. The chunk will be loaded from disk if it exists, or created with default tile data if it does not.
void ChunkManager::EnqueueLoadChunk(int chunkX, int chunkY) {
	// Capture the current editVersion of this chunk so FinalizeLoadedChunk can reject stale loads
	uint32_t versionAtEnqueue = 0;
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		auto it = m_chunks.find(GetChunkKey(chunkX, chunkY));
		if (it != m_chunks.end()) versionAtEnqueue = it->second.editVersion;
	}
	// Start a background thread to load the chunk data from disk. The chunk will be loaded from disk if it exists, or created with default tile data if it does not.
	std::thread([chunkX, chunkY, versionAtEnqueue, this]() {
		std::filesystem::path p(m_basePath);
		std::string name = std::string("chunk_") + std::to_string(chunkX) + "_" + std::to_string(chunkY) + ".dat";
		std::string filename = (p / name).string(); // Construct the filename for the chunk based on its coordinates
		std::vector<int> tileData(m_chunkWidth * m_chunkHeight,	0); // Create a vector to hold the tile data for the chunk, initialized with default values (0 = empty)

		if (fs::exists(filename)) {
			std::ifstream inFile(filename, std::ios::binary);
			if (inFile) {
				inFile.read(reinterpret_cast<char*>(tileData.data()), tileData.size() * sizeof(int));
			} else {
				std::cerr << "ChunkManager: Error opening chunk file for reading: " << filename << std::endl;
			}
		}
		// After loading the chunk data, add it to the pending queue with the version captured at enqueue time
		{
			std::lock_guard<std::mutex> lock(s_pendingMutex);
			s_pendingChunks.emplace_back(chunkX, chunkY, tileData, versionAtEnqueue);
		}
	}).detach(); // Detach the thread to allow it to run independently without blocking the main thread
}
/////////////////////////////////



/////////////////////////////////
// RebuildChunkEntities - Rebuilds the collider entities for a chunk based on its current tile data. This should be called whenever the tile data changes to ensure that the colliders match the visual representation of the chunk.
void ChunkManager::RebuildChunkEntities(Chunk& c) {
	// First, safely kill any existing entities that were generated for this chunk to avoid leaving orphaned entities in the world. We use SafeKillEntity to ensure that we don't attempt to access entities that may have already been destroyed.
	try {
		EntityManager& em = GameEngine::GetInstance().GetEntityManager();
		for (Entity* ge : c.generatedEntities) {
			if (ge) em.SafeKillEntity(ge);
		}
		c.generatedEntities.clear();
		std::vector<char> used(c.width * c.height, 0);
		for (int y = 0; y < c.height; ++y) {
			for (int x = 0; x < c.width; ++x) {
				int idx = y * c.width + x;
				if (used[idx]) continue;
				int val = c.tiles[idx];
				if (val == 0) continue;
				int w = 1;
				while (x + w < c.width && c.tiles[y * c.width + (x + w)] == val && !used[y * c.width + (x + w)]) ++w;
				int h = 1;
				bool canExtend = true;
				while (y + h < c.height && canExtend) {
					for (int xi = 0; xi < w; ++xi) {
						if (c.tiles[(y + h) * c.width + (x + xi)] != val || used[(y + h) * c.width + (x + xi)]) { canExtend = false; break; }
					}
					if (canExtend) ++h;
				}
				for (int yy = 0; yy < h; ++yy) for (int xx = 0; xx < w; ++xx) used[(y + yy) * c.width + (x + xx)] = 1;
				float tileW = c.tileSize * w;
				float tileH = c.tileSize * h;
				float posX = (c.chunkX * c.width + x) * c.tileSize;
				float posY = (c.chunkY * c.height + y) * c.tileSize;
				Entity* ent = em.AddEntity(EntityType::Tile);
				if (ent) {
					ent->AddComponent<CTransform>(Vec2(posX, posY), Vec2::Zero);
					auto rect = std::make_unique<CRectangle>(tileW, tileH);
					rect->SetColor(160.0f, 160.0f, 160.0f, 200);
					ent->AddComponentPtr<CShape>(std::move(rect));
					ent->AddComponent<CStatic>();
					c.generatedEntities.push_back(ent);
				}
			}
		}
	} catch(...) {}
}
/////////////////////////////////



/////////////////////////////////
// FinalizeLoadedChunk
void ChunkManager::FinalizeLoadedChunk(int chunkX, int chunkY, std::vector<int> tileData, uint32_t versionAtEnqueue) {
	long long key = GetChunkKey(chunkX, chunkY);
	std::lock_guard<std::mutex> lock(m_mutex);
	auto itr = m_chunks.find(key);

	if (itr == m_chunks.end()) {
		// Chunk was evicted before we could finalize — discard the load
		return;
	}

	Chunk& chunk = itr->second;
	// If the chunk's editVersion changed since the load was enqueued, the disk data is stale — skip overwriting
	if (chunk.editVersion != versionAtEnqueue) {
		// The chunk was edited after enqueue; keep current in-memory tiles and just mark ready for rendering
		chunk.readyForRendering = true;
	} else {
		// Safe to apply loaded data
		if ((int)tileData.size() == chunk.width * chunk.height) {
			chunk.tiles = std::move(tileData);
		} else {
			chunk.tiles.assign(chunk.width * chunk.height, 0);
		}
		chunk.dirty = false;
		chunk.readyForRendering = true;
	}
	// Build GPU vertex array on the main thread
	chunk.cpuVertexBuffer.clear();
	std::shared_ptr<TextureAtlas> atlasPtr;
	if (!m_tilesetKey.empty()) {
		auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(m_tilesetKey);
		if (atlasOpt.has_value() && *atlasOpt) atlasPtr = *atlasOpt;
	}
	BuildChunkVertexArray(chunk, atlasPtr);
	// O(1) LRU touch
	{
		auto lruIt = m_lruIndex.find(key);
		if (lruIt != m_lruIndex.end()) m_lruList.erase(lruIt->second);
	}
	m_lruList.push_front(key);
	m_lruIndex[key] = m_lruList.begin();

	// register colliders
	RebuildChunkEntities(m_chunks[key]);
}
/////////////////////////////////



/////////////////////////////////
// EvictIfNeeded - Evicts least recently used chunks if the number of loaded chunks exceeds the maximum limit. This will unload chunks from memory but will save them to disk if they are dirty.
void ChunkManager::EvictIfNeeded() {
	std::lock_guard<std::mutex> lock(m_mutex);
	while (m_chunks.size() > m_maxLoadedChunks && !m_lruList.empty()) {
		long long key = m_lruList.back();
		auto itr = m_chunks.find(key);
		if (itr != m_chunks.end()) {
			Chunk& chunk = itr->second;
			
			// Save if dirty
			if (chunk.dirty) {
			std::filesystem::path p(m_basePath);
			std::string filename = (p / ("chunk_" + std::to_string(chunk.chunkX) + "_" + std::to_string(chunk.chunkY) + ".dat")).string();
			std::ofstream outFile(filename, std::ios::binary);
				
						if (outFile) {
							outFile.write(reinterpret_cast<const char*>(chunk.tiles.data()), chunk.tiles.size() * sizeof(int));
						} else {
							std::cerr << "ChunkManager: eviction save failed: " << filename << "\n";
						}
					}

					try {
						EntityManager& em = GameEngine::GetInstance().GetEntityManager();
						for (Entity* ge : chunk.generatedEntities) {
							if (ge) em.SafeKillEntity(ge);
						}
					} catch(...) {}

					m_chunks.erase(itr);
				}
				m_lruIndex.erase(key);
				m_lruList.pop_back();
				}
}
/////////////////////////////////



/////////////////////////////////
// RegisterChunkColliders - Registers the collider entities generated for all chunks. This should be called after loading chunks to ensure that the colliders are present in the game world for physics and collision detection.
void ChunkManager::RegisterChunkColliders(EntityManager& em) {}
/////////////////////////////////



/////////////////////////////////
// UnregisterChunkColliders - Unregisters the collider entities generated for all chunks. 
// This should be called when the chunk data changes or when chunks are evicted to ensure that outdated colliders are removed from the game world.
void ChunkManager::UnregisterChunkColliders(EntityManager& em) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto &pr : m_chunks) {
		Chunk &c = pr.second;
		for (Entity* ge : c.generatedEntities) {
			if (ge) em.KillEntity(ge);
		}
		c.generatedEntities.clear();
	}
}
/////////////////////////////////