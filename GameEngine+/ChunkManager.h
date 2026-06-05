/////////////////////////////////
// The ChunkManager class is responsible for managing the loading, saving, and rendering of chunks in a tile-based game world. It handles the creation and maintenance of chunk data, including tile information, vertex buffers for rendering, 
// and generated entities for collisions. The ChunkManager also implements an LRU (Least Recently Used) eviction policy to manage memory usage when the number of loaded chunks exceeds a specified limit
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the ChunkManager class. We include necessary headers for data structures, threading, synchronization, optional values, and SFML graphics. We also forward declare related classes to avoid include cycles.
#pragma once
#include <unordered_map>
#include <list>
#include <mutex>
#include <optional>
#include <string>

#include "TileMap.h"
#include "RenderQueue.h"
#include <SFML/Graphics.hpp>

// forward declarations to avoid include cycles
class Entity;
class EntityManager;
class TextureAtlas;

// SAL annotation macro for thread-safety documentation (MSVC static analysis).
// This marks members/sections as guarded by a specific mutex.
#ifdef _MSC_VER
	#define _GUARDED_BY(lock) // no-op; only used for documentation
#else
	#define _GUARDED_BY(lock) // no-op on non-MSVC
#endif
/////////////////////////////////



/////////////////////////////////
// Chunk struct represents a section of the tile map, containing tile data and metadata for rendering and collision. 
// Each chunk corresponds to a specific area of the game world and can be loaded/unloaded independently to optimize performance and memory usage.
struct Chunk {
	int chunkX = 0,	chunkY = 0; // chunk coordinates (e.g., chunkX = 0, chunkY = 0 is the origin chunk)
	int width = 0, height = 0; // chunk size in tiles (e.g., 16x16 tiles per chunk)

	float tileSize = 32.0f; // size of each tile in pixels (for rendering and collision)
	// per-layer storage: tilesPerLayer[layer][y * width + x]
	int numLayers = 1;
	std::vector<std::vector<int>> tilesPerLayer;
	std::vector<char> dirty; // per-layer dirty flags
	std::vector<char> readyForRendering; // per-layer ready flags
	std::vector<uint32_t> editVersion; // per-layer edit versions

	// Entities generated for this chunk's merged collision rectangles (Tile static entities)
	std::vector<Entity*> generatedEntities;

	std::vector<float> cpuVertexBuffer; // CPU-side vertex buffer for the chunk's tiles, used for rendering.
	std::vector<sf::VertexArray> vertexArrays; // one vertex array per layer
	std::shared_ptr<sf::Texture> vertexTexture; // texture used by the vertexArrays (if any)

	std::vector<sf::VertexArray> tempVertexArraysForRendering; // temporary storage per-layer for alpha-modulated vertex data during enqueue

	Chunk() = default;

	Chunk(int x, int y, int width, int height, float tileSize, int numLayers)
		: chunkX(x), chunkY(y), width(width), height(height), tileSize(tileSize) {
		numLayers = std::max(1, numLayers);
		this->numLayers = numLayers;
		tilesPerLayer.resize(numLayers);
		dirty.resize(numLayers, false);
		readyForRendering.resize(numLayers, false);
		editVersion.resize(numLayers, 0);
		for (int i = 0; i < numLayers; ++i) {
			tilesPerLayer[i].resize(width * height, 0);
		}
		vertexArrays.resize(numLayers);
		for (auto &va : vertexArrays) va.setPrimitiveType(sf::PrimitiveType::Triangles);
		tempVertexArraysForRendering.resize(numLayers);
		for (auto &va : tempVertexArraysForRendering) va.setPrimitiveType(sf::PrimitiveType::Triangles);
	}

	// Backwards compat helpers for single-layer access
	int GetTileSingleLayer(int x, int y) const { return (tilesPerLayer.empty() ? 0 : tilesPerLayer[0][y * width + x]); }
	void SetTileSingleLayer(int x, int y, int v) { if (!tilesPerLayer.empty()) tilesPerLayer[0][y * width + x] = v; }
};
/////////////////////////////////



