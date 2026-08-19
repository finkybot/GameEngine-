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
//								|
//								|_______________________________________________________________________
struct Chunk {
	/////////////////////////////////
	// Public member variables for the Chunk struct, including chunk coordinates, size, tile data, rendering information, and generated entities for collision.
	int chunkX = 0,	chunkY = 0; // chunk coordinates (e.g., chunkX = 0, chunkY = 0 is the origin chunk)
	int width = 0, height = 0; // chunk size in tiles (e.g., 16x16 tiles per chunk)
	/////////////////////////////////



	/////////////////////////////////
	float tileSize = 32.0f; // size of each tile in pixels (for rendering and collision)
	// per-layer storage: tilesPerLayer[layer][y * width + x]
	int numLayers = 1;
	std::vector<std::vector<int>> tilesPerLayer;
	std::vector<char> dirty; // per-layer dirty flags
	std::vector<char> readyForRendering; // per-layer ready flags
	std::vector<uint32_t> editVersion; // per-layer edit versions
	/////////////////////////////////



	/////////////////////////////////
	// Entities generated for this chunk's merged collision rectangles (Tile static entities)
	std::vector<Entity*> generatedEntities;
	/////////////////////////////////



	/////////////////////////////////
	std::vector<float> cpuVertexBuffer; // CPU-side vertex buffer for the chunk's tiles, used for rendering.
	std::vector<sf::VertexArray> vertexArrays; // one vertex array per layer
	std::shared_ptr<sf::Texture> vertexTexture; // texture used by the vertexArrays (if any)

	std::vector<sf::VertexArray> tempVertexArraysForRendering; // temporary storage per-layer for alpha-modulated vertex data during enqueue
	/////////////////////////////////


	/////////////////////////////////
	// Default constructor for the Chunk struct, initializes member variables to default values.
	Chunk() = default;
	/////////////////////////////////



	/////////////////////////////////
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
	/////////////////////////////////



	/////////////////////////////////
	// Backwards compat helpers for single-layer access
	int GetTileSingleLayer(int x, int y) const { return (tilesPerLayer.empty() ? 0 : tilesPerLayer[0][y * width + x]); }
	void SetTileSingleLayer(int x, int y, int v) { if (!tilesPerLayer.empty()) tilesPerLayer[0][y * width + x] = v; }
};
/////////////////////////////////



/////////////////////////////////
// ChunkManager class declaration. This class manages the lifecycle of chunks, including loading from disk, saving to disk, 
// generating vertex buffers for rendering, and managing memory usage through an LRU eviction policy.
//								|
//								|_______________________________________________________________________
class ChunkManager {
	/////////////////////////////////
	// Public interface for the ChunkManager class, including methods for getting and setting tile values, ensuring chunks are 
	// loaded for a given area, updating the main thread with loaded chunks, saving chunks to disk, and managing configuration 
	// such as base path and tileset key.
public:
	/////////////////////////////////
	// Constructor and destructor for the ChunkManager class
	ChunkManager(int chunkWidth = 32, int chunkHeight = 32, float tileSize = 32.0f, int numLayers = 3);
	~ChunkManager();
	/////////////////////////////////


			
	/////////////////////////////////
	// DrawInfo struct is used to store information about a chunk's vertex array and texture for rendering. It includes the vertex 
	// array, texture, rendering readiness, chunk coordinates, dimensions, and tile size.
	struct DrawInfo {
		sf::VertexArray vertexArray;
		std::shared_ptr<sf::Texture> vertexTexture;

		bool readyForRendering = false;
		int chunkX = 0, chunkY = 0, width = 0, height = 0;
		float tileSize = 32.f;
	};
	/////////////////////////////////

	
	/////////////////////////////////
	// Public member variables for the ChunkManager class for world mask for collision, pathfinding, and other gameplay mechanics. 
	// Each value corresponds to a specific tile's collision properties.
	std::vector<uint8_t> worldMask;
	int worldOffsetX = 0, worldOffsetY = 0; // world offset in pixels for coordinate system alignment
	int worldWidth = 0, worldHeight = 0;



	/////////////////////////////////
	// Public methods for getting and setting tile values, ensuring chunks are loaded for a given area, updating the main thread 
	// with loaded chunks, saving chunks to disk, and managing configuration such as base path and tileset key.
	// layerIndex defaults to 0 (main layer) for backward compatibility
	int GetTileAt(int tileX, int tileY, int layerIndex = 0);
	int SetTileAt(int tileX, int tileY, int tileValue, int layerIndex = 0);


	//int GetWorldWidth() const { return worldWidth; }
	//int GetWorldHeight() const { return worldHeight; }
	std::vector<uint8_t> GetWorldMask() const { return worldMask; }

	//int GetWorldOffsetX() const { return worldOffsetX; }
	//int GetWorldOffsetY() const { return worldOffsetY; }
	/////////////////////////////////
	
	

	/////////////////////////////////
	// LRU
	void TouchChunkLRU(long long key);
	void RemoveChunkFromLRU(long long key);
	/////////////////////////////////

	

	/////////////////////////////////
	// EnsureChunksInTileRect - Ensure all chunks that intersect the specified tile rectangle (tileX0, tileY0, tileX1, tileY1) are 
	// loaded and ready for rendering. The marginChunks parameter allows for loading additional chunks around the specified rectangle 
	// to prevent visual gaps during rendering.
	void EnsureChunksInTileRect(int tileX0, int tileY0, int tileX1, int tileY1, int marginChunks);
	void EnsureChunksInTileRect_NoLock(int tileX0, int tileY0, int tileX1, int tileY1, int marginChunks);
	/////////////////////////////////



	/////////////////////////////////
	void EvictIfNeeded();
	/////////////////////////////////



	/////////////////////////////////
	// UpdateMainThread - Called from the main thread to process any chunks that have finished loading in the background.
	void UpdateMainThread(); 
	void UpdateMainThread_NoLock(); // version that assumes caller holds m_mutex
	/////////////////////////////////



	/////////////////////////////////
	// SaveAllChunks - Save all dirty chunks to disk in the specified directory. Each chunk will be saved as a separate file named 
	// "chunk_X_Y.dat" where X and Y are the chunk coordinates.
	void SaveAllChunks();
	/////////////////////////////////



	/////////////////////////////////
	// SetBasePath - Set the base directory path for chunk files. This path is used for loading and saving chunk data to disk.
	void SetBasePath(const std::string& basePath);
	/////////////////////////////////



	/////////////////////////////////
	void SetWorldOffset(int offsetX, int offsetY) {
		worldOffsetX = offsetX;
		worldOffsetY = offsetY;
	}
	/////////////////////////////////

	/////////////////////////////////
	// SetWorldSize - Set the size of the world in pixels. This is used to
	void SetWorldSize(int width, int height); 
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// GetBasePath - Get the current base directory path for chunk files.
	std::string GetBasePath() const { return m_basePath; }
	/////////////////////////////////



	/////////////////////////////////
	void BuildWorldMask(); // Build the world mask for collision/pathfinding
	void RefreshWorldBoundsFromLoadedChunks(); // Recompute world offset/size from currently loaded chunks
	/////////////////////////////////



	/////////////////////////////////
	// Load all saved chunk files from disk into memory (called on startup)
	void LoadAllSavedChunks();
	/////////////////////////////////



	/////////////////////////////////
	// ClearAllLoadedChunks - Remove all loaded chunks from memory without saving; used when switching levels or resetting the world.
	void ClearAllLoadedChunks();
	/////////////////////////////////



	/////////////////////////////////
	// DebugPrintLayer1 - DEBUG: Print layer 1 obstacle grid to console for debugging purposes. This function outputs the tile values of 
	// layer 1 for all loaded chunks to the console, allowing developers to inspect the current state of the level's obstacles.
	void DebugPrintLayer1();
	/////////////////////////////////



	/////////////////////////////////
	// LoadLevelFromFile - Load a TileMap JSON file into the chunked world. This will replace current in-memory chunks with
	// the data from the file. Returns true on success and writes an optional error message to outErr.
	bool LoadLevelFromFile(const std::string& path, std::string* outErr = nullptr);
	/////////////////////////////////



	/////////////////////////////////
	// GetSavedChunkBounds - Scan saved chunk filenames on disk and return the bounding box of all saved chunks in world pixels.
	// Returns false if no saved chunks exist. Does not load tile data.
	bool GetSavedChunkBounds(float& outMinX, float& outMinY, float& outMaxX, float& outMaxY) const;
	/////////////////////////////////



	/////////////////////////////////
	// GetMinChunkCoords - Get the minimum loaded chunk coordinates (for coordinate system offset in pathfinding)
	void GetMinChunkCoords(int& outMinCx, int& outMinCy) const;
	/////////////////////////////////



	/////////////////////////////////
	// ShiftChunksToPositiveCoords - Shift all loaded chunks so that minChunkX >= 0 and minChunkY >= 0.
	// Saves shifted chunks to disk. This is useful for level editors to ensure pathfinding doesn't cross negative boundaries.
	void ShiftChunksToPositiveCoords();
	/////////////////////////////////



	/////////////////////////////////
	// SetTilesetKey - Set the key for the tileset atlas used to texture chunk vertex arrays. This allows the ChunkManager to use a 
	// specific tileset for rendering.
	void SetTilesetKey(const std::string& key) { m_tilesetKey = key; }
	/////////////////////////////////



	/////////////////////////////////
	// GetTilesetKey - Get the current key for the tileset atlas used to texture chunk vertex arrays.
	std::string GetTilesetKey() const { return m_tilesetKey; }
	/////////////////////////////////



	/////////////////////////////////
	// SetMaxLoadedChunks - Set the maximum number of chunks that can be loaded in memory at once. If the limit is exceeded, least recently used chunks will be unloaded.
	void SetMaxLoadedChunks(size_t maxChunks) {	m_maxLoadedChunks = maxChunks; }
	/////////////////////////////////



	/////////////////////////////////
	// SetActiveLayer - Set the active layer for editing or rendering. This allows the caller to control which layer is currently active.
	void SetActiveLayer(int layer) { m_activeLayer = std::max(0, std::min(layer, m_numLayers-1)); }
	/////////////////////////////////



	/////////////////////////////////
	// GetActiveLayer - Get the current active layer for editing or rendering.
	int GetActiveLayer() const { return m_activeLayer; }
	/////////////////////////////////



	/////////////////////////////////
	// SetNumLayers - Allow changing number of layers at runtime based on level meta
	void SetNumLayers(int n) { m_numLayers = std::max(1, n); }
	/////////////////////////////////



	/////////////////////////////////
	// GetNumLayers - Get the current number of layers in the chunk manager.
	int GetNumLayers() const { return m_numLayers; }
	/////////////////////////////////



	/////////////////////////////////
	// SetUnselectedLayerAlpha - Set the alpha value for unselected layers, controlling their transparency in the editor or renderer.
	void SetUnselectedLayerAlpha(float a) { m_unselectedLayerAlpha = std::clamp(a, 0.0f, 1.0f); }
	/////////////////////////////////



	/////////////////////////////////
	// GetUnselectedLayerAlpha - Get the current alpha value for unselected layers, allowing the caller to inspect the transparency setting.
	float GetUnselectedLayerAlpha() const { return m_unselectedLayerAlpha; }
	/////////////////////////////////



	/////////////////////////////////
	// Accessors for chunk dimensions
	int GetChunkWidth() const { return m_chunkWidth; }
	int GetChunkHeight() const { return m_chunkHeight; }
	float GetTileSize() const { return m_tileSize; }


	/////////////////////////////////



	/////////////////////////////////
	// Accessors for read-only inspection by renderers / scenes. These return references guarded by the caller using the mutex returned by GetMutex().
	std::unordered_map<long long, Chunk>& GetChunks() { return m_chunks; }
	std::mutex& GetMutex() { return m_mutex; }
	uint64_t GetWorldRevision() const noexcept { return m_worldRevision.load(std::memory_order_relaxed); }
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods for loading, saving, and managing chunks, as well as building vertex arrays for rendering.
private:
	/////////////////////////////////
	// Internal helper methods for loading, saving, and managing chunks
	void EnqueueLoadChunk(int chunkX, int chunkY);
	void EnqueueLoadChunk(int chunkX, int chunkY, int layer);
	void EnqueueLoadChunk_NoLock(int chunkX, int chunkY);
	void FinalizeLoadedChunk(int chunkX, int chunkY, int layer, std::vector<int> tileData, uint32_t versionAtEnqueue);
	void RebuildChunkEntities(Chunk& chunk); // Rebuilds merged collider entities for a chunk; call after tile edits.
	void ScheduleChunkForRebuild(Chunk& chunk); // Schedule chunk for main-thread-only GPU/collider rebuild
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// BuildChunkVertexArray - Builds the vertex array for a chunk using the given atlas (may be nullptr for colour fallback)
	static void BuildChunkVertexArray(Chunk& chunk, const std::shared_ptr<TextureAtlas>& atlas);
	/////////////////////////////////
	 

	
	/////////////////////////////////
	// GetChunkKey - Helper function to combine chunkX and chunkY into a single key for the chunks map
	static long long GetChunkKey(int chunkX, int chunkY) {
		return (static_cast<long long>(chunkX) << 32) | static_cast<unsigned int>(chunkY);
	}
	/////////////////////////////////



	/////////////////////////////////
	// FloorDiv - Helper function to perform floor division of two integers, ensuring that the result is rounded down to the nearest integer. 
	// This is useful for calculating chunk coordinates from tile coordinates. Note :- For optimal performance, there is no actual floor 
	// division operator in C++, so we implement it manually. The function asserts that the divisor is positive to avoid undefined behavior.
	static inline int FloorDiv(int a, int b) { 
		assert(b > 0); // Ensure divisor is positive
		if (a >= 0) return a / b;
		return -(((-a) + (b - 1)) / b);
	}
	/////////////////////////////////


	/////////////////////////////////
	// Safety: maximum number of chunks to attempt to load in a single EnsureChunksInTileRect call (span in each axis)
	static constexpr int kMaxChunkSpan = 256;
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the ChunkManager class, including configuration for chunk size and tile size, base path for chunk 
	// files, maximum loaded chunks, data structures for managing loaded chunks and LRU eviction, and synchronization primitives for 
	// thread safety.
	int m_chunkWidth; // Width of each chunk in tiles
	int m_chunkHeight; // Height of each chunk in tiles
	float m_tileSize;  // Size of each tile in pixels
	/////////////////////////////////



	/////////////////////////////////
	// Configuration for file paths and maximum loaded chunks
	std::string m_basePath = "levels/"; // Base directory path for chunk files
	size_t m_maxLoadedChunks = 256; // Maximum number of chunks that can be loaded in memory at once
	/////////////////////////////////



	/////////////////////////////////
	// Data structures for managing loaded chunks and LRU eviction
	std::mutex m_mutex;	// Mutex to protect access to the chunks map and LRU list across threads
	std::unordered_map<long long, Chunk> _GUARDED_BY(m_mutex) m_chunks; // Map of loaded chunks, keyed by a combined chunkX and chunkY value (e.g., (chunkX << 32) | chunkY)
	std::list<long long> _GUARDED_BY(m_mutex) m_lruList;				// LRU (Least Recently Used) chunks for eviction (stores 64bit chunk keys)
	std::unordered_map<long long, std::list<long long>::iterator> _GUARDED_BY(m_mutex) m_lruIndex; // O(1) iterator lookup into m_lruList
	std::atomic<uint64_t> m_worldRevision{1}; // Monotonic change counter for loaded chunk/world mutation detection
	/////////////////////////////////



	/////////////////////////////////
	// Rendering and layer management
	std::string m_tilesetKey;				// optional tileset atlas key used to texture chunk vertex arrays
	int m_numLayers = 1;					// number of layers per chunk (background, main, upper)
	int m_activeLayer = 0;					// drawing: which layer is fully opaque
	float m_unselectedLayerAlpha = 0.3f;	// opacity for unselected layers (0..1)
	/////////////////////////////////



	/////////////////////////////////
	// Queue of chunks that need GPU-side vertex rebuilds or collider rebuilds. Only the main thread
	// should perform the actual SFML/OpenGL work. These structures are guarded by m_mutex.
	std::vector<long long> m_rebuildQueue; // chunk keys scheduled for rebuild
	std::unordered_set<long long> m_rebuildSet; // quick membership check to avoid duplicate enqueues
	/////////////////////////////////



	/////////////////////////////////
	// Public methods for drawing chunks and registering/unregistering chunk colliders with the EntityManager.
public:
	/////////////////////////////////
	// DrawChunks - Draw all chunks that intersect the provided view. This will perform a short copy of visible chunk data under 
	// the mutex and then draw them without holding the lock to minimize contention; *** DEPRECATED: Use EnqueueChunks instead ***
	void DrawChunks(sf::RenderWindow& window, const sf::View& view); 
	/////////////////////////////////




	/////////////////////////////////
	// UpdateStreaming - Update streaming of chunks based on the current view (load/unload as needed). This method will determine which 
	// chunks are visible in the current view and ensure they are loaded, while also evicting any chunks that are no longer needed.
	void UpdateStreaming(const sf::View& view); // Update streaming of chunks based on the current view (load/unload as needed)
	/////////////////////////////////



	/////////////////////////////////
	// EnqueueChunks - Enqueue all visible chunks to the render queue for rendering. This method will determine which chunks are visible
	void EnqueueChunks(RenderQueue& queue, const sf::View& view);
	/////////////////////////////////



	/////////////////////////////////
	// EnsureChunkLoaded - Ensure a specific chunk is loaded and ready for rendering. If the chunk is not already loaded, it will be enqueued 
	// for loading.
	void EnsureChunkLoaded(int cx, int cy);
	/////////////////////////////////



	/////////////////////////////////
	// EvictChunksOutsideRadius - Evict chunks that are outside the specified radius from the given chunk coordinates.
	void EvictChunksOutsideRadius(int minCx, int maxCx, int minCy, int maxCy); 
	/////////////////////////////////



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