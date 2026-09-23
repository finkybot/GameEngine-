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
#include <unordered_set>
#include <string>
#include <atomic>
#include "BVHSystem.h"

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
//	|	Chunk struct represents a section of the tile map, containing tile data and metadata for rendering and collision. Each chunk corresponds to a specific area of the game world and can be loaded/unloaded independently to optimize performance and memory usage.
//	|_______________________________________________________________________
struct Chunk {
	// Public member variables for the Chunk struct, including chunk coordinates, size, tile data, rendering information, and generated entities for collision.
	int chunkX = 0,	chunkY = 0; // chunk coordinates (e.g., chunkX = 0, chunkY = 0 is the origin chunk)
	int width = 0, height = 0; // chunk size in tiles (e.g., 16x16 tiles per chunk)

	float tileSize = 32.0f; // size of each tile in pixels (for rendering and collision)
	
	// per-layer storage: tilesPerLayer[layer][y * width + x]
	int numLayers = 1;
	std::vector<std::vector<int>> tilesPerLayer;
	std::vector<char> dirty;					// per-layer dirty flags
	std::vector<char> readyForRendering;		// per-layer ready flags
	std::vector<uint32_t> editVersion;			// per-layer edit versions

	std::vector<Entity*> generatedEntities; // generated entities for collision/pathfinding, one per tile (if any)
	std::vector<Entity*> dynamicEntities;   // dynamic entities (e.g., enemies, items) that are spawned in this chunk

	BVHSystem dynamicBVH; // BVH for dynamic entities in this chunk, used for efficient collision detection and raycasting.

	std::vector<float> cpuVertexBuffer;			// CPU-side vertex buffer for the chunk's tiles, used for rendering.
	std::vector<sf::VertexArray> vertexArrays;	// one vertex array per layer
	std::shared_ptr<sf::Texture> vertexTexture; // texture used by the vertexArrays (if any)

	std::vector<sf::VertexArray> tempVertexArraysForRendering; // temporary storage per-layer for alpha-modulated vertex data during enqueue

	Chunk() = default; // default constructor

	// Parameterized constructor for the Chunk struct, initializes member variables based on provided parameters and sets up per-layer storage for tiles and rendering information.
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
//	|	ChunkManager class declaration. This class manages the lifecycle of chunks, including loading from disk, saving to disk, generating vertex buffers for rendering, and managing memory usage through an LRU eviction policy.
//	|_______________________________________________________________________
class ChunkManager {
public:
	// Constructor and destructor for the ChunkManager class
	ChunkManager(int chunkWidth = 32, int chunkHeight = 32, float tileSize = 32.0f, int numLayers = 3);
	~ChunkManager();

	// DrawInfo struct is used to store information about a chunk's vertex array and texture for rendering. It includes the vertex array, texture, rendering readiness, chunk coordinates, dimensions, and tile size.
	struct DrawInfo {
		sf::VertexArray vertexArray;
		std::shared_ptr<sf::Texture> vertexTexture;

		bool readyForRendering = false;
		int chunkX = 0, chunkY = 0, width = 0, height = 0;
		float tileSize = 32.f;
	};

	// BuildWorldMask - Builds a world mask for collision/pathfinding based on the currently loaded chunks. The world mask is a 2D grid of uint8_t values, where each value corresponds to a specific tile's collision properties.
	void BuildWorldMask(std::vector<uint8_t>& outMask, int& outW, int& outH);

	// Public member variables for the ChunkManager class for world mask for collision, pathfinding, and other gameplay mechanics. Each value corresponds to a specific tile's collision properties.
	std::vector<uint8_t> worldMask;
	int worldOffsetX = 0, worldOffsetY = 0; // World offset in pixels for coordinate system alignment
	int worldWidth = 0, worldHeight = 0;

	Chunk* GetChunkForPosition(const Vec2& pos)	const; // Returns a pointer to the chunk that contains the specified world position. Returns nullptr if no chunk is loaded for that position.

	int GetTileAt(int tileX, int tileY, int layerIndex = 0);				// Returns the tile value at the specified tile coordinates (tileX, tileY) and layer index. Returns -1 if the tile is not found or if the layer index is invalid.
	int SetTileAt(int tileX, int tileY, int tileValue, int layerIndex = 0); // Sets the tile value at the specified tile coordinates (tileX, tileY) and layer index. Returns 0 on success, or -1 if the tile is not found or if the layer index is invalid.

	std::vector<uint8_t> GetWorldMask() const { return worldMask; }

