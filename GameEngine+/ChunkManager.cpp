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
#include <set>
#include <SFML/Graphics/RectangleShape.hpp>
/////////////////////////////////



/////////////////////////////////
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
static std::vector<std::tuple<int, int, int, std::vector<int>, uint32_t>> s_pendingChunks; // Queue: (cx, cy, layer, tiles, editVersion at enqueue)
// Global loader task queue and worker threads to limit concurrency and avoid spawn storms
static std::mutex s_loadQueueMutex;
static std::condition_variable s_loadCv;
static std::vector<std::tuple<int,int,int,std::string,uint32_t>> s_loadQueue; // (cx, cy, layer, basePath, versionAtEnqueue)
static bool s_loaderStarted = false;
static int s_maxLoaderThreads = 2; // conservative default
/////////////////////////////////



/////////////////////////////////
// ****** ChunkManager Implementation ******
// Constructor - initializes the ChunkManager with specified chunk dimensions and tile size. Default values are provided for convenience.
ChunkManager::ChunkManager(int chunkWidth, int chunkHeight, float tileSize, int numLayers)
	: m_chunkWidth(chunkWidth), m_chunkHeight(chunkHeight), m_tileSize(tileSize), m_numLayers(std::max(1, numLayers)) {
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
// Destructor - currently does not have any special cleanup logic, but we could add it if needed in the future (e.g., to save dirty chunks before exiting)
ChunkManager::~ChunkManager() {
	SaveAllChunks(); // Ensure all dirty chunks are saved to disk when the ChunkManager is destroyed to prevent data loss.
	std::lock_guard<std::mutex> lock(s_pendingMutex); // Lock the mutex to safely clear the pending chunks queue
	s_pendingChunks.clear();						  // Clear the pending chunks queue to free memory
}
/////////////////////////////////



/////////////////////////////////
// ClearAllLoadedChunks - remove all loaded chunks from memory without saving; used when switching levels
void ChunkManager::ClearAllLoadedChunks() {
	std::lock_guard<std::mutex> lk(m_mutex);
	// kill generated colliders safely
	try {
		EntityManager& em = GameEngine::GetInstance().GetEntityManager();
		for (auto &pr : m_chunks) {
			Chunk &c = pr.second;
			for (Entity* ge : c.generatedEntities) if (ge) em.SafeKillEntity(ge);
			c.generatedEntities.clear();
		}
	} catch(...) {}
	m_chunks.clear();
	m_lruIndex.clear();
	m_lruList.clear();
}
/////////////////////////////////



/////////////////////////////////
// BuildWorldMask - constructs a world mask for collision, pathfinding, and other gameplay mechanics. Each value corresponds to a 
// specific tile's collision properties.
void ChunkManager::BuildWorldMask() {
	worldMask.assign(worldWidth * worldHeight, false); // Initialize world mask to false (no collision)
	
	int layer = 1; // For now, we only consider the main layer for collision

	for (auto& [key, chunk] : m_chunks) {

		for (int i = 0; i < chunk.width * chunk.height; ++i) {
			int localX = i % chunk.width; // Calculate local tile coordinates within the chunk
			int localY = i / chunk.width;

			// absolute tile coords in world space
			int absTx = chunk.chunkX * chunk.width + localX;
			int absTy = chunk.chunkY * chunk.height + localY;

            // convert to mask space using worldOffset
			int relTx = absTx - m_worldOffsetX; // minTileX
			int relTy = absTy - m_worldOffsetY; // minTileY

			if (relTx < 0 || relTy < 0 || relTx >= worldWidth || relTy >= worldHeight) {
				continue;
			}

			bool solid = (chunk.tilesPerLayer[layer][i] != 0);
			if (solid) {
				worldMask[relTy * worldWidth + relTx] = true;
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// LoadAllSavedChunks - Scans the base directory for saved chunk files and enqueues them for loading in the background thread. 
// Each chunk file is expected to be named in the format "chunk_X_Y.dat" where X and Y are the chunk coordinates.
void ChunkManager::LoadAllSavedChunks() {
	if (m_basePath.empty()) {
		std::cout << "[ChunkManager] ERROR: m_basePath is empty!\n";
		return;
	}
	std::cout << "[ChunkManager] LoadAllSavedChunks from: " << m_basePath << "\n";
	std::cout << "[ChunkManager] m_numLayers = " << m_numLayers << "\n";
	try {
		int fileCount = 0;
		std::set<std::pair<int, int>> chunksToCreate;  // Track unique (cx, cy) pairs
		std::vector<std::string> filenames;

		// First pass: discover all chunk files and which chunks need to be created
		for (auto &p : fs::directory_iterator(m_basePath)) {
			if (!p.is_regular_file()) continue;
			std::string fname = p.path().filename().string();
			filenames.push_back(fname);
			if (fname.rfind("chunk_", 0) != 0) continue;
			fileCount++;
			
			// parse chunk_<layer>_<cx>_<cy>.dat
			std::string body = fname.substr(6); // after "chunk_"
			size_t us1 = body.find('_');
			size_t us2 = body.find('_', us1 + 1);
			size_t dot = body.find('.');
			
			if (us1 == std::string::npos || us2 == std::string::npos || dot == std::string::npos) continue;
			int layer = std::stoi(body.substr(0, us1));
			int cx = std::stoi(body.substr(us1+1, us2 - (us1+1)));
			int cy = std::stoi(body.substr(us2+1, dot - (us2+1)));

			std::cout << "[LOADALL] Found saved chunk (" << cx << "," << cy << ")\n";

			chunksToCreate.insert({cx, cy});
		}

		// Second pass: create all chunks that need to exist
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			for (const auto& [cx, cy] : chunksToCreate) {
				long long key = GetChunkKey(cx, cy);
				if (m_chunks.find(key) == m_chunks.end()) {
					// Create chunk with all layers
					m_chunks[key] = Chunk(cx, cy, m_chunkWidth, m_chunkHeight, m_tileSize, m_numLayers);
				}
			}
		}

		// Third pass: enqueue load jobs for all discovered files
		for (auto &p : fs::directory_iterator(m_basePath)) {
			if (!p.is_regular_file()) continue;
			std::string fname = p.path().filename().string();
			if (fname.rfind("chunk_", 0) != 0) continue;
			// parse chunk_<layer>_<cx>_<cy>.dat
			std::string body = fname.substr(6); // after "chunk_"
			size_t us1 = body.find('_');
			size_t us2 = body.find('_', us1 + 1);
			size_t dot = body.find('.');
			if (us1 == std::string::npos || us2 == std::string::npos || dot == std::string::npos) continue;
			int layer = std::stoi(body.substr(0, us1));
			int cx = std::stoi(body.substr(us1+1, us2 - (us1+1)));
			int cy = std::stoi(body.substr(us2+1, dot - (us2+1)));
			EnqueueLoadChunk(cx, cy, layer);
		}
		std::cout << "[ChunkManager] Total chunk files found: " << fileCount << "\n";
		if (fileCount == 0) {
			std::cout << "[ChunkManager] WARNING: No chunk files found! All files in directory:\n";
			for (const auto& f : filenames) {
				std::cout << "  - " << f << "\n";
			}
		}
	} catch(const std::exception& e) {
		std::cout << "[ChunkManager] Exception in LoadAllSavedChunks: " << e.what() << "\n";
	} catch(...) {
		std::cout << "[ChunkManager] Unknown exception in LoadAllSavedChunks\n";
	}
}
/////////////////////////////////



/////////////////////////////////
// DebugPrintLayer1 - Print layer 1 (obstacle layer) as a grid of 0s and 1s for all loaded chunks
void ChunkManager::DebugPrintLayer1() {
	std::cout << "\n========== DEBUG: Layer 1 (Obstacle Layer) Visualization ==========\n";
	std::cout.flush();

	if (m_chunks.empty()) {
		std::cout << "No chunks loaded!\n";
		std::cout.flush();
		return;
	}

	std::cout << "Total chunks loaded: " << m_chunks.size() << "\n";
	std::cout.flush();

	// Find bounds
	int minCx = INT_MAX, maxCx = INT_MIN;
	int minCy = INT_MAX, maxCy = INT_MIN;
	for (const auto& [key, chunk] : m_chunks) {
		minCx = std::min(minCx, chunk.chunkX);
		maxCx = std::max(maxCx, chunk.chunkX);
		minCy = std::min(minCy, chunk.chunkY);
		maxCy = std::max(maxCy, chunk.chunkY);
	}

	std::cout << "Chunk bounds: cx=" << minCx << ".." << maxCx << " cy=" << minCy << ".." << maxCy << "\n\n";
	std::cout.flush();

	// Print each chunk
	for (int cy = minCy; cy <= maxCy; ++cy) {
		for (int cx = minCx; cx <= maxCx; ++cx) {
			auto key = (static_cast<long long>(cx) << 32) | static_cast<unsigned int>(cy);
			auto it = m_chunks.find(key);

			if (it == m_chunks.end()) {
				std::cout << "[Chunk (" << cx << "," << cy << ") NOT LOADED]\n";
				std::cout.flush();
				continue;
			}

			const Chunk& chunk = it->second;
			std::cout << "[Chunk (" << cx << "," << cy << ") Layer 1]:\n";

			if (chunk.tilesPerLayer.size() <= 1) {
				std::cout << "  (No layer 1 data - size=" << chunk.tilesPerLayer.size() << ")\n\n";
				std::cout.flush();
				continue;
			}

			const auto& layer1 = chunk.tilesPerLayer[1];

			// ====== DEBUG ========
			// Print as grid
			for (int y = 0; y < chunk.height; ++y) {
				std::cout << "  ";
				for (int x = 0; x < chunk.width; ++x) {
					int tileValue = layer1[y * chunk.width + x];
					//std::cout << (tileValue != 0 ? "1" : "0");
					worldMask.push_back((tileValue != 0) ? 1 : 0); // Store in worldMask for potential further processing
					std::cout << (worldMask.back() ? "1" : "0");
				}
				std::cout << "\n";
			}
			std::cout << "\n";
			std::cout.flush();
		}
	}
	std::cout << "========== END DEBUG ==========\n\n";
	std::cout.flush();
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
			size_t us1  = body.find('_');
			size_t us2  = body.find('_', us1 + 1);
			size_t dot = body.find('.');
			if (us1 == std::string::npos || us2 == std::string::npos || dot == std::string::npos) continue;
			// layer = body.substr(0, us1)
			int cx = std::stoi(body.substr(us1 + 1, us2 - (us1 + 1)));
			int cy = std::stoi(body.substr(us2 + 1, dot - (us2 + 1)));
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
// GetMinChunkCoords - Get the minimum chunk coordinates from loaded chunks
void ChunkManager::GetMinChunkCoords(int& outMinCx, int& outMinCy) const {
	outMinCx = 0;
	outMinCy = 0;

	if (m_chunks.empty()) return;

	outMinCx = INT_MAX;
	outMinCy = INT_MAX;
	for (const auto& [key, chunk] : m_chunks) {
		outMinCx = std::min(outMinCx, chunk.chunkX);
		outMinCy = std::min(outMinCy, chunk.chunkY);
	}
	if (outMinCx == INT_MAX) outMinCx = 0;
	if (outMinCy == INT_MAX) outMinCy = 0;
}
/////////////////////////////////



/////////////////////////////////
// ShiftChunksToPositiveCoords - Shift all chunks so min coordinates are >= 0
void ChunkManager::ShiftChunksToPositiveCoords() {
	std::lock_guard<std::mutex> lk(m_mutex);

	if (m_chunks.empty()) return;

	// Find minimum coordinates
	int minCx = INT_MAX, minCy = INT_MAX;
	for (const auto& [key, chunk] : m_chunks) {
		minCx = std::min(minCx, chunk.chunkX);
		minCy = std::min(minCy, chunk.chunkY);
	}

	// If already positive, no need to shift
	if (minCx >= 0 && minCy >= 0) return;

	// Calculate offset
	int offsetX = (minCx < 0) ? -minCx : 0;
	int offsetY = (minCy < 0) ? -minCy : 0;

	if (offsetX == 0 && offsetY == 0) return; // No shift needed

	std::cout << "[ChunkManager] Shifting chunks by (" << offsetX << ", " << offsetY << ") to ensure positive coordinates\n";

	// Collect old chunks and rebuild map with new coordinates
	std::vector<Chunk> chunksToShift;
	std::vector<long long> keysToRemove;

	for (auto& [key, chunk] : m_chunks) {
		chunksToShift.push_back(chunk);
		keysToRemove.push_back(key);
	}

	// Remove old entries
	for (auto key : keysToRemove) {
		m_chunks.erase(key);
	}

	// Reinsert with shifted coordinates
	for (Chunk& chunk : chunksToShift) {
		chunk.chunkX += offsetX;
		chunk.chunkY += offsetY;
		long long newKey = (static_cast<long long>(chunk.chunkX) << 32) | static_cast<unsigned int>(chunk.chunkY);
		m_chunks[newKey] = chunk;
	}

	// Save all shifted chunks to disk
	// Note: Unlock mutex before calling SaveAllChunks as it may use mutexes internally
	m_mutex.unlock();
	SaveAllChunks();
	m_mutex.lock();

	std::cout << "[ChunkManager] Shifted " << chunksToShift.size() << " chunks and saved to disk. New min coords: (" 
			  << (minCx >= 0 ? minCx : 0) << ", " << (minCy >= 0 ? minCy : 0) << ")\n";
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
	// When base path changes, ensure per-level meta file exists (created by LevelEditorScene). We'll not create it here, but callers should write it.
}
/////////////////////////////////



/////////////////////////////////
// SetWorldSize - Sets the world size in tiles, which is used to create a world mask for collision and pathfinding
void ChunkManager::SetWorldSize(int width, int height) {
	std::cout << "[ChunkManager] SetWorldSize called with width=" << width << " height=" << height << "\n";
	worldWidth = width;
	worldHeight = height;

	if (width > 0 && height > 0)
		worldMask.assign(width * height, false);
	else
		worldMask.clear();
}
/////////////////////////////////



/////////////////////////////////
// RebuildAllChunksFromTileset - Rebuilds the vertex arrays for all loaded chunks using the current tileset atlas. This is called when the tileset changes to update the visual representation of all chunks.
void ChunkManager::RebuildAllChunksFromTileset() {
	// Fetch atlas once outside the per-chunk loop
	std::shared_ptr<TextureAtlas> atlasPtr;
	if (!m_tilesetKey.empty()) {
		auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(m_tilesetKey);
		if (atlasOpt.has_value() && *atlasOpt) {
			atlasPtr = *atlasOpt;
			std::cout << "ChunkManager::RebuildAllChunksFromTileset - Atlas '" << m_tilesetKey << "' loaded successfully\n";
		} else {
			std::cout << "ChunkManager::RebuildAllChunksFromTileset - Atlas '" << m_tilesetKey << "' NOT FOUND!\n";
		}
	} else {
		std::cout << "ChunkManager::RebuildAllChunksFromTileset - No tileset key set\n";
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& pr : m_chunks) {
		BuildChunkVertexArray(pr.second, atlasPtr);
	}
}
/////////////////////////////////



/////////////////////////////////
// BuildChunkVertexArray - Builds the sf::VertexArray for a chunk from its tile data. Called from RebuildAllChunksFromTileset, 
// FinalizeLoadedChunk, and SetTileAt. Pass a nullptr atlas to use the solid-colour fallback.
void ChunkManager::BuildChunkVertexArray(Chunk& chunk, const std::shared_ptr<TextureAtlas>& atlas) {
	// Ensure vertexArrays vector matches numLayers
	if (chunk.vertexArrays.size() != (size_t)chunk.numLayers) chunk.vertexArrays.resize(chunk.numLayers);
	
	// Set the vertex texture for the chunk based on the provided atlas. If no atlas is provided, reset the vertex texture to null.
	if (atlas) {
		chunk.vertexTexture = atlas->GetTexture();
	} else {
		chunk.vertexTexture.reset();
	}

	// Precompute base positions and tile size for efficiency
	const float tileSize = chunk.tileSize;
	const int baseX = chunk.chunkX * chunk.width;
	const int baseY = chunk.chunkY * chunk.height;

	// Iterate through each layer of the chunk to build its vertex array
	for (int layer = 0; layer < chunk.numLayers; ++layer) {
		sf::VertexArray &vertArray = chunk.vertexArrays[layer];
		vertArray.clear();
		vertArray.setPrimitiveType(sf::PrimitiveType::Triangles);
		
		// iterate tiles for this layer's x,y and build vertex array
		for (int y = 0; y < chunk.height; ++y) {
			for (int x = 0; x < chunk.width; ++x) {

				// Get the tile value for this position and layer. If the layer index is out of bounds, default to 0 (no tile).
				int value = 0;

				// Check if the layer index is valid and retrieve the tile value from the chunk's tilesPerLayer vector
				if (layer < (int)chunk.tilesPerLayer.size()) value = chunk.tilesPerLayer[layer][y * chunk.width + x];

				// Skip empty tiles (value 0) to avoid unnecessary vertex generation
				if (value == 0) continue;

				// Snap tile positions to whole pixels to prevent sub-pixel jitter and seam artifacts
				const float px = std::round((baseX + x) * tileSize);
				const float py = std::round((baseY + y) * tileSize);

				// Determine the texture coordinates (UVs) for the tile based on the atlas. If the atlas 
				// is not provided or the tile index is invalid, fallback to a solid color.
				bool usedTexture = false;
				sf::Vector2f uv00, uv11; // UV coordinates for the tile in the texture atlas

				// If an atlas is provided, attempt to retrieve the texture coordinates for the tile index (value - 1)
				if (atlas) {
					auto rectOpt = atlas->GetSfFloatRectForTile((size_t)(value - 1)); // Convert 1-based tile index to 0-based for atlas lookup
					
					// If the atlas provides valid texture coordinates, set the UVs and mark that we are using a texture
					if (rectOpt.has_value()) {
						const sf::FloatRect& fr = *rectOpt;
						uv00 = { fr.position.x, fr.position.y };
						uv11 = { fr.position.x + fr.size.x, fr.position.y + fr.size.y };
						usedTexture = true;
					}
				}

				// Append vertices for the tile quad to the vertex array. If a texture is used, include UVs; otherwise, use a fallback color.
				if (usedTexture && chunk.vertexTexture) {
					const float snapTs = std::round(tileSize);  // Round tile size to whole pixels for consistent rendering
					vertArray.append({ {px,				py				}, sf::Color::White, uv00 });
					vertArray.append({ {px + snapTs,	py				}, sf::Color::White, {uv11.x, uv00.y} });
					vertArray.append({ {px + snapTs,	py +	snapTs	}, sf::Color::White, uv11 });
					vertArray.append({ {px,				py				}, sf::Color::White, uv00 });
					vertArray.append({ {px + snapTs,	py +	snapTs	}, sf::Color::White, uv11 });
					vertArray.append({ {px,				py +	snapTs	}, sf::Color::White, {uv00.x, uv11.y} });
				} else {
					// Fallback color for tiles without texture. In-editor, show an opaque placeholder so painted tiles are visible even
					// when a tileset isn't loaded. If a tileset key is configured we keep transparent fallback so cleared tiles show through.
					const float snapTs = std::round(tileSize);  // Round tile size to whole pixels for consistent rendering
					uint8_t alpha = atlas ? 0u : 255u;
					sf::Color fallback(120, 120, 120, alpha);
					vertArray.append({ {px,				py				}, fallback });
					vertArray.append({ {px + snapTs,	py				}, fallback });
					vertArray.append({ {px + snapTs,	py +	snapTs	}, fallback });
					vertArray.append({ {px,				py				}, fallback });
					vertArray.append({ {px + snapTs,	py +	snapTs	}, fallback });
					vertArray.append({ {px,				py +	snapTs	}, fallback });
				}
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


	
	// Prepare a list of visible chunks to draw. We copy only the necessary draw information to avoid holding the mutex for too long.
	std::vector<DrawInfo> visible;
	{
		// Lock the mutex to safely access the m_chunks map and determine which chunks intersect the current view
		std::lock_guard<std::mutex> lock(m_mutex);
		visible.reserve(32);
		
		// Iterate through all loaded chunks and determine which ones intersect the current view. Only those chunks will be drawn.
		for (const auto& pair : m_chunks) {
			const Chunk& chunks = pair.second;
			
			// Skip chunks with zero width or height, as they have no visible content
			if (chunks.width <= 0 || chunks.height <= 0) continue;
			const float cx      = (float)(chunks.chunkX * chunks.width)  * chunks.tileSize;
			const float cy      = (float)(chunks.chunkY * chunks.height) * chunks.tileSize;
			const float cright  = cx + (float)chunks.width  * chunks.tileSize;
			const float cbottom = cy + (float)chunks.height * chunks.tileSize;
			
			// Skip chunks that are completely outside the view bounds to avoid unnecessary draw calls
			if (cright <= vleft || cx >= vright || cbottom <= vtop || cy >= vbottom) continue;
			DrawInfo& drawInfo = visible.emplace_back();

			// Merge per-layer vertex arrays into a single array for drawing (layers are drawn in order)
			drawInfo.vertexArray.clear();
			drawInfo.vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);
			
			// Iterate through each layer of the chunk and append its vertex data to the DrawInfo's vertex array. 
			// This allows for a single draw call per chunk.
			for (int layer = 0; layer < chunks.numLayers; ++layer) {
				if (chunks.vertexArrays.size() > (size_t)layer && chunks.vertexArrays[layer].getVertexCount() > 0) {					
					// Append the vertices from this layer's vertex array to the DrawInfo's vertex array
					for (size_t vertexIndex = 0; vertexIndex < chunks.vertexArrays[layer].getVertexCount(); ++vertexIndex) {
						drawInfo.vertexArray.append(chunks.vertexArrays[layer][vertexIndex]);
					}
				}
			}

			// Store the texture pointer for the chunk's vertex arrays. This allows the draw call to 
			// use the correct texture for rendering.
			drawInfo.vertexTexture = chunks.vertexTexture;
			bool allZero = true;
			
			// Check if all layers are empty (all tiles are zero). If so, we can mark the chunk as ready 
			// for rendering even if no layers are marked ready.
			for (int layer = 0; layer < chunks.numLayers; ++layer) { 
				for (int t : chunks.tilesPerLayer[layer]) { 
					if (t != 0) { allZero = false; break; } 
				} 
				if (!allZero) break; 
			}
			
			// Ready if any layer marked ready or all layers empty
			bool anyReady = false; 
			for (int layer = 0; layer < chunks.numLayers; ++layer) {
				if (chunks.readyForRendering[layer]) {
					anyReady = true;
					break;
				}
			}

			// Set the readyForRendering flag for this DrawInfo based on whether any layer is ready or if all layers are empty
			drawInfo.readyForRendering = anyReady || allZero;
			drawInfo.chunkX = chunks.chunkX; drawInfo.chunkY = chunks.chunkY;
			drawInfo.width  = chunks.width;  drawInfo.height = chunks.height;
			drawInfo.tileSize = chunks.tileSize;
			
			// *** Debug: report vertex counts for this chunk
			if (drawInfo.vertexArray.getVertexCount() > 0) {
				// debug output removed: verbose per-chunk draw logging
			} // ***

			// Add the DrawInfo for this chunk to the list of visible chunks to be drawn
			visible.push_back(std::move(drawInfo));
		}
	}

	// Draw all visible chunks. We iterate through the list of visible DrawInfo structs and issue draw calls for each one.
	for (const DrawInfo& drawInfo : visible) {
		if (!drawInfo.readyForRendering) {
			// placeholder for non-ready chunks: make fully transparent so empty areas are visible
			// (previously drew a semi-opaque grey rectangle here)
			// sf::RectangleShape r(sf::Vector2f((float)d.width * d.tileSize, (float)d.height * d.tileSize));
			// r.setPosition({wx, wy});
			// r.setFillColor(sf::Color(60, 60, 60, 80));
			// window.draw(r);
			continue;
		}

		// Draw the merged vertex array for this chunk. If the vertex array is empty, we skip drawing to avoid unnecessary draw calls.
		if (drawInfo.vertexArray.getVertexCount() > 0) {
			sf::RenderStates states; // default render states
			
			// Set the texture for the draw call if the chunk has a valid vertex texture. This allows the draw call to use the correct texture for rendering.
			if (drawInfo.vertexTexture) states.texture = drawInfo.vertexTexture.get();
			
			// Ensure alpha blending is used so texture transparency is visible
			states.blendMode = sf::BlendAlpha;
			
			// If the engine has an active layer set, we need to draw other layers dimmed. We rendered merged vertexArray in order so
			// we cannot distinguish layers at draw time. Simpler approach: draw per-layer instead of merged when alpha-dimming is active.
			if (true) {
				// per-layer draw to allow dimming, stub
				for (const auto& pair : m_chunks) { /* No-op to keep symbol referenced */ }
				
				// Re-lock and draw per-chunk per-layer to allow alpha modulation
				std::lock_guard<std::mutex> lock(m_mutex);

				// Iterate through all loaded chunks and draw each layer separately, 
				// applying alpha modulation for unselected layers
				for (const auto& pair : m_chunks) {
					const Chunk& chunk = pair.second;

					// Skip chunks that are not ready for rendering or have no vertex arrays
					if (chunk.chunkX != drawInfo.chunkX || chunk.chunkY != drawInfo.chunkY) continue;

					// Draw each layer of the chunk separately, applying alpha modulation for unselected layers
					for (int layer = 0; layer < chunk.numLayers; ++layer) {
						
						// Skip layers that are not ready for rendering or have no vertex arrays
						if (chunk.vertexArrays.size() <= (size_t)layer) continue;
						auto &vertArray = chunk.vertexArrays[layer];

						// Skip empty vertex arrays to avoid unnecessary draw calls
						if (vertArray.getVertexCount() == 0) continue;
						
						// create a copy to modulate alpha
						sf::VertexArray temp = vertArray;
						float alphaMul = 1.0f;
						
						// If an active layer is set and this layer is not the active one, apply the unselected layer alpha multiplier
						if (m_activeLayer >= 0 && layer != m_activeLayer) alphaMul = m_unselectedLayerAlpha; 
						
						// Modulate the alpha of each vertex in the copied vertex array
						for (size_t vertIndex = 0; vertIndex < temp.getVertexCount(); ++vertIndex) {
							sf::Color ccol = temp[vertIndex].color;
							ccol.a = static_cast<uint8_t>((float)ccol.a * alphaMul);
							temp[vertIndex].color = ccol;
						}

						// Draw the modulated vertex array for this layer with the appropriate render states
						sf::RenderStates layerStates = states;

						// Set the texture for the draw call if the chunk has a valid vertex texture. This allows the draw call to use 
						// the correct texture for rendering.
						if (chunk.vertexTexture) layerStates.texture = chunk.vertexTexture.get();
						window.draw(temp, layerStates);
					}
				}
			} else { /* otherwise draw the merged vertex array */
				window.draw(drawInfo.vertexArray, states);
			}
		}
		// Optional diagnostics: draw a faint overlay for chunks that have no texture or are all-zero
		// (controlled from LevelEditorScene debug UI)
	}
}
/////////////////////////////////



/////////////////////////////////
// UpdateStreaming - Placeholder for future streaming logic. Currently does nothing, but can be extended to implement dynamic 
// loading/unloading of chunks based on the view.
void ChunkManager::UpdateStreaming(const sf::View& view)
{
	// 1. Convert view bounds → chunk coords
	int minCx = floor((view.getCenter().x - view.getSize().x * 0.5f) / (m_chunkWidth * m_tileSize));
	int maxCx = floor((view.getCenter().x + view.getSize().x * 0.5f) / (m_chunkWidth * m_tileSize));
	int minCy = floor((view.getCenter().y - view.getSize().y * 0.5f) / (m_chunkHeight * m_tileSize));
	int maxCy = floor((view.getCenter().y + view.getSize().y * 0.5f) / (m_chunkHeight * m_tileSize));

	// 2. For each chunk in bounds → ensure loaded
	for (int cy = minCy; cy <= maxCy; ++cy)
		for (int cx = minCx; cx <= maxCx; ++cx)
			EnsureChunkLoaded(cx, cy);

	// 3. Evict chunks outside radius
	EvictChunksOutsideRadius(minCx, maxCx, minCy, maxCy);
}
/////////////////////////////////



/////////////////////////////////
// EnqueueChunks - Enqueue all visible chunks to the render queue with depth-based sorting
// This is the ECS-aligned rendering method that replaces DrawChunks for use with the render queue
void ChunkManager::EnqueueChunks(RenderQueue& queue, const sf::View& view) {
	
	// Compute the view bounds in world coordinates to determine which chunks are visible
	const sf::Vector2f viewCenter = view.getCenter();
	const sf::Vector2f viewSize = view.getSize();
	const float vleft = viewCenter.x - viewSize.x * 0.5f;
	const float vtop = viewCenter.y - viewSize.y * 0.5f;
	const float vright = vleft + viewSize.x;
	const float vbottom = vtop + viewSize.y;

	// Lock the mutex to safely access the m_chunks map and determine which chunks intersect the current view
	std::lock_guard<std::mutex> lock(m_mutex);
	
	// Iterate through all loaded chunks and enqueue those that intersect the current view for rendering
	for (auto& pair : m_chunks) {
		Chunk& c = pair.second;
		
		
		if (c.width <= 0 || c.height <= 0) continue;
		const float cx = (float)(c.chunkX * c.width) * c.tileSize;
		const float cy = (float)(c.chunkY * c.height) * c.tileSize;
		const float cright = cx + (float)c.width * c.tileSize;
		const float cbottom = cy + (float)c.height * c.tileSize;
		if (cright <= vleft || cx >= vright || cbottom <= vtop || cy >= vbottom) continue;

		// Check if this chunk is ready to render
		bool anyReady = false; 
		for (int L = 0; L < c.numLayers; ++L) if (c.readyForRendering[L]) { anyReady = true; break; }
		bool allZero = true;
		for (int L = 0; L < c.numLayers; ++L) { for (int t : c.tilesPerLayer[L]) { if (t != 0) { allZero = false; break; } } if (!allZero) break; }
		if (!anyReady && !allZero) continue; // Skip if not ready and has tiles

		// Enqueue per-layer vertex arrays with alpha modulation
		for (int L = 0; L < c.numLayers; ++L) {
			if (c.vertexArrays.size() <= (size_t)L) continue;
			auto& va = c.vertexArrays[L];
			if (va.getVertexCount() == 0) continue;

			// Create a modified copy that will stay alive in tempVertexArraysForRendering[L]
			if (c.tempVertexArraysForRendering.size() <= (size_t)L) continue;
			c.tempVertexArraysForRendering[L] = va; // Store a copy in the chunk's per-layer temp buffer

			// Apply alpha modulation to the copy
			float alphaMul = 1.0f;
			if (m_activeLayer >= 0 && L != m_activeLayer) alphaMul = m_unselectedLayerAlpha;

			for (size_t vi = 0; vi < c.tempVertexArraysForRendering[L].getVertexCount(); ++vi) {
				sf::Color col = c.tempVertexArraysForRendering[L][vi].color;
				col.a = static_cast<uint8_t>((float)col.a * alphaMul);
				c.tempVertexArraysForRendering[L][vi].color = col;
			}

			DrawRequest req;
			req.drawable = &c.tempVertexArraysForRendering[L];
			req.texture = c.vertexTexture;
			req.blendMode = sf::BlendAlpha;
			req.depth = 10 + L; // depth 10-12 for layers to maintain ordering

			queue.Enqueue(req);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// EnsureChunkLoaded - Ensures that the chunk at (cx, cy) is loaded. If it is not already loaded, it will be enqueued 
// for loading in the background thread.
void ChunkManager::EnsureChunkLoaded(int cx, int cy) {
	std::lock_guard<std::mutex> lk(m_mutex);

	long long key = GetChunkKey(cx, cy);

	// Already loaded or already created
	if (m_chunks.find(key) != m_chunks.end()) return;

	// Create empty chunk with correct dimensions
	m_chunks[key] = Chunk(cx, cy, m_chunkWidth, m_chunkHeight, m_tileSize, m_numLayers);

	// Enqueue background load for each layer
	for (int layer = 0; layer < m_numLayers; ++layer)
		EnqueueLoadChunk(cx, cy, layer);

	// Mark as recently used (for LRU)
	TouchChunkLRU(key);
}
/////////////////////////////////



/////////////////////////////////
// EvictChunksOutsideRadius - Evicts chunks that are outside the specified chunk coordinate bounds (minCx, maxCx, minCy, maxCy).
void ChunkManager::EvictChunksOutsideRadius(int minCx, int maxCx, int minCy, int maxCy) {
	std::lock_guard<std::mutex> lk(m_mutex);

	std::vector<long long> toRemove;

	for (auto& [key, chunk] : m_chunks) {
		int cx = chunk.chunkX;
		int cy = chunk.chunkY;

		bool outside = cx < minCx || cx > maxCx || cy < minCy || cy > maxCy;
		if (outside)
		{
			toRemove.push_back(key);
		}
		}
			

	// Remove chunks safely
	EntityManager& em = GameEngine::GetInstance().GetEntityManager();

	for (long long key : toRemove) {
		Chunk& c = m_chunks[key];

		// Kill generated entities
		for (Entity* ge : c.generatedEntities)
			if (ge)
				em.SafeKillEntity(ge);


		// Clear world mask for this chunk
		if (worldWidth > 0 && worldHeight > 0 && !worldMask.empty()) {

			int baseX = c.chunkX * c.width;
			int baseY = c.chunkY * c.height;

			for (int y = 0; y < c.height; ++y) {
				for (int x = 0; x < c.width; ++x) {

					int worldX = baseX + x;
					int worldY = baseY + y;

					int maskX = worldX - m_worldOffsetX;
					int maskY = worldY - m_worldOffsetY;

					if (maskX < 0 || maskY < 0 || maskX >= worldWidth || maskY >= worldHeight)
						continue;

					worldMask[maskY * worldWidth + maskX] = 0; // clear tile
				}
			}
		}

		// Remove from LRU
		RemoveChunkFromLRU(key);

		// Remove chunk
		m_chunks.erase(key);
	}
}
/////////////////////////////////



/////////////////////////////////
// GetTileAt - Retrieves the tile value at the specified tile coordinates (tileX, tileY). If the corresponding chunk is not loaded, it will be enqueued for loading in the 
// background thread. Returns 0 if the chunk is not loaded or if the tile coordinates are out of bounds within the chunk.
int ChunkManager::GetTileAt(int tileX, int tileY, int layerIndex) {
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
			if (layerIndex < 0 || layerIndex >= chunk.numLayers) return 0;
			if (localX >= 0 && localX < chunk.width && localY >= 0 && localY < chunk.height)
				return chunk.tilesPerLayer[layerIndex][localY * chunk.width + localX];
			return 0;
		}
	} // release lock before enqueue to avoid re-entrancy / deadlock
	EnqueueLoadChunk(chunkX, chunkY, layerIndex);
	return 0;
}
/////////////////////////////////



/////////////////////////////////
// SetTileAt - Sets the tile value at the specified tile coordinates (tileX, tileY) to the given tileValue. If the corresponding chunk is not loaded, it will be enqueued 
// for loading in the background thread.
int ChunkManager::SetTileAt(int tileX, int tileY, int tileValue, int layerIndex) {
	const int chunkX = FloorDiv(tileX, m_chunkWidth);
	const int chunkY = FloorDiv(tileY, m_chunkHeight);
	const int localX = tileX - chunkX * m_chunkWidth;
	const int localY = tileY - chunkY * m_chunkHeight;
	const long long key = GetChunkKey(chunkX, chunkY);

	bool insertedNow = false;

	// PUBLIC LOCK — only once
	std::lock_guard<std::mutex> lock(m_mutex);

	// Create placeholder chunk if needed
	auto [itr, inserted] =
		m_chunks.emplace(key, Chunk(chunkX, chunkY, m_chunkWidth, m_chunkHeight, m_tileSize, m_numLayers));

	insertedNow = inserted;

	if (insertedNow) {
		// LRU insert
		m_lruList.push_front(key);
		m_lruIndex[key] = m_lruList.begin();
	}

	Chunk& chunk = itr->second;

	// Bounds check
	if (localX < 0 || localX >= chunk.width || localY < 0 || localY >= chunk.height || layerIndex < 0 ||
		layerIndex >= chunk.numLayers) {
		return 0;
	}

	const int index = localY * chunk.width + localX;
	const int prevValue = chunk.tilesPerLayer[layerIndex][index];

	// Placeholder chunks must always accept edits
	if (prevValue == tileValue && !insertedNow && tileValue != 0)
		return prevValue;

	// Apply tile
	chunk.tilesPerLayer[layerIndex][index] = tileValue;
	chunk.dirty[layerIndex] = 1;
	chunk.editVersion[layerIndex]++;

	// LRU touch
	auto lruIt = m_lruIndex.find(key);
	if (lruIt != m_lruIndex.end())
		m_lruList.erase(lruIt->second);

	m_lruList.push_front(key);
	m_lruIndex[key] = m_lruList.begin();

	// Schedule rebuild
	ScheduleChunkForRebuild(chunk);
	chunk.readyForRendering[layerIndex] = 1;

	// Save immediately (same as your original code)
	if (!m_basePath.empty()) {
		try {
			bool anyNonZero = false;

			for (int layer = 0; layer < chunk.numLayers; ++layer) {
				std::string filename =
					(fs::path(m_basePath) / ("chunk_" + std::to_string(layer) + "_" + std::to_string(chunk.chunkX) +
											 "_" + std::to_string(chunk.chunkY) + ".dat"))
						.string();

				bool allZero = true;
				for (int t : chunk.tilesPerLayer[layer]) {
					if (t != 0) {
						allZero = false;
						break;
					}
				}

				if (allZero) {
					try {
						fs::remove(filename);
					} catch (...) {}
					chunk.dirty[layer] = false;
				} else {
					anyNonZero = true;
					std::ofstream outFile(filename, std::ios::binary);
					if (outFile) {
						outFile.write(reinterpret_cast<const char*>(chunk.tilesPerLayer[layer].data()),
									  chunk.tilesPerLayer[layer].size() * sizeof(int));
						chunk.dirty[layer] = false;
					}
				}
			}
		} catch (...) {}
	}

	// IMPORTANT:
	// Call NO-LOCK version so we do NOT lock m_mutex again.
	if (insertedNow) {
		EnqueueLoadChunk_NoLock(chunkX, chunkY);
	}

	return prevValue;
}

/////////////////////////////////



/////////////////////////////////
// RemoveChunkFromLRU - Removes the chunk with the specified key from the LRU (Least Recently Used) list. This is called when a chunk is evicted to ensure 
// it is no longer tracked in the LRU.
void ChunkManager::RemoveChunkFromLRU(long long key) {
	auto it = m_lruIndex.find(key);
	if (it == m_lruIndex.end())
		return; // Chunk not in LRU list

	// Erase the node from the list
	m_lruList.erase(it->second);

	// Remove the index entry
	m_lruIndex.erase(it);
}
/////////////////////////////////



/////////////////////////////////
// TouchChunkLRU - Updates the LRU (Least Recently Used) list to mark the chunk with the specified key as recently used. If the chunk is already in the LRU list, 
// it is moved to the front; otherwise, it is added to the front.
void ChunkManager::TouchChunkLRU(long long key) {
	// If key already exists in LRU, move it to the front
	auto it = m_lruIndex.find(key);
	if (it != m_lruIndex.end()) {
		// Move existing entry to front
		m_lruList.splice(m_lruList.begin(), m_lruList, it->second);
		it->second = m_lruList.begin();
		return;
	}

	// Otherwise insert new entry at front
	m_lruList.push_front(key);
	m_lruIndex[key] = m_lruList.begin();
}
/////////////////////////////////



/////////////////////////////////
// EnsureChunksInTileRect - Ensures that all chunks that intersect the specified tile rectangle (tileX0, tileY0, tileX1, tileY1) are loaded and ready for rendering. 
// The marginChunks parameter specifies how many additional chunks to load around the edges of the rectangle to ensure smooth rendering when the player moves.
void ChunkManager::EnsureChunksInTileRect(int tileX0, int tileY0, int tileX1, int tileY1, int marginChunks) {
	if (tileX0 > tileX1)
		std::swap(tileX0, tileX1);
	if (tileY0 > tileY1)
		std::swap(tileY0, tileY1);

	int cX0 = FloorDiv(tileX0, m_chunkWidth) - marginChunks;
	int cY0 = FloorDiv(tileY0, m_chunkHeight) - marginChunks;
	int cX1 = FloorDiv(tileX1, m_chunkWidth) + marginChunks;
	int cY1 = FloorDiv(tileY1, m_chunkHeight) + marginChunks;

	// Safety clamp: avoid attempting to load an extremely large span of chunks
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

	// === FIX: clamp chunk coords to world bounds ===
	const int minChunkX = 0;
	const int maxChunkX = 1;
	const int minChunkY = 0;
	const int maxChunkY = 13;
	// ===============================================

	for (int cy = cY0; cy <= cY1; ++cy) {
		for (int cx = cX0; cx <= cX1; ++cx) {

			// FIX: skip invalid chunk coords
			if (cx < minChunkX || cx > maxChunkX || cy < minChunkY || cy > maxChunkY)
				continue;

			const long long key = GetChunkKey(cx, cy);
			bool needLoad = false;

			{
				std::lock_guard<std::mutex> lock(m_mutex);

				if (m_chunks.find(key) != m_chunks.end()) {
					auto lruIt = m_lruIndex.find(key);
					if (lruIt != m_lruIndex.end())
						m_lruList.erase(lruIt->second);

					m_lruList.push_front(key);
					m_lruIndex[key] = m_lruList.begin();
					continue;
				}

				auto insertRes =
					m_chunks.emplace(key, Chunk(cx, cy, m_chunkWidth, m_chunkHeight, m_tileSize, m_numLayers));

				auto insertedItr = insertRes.first;
				bool insertedNow = insertRes.second;

				m_lruList.push_front(key);
				m_lruIndex[key] = m_lruList.begin();

				if (insertedNow) {
					Chunk& newChunk = insertedItr->second;
					bool allZero = true;

					for (int i = 0; i < newChunk.width * newChunk.height; ++i) {
						for (int L = 0; L < newChunk.numLayers; ++L) {
							if (!newChunk.tilesPerLayer.empty() && newChunk.tilesPerLayer[L][i] != 0) {
								allZero = false;
								break;
							}
						}
						if (!allZero)
							break;
					}

					if (allZero) {
						for (int L = 0; L < newChunk.numLayers; ++L)
							if (!newChunk.readyForRendering.empty())
								newChunk.readyForRendering[L] = 1;
					}
				}

				needLoad = true;
			}

			if (needLoad)
				EnqueueLoadChunk(cx, cy);
		}
	}
}
/////////////////////////////////


/////////////////////////////////
// EnsureChunksInTileRect_NoLock - Similar to EnsureChunksInTileRect, but does not acquire the mutex lock. This is intended for use in contexts where the caller already holds the lock,
void ChunkManager::EnsureChunksInTileRect_NoLock(int tileX0, int tileY0, int tileX1, int tileY1, int marginChunks) {
	if (tileX0 > tileX1)
		std::swap(tileX0, tileX1);
	if (tileY0 > tileY1)
		std::swap(tileY0, tileY1);

	int cX0 = FloorDiv(tileX0, m_chunkWidth) - marginChunks;
	int cY0 = FloorDiv(tileY0, m_chunkHeight) - marginChunks;
	int cX1 = FloorDiv(tileX1, m_chunkWidth) + marginChunks;
	int cY1 = FloorDiv(tileY1, m_chunkHeight) + marginChunks;

	// Safety clamp
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

			// Check existence WITHOUT locking
			auto it = m_chunks.find(key);
			if (it != m_chunks.end()) {
				// LRU touch (no lock)
				auto lruIt = m_lruIndex.find(key);
				if (lruIt != m_lruIndex.end())
					m_lruList.erase(lruIt->second);

				m_lruList.push_front(key);
				m_lruIndex[key] = m_lruList.begin();
				continue;
			}

			// Insert placeholder chunk (no lock)
			auto [itr2, insertedNow] =
				m_chunks.emplace(key, Chunk(cx, cy, m_chunkWidth, m_chunkHeight, m_tileSize, m_numLayers));

			m_lruList.push_front(key);
			m_lruIndex[key] = m_lruList.begin();

			if (insertedNow) {
				Chunk& newChunk = itr2->second;

				bool allZero = true;
				for (int i = 0; i < newChunk.width * newChunk.height; ++i) {
					for (int L = 0; L < newChunk.numLayers; ++L) {
						if (!newChunk.tilesPerLayer.empty() && newChunk.tilesPerLayer[L][i] != 0) {
							allZero = false;
							break;
						}
					}
					if (!allZero)
						break;
				}

				if (allZero) {
					for (int L = 0; L < newChunk.numLayers; ++L)
						if (!newChunk.readyForRendering.empty())
							newChunk.readyForRendering[L] = 1;
				}
			}

			// Queue background load (no lock)
			EnqueueLoadChunk_NoLock(cx, cy);
		}
	}
}



/////////////////////////////////
// UpdateMainThread - This method should be called from the main thread to perform any necessary updates, such as processing dirty 
// chunks or preparing vertex buffers for rendering.
void ChunkManager::UpdateMainThread() {
	
	// Process pending loaded chunks in small batches to avoid spending a long time on the main thread; limit the number of pending 
	// chunks processed per frame to 128 this should avoid frame stalls, if there are more pending chunks they will be processed 
	// in following frames
	const int kMaxPendingPerFrame = 128;

	// Copy pending chunks to a local vector to minimize lock time
	std::vector<std::tuple<int, int, int, std::vector<int>, uint32_t>> pendingChunksCopy;


	// Lock the pending chunks mutex and copy up to kMaxPendingPerFrame chunks to the local vector, 
	// then erase them from the global queue
	{ 
		std::lock_guard<std::mutex> lock(s_pendingMutex);
		int take = (int)std::min<size_t>(s_pendingChunks.size(), (size_t)kMaxPendingPerFrame);
		pendingChunksCopy.reserve(take);
		for (int i = 0; i < take; ++i) pendingChunksCopy.emplace_back(std::move(s_pendingChunks[i]));
		if (take > 0) s_pendingChunks.erase(s_pendingChunks.begin(), s_pendingChunks.begin() + take);
	}

	// Finalize each loaded chunk that we copied (bounded count)
	for (const auto& pending : pendingChunksCopy) {
		const auto& [chunkX, chunkY, layer, tileData, version] = pending;
		FinalizeLoadedChunk(chunkX, chunkY, layer, tileData, version);
	}

	// Process scheduled rebuilds (GPU / SFML dependent work) on the main thread.
	std::vector<long long> rebuilds;

	// Lock the mutex and swap the rebuild queue into a local vector to minimize lock time
	{ 
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_rebuildQueue.empty()) {
			rebuilds.swap(m_rebuildQueue);
			
			// Remove these keys from the rebuild, avoids duplicate rebuilds if chunk has be re-scheduled 
			// while we are processing the current rebuilds
			for (auto key : rebuilds) m_rebuildSet.erase(key);
		}
	}

	// Rebuild vertex arrays and colliders for all scheduled chunks on the main thread
	for (long long key : rebuilds) {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_chunks.find(key);
		if (it == m_chunks.end()) continue;

		Chunk& chunk = it->second;
		// Rebuild vertex arrays and colliders for all layers of this chunk using the main-thread-only API
		std::shared_ptr<TextureAtlas> atlasPtr;
		
		// Fetch the current tileset atlas for this chunk. If no tileset is set, atlasPtr will remain nullptr 
		// and BuildChunkVertexArray will use the fallback color.
		if (!m_tilesetKey.empty()) {
			auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(m_tilesetKey);
			if (atlasOpt.has_value() && *atlasOpt) atlasPtr = *atlasOpt;
		}

		// Rebuild the vertex array and entity colliders for this chunk
		BuildChunkVertexArray(chunk, atlasPtr);
		RebuildChunkEntities(chunk);
		
		// mark ready
		for (int L = 0; L < chunk.numLayers; ++L) chunk.readyForRendering[L] = 1;
	}
}
/////////////////////////////////



/////////////////////////////////
// SaveAllChunks - Saves all dirty chunks to disk in the specified directory. Each chunk will be saved as a separate file named "chunk_X_Y.dat" where X and Y are the chunk coordinates.
void ChunkManager::SaveAllChunks() {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& pr : m_chunks) {
		Chunk& chunk = pr.second;
		for (int layer = 0; layer < chunk.numLayers; ++layer) {
			if (!chunk.dirty[layer]) continue;
			std::filesystem::path p(m_basePath);
			std::string filename = (p / ("chunk_" + std::to_string(layer) + "_" + std::to_string(chunk.chunkX) + "_" + std::to_string(chunk.chunkY) + ".dat")).string();
			bool allZero = true;
			for (int t : chunk.tilesPerLayer[layer]) { if (t != 0) { allZero = false; break; } }
			if (allZero) {
				try { fs::remove(filename); } catch(...) {}
				chunk.dirty[layer] = false;
				continue;
			}
			std::ofstream outFile(filename, std::ios::binary);
			if (outFile) {
				outFile.write(reinterpret_cast<const char*>(chunk.tilesPerLayer[layer].data()), chunk.tilesPerLayer[layer].size() * sizeof(int));
				chunk.dirty[layer] = false;
			} else {
				std::cerr << "Error saving chunk to file: " << filename << std::endl;
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// EnqueueLoadChunk - Enqueues a chunk to be loaded in the background thread. The chunk will be loaded from disk if it exists, or created with default tile data if it does not.
// This overload loads all layers of the chunk. If you want to load a specific layer, use the overload that accepts a layer index.
void ChunkManager::EnqueueLoadChunk(int chunkX, int chunkY) {
	EnqueueLoadChunk(chunkX, chunkY, -1); // delagate to overload and use -1 to indicate all layers
}
/////////////////////////////////



/////////////////////////////////
// EnqueueLoadChunk - Overload that accepts specific layer to load. If layer is -1, loads all layers. Captures the current editVersion of the chunk at the time of enqueue to allow 
// FinalizeLoadedChunk to reject stale loads.
void ChunkManager::EnqueueLoadChunk(int chunkX, int chunkY, int layer) {
	// Capture the current editVersion of this chunk so FinalizeLoadedChunk can reject stale loads
	uint32_t versionAtEnqueue = 0;
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		auto it = m_chunks.find(GetChunkKey(chunkX, chunkY));
		if (it != m_chunks.end()) {
			if (!it->second.editVersion.empty()) versionAtEnqueue = it->second.editVersion[std::max(0, layer)];
		}
	}
	// Enqueue the load job with chunk coordinates, layer, base path, and captured version. The background loader threads will process this queue and perform the actual loading from disk.
	int localChunkW = m_chunkWidth;
	int localChunkH = m_chunkHeight;
	int localNumLayers = m_numLayers;
	{
		std::lock_guard<std::mutex> lk(s_loadQueueMutex);
		s_loadQueue.emplace_back(chunkX, chunkY, layer, m_basePath, versionAtEnqueue);
		
		// start loader pool once
		if (!s_loaderStarted) {
			s_loaderStarted = true;
			for (int i = 0; i < s_maxLoaderThreads; ++i) {
				std::thread([localChunkW, localChunkH, localNumLayers]() {
					while (true) {
						std::tuple<int,int,int,std::string,uint32_t> job;
						{
							std::unique_lock<std::mutex> ql(s_loadQueueMutex);
							s_loadCv.wait(ql, [] { return !s_loadQueue.empty(); });
							job = s_loadQueue.back(); s_loadQueue.pop_back();
						}
						int jobCx = std::get<0>(job);
						int jobCy = std::get<1>(job);
						int jobLayer = std::get<2>(job);
						std::string jobBase = std::get<3>(job);
						uint32_t jobVer = std::get<4>(job);
						
						// perform load for this job: attempt to read the specified layer only (or all layers if layer==-1)
						if (jobLayer < 0) {
							for (int L = 0; L < localNumLayers; ++L) {
								std::vector<int> tileData(localChunkW * localChunkH, 0);
								std::string name = std::string("chunk_") + std::to_string(L) + "_" + std::to_string(jobCx) + "_" + std::to_string(jobCy) + ".dat";
								std::string filename = (fs::path(jobBase) / name).string();
								
								std::cout << "[LOAD] C(" << jobCx << "," << jobCy << ")"
										  << " L=" << L << " file=" << filename << " exists=" << fs::exists(filename)
										  << " ver=" << jobVer << "\n";
								
								if (fs::exists(filename)) {
									std::ifstream inFile(filename, std::ios::binary);
									if (inFile) inFile.read(reinterpret_cast<char*>(tileData.data()), tileData.size() * sizeof(int));
								}
								{
									std::lock_guard<std::mutex> lock(s_pendingMutex);
									s_pendingChunks.emplace_back(jobCx, jobCy, L, std::move(tileData), jobVer);
								}
							}
						} else {
							std::vector<int> tileData(localChunkW * localChunkH, 0);
							std::string name = std::string("chunk_") + std::to_string(jobLayer) + "_" + std::to_string(jobCx) + "_" + std::to_string(jobCy) + ".dat";
							std::string filename = (fs::path(jobBase) / name).string();
							if (fs::exists(filename)) {
								std::ifstream inFile(filename, std::ios::binary);
								if (inFile) inFile.read(reinterpret_cast<char*>(tileData.data()), tileData.size() * sizeof(int));
							}
							{
								std::lock_guard<std::mutex> lock(s_pendingMutex);
								s_pendingChunks.emplace_back(jobCx, jobCy, jobLayer, std::move(tileData), jobVer);
							}
						}
					}
				}).detach();
			}
		}
	}
	s_loadCv.notify_one();
}
/////////////////////////////////



/////////////////////////////////
// EnqueueLoadChunk_NoLock - version safe to call when m_mutex is already held.
// This MUST NOT lock m_mutex. It only touches the global load queue mutex.
void ChunkManager::EnqueueLoadChunk_NoLock(int chunkX, int chunkY) {
	// Capture editVersion WITHOUT locking m_mutex
	uint32_t versionAtEnqueue = 0;

	auto it = m_chunks.find(GetChunkKey(chunkX, chunkY));
	if (it != m_chunks.end()) {
		// Use layer 0's version (your original two‑argument version implicitly loads all layers)
		if (!it->second.editVersion.empty()) {
			versionAtEnqueue = it->second.editVersion[0];
		}
	}

	// Local copies (no locking)
	int localChunkW = m_chunkWidth;
	int localChunkH = m_chunkHeight;
	int localNumLayers = m_numLayers;
	std::string base = m_basePath;

	// Push job into global load queue (this uses its own mutex)
	{
		std::lock_guard<std::mutex> lk(s_loadQueueMutex);

		// Two‑argument version: load ALL layers
		s_loadQueue.emplace_back(chunkX, chunkY, -1, base, versionAtEnqueue);

		if (!s_loaderStarted) {
			s_loaderStarted = true;

			for (int i = 0; i < s_maxLoaderThreads; ++i) {
				std::thread([localChunkW, localChunkH, localNumLayers]() {
					while (true) {
						std::tuple<int, int, int, std::string, uint32_t> job;

						{
							std::unique_lock<std::mutex> ql(s_loadQueueMutex);
							s_loadCv.wait(ql, [] { return !s_loadQueue.empty(); });
							job = s_loadQueue.back();
							s_loadQueue.pop_back();
						}

						int jobCx = std::get<0>(job);
						int jobCy = std::get<1>(job);
						int jobLayer = std::get<2>(job); // -1 = load all layers
						std::string jobBase = std::get<3>(job);
						uint32_t jobVer = std::get<4>(job);

						if (jobLayer < 0) {
							// Load ALL layers
							for (int L = 0; L < localNumLayers; ++L) {
								std::vector<int> tileData(localChunkW * localChunkH, 0);

								std::string filename =
									(fs::path(jobBase) / ("chunk_" + std::to_string(L) + "_" + std::to_string(jobCx) +
														  "_" + std::to_string(jobCy) + ".dat"))
										.string();

								if (fs::exists(filename)) {
									std::ifstream inFile(filename, std::ios::binary);
									if (inFile)
										inFile.read(reinterpret_cast<char*>(tileData.data()),
													tileData.size() * sizeof(int));
								}

								{
									std::lock_guard<std::mutex> lock(s_pendingMutex);
									s_pendingChunks.emplace_back(jobCx, jobCy, L, std::move(tileData), jobVer);
								}
							}
						} else {
							// (Not used in your two‑argument version)
						}
					}
				}).detach();
			}
		}
	}

	s_loadCv.notify_one();
}
	/////////////////////////////////



/////////////////////////////////
// RebuildChunkEntities - Rebuilds the collider entities for a chunk based on its current tile data. This should be called whenever the tile data changes to ensure that the 
// colliders match the visual representation of the chunk.
void ChunkManager::RebuildChunkEntities(Chunk& chunk) {
	// First, safely kill any existing entities that were generated for this chunk to avoid leaving orphaned entities in the world. We use SafeKillEntity to ensure that we 
	// don't attempt to access entities that may have already been destroyed.
	try {
		EntityManager& em = GameEngine::GetInstance().GetEntityManager();
		for (Entity* ge : chunk.generatedEntities) {
			if (ge) em.SafeKillEntity(ge);
		}

		// Clear the list of generated entities for this chunk so we can repopulate it with new colliders based on the current tile data.
		chunk.generatedEntities.clear();

		// Create a 2D array to track which tiles have already been processed into colliders. This prevents creating multiple colliders for the same contiguous area of tiles.
		std::vector<char> used(chunk.width * chunk.height, 0);
		for (int y = 0; y < chunk.height; ++y) {
			for (int x = 0; x < chunk.width; ++x) {
				int index = y * chunk.width + x;
				
				// Skip tiles that have already been processed into colliders
				if (used[index]) continue;
				
				// Get the tile value at this position. If the tile value is 0, (no tile) so skip.
				int val = 0;
				if (!chunk.tilesPerLayer.empty()) val = chunk.tilesPerLayer[0][index];
				if (val == 0) continue;
				
				// Determine the width of the contiguous area of tiles with the same value starting from (x, y). We expand to the right until we hit a different tile value or the edge of the chunk. 
				int width = 1;
				while (x + width < chunk.width &&
					   (!chunk.tilesPerLayer.empty() && chunk.tilesPerLayer[0][y * chunk.width + (x + width)] == val) 
						&& !used[y * chunk.width + (x + width)]) {
					++width;
				}

				// Determine the height of the contiguous area of tiles with the same value starting from (x, y). We expand downward until we hit a different tile value or the edge of the chunk.
				int height = 1;
				bool canExtend = true;
				while (y + height < chunk.height && canExtend) {
					for (int xi = 0; xi < width; ++xi) {
						if ((!chunk.tilesPerLayer.empty() && chunk.tilesPerLayer[0][(y + height) * chunk.width + (x + xi)] != val) || used[(y + height) * chunk.width + (x + xi)]) { canExtend = false; break; }
					}
					if (canExtend) ++height;
				}

				// Mark all tiles in the contiguous area as used so we don't process them again
				for (int yy = 0; yy < height; ++yy) {
					for (int xx = 0; xx < width; ++xx) { 
						used[(y + yy) * chunk.width + (x + xx)] = 1; 
					}
				}

				
				// Calculate the world position and size of the collider based on the chunk's position, tile size, and the dimensions of the contiguous area.
				float tileW = chunk.tileSize * width;
				float tileH = chunk.tileSize * height;
				float posX = (chunk.chunkX * chunk.width + x) * chunk.tileSize;
				float posY = (chunk.chunkY * chunk.height + y) * chunk.tileSize;
				
				// Create a new entity for this contiguous area of tiles. The entity will have a transform component, a rectangle shape component, and a static component to indicate it is not dynamic.
				Entity* ent = em.AddEntity(EntityType::Tile);
				if (ent) {
					ent->AddComponent<CTransform>(Vec2(posX, posY), Vec2::Zero);
					auto rect = std::make_unique<CRectangle>(tileW, tileH);
					rect->SetColor(160.0f, 160.0f, 160.0f, 200);
					ent->AddComponentPtr<CShape>(std::move(rect));
					ent->AddComponent<CStatic>();
					chunk.generatedEntities.push_back(ent);
				}
			}
		}
	} catch(...) {}
}
/////////////////////////////////



/////////////////////////////////
// ScheduleChunkForRebuild - Mark a chunk as requiring GPU/collider rebuild on the main thread.
void ChunkManager::ScheduleChunkForRebuild(Chunk& c) {
	long long key = GetChunkKey(c.chunkX, c.chunkY);
	// NOTE: callers (e.g. SetTileAt) may already hold m_mutex. Do not re-lock here
	// to avoid deadlock / std::system_error from double-locking a non-recursive mutex.
	if (m_rebuildSet.find(key) == m_rebuildSet.end()) {
		m_rebuildSet.insert(key);
		m_rebuildQueue.push_back(key);
	}
}
/////////////////////////////////



/////////////////////////////////
// FinalizeLoadedChunk
// This method is called on the main thread after a chunk has been loaded in the background. It finalizes the loaded chunk by updating its tile data, 
// marking it as ready for rendering, and scheduling it for GPU/collider rebuild. If the chunk was evicted before finalization or if the editVersion 
// has changed since the load was enqueued, the loaded data is discarded to avoid overwriting newer changes.
void ChunkManager::FinalizeLoadedChunk(int chunkX, int chunkY, int layer, std::vector<int> tileData, uint32_t versionAtEnqueue) {

	std::cout << "[FINALIZE] C(" << chunkX << "," << chunkY << ")"
			  << " L=" << layer << " ver=" << versionAtEnqueue << "\n";

	// Lock the mutex to ensure thread safety while accessing and modifying the chunk data structures
	std::lock_guard<std::mutex> lock(m_mutex);
	
	long long key = GetChunkKey(chunkX, chunkY);
	auto itr = m_chunks.find(key);
	if (itr == m_chunks.end()) {
		// Chunk was evicted before we could finalize — discard the load
		return;
	}

	Chunk& chunk = itr->second;
	if (layer < 0 || layer >= chunk.numLayers) return;
	// If the chunk's editVersion changed since the load was enqueued, the disk data is stale — skip overwriting
	if (chunk.editVersion[layer] != versionAtEnqueue) {
		chunk.readyForRendering[layer] = 1;
	} else {
		if ((int)tileData.size() == chunk.width * chunk.height) {
			chunk.tilesPerLayer[layer] = std::move(tileData);
		} else {
			chunk.tilesPerLayer[layer].assign(chunk.width * chunk.height, 0);
		}
		chunk.dirty[layer] = 0;
		chunk.readyForRendering[layer] = 1;
	}

	
	// Update world mask from obstacle layer (layer 1)
	if (worldWidth > 0 && worldHeight > 0 && !worldMask.empty()) {

		int baseX = chunk.chunkX * chunk.width;
		int baseY = chunk.chunkY * chunk.height;

		for (int y = 0; y < chunk.height; ++y) {
			for (int x = 0; x < chunk.width; ++x) {

				int idx = y * chunk.width + x;

				// Read obstacle tile from layer 1
				int tileVal = (chunk.tilesPerLayer.size() > 1) ? chunk.tilesPerLayer[1][idx] : 0;

				int worldX = baseX + x;
				int worldY = baseY + y;

				int maskX = worldX - m_worldOffsetX;
				int maskY = worldY - m_worldOffsetY;

				if (maskX < 0 || maskY < 0 || maskX >= worldWidth || maskY >= worldHeight)
					continue;

				// true = blocked, false = walkable
				worldMask[maskY * worldWidth + maskX] = (tileVal != 0);
			}
		}
	}

	// Defer GPU / SFML dependent work: schedule a rebuild on the main thread instead of building vertex arrays here.
	// This avoids touching SFML/OpenGL from background loader threads.
	// Note: FinalizeLoadedChunk runs on the main thread via UpdateMainThread normally; we still schedule to be safe.
	ScheduleChunkForRebuild(chunk);
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

	// lock the mutex to ensure thread safety while accessing and modifying the chunk data structures
	std::lock_guard<std::mutex> lock(m_mutex);

	// Evict least recently used chunks until we are within the limit 
	while (m_chunks.size() > m_maxLoadedChunks && !m_lruList.empty()) {
		
		// Get the least recently used chunk key from the back of the LRU list
		long long key = m_lruList.back();
		auto itr = m_chunks.find(key);
		if (itr != m_chunks.end()) {
			Chunk& chunk = itr->second;
			
			// Save if dirty
			for (int layer = 0; layer < chunk.numLayers; ++layer) {
				if (!chunk.dirty.empty() && !chunk.dirty[layer]) continue;
				std::filesystem::path p(m_basePath);
				std::string filename = (p / ("chunk_" + std::to_string(layer) + "_" + std::to_string(chunk.chunkX) + "_" + std::to_string(chunk.chunkY) + ".dat")).string();
				std::ofstream outFile(filename, std::ios::binary);
				if (outFile) {
					outFile.write(reinterpret_cast<const char*>(chunk.tilesPerLayer[layer].data()), chunk.tilesPerLayer[layer].size() * sizeof(int));
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

			// Update world mask to clear the tiles of the evicted chunk if it was an obstacle layer (layer 1)
			if (worldWidth > 0 && worldHeight > 0 && !worldMask.empty()) {

				int baseX = chunk.chunkX * chunk.width;
				int baseY = chunk.chunkY * chunk.height;

				for (int y = 0; y < chunk.height; ++y) {
					for (int x = 0; x < chunk.width; ++x) {

						int worldX = baseX + x;
						int worldY = baseY + y;

						int maskX = worldX - m_worldOffsetX;
						int maskY = worldY - m_worldOffsetY;

						if (maskX < 0 || maskY < 0 || maskX >= worldWidth || maskY >= worldHeight)
							continue;

						worldMask[maskY * worldWidth + maskX] = 0; // clear tile
					}
				}
			}



			m_chunks.erase(itr);
		}
		m_lruIndex.erase(key);
		m_lruList.pop_back();
	}
}
/////////////////////////////////



/////////////////////////////////
// LoadLevelFromFile - Load a TileMap JSON file into memory and replace active chunks.
bool ChunkManager::LoadLevelFromFile(const std::string& path, std::string* outErr) {
	//// Load the TileMap from JSON file.
	//auto loaded = TileMap::LoadFromJSON(path, outErr);

	//// Guard: if loading failed, return false and append error message to outErr.
	//if (!loaded.has_value()) {
	//	outErr->append("\nFailed to load TileMap from file: " + path + "\n");
	//	return false;
	//}

	//// No issues, then move the loaded TileMap into a local variable for processing.
	//TileMap map = std::move(*loaded);

	//// Determine the number of layers to load. If the TileMap has no layers, we will still create a single default layer.
	//const int newLayerCount = std::max(1, (int)map.layers.size());

	//// Prepare a new chunk map to hold the loaded chunks. This will replace the current m_chunks after loading.
	//std::unordered_map<long long, Chunk> newChunks;

	//// Get the width and height of the TileMap in tiles.
	//worldWidth = map.width;
	//worldHeight = map.height;
	//// Initialize world-scale walkability mask
	//worldMask.resize(worldWidth * worldHeight, false);


	//// Update world dimensions and resize the world-scale walkability mask to match the new map size.




	//// OPTIMIZATION: Instead of triple-nested loop (ly, y, x) with hash lookups,
	//// we now pre-create all chunks, then fill them by chunk with better cache locality.

	//// FIRST PASS: Determine all chunks that need to exist
	//std::set<long long> chunkKeys; // create a set to hold unique (64-bit) chunk keys
	//for (int y = 0; y < worldHeight; ++y) {
	//	for (int x = 0; x < worldWidth; ++x) {
	//		int chunkX = FloorDiv(x, m_chunkWidth);
	//		int chunkY = FloorDiv(y, m_chunkHeight);
	//		long long key = GetChunkKey(chunkX, chunkY);
	//		chunkKeys.insert(key);
	//	}
	//}

	//// SECOND PASS: Create all chunks upfront (batch operation, no scatter)
	//for (long long key : chunkKeys) {
	//	int chunkX = (int)(key >> 32);
	//	int chunkY = (int)(key & 0xFFFFFFFF);
	//	newChunks.emplace(key, Chunk(chunkX, chunkY, m_chunkWidth, m_chunkHeight, m_tileSize, newLayerCount));
	//}

	//// THIRD PASS: Fill chunks by layer, then by chunk, for better cache locality
	//for (int LayerIndex = 0; LayerIndex < newLayerCount; ++LayerIndex) {
	//	const auto& layerTiles = (LayerIndex < (int)map.layers.size()) ? map.layers[LayerIndex].tiles : std::vector<int>();

	//	// Iterate by chunks first... so get their coordinates and base world positions...
	//	for (auto& [key, chunk] : newChunks) {
	//		int chunkX = chunk.chunkX;
	//		int chunkY = chunk.chunkY;
	//		int baseX = chunkX * m_chunkWidth;
	//		int baseY = chunkY * m_chunkHeight;

	//		// ...then iterate by local tile coordinates within the selected chunk, and compute the corresponding world coordinates 
	//		// to fetch the tile value from the layer data.


	//		for (int localY = 0; localY < chunk.height; ++localY) { // move across y axis 

	//			std::cout << "/n";

	//			for (int localX = 0; localX < chunk.width; ++localX) { // then move across x axis
	//			
	//				// **** Small Note of on how we cacluate world coordinates from chunk and local tile coordinates ****
	//				// World coordinates = are calculated by getting a base position which is chunkX * chunkWidth and chunkY * chunkHeight,
	//				// then adding the local tile coordinates (localX, localY) to get the absolute world position.

	//				int worldX = baseX + localX;
	//				int worldY = baseY + localY;

	//				int val = 0;
	//				if (!layerTiles.empty()) {
	//					size_t idx = (size_t)worldY * (size_t)worldWidth + (size_t)worldX;
	//					if (idx < layerTiles.size())
	//						val = layerTiles[idx];
	//				} else if (LayerIndex == 0 && worldX < worldWidth && worldY < worldHeight) {
	//					val = map.GetTile(worldX, worldY);
	//				}

	//				chunk.tilesPerLayer[LayerIndex][localY * chunk.width + localX] = val;
	//				
	//				// Build world-scale walkability mask from obstacle layer (e.g. LayerIndex == 1)
	//				if (LayerIndex == 1) // or whatever layer is your obstacle/collision layer
	//				{
	//					int wx = baseX + localX; // world tile X
	//					int wy = baseY + localY; // world tile Y

	//					// Ensure worldMask is sized: worldWidth = W, worldHeight = H
	//					worldMask[wy * worldWidth + wx] = (val != 0); // 0 = walkable, non-zero = blocked

	//					std::cout << val;
	//				}
	//			}
	//		}
	//		chunk.dirty[LayerIndex] = 0;
	//	}

	//	{
	//		std::lock_guard<std::mutex> lock(m_mutex);
	//		try {
	//			EntityManager& entityMan = GameEngine::GetInstance().GetEntityManager();
	//			for (auto& chunkPair : m_chunks) {
	//				Chunk& chunk = chunkPair.second;
	//				for (Entity* genEnity : chunk.generatedEntities)
	//					if (genEnity)
	//						entityMan.SafeKillEntity(genEnity);
	//				chunk.generatedEntities.clear();
	//			}
	//		} catch (...) {}

	//		m_numLayers = newLayerCount;
	//		m_chunks.swap(newChunks);
	//		m_lruList.clear();
	//		m_lruIndex.clear();
	//		m_rebuildQueue.clear();
	//		m_rebuildSet.clear();

	//		for (auto& chunkPair : m_chunks) {
	//			const long long key = chunkPair.first;
	//			m_lruList.push_front(key);
	//			m_lruIndex[key] = m_lruList.begin();
	//			m_rebuildQueue.push_back(key);
	//			m_rebuildSet.insert(key);
	//		}

	//		if (!map.tilesetKey.empty())
	//			m_tilesetKey = map.tilesetKey;
	//	}

		return true;
	//}
}
/////////////////////////////////



/////////////////////////////////
// RegisterChunkColliders - Registers the collider entities generated for all chunks. This should be called after loading chunks to ensure that the colliders are 
// present in the game world for physics and collision detection.
void ChunkManager::RegisterChunkColliders(EntityManager& em) {}
/////////////////////////////////



/////////////////////////////////
// UnregisterChunkColliders - Unregisters the collider entities generated for all chunks. 
// This should be called when the chunk data changes or when chunks are evicted to ensure that outdated colliders are removed from the game world.
void ChunkManager::UnregisterChunkColliders(EntityManager& em) {
	std::lock_guard<std::mutex> lock(m_mutex); // lock the mutex to ensure thread safety
	
	// Iterate through all chunks and safely kill any generated collider entities to remove them from the game world; i'll loop through the tuple
	// of generated entities for each chunk and call KillEntity on each one, then clear the generatedEntities vector; this should avoid dangling pointers.
	// of generated entities for each chunk and call KillEntity on each one, then clear the generatedEntities vector; this should avoid dangling pointers.
	for (auto &chunkPair : m_chunks) {
		Chunk &chunk = chunkPair.second;
		for (Entity* genEnt : chunk.generatedEntities) {
			if (genEnt) em.KillEntity(genEnt);
		}
		chunk.generatedEntities.clear();
	}
}
/////////////////////////////////