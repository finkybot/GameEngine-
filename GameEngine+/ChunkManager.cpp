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
///////////////////////////////



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
	// Ensure vertexArrays vector matches numLayers
	if (chunk.vertexArrays.size() != (size_t)chunk.numLayers) chunk.vertexArrays.resize(chunk.numLayers);
	if (atlas) {
		chunk.vertexTexture = atlas->GetTexture();
	} else {
		chunk.vertexTexture.reset();
	}

	const float ts = chunk.tileSize;
	const int baseX = chunk.chunkX * chunk.width;
	const int baseY = chunk.chunkY * chunk.height;

	for (int L = 0; L < chunk.numLayers; ++L) {
		sf::VertexArray &va = chunk.vertexArrays[L];
		va.clear();
		va.setPrimitiveType(sf::PrimitiveType::Triangles);
		// iterate tiles for this layer
		for (int y = 0; y < chunk.height; ++y) {
			for (int x = 0; x < chunk.width; ++x) {
				int v = 0;
				if (L < (int)chunk.tilesPerLayer.size()) v = chunk.tilesPerLayer[L][y * chunk.width + x];
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
					va.append({ {px,      py      }, sf::Color::White, uv00 });
					va.append({ {px + ts, py      }, sf::Color::White, {uv11.x, uv00.y} });
					va.append({ {px + ts, py + ts }, sf::Color::White, uv11 });
					va.append({ {px,      py      }, sf::Color::White, uv00 });
					va.append({ {px + ts, py + ts }, sf::Color::White, uv11 });
					va.append({ {px,      py + ts }, sf::Color::White, {uv00.x, uv11.y} });
				} else {
					// Fallback color for tiles without texture. In-editor, show an opaque placeholder so painted tiles are visible even
					// when a tileset isn't loaded. If a tileset key is configured we keep transparent fallback so cleared tiles show through.
					uint8_t alpha = atlas ? 0u : 255u;
					sf::Color fallback(120, 120, 120, alpha);
					va.append({ {px,      py      }, fallback });
					va.append({ {px + ts, py      }, fallback });
					va.append({ {px + ts, py + ts }, fallback });
					va.append({ {px,      py      }, fallback });
					va.append({ {px + ts, py + ts }, fallback });
					va.append({ {px,      py + ts }, fallback });
				}
			}
		}
	}
	// Debug: log vertex counts when rebuilding (can be noisy)
	// std::cout << "BuildChunkVertexArray chunk(" << chunk.chunkX << "," << chunk.chunkY << ") layers=" << chunk.numLayers << " atlas=" << (atlas?"yes":"no") << "\n";
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
			// Merge per-layer vertex arrays into a single array for drawing (layers are drawn in order)
			di.vertexArray.clear();
			di.vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);
			for (int L = 0; L < c.numLayers; ++L) {
				if (c.vertexArrays.size() > (size_t)L && c.vertexArrays[L].getVertexCount() > 0) {
					// append vertices (simple approach: copy)
					// If this layer is not the active drawing layer, we dim by multiplying alpha of each vertex color
					bool dim = (L != m_numLayers ? false : false); // placeholder, actual dim handled below when drawing
					for (size_t vi = 0; vi < c.vertexArrays[L].getVertexCount(); ++vi) di.vertexArray.append(c.vertexArrays[L][vi]);
				}
			}
			di.vertexTexture = c.vertexTexture;
			bool allZero = true;
			for (int L = 0; L < c.numLayers; ++L) { for (int t : c.tilesPerLayer[L]) { if (t != 0) { allZero = false; break; } } if (!allZero) break; }
			// ready if any layer marked ready or all layers empty
			bool anyReady = false; for (int L = 0; L < c.numLayers; ++L) if (c.readyForRendering[L]) { anyReady = true; break; }
			di.readyForRendering = anyReady || allZero;
			di.chunkX = c.chunkX; di.chunkY = c.chunkY;
			di.width  = c.width;  di.height = c.height;
			di.tileSize = c.tileSize;
			// Debug: report vertex counts for this chunk
			if (di.vertexArray.getVertexCount() > 0) {
				// debug output removed: verbose per-chunk draw logging
			}
			visible.push_back(std::move(di));
		}
	}

	for (const DrawInfo& d : visible) {
		if (!d.readyForRendering) {
			// placeholder for non-ready chunks: make fully transparent so empty areas are visible
			// (previously drew a semi-opaque grey rectangle here)
			// sf::RectangleShape r(sf::Vector2f((float)d.width * d.tileSize, (float)d.height * d.tileSize));
			// r.setPosition({wx, wy});
			// r.setFillColor(sf::Color(60, 60, 60, 80));
			// window.draw(r);
			continue;
		}
		if (d.vertexArray.getVertexCount() > 0) {
			sf::RenderStates states;
			if (d.vertexTexture) states.texture = d.vertexTexture.get();
			// Ensure alpha blending is used so texture transparency is visible
			states.blendMode = sf::BlendAlpha;
			// If the engine has an active layer set, we need to draw other layers dimmed. We rendered merged vertexArray in order so
			// we cannot distinguish layers at draw time. Simpler approach: draw per-layer instead of merged when alpha-dimming is active.
			if (true) {
				// per-layer draw to allow dimming
				for (const auto &pr : m_chunks) { /* noop to keep symbol referenced */ }
				// Re-lock and draw per-chunk per-layer to allow alpha modulation
				std::lock_guard<std::mutex> lock(m_mutex);
				for (const auto& pr : m_chunks) {
					const Chunk& c = pr.second;
					if (c.chunkX != d.chunkX || c.chunkY != d.chunkY) continue;
					for (int L = 0; L < c.numLayers; ++L) {
						if (c.vertexArrays.size() <= (size_t)L) continue;
						auto &va = c.vertexArrays[L];
						if (va.getVertexCount() == 0) continue;
						// create a copy to modulate alpha
						sf::VertexArray temp = va;
						float alphaMul = 1.0f;
						// Dim unselected layers using configured alpha
						if (m_activeLayer >= 0 && L != m_activeLayer) alphaMul = m_unselectedLayerAlpha; 
						for (size_t vi = 0; vi < temp.getVertexCount(); ++vi) {
							sf::Color ccol = temp[vi].color;
							ccol.a = static_cast<uint8_t>((float)ccol.a * alphaMul);
							temp[vi].color = ccol;
						}
						sf::RenderStates s2 = states;
						if (c.vertexTexture) s2.texture = c.vertexTexture.get();
						window.draw(temp, s2);
					}
				}
			} else {
				window.draw(d.vertexArray, states);
			}
		}
		// Optional diagnostics: draw a faint overlay for chunks that have no texture or are all-zero
		// (controlled from LevelEditorScene debug UI)
	}
}
/////////////////////////////////



/////////////////////////////////
// EnqueueChunks - Enqueue all visible chunks to the render queue with depth-based sorting
// This is the ECS-aligned rendering method that replaces DrawChunks for use with the render queue
void ChunkManager::EnqueueChunks(RenderQueue& queue, const sf::View& view) {
	const sf::Vector2f viewCenter = view.getCenter();
	const sf::Vector2f viewSize = view.getSize();
	const float vleft = viewCenter.x - viewSize.x * 0.5f;
	const float vtop = viewCenter.y - viewSize.y * 0.5f;
	const float vright = vleft + viewSize.x;
	const float vbottom = vtop + viewSize.y;

	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& pr : m_chunks) {
		Chunk& c = pr.second;
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
// Destructor - currently does not have any special cleanup logic, but we could add it if needed in the future (e.g., to save dirty chunks before exiting)
ChunkManager::~ChunkManager() { 
	SaveAllChunks(); // Ensure all dirty chunks are saved to disk when the ChunkManager is destroyed to prevent data loss.
	std::lock_guard<std::mutex> lock(s_pendingMutex); // Lock the mutex to safely clear the pending chunks queue
	s_pendingChunks.clear();						  // Clear the pending chunks queue to free memory	
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

	std::lock_guard<std::mutex> lock(m_mutex);

	// Create placeholder chunk on first paint
	auto [itr, inserted] = m_chunks.emplace(key, Chunk(chunkX, chunkY, m_chunkWidth, m_chunkHeight, m_tileSize, m_numLayers));
	if (inserted) {
		m_lruList.push_front(key);
		m_lruIndex[key] = m_lruList.begin();
		EnqueueLoadChunk(chunkX, chunkY);
	}

	Chunk& chunk = itr->second;
	if (localX < 0 || localX >= chunk.width || localY < 0 || localY >= chunk.height) return 0;
	if (layerIndex < 0 || layerIndex >= chunk.numLayers) return 0;

	const int index     = localY * chunk.width + localX;
	const int prevValue = chunk.tilesPerLayer[layerIndex][index];

	// If this chunk was just inserted as a placeholder (we created it because it wasn't loaded) then still apply the requested change even if the placeholder value matches 
	// the requested value. This handles erasing areas of maps saved on disk: the placeholder starts as zeros and an erase should overwrite the on-disk non-zero tiles once 
	// the background load finalizes. Always process clears (tileValue == 0) even if prevValue is already 0 — the chunk may be an unfinalized placeholder whose real on-disk 
	// tiles are non-zero. Bumping editVersion here ensures FinalizeLoadedChunk rejects the stale background load and keeps the cleared in-memory state.
	if (prevValue == tileValue && !inserted && tileValue != 0) return prevValue;

	chunk.tilesPerLayer[layerIndex][index] = tileValue;
	chunk.dirty[layerIndex]        = 1;
	chunk.editVersion[layerIndex]++;

	// Debug log for paints so we can confirm SetTileAt is being called and values are applied
	std::cout << "ChunkManager::SetTileAt chunk=(" << chunk.chunkX << "," << chunk.chunkY << ") local=(" << localX << "," << localY << ") layer=" << layerIndex << " val=" << tileValue << " prev=" << prevValue << "\n";

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
	chunk.readyForRendering[layerIndex] = 1;
	// Rebuild collider entities so removed tiles don't leave grey rectangles behind
	RebuildChunkEntities(chunk);

	// Immediately persist to disk — delete the file if the chunk is entirely empty so
	// GetSavedChunkBounds only counts chunks that actually contain tiles.
	if (!m_basePath.empty()) {
		try {
			// Save per-layer chunk files named: chunk_<layer>_<cx>_<cy>.dat
			bool anyNonZero = false;
			for (int layer = 0; layer < chunk.numLayers; ++layer) {
				std::string filename = (fs::path(m_basePath) / ("chunk_" + std::to_string(layer) + "_" + std::to_string(chunk.chunkX) + "_" + std::to_string(chunk.chunkY) + ".dat")).string();
				bool allZero = true;
				for (int t : chunk.tilesPerLayer[layer]) { if (t != 0) { allZero = false; break; } }
				if (allZero) {
					try { fs::remove(filename); } catch(...) {}
					chunk.dirty[layer] = false;
				} else {
					anyNonZero = true;
					std::ofstream outFile(filename, std::ios::binary);
					if (outFile) {
						outFile.write(reinterpret_cast<const char*>(chunk.tilesPerLayer[layer].data()), chunk.tilesPerLayer[layer].size() * sizeof(int));
						chunk.dirty[layer] = false;
					} else {
						std::cerr << "ChunkManager: failed to save chunk " << filename << "\n";
					}
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
				auto insertRes = m_chunks.emplace(key, Chunk(cx, cy, m_chunkWidth, m_chunkHeight, m_tileSize, m_numLayers));
				auto insertedItr = insertRes.first;
				bool insertedNow = insertRes.second;
				m_lruList.push_front(key);
				m_lruIndex[key] = m_lruList.begin();
					if (insertedNow) {
						Chunk& newChunk = insertedItr->second;
						// placeholder chunks are initialized with zero tiles; treat them as ready so they render empty immediately.
					bool allZero = true;
					for (int i = 0; i < newChunk.width * newChunk.height; ++i) { 
						for (int L = 0; L < newChunk.numLayers; ++L) { if (!newChunk.tilesPerLayer.empty() && newChunk.tilesPerLayer[L][i] != 0) { allZero = false; break; } }
						if (!allZero) break;
					}
					if (allZero) for (int L = 0; L < newChunk.numLayers; ++L) if (!newChunk.readyForRendering.empty()) newChunk.readyForRendering[L] = 1;
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
	// Process pending loaded chunks in small batches to avoid spending a long time on the main thread
	const int kMaxPendingPerFrame = 16;
	std::vector<std::tuple<int, int, int, std::vector<int>, uint32_t>> pendingChunksCopy;
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
// EnqueueLoadChunk - new overload accepts optional layer; if layer==-1 load all layers
void ChunkManager::EnqueueLoadChunk(int chunkX, int chunkY) {
	// existing behavior: load all layers (delegate to layer-aware overload)
	EnqueueLoadChunk(chunkX, chunkY, -1);
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
// RebuildChunkEntities - Rebuilds the collider entities for a chunk based on its current tile data. This should be called whenever the tile data changes to ensure that the 
// colliders match the visual representation of the chunk.
void ChunkManager::RebuildChunkEntities(Chunk& c) {
	// First, safely kill any existing entities that were generated for this chunk to avoid leaving orphaned entities in the world. We use SafeKillEntity to ensure that we 
	// don't attempt to access entities that may have already been destroyed.
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
				int val = 0;
				if (!c.tilesPerLayer.empty()) val = c.tilesPerLayer[0][idx];
				if (val == 0) continue;
				int w = 1;
				while (x + w < c.width && (!c.tilesPerLayer.empty() && c.tilesPerLayer[0][y * c.width + (x + w)] == val) && !used[y * c.width + (x + w)]) ++w;
				int h = 1;
				bool canExtend = true;
				while (y + h < c.height && canExtend) {
					for (int xi = 0; xi < w; ++xi) {
						if ((!c.tilesPerLayer.empty() && c.tilesPerLayer[0][(y + h) * c.width + (x + xi)] != val) || used[(y + h) * c.width + (x + xi)]) { canExtend = false; break; }
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
void ChunkManager::FinalizeLoadedChunk(int chunkX, int chunkY, int layer, std::vector<int> tileData, uint32_t versionAtEnqueue) {
	long long key = GetChunkKey(chunkX, chunkY);
	std::lock_guard<std::mutex> lock(m_mutex);
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
	// Build GPU vertex arrays for this chunk layer on the main thread
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

					m_chunks.erase(itr);
				}
				m_lruIndex.erase(key);
				m_lruList.pop_back();
				}
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