	// Least Recently Used (LRU) eviction policy for managing loaded chunks in memory. The ChunkManager maintains a list of loaded chunks and their usage order, allowing it to unload the least recently used chunks when memory limits are exceeded.
	void TouchChunkLRU(uint64_t key);		// Mark a chunk as recently used in the LRU list. This is called whenever a chunk is accessed to update its usage order.
	void RemoveChunkFromLRU(uint64_t key);	// Remove a chunk from the LRU list. This is called when a chunk is unloaded or evicted from memory.

	// EnsureChunksInTileRect - Ensure all chunks that intersect the specified tile rectangle (tileX0, tileY0, tileX1, tileY1) are loaded and ready for rendering. The marginChunks parameter allows for loading additional chunks around the specified rectangle 
	// to prevent visual gaps during rendering.
	void EnsureChunksInTileRect(int tileX0, int tileY0, int tileX1, int tileY1, int marginChunks);
	void EnsureChunksInTileRect_NoLock(int tileX0, int tileY0, int tileX1, int tileY1, int marginChunks); // version that assumes caller holds m_mutex

	void EvictIfNeeded(); // Check if the number of loaded chunks exceeds the maximum allowed, and evict least recently used chunks if necessary to free up memory.

	void UpdateMainThread();		// Called from the main thread to process any chunks that have finished loading in the background.
	void UpdateMainThread_NoLock(); // version that assumes caller holds m_mutex

	void SaveAllChunks();									// Save all dirty chunks to disk in the specified directory. Each chunk will be saved as a separate file named "chunk_X_Y.dat" where X and Y are the chunk coordinates.
	void SetBasePath(const std::string& basePath);			// Set the base directory path for chunk files. This path is used for loading and saving chunk data to disk.
	std::string GetBasePath() const { return m_basePath; }	// Get the current base directory path for chunk files.
	
	// SetWorldOffset - Set the world offset in pixels for coordinate system alignment. This offset is used to translate world coordinates to chunk coordinates and vice versa.
	void SetWorldOffset(int offsetX, int offsetY) {
		worldOffsetX = offsetX;
		worldOffsetY = offsetY;
	}
	
	void SetWorldSize(int width, int height);	// Set the size of the world in pixels. This is used to

	void BuildWorldMask();						// Build the world mask for collision/pathfinding
	void RefreshWorldBoundsFromLoadedChunks();	// Recompute world offset/size from currently loaded chunks
	/////////////////////////////////

	
	void LoadAllSavedChunks();		// Load all saved chunk files from disk into memory (called on startup)
	void ClearAllLoadedChunks();	// Remove all loaded chunks from memory without saving; used when switching levels or resetting the world.


	void DebugPrintLayer1();		// DEBUG: Print layer 1 obstacle grid to console for debugging purposes. This function outputs the tile values of layer 1 for all loaded chunks to the console, allowing developers to inspect the current state of the level's obstacles.


	
	bool LoadLevelFromFile(const std::string& path, std::string* outErr = nullptr);	// LoadLevelFromFile - Load a TileMap JSON file into the chunked world. This will replace current in-memory chunks with the data from the file. Returns true on success and writes an optional error message to outErr.

	
	bool GetSavedChunkBounds(float& outMinX, float& outMinY, float& outMaxX, float& outMaxY) const;	// Scan saved chunk filenames on disk and return the bounding box of all saved chunks in world pixels. Returns false if no saved chunks exist. Does not load tile data.
	void GetMinChunkCoords(int& outMinCx, int& outMinCy) const;										// Get the minimum loaded chunk coordinates (for coordinate system offset in pathfinding)	
	void ShiftChunksToPositiveCoords();																// Shift all loaded chunks so that minChunkX >= 0 and minChunkY >= 0. Saves shifted chunks to disk. This is useful for level editors to ensure pathfinding doesn't cross negative boundaries.


	void SetTilesetKey(const std::string& key) { m_tilesetKey = key; }	// Set the key for the tileset atlas used to texture chunk vertex arrays. This allows the ChunkManager to use a specific tileset for rendering.
	std::string GetTilesetKey() const { return m_tilesetKey; }			// Get the current key for the tileset atlas used to texture chunk vertex arrays.
	