/////////////////////////////////
// ChunkManager class declaration. This class manages the lifecycle of chunks, including loading from disk, saving to disk, generating vertex buffers for rendering, and managing memory usage through an LRU eviction policy.
class ChunkManager {
	/////////////////////////////////
	// Public interface for the ChunkManager class, including methods for getting and setting tile values, ensuring chunks are loaded for a given area, updating the main thread with loaded chunks, saving chunks to disk, and managing configuration such as base path and tileset key.
public:
	/////////////////////////////////
	// Constructor and destructor for the ChunkManager class
	ChunkManager(int chunkWidth = 32, int chunkHeight = 32, float tileSize = 32.0f, int numLayers = 3);
	~ChunkManager();
	/////////////////////////////////


	
	/////////////////////////////////
	// Public methods for getting and setting tile values, ensuring chunks are loaded for a given area, updating the main thread with loaded chunks, saving chunks to disk, and managing configuration such as base path and tileset key.
	// layerIndex defaults to 0 (main layer) for backward compatibility
	int GetTileAt(int tileX, int tileY, int layerIndex = 0);
	int SetTileAt(int tileX, int tileY, int tileValue, int layerIndex = 0);
	/////////////////////////////////

	

	/////////////////////////////////
	// Methods for ensuring chunks are loaded, updating the main thread, and saving chunks to disk.
	void EnsureChunksInTileRect(int tileX0, int tileY0, int tileX1, int tileY1, int marginChunks); // Ensure all chunks that intersect the specified tile rectangle (tileX0, tileY0, tileX1, tileY1) are loaded and ready for rendering.
	void UpdateMainThread(); // Call this from the main thread to perform any necessary updates, such as processing dirty chunks or preparing vertex buffers for rendering.
	void SaveAllChunks(); // Save all dirty chunks to disk in the specified directory. Each chunk will be saved as a separate file named "chunk_X_Y.dat" where X and Y are the chunk coordinates.

	void SetBasePath(const std::string& basePath); // Set the base directory path where chunk files will be saved and loaded from.
	std::string GetBasePath() const { return m_basePath; }
	/////////////////////////////////
	


	/////////////////////////////////
	// Load all saved chunk files from disk into memory (called on startup)
	void LoadAllSavedChunks();
	void ClearAllLoadedChunks();

	// Scan saved chunk filenames on disk and return the bounding box of all saved chunks in world pixels.
	// Returns false if no saved chunks exist. Does not load tile data.
	bool GetSavedChunkBounds(float& outMinX, float& outMinY, float& outMaxX, float& outMaxY) const;

	void SetTilesetKey(const std::string& key) { m_tilesetKey = key; }
	std::string GetTilesetKey() const { return m_tilesetKey; }
	void SetMaxLoadedChunks(size_t maxChunks) {	m_maxLoadedChunks = maxChunks; } // Set the maximum number of chunks that can be loaded in memory at once. If the limit is exceeded, least recently used chunks will be unloaded.
	/////////////////////////////////



	/////////////////////////////////
	// Expose active layer control to callers (which may be editor UI)
	void SetActiveLayer(int layer) { m_activeLayer = std::max(0, std::min(layer, m_numLayers-1)); }
	int GetActiveLayer() const { return m_activeLayer; }

	// Allow changing number of layers at runtime based on level meta
	void SetNumLayers(int n) { m_numLayers = std::max(1, n); }
	int GetNumLayers() const { return m_numLayers; }

	void SetUnselectedLayerAlpha(float a) { m_unselectedLayerAlpha = std::clamp(a, 0.0f, 1.0f); }
	float GetUnselectedLayerAlpha() const { return m_unselectedLayerAlpha; }
	/////////////////////////////////



