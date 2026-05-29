// ****** ChunkManager.h Definition and Chunk stuct ******
#pragma once
#include <unordered_map>
#include <list>
#include <mutex>
#include <optional>
#include <string>

#include "TileMap.h"
#include <SFML/Graphics.hpp>

// Chunk struct represents a section of the tile map, containing tile data and metadata for rendering and collision. 
// Each chunk corresponds to a specific area of the game world and can be loaded/unloaded independently to optimize performance and memory usage.
struct Chunk {
	int chunkX = 0,	chunkY = 0; // chunk coordinates (e.g., chunkX = 0, chunkY = 0 is the origin chunk)
	int width = 0, height = 0; // chunk size in tiles (e.g., 16x16 tiles per chunk)

	float tileSize = 32.0f; // size of each tile in pixels (for rendering and collision)
	std::vector<int> tiles; // tile data for the chunk (0 = empty, non-zero = solid)
	
	bool dirty = false;		// flag to indicate if the chunk has been modified and needs saving
	bool readyForRendering = false; // flag to indicate if the chunk is ready to be rendered (e.g., after loading or generating

	std::vector<float> cpuVertexBuffer; // CPU-side vertex buffer for the chunk's tiles, used for rendering.
	sf::VertexArray vertexArray; // GPU-friendly vertex array for fast rendering (built on main thread)
	std::shared_ptr<sf::Texture> vertexTexture; // texture used by the vertexArray (if any)

	Chunk() = default;

	Chunk(int x, int y, int width, int height, float tileSize)
		: chunkX(x), chunkY(y), width(width), height(height), tileSize(tileSize) {
		tiles.resize(width * height, 0); // Initialize all tiles to empty (0)
		vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);
	}
};


class ChunkManager {
public:

	// Constructor and destructor for the ChunkManager class
	ChunkManager(int chunkWidth = 32, int chunkHeight = 32, float tileSize = 32.0f);
	~ChunkManager();
	
	// Load a chunk at the specified chunk coordinates (chunkX, chunkY). If the chunk does not exist, it will be created with default tile data.
	int GetTileAt(int tileX, int tileY);
	int SetTileAt(int tileX, int tileY, int tileValue);

	
	void EnsureChunksInTileRect(int tileX0, int tileY0, int tileX1, int tileY1, int marginChunks); // Ensure all chunks that intersect the specified tile rectangle (tileX0, tileY0, tileX1, tileY1) are loaded and ready for rendering.
	void UpdateMainThread(); // Call this from the main thread to perform any necessary updates, such as processing dirty chunks or preparing vertex buffers for rendering.
	void SaveAllChunks(); // Save all dirty chunks to disk in the specified directory. Each chunk will be saved as a separate file named "chunk_X_Y.dat" where X and Y are the chunk coordinates.

	void SetBasePath(const std::string& basePath) { m_basePath = basePath; } // Set the base directory path where chunk files will be saved and loaded from.
	void SetTilesetKey(const std::string& key) { m_tilesetKey = key; }
	std::string GetTilesetKey() const { return m_tilesetKey; }
	void SetMaxLoadedChunks(size_t maxChunks) {	m_maxLoadedChunks = maxChunks; } // Set the maximum number of chunks that can be loaded in memory at once. If the limit is exceeded, least recently used chunks will be unloaded.

	// Accessors for read-only inspection by renderers / scenes. These return references guarded by the caller using the mutex returned by GetMutex().
	std::unordered_map<long long, Chunk>& GetChunks() { return m_chunks; }
	std::mutex& GetMutex() { return m_mutex; }

private:
	// Internal helper methods for loading, saving, and managing chunks
	void EnqueueLoadChunk(int chunkX, int chunkY); // Enqueue a chunk to be loaded in the background thread. The chunk will be loaded from disk if it exists, or created with default tile data if it does not.
	void FinalizeLoadedChunk(int chunkX, int chunkY, std::vector<int> tileData); // Finalize the loading of a chunk by setting its tile data and marking it as ready for rendering. This should be called from the main thread after a chunk has been loaded in the background.
	void EvictIfNeeded(); // Evict least recently used chunks if the number of loaded chunks exceeds the maximum limit. This will unload chunks from memory but will save them to disk if they are dirty.

	// Helper function to combine chunkX and chunkY into a single key for the chunks map
	static long long GetChunkKey(int chunkX, int chunkY) {
		return (static_cast<long long>(chunkX) << 32) | static_cast<unsigned int>(chunkY);
	}

	// Helper function for floor division of integers, since C++ integer division truncates towards zero. This ensures that negative coordinates are handled correctly when determining chunk coordinates from tile coordinates.
	static inline int FloorDiv(int a, int b) { 
		return (int)std::floor(static_cast<double>(a) / static_cast<double>(b));
	}

	int m_chunkWidth; // Width of each chunk in tiles
	int m_chunkHeight; // Height of each chunk in tiles
	float m_tileSize;  // Size of each tile in pixels

	std::string m_basePath = "levels/"; // Base directory path for chunk files
	size_t m_maxLoadedChunks = 256; // Maximum number of chunks that can be loaded in memory at once

	std::unordered_map<long long, Chunk> m_chunks; // Map of loaded chunks, keyed by a combined chunkX and chunkY value (e.g., (chunkX << 32) | chunkY)
	std::list<long long> m_lruList; // List to track the least recently used chunks for eviction (stores chunk keys)
	std::mutex m_mutex;	// Mutex to protect access to the chunks map and LRU list across threads

	std::string m_tilesetKey; // optional tileset atlas key used to texture chunk vertex arrays

public:
	// Draw all chunks that intersect the provided view. This will perform a short copy of visible chunk data
	// under the mutex and then draw them without holding the lock to minimize contention.
	void DrawChunks(sf::RenderWindow& window, const sf::View& view);
	// Rebuild vertex arrays for all loaded chunks using the currently configured tileset key (if any).
	void RebuildAllChunksFromTileset();
};