	void SetMaxLoadedChunks(size_t maxChunks) {	m_maxLoadedChunks = maxChunks; }	// Set the maximum number of chunks that can be loaded in memory at once. If the limit is exceeded, least recently used chunks will be unloaded.

	
	void SetActiveLayer(int layer) { m_activeLayer = std::max(0, std::min(layer, m_numLayers-1)); } // Set the active layer for editing or rendering. This allows the caller to control which layer is currently active.
	int GetActiveLayer() const {return m_activeLayer; }												// Get the current active layer for editing or rendering. This allows the caller to inspect which layer is currently active.
	void SetNumLayers(int n) { m_numLayers = std::max(1, n); }										// Set the number of layers in the chunk manager. This allows the caller to control how many layers are used for tile data and rendering.
	
	int GetNumLayers() const { return m_numLayers; }												// Get the current number of layers in the chunk manager.
	
	void SetUnselectedLayerAlpha(float a) { m_unselectedLayerAlpha = std::clamp(a, 0.0f, 1.0f); }	// Set the alpha value for unselected layers, controlling their transparency in the editor or renderer.
	float GetUnselectedLayerAlpha() const { return m_unselectedLayerAlpha; }						// Get the current alpha value for unselected layers, allowing the caller to inspect the transparency setting.

	// Accessors for chunk dimensions
	int GetChunkWidth() const {	return m_chunkWidth; }		// Get the width of each chunk in tiles. This allows the caller to inspect the configured chunk width.
	int GetChunkHeight() const { return m_chunkHeight; }	// Get the height of each chunk in tiles. This allows the caller to inspect the configured chunk height.
	float GetTileSize() const { return m_tileSize; }		// Get the size of each tile in world units. This allows the caller to inspect the configured tile size.

	// Accessors for read-only inspection by renderers / scenes. These return references guarded by the caller using the mutex returned by GetMutex().
	std::unordered_map<uint64_t, Chunk>& GetChunks() { return m_chunks; }									// Get a reference to the map of loaded chunks, allowing the caller to inspect or modify chunk data. The caller must hold the mutex returned by GetMutex() to ensure thread safety.
	std::mutex& GetMutex() { return m_mutex; }																// Get a reference to the mutex used to protect access to the chunks map and LRU list. Caller must lock mutex before accessing chunks or LRU structures to ensure thread safety.
	uint64_t GetWorldRevision() const noexcept { return m_worldRevision.load(std::memory_order_relaxed);}	// Get the current world revision number, which is a monotonic counter that increments whenever chunks are loaded, unloaded, or modified. This allows the caller to detect changes in the world state.

	// FloorDiv - Helper function for floor division of integers, ensuring that the result is rounded down to the nearest integer. This is useful for calculating chunk coordinates from tile coordinates.	
	static inline int FloorDiv(int a, int b) {
		// Ensure divisor is positive
		assert(b > 0); 

		// Perform floor division, rounding down for negative values of a
		if (a >= 0)
			return a / b;
		return -(((-a) + (b - 1)) / b);
	}

	// GetChunkKey - Generate a unique 64-bit key for a chunk based on its chunkX and chunkY coordinates. This key is used to index the chunks map and LRU list, allowing for efficient retrieval and management of chunks in memory.
	static uint64_t GetChunkKey(int chunkX, int chunkY) {
		return (static_cast<uint64_t>(chunkX) << 32) | static_cast<unsigned int>(chunkY);
	}



private:
	// Internal helper methods for loading, saving, and managing chunks
	void EnqueueLoadChunk(int chunkX, int chunkY);				// Enqueue a chunk for loading in the background. This method adds the specified chunk coordinates to a queue for asynchronous loading, allowing the main thread to continue processing without blocking.
	void EnqueueLoadChunk(int chunkX, int chunkY, int layer);	// Enqueue a specific layer of a chunk for loading in the background.
	void EnqueueLoadChunk_NoLock(int chunkX, int chunkY);		// version that assumes callder holds m_mutex
	
	void FinalizeLoadedChunk(int chunkX, int chunkY, int layer, std::vector<int> tileData, uint32_t versionAtEnqueue);	// Finalize a chunk that has finished loading in the background. This method updates the chunk's tile data, marks it as ready for rendering, and increments the world revision counter.
	void RebuildChunkEntities(Chunk& chunk);																			// Rebuilds merged collider entities for a chunk; call after tile edits.
	void ScheduleChunkForRebuild(Chunk& chunk);																			// Schedule chunk for main-thread-only GPU/collider rebuild

	static void BuildChunkVertexArray(Chunk& chunk, const std::shared_ptr<TextureAtlas>& atlas);	// Build the vertex array for a chunk based on its tile data and the provided texture atlas. This method generates the vertex positions and texture coordinates for rendering the chunk's tiles.
	static constexpr int kMaxChunkSpan = 256;														// Safety: maximum number of chunks to attempt to load in a single EnsureChunksInTileRect call (span in each axis)

	int m_chunkWidth;	// Width of each chunk in tiles
	int m_chunkHeight;	// Height of each chunk in tiles
	float m_tileSize;	// Size of each tile in pixels

	// Configuration for file paths and maximum loaded chunks
	std::string m_basePath = "levels/"; // Base directory path for chunk files
	size_t m_maxLoadedChunks = 256;		// Maximum number of chunks that can be loaded in memory at once

	// Data structures for managing loaded chunks and LRU eviction
	std::mutex m_mutex;																				// Mutex to protect access to the chunks map and LRU list across threads
	std::unordered_map<uint64_t, Chunk> _GUARDED_BY(m_mutex) m_chunks;								// Map of loaded chunks, keyed by a combined chunkX and chunkY value (e.g., (chunkX << 32) | chunkY)
	std::list<uint64_t> _GUARDED_BY(m_mutex) m_lruList;												// LRU (Least Recently Used) chunks for eviction (stores 64bit chunk keys)
	std::unordered_map<uint64_t, std::list<uint64_t>::iterator> _GUARDED_BY(m_mutex) m_lruIndex;	// O(1) iterator lookup into m_lruList
	std::atomic<uint64_t> m_worldRevision{1};														// Monotonic change counter for loaded chunk/world mutation detection

	// Rendering and layer management
	std::string m_tilesetKey;				// optional tileset atlas key used to texture chunk vertex arrays
	int m_numLayers = 1;					// number of layers per chunk (background, main, upper)
	int m_activeLayer = 0;					// drawing: which layer is fully opaque
	float m_unselectedLayerAlpha = 0.3f;	// opacity for unselected layers (0..1)

	std::vector<uint64_t> m_rebuildQueue;		// chunk keys scheduled for rebuild
	std::unordered_set<uint64_t> m_rebuildSet;	// quick membership check to avoid duplicate enqueues



public:
	// DrawChunks - Draw all chunks that intersect the provided view. This will perform a short copy of visible chunk data under  the mutex and then draw them without holding the lock to minimize contention; *** DEPRECATED: Use EnqueueChunks instead ***
	void DrawChunks(sf::RenderWindow& window, const sf::View& view); 

	// PackChunkKey - Pack chunkX and chunkY into a single 64-bit key for use in the chunks map. This is used to create a unique identifier for each chunk based on its coordinates.
	inline uint64_t PackChunkKey(int chunkX, int chunkY) const {
		return (static_cast<uint64_t>(chunkX) << 32) | static_cast<unsigned int>(chunkY);
	}

	// UnpackChunkKey - Unpack a 64-bit chunk key into its original chunkX and chunkY coordinates. This is used to retrieve the individual coordinates from the combined key used in the chunks map.
	inline void UnpackChunkKey(uint64_t key, int& outChunkX, int& outChunkY) const {
		outChunkX = static_cast<int>(key >> 32);
		outChunkY = static_cast<int>(key & 0xFFFFFFFF);
	}

	// UpdateStreaming - Update streaming of chunks based on the current view (load/unload as needed). This method will determine which chunks are visible in the current view and ensure they are loaded, while also evicting any chunks that are no longer needed.
	void UpdateStreaming(const sf::View& view); // Update streaming of chunks based on the current view (load/unload as needed)

	// EnqueueChunks - Enqueue all visible chunks to the render queue for rendering. This method will determine which chunks are visible
	void EnqueueChunks(RenderQueue& queue, const sf::View& view);

	// EnsureChunkLoaded - Ensure a specific chunk is loaded and ready for rendering. If the chunk is not already loaded, it will be enqueued 
	// for loading.
	void EnsureChunkLoaded(int cx, int cy);

	// EvictChunksOutsideRadius - Evict chunks that are outside the specified radius from the given chunk coordinates.
	void EvictChunksOutsideRadius(int minCx, int maxCx, int minCy, int maxCy); 

	// Rebuild vertex arrays for all loaded chunks using the currently configured tileset key (if any).
	void RebuildAllChunksFromTileset();

	// Create merged rectangle entities for collisions from chunk tiles and register them with the EntityManager
	void RegisterChunkColliders(EntityManager& em);
	void UnregisterChunkColliders(EntityManager& em);
	/////////////////////////////////
};
/////////////////////////////////