	/////////////////////////////////
	// Accessors for read-only inspection by renderers / scenes. These return references guarded by the caller using the mutex returned by GetMutex().
	std::unordered_map<long long, Chunk>& GetChunks() { return m_chunks; }
	std::mutex& GetMutex() { return m_mutex; }
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods for loading, saving, and managing chunks, as well as building vertex arrays for rendering.
private:
	/////////////////////////////////
	// Internal helper methods for loading, saving, and managing chunks
	void EnqueueLoadChunk(int chunkX, int chunkY);
	void EnqueueLoadChunk(int chunkX, int chunkY, int layer);
	void FinalizeLoadedChunk(int chunkX, int chunkY, int layer, std::vector<int> tileData, uint32_t versionAtEnqueue);
	void RebuildChunkEntities(Chunk& chunk); // Rebuilds merged collider entities for a chunk; call after tile edits.
	void EvictIfNeeded();
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// Builds the vertex array for a chunk using the given atlas (may be nullptr for colour fallback)
	static void BuildChunkVertexArray(Chunk& chunk, const std::shared_ptr<TextureAtlas>& atlas);
	/////////////////////////////////
	 

	
	/////////////////////////////////
	// Helper function to combine chunkX and chunkY into a single key for the chunks map
	static long long GetChunkKey(int chunkX, int chunkY) {
		return (static_cast<long long>(chunkX) << 32) | static_cast<unsigned int>(chunkY);
	}
	/////////////////////////////////



	/////////////////////////////////
	// Helper function for floor division of integers, since C++ integer division truncates towards zero. This ensures that negative coordinates are handled correctly when determining chunk coordinates from tile coordinates.
	static inline int FloorDiv(int a, int b) { 
		return (int)std::floor(static_cast<double>(a) / static_cast<double>(b));
	}
	/////////////////////////////////


	/////////////////////////////////
	// Safety: maximum number of chunks to attempt to load in a single EnsureChunksInTileRect call (span in each axis)
	static constexpr int kMaxChunkSpan = 256;
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the ChunkManager class, including configuration for chunk size and tile size, base path for chunk files, maximum loaded chunks, data structures for managing loaded chunks and LRU eviction, and synchronization primitives for thread safety.
	int m_chunkWidth; // Width of each chunk in tiles
	int m_chunkHeight; // Height of each chunk in tiles
	float m_tileSize;  // Size of each tile in pixels

	std::string m_basePath = "levels/"; // Base directory path for chunk files
	size_t m_maxLoadedChunks = 256; // Maximum number of chunks that can be loaded in memory at once

	std::mutex m_mutex;	// Mutex to protect access to the chunks map and LRU list across threads
	std::unordered_map<long long, Chunk> _GUARDED_BY(m_mutex) m_chunks; // Map of loaded chunks, keyed by a combined chunkX and chunkY value (e.g., (chunkX << 32) | chunkY)
	std::list<long long> _GUARDED_BY(m_mutex) m_lruList; // List to track the least recently used chunks for eviction (stores chunk keys)
	std::unordered_map<long long, std::list<long long>::iterator> _GUARDED_BY(m_mutex) m_lruIndex; // O(1) iterator lookup into m_lruList

	std::string m_tilesetKey; // optional tileset atlas key used to texture chunk vertex arrays
	int m_numLayers = 1; // number of layers per chunk (background, main, upper)
	int m_activeLayer = 0; // drawing: which layer is fully opaque
	float m_unselectedLayerAlpha = 0.3f; // opacity for unselected layers (0..1)
	/////////////////////////////////



	/////////////////////////////////
	// Public methods for drawing chunks and registering/unregistering chunk colliders with the EntityManager.
public:
	/////////////////////////////////
	// Draw all chunks that intersect the provided view. This will perform a short copy of visible chunk data under the mutex and then draw them without holding the lock to minimize contention.
	void DrawChunks(sf::RenderWindow& window, const sf::View& view); // DEPRECATED: Use EnqueueChunks instead
	void EnqueueChunks(RenderQueue& queue, const sf::View& view); // Enqueue all visible chunks to the render queue



	/////////////////////////////////
	// Rebuild vertex arrays for all loaded chunks using the currently configured tileset key (if any).
	void RebuildAllChunksFromTileset();
	/////////////////////////////////



	/////////////////////////////////
	// Create merged rectangle entities for collisions from chunk tiles and register them with the EntityManager
	void RegisterChunkColliders(EntityManager& em);
	void UnregisterChunkColliders(EntityManager& em);
	/////////////////////////////////
};
/////////////////////////////////