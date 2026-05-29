// ChunkManager.cpp - Implementation of the ChunkManager class for managing tile-based chunks in a game world. This class handles loading, saving, and managing chunks of tiles, including background loading and eviction of least recently used chunks when memory limits are exceeded.

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


// Static members for background loading; pending chunks are stored in a thread-safe queue and processed in the main thread
static std::mutex s_pendingMutex; // Mutex to protect access to the pending chunks queue
static std::vector<std::tuple<int, int, std::vector<int>>> s_pendingChunks; // Queue of chunks pending loading


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

void ChunkManager::RebuildAllChunksFromTileset() {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto &pr : m_chunks) {
		auto &chunk = pr.second;
		chunk.vertexArray.clear();
		chunk.vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);
		chunk.vertexTexture.reset();
		std::shared_ptr<TextureAtlas> atlasPtr;
		if (!m_tilesetKey.empty()) {
			auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(m_tilesetKey);
			if (atlasOpt.has_value() && *atlasOpt) {
				atlasPtr = *atlasOpt;
				chunk.vertexTexture = atlasPtr->GetTexture();
			}
		// no debug logging
		}
		for (int y = 0; y < chunk.height; ++y) {
			for (int x = 0; x < chunk.width; ++x) {
				int v = chunk.tiles[y * chunk.width + x];
				if (v == 0) continue;
				float px = (chunk.chunkX * chunk.width + x) * chunk.tileSize;
				float py = (chunk.chunkY * chunk.height + y) * chunk.tileSize;
				bool usedTexture = false;
				sf::Vector2f uv00(0.f, 0.f), uv11(0.f, 0.f);
				if (atlasPtr) {
					// map stored tile value (1-based) to atlas 0-based index
					size_t atlasIdx = (size_t)(v - 1);
					auto rectOpt = atlasPtr->GetSfFloatRectForTile(atlasIdx);
					if (rectOpt.has_value()) {
						sf::FloatRect fr = *rectOpt;
						uv00 = sf::Vector2f(fr.position.x, fr.position.y);
						uv11 = sf::Vector2f(fr.position.x + fr.size.x, fr.position.y + fr.size.y);
						usedTexture = true;
					}
				}
				if (usedTexture && chunk.vertexTexture) {
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px, py), sf::Color::White, uv00));
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px + chunk.tileSize, py), sf::Color::White, sf::Vector2f(uv11.x, uv00.y)));
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px + chunk.tileSize, py + chunk.tileSize), sf::Color::White, uv11));
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px, py), sf::Color::White, uv00));
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px + chunk.tileSize, py + chunk.tileSize), sf::Color::White, uv11));
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px, py + chunk.tileSize), sf::Color::White, sf::Vector2f(uv00.x, uv11.y)));
				} else {
					sf::Vertex vv0(sf::Vector2f(px, py), sf::Color(120, 120, 120, 200));
					sf::Vertex vv1(sf::Vector2f(px + chunk.tileSize, py), sf::Color(120, 120, 120, 200));
					sf::Vertex vv2(sf::Vector2f(px + chunk.tileSize, py + chunk.tileSize), sf::Color(120, 120, 120, 200));
					sf::Vertex vv3(sf::Vector2f(px, py + chunk.tileSize), sf::Color(120, 120, 120, 200));
					chunk.vertexArray.append(vv0);
					chunk.vertexArray.append(vv1);
					chunk.vertexArray.append(vv2);
					chunk.vertexArray.append(vv0);
					chunk.vertexArray.append(vv2);
					chunk.vertexArray.append(vv3);
				}
			}
		}
	}

}

// DrawChunks - Renders the visible chunks to the provided SFML RenderWindow based on the current view. This method computes which chunks are visible within the view's AABB and draws them accordingly. 
// Chunks that are not ready for rendering will be drawn with a semi-transparent overlay to indicate they are still loading.
void ChunkManager::DrawChunks(sf::RenderWindow& window, const sf::View& view) {
	// Compute view AABB
	sf::Vector2f viewCenter = view.getCenter();
	sf::Vector2f viewSize = view.getSize();
	float vleft = viewCenter.x - viewSize.x * 0.5f;
	float vtop = viewCenter.y - viewSize.y * 0.5f;
	float vright = vleft + viewSize.x;
	float vbottom = vtop + viewSize.y;

	// Collect visible chunks under lock (copy them so we don't hold lock while drawing)
	std::vector<Chunk> visibleCopies;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (const auto& pr : m_chunks) {
			const Chunk& c = pr.second;
			if (c.width <= 0 || c.height <= 0) continue;
			if ((int)c.tiles.size() < c.width * c.height) continue;
			float cx = (float)(c.chunkX * c.width) * c.tileSize;
			float cy = (float)(c.chunkY * c.height) * c.tileSize;
			float cright = cx + (float)c.width * c.tileSize;
			float cbottom = cy + (float)c.height * c.tileSize;
			// AABB overlap test (half-open)
			if (!(cright <= vleft || cx >= vright || cbottom <= vtop || cy >= vbottom)) {
				visibleCopies.push_back(c);
			}
		}
	}

	// Draw copied chunks
	for (const Chunk& c : visibleCopies) {
		float wx = (float)(c.chunkX * c.width) * c.tileSize;
		float wy = (float)(c.chunkY * c.height) * c.tileSize;
		if (!c.readyForRendering) {
			sf::RectangleShape r(sf::Vector2f((float)c.width * c.tileSize, (float)c.height * c.tileSize));
			r.setPosition(sf::Vector2f(wx, wy));
			r.setFillColor(sf::Color(60, 60, 60, 80));
			window.draw(r);
			continue;
		}
		// If vertexArray is populated, draw it directly. vertexArray contains absolute positions.
		if (c.vertexArray.getVertexCount() > 0) {
			sf::RenderStates states;
			if (c.vertexTexture) states.texture = c.vertexTexture.get();
			window.draw(c.vertexArray, states);
		} else {
			// Fallback: draw per-tile rects
			for (int y = 0; y < c.height; ++y) {
				for (int x = 0; x < c.width; ++x) {
					int v = c.tiles[y * c.width + x];
					if (v == 0) continue;
					sf::RectangleShape r(sf::Vector2f(c.tileSize, c.tileSize));
					r.setPosition(sf::Vector2f((float)(c.chunkX * c.width + x) * c.tileSize, (float)(c.chunkY * c.height + y) * c.tileSize));
					r.setFillColor(sf::Color(120, 120, 120, 200));
					window.draw(r);
				}
			}
		}
	}
}


// Destructor - currently does not have any special cleanup logic, but we could add it if needed in the future (e.g., to save dirty chunks before exiting)
ChunkManager::~ChunkManager() { 
	SaveAllChunks(); // Ensure all dirty chunks are saved to disk when the ChunkManager is destroyed to prevent data loss.
	std::lock_guard<std::mutex> lock(s_pendingMutex); // Lock the mutex to safely clear the pending chunks queue
	s_pendingChunks.clear();						  // Clear the pending chunks queue to free memory	
}


// GetTileAt - Retrieves the tile value at the specified tile coordinates (tileX, tileY). If the corresponding chunk is not loaded, it will be enqueued for loading in the background thread. 
// Returns 0 if the chunk is not loaded or if the tile coordinates are out of bounds within the chunk.
int ChunkManager::GetTileAt(int tileX, int tileY) {
	int chunkX = std::floor(tileX / static_cast<float>(m_chunkWidth)); // Calculate the chunk X coordinate based on the tile X coordinate and chunk width
	int chunkY = std::floor(tileY / static_cast<float>(m_chunkHeight)); // Calculate the chunk Y coordinate based on the tile Y coordinate and chunk height

	int localX = tileX - chunkX * m_chunkWidth; // Calculate the local X coordinate within the chunk
	int localY = tileY - chunkY * m_chunkHeight; // Calculate the local Y coordinate within the chunk

	long long key = GetChunkKey(chunkX, chunkY); // Get the unique key for the chunk based on its coordinates
	
	std::lock_guard<std::mutex> lock(m_mutex); // Lock the mutex to safely access the chunks map

	auto itr = m_chunks.find(key); // Attempt to find the chunk in the loaded chunks map
	if (itr == m_chunks.end()) {
		EnqueueLoadChunk(chunkX, chunkY); // If the chunk is not found, enqueue it to be loaded in the background thread
		return 0; // Chunk not found, return default value
	}

	Chunk& chunk = itr->second; // Get a reference to the found chunk
	// Check if the local tile coordinates are within the bounds of the chunk
	if (localX >= 0 && localX < chunk.width && localY >= 0 && localY < chunk.height) {
		return chunk.tiles[localY * chunk.width + localX]; // Return the tile value at the specified local coordinates within the chunk
	}

	return 0;
}


// SetTileAt - Sets the tile value at the specified tile coordinates (tileX, tileY) to the given tileValue. 
// If the corresponding chunk is not loaded, it will be enqueued for loading in the background thread.
int ChunkManager::SetTileAt(int tileX, int tileY, int tileValue) {
	int chunkX = std::floor(tileX / static_cast<float>(m_chunkWidth)); // Calculate the chunk X coordinate based on the tile X coordinate and chunk width
	int chunkY = std::floor(tileY / static_cast<float>(m_chunkHeight)); // Calculate the chunk Y coordinate based on the tile Y coordinate and chunk height

	int localX = tileX - chunkX * m_chunkWidth; // Calculate the local X coordinate within the chunk
	int localY = tileY - chunkY * m_chunkHeight; // Calculate the local Y coordinate within the chunk

	long long key = GetChunkKey(chunkX, chunkY); // Get the unique key for the chunk based on its coordinates

	{
		// Lock the mutex to safely access the chunks map
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_chunks.find(key) == m_chunks.end()) {
			// create placeholder chunk if it doesn't exist
			m_chunks.emplace(
				key, Chunk(chunkX, chunkY, m_chunkWidth, m_chunkHeight,
						   m_tileSize)); // Create a new chunk with default tile data and add it to the chunks map
			m_lruList.push_front(key);	 // Add the new chunk to the front of the LRU list to mark it as recently used
			EnqueueLoadChunk(chunkX, chunkY); // If the chunk is not found, enqueue it to be loaded in the background thread
		}
	}

	std::lock_guard<std::mutex> lock(m_mutex); // Lock the mutex again to safely access the chunk after ensuring it exists
	auto itr = m_chunks.find(key); // Find the chunk in the loaded chunks map
	if (itr == m_chunks.end()) {
		return 0; // This should not happen since we just ensured the chunk exists, but return default value if it does
	}

	Chunk& chunk = itr->second; // Get a reference to the found chunk
	if (localX < 0 || localX >= chunk.width || localY < 0 || localY >= chunk.height) {
		return 0; // Local tile coordinates are out of bounds within the chunk, return default value
	}

	int index = localY * chunk.width + localX; // Calculate the index in the chunk's tile data vector based on local coordinates
	int prevValue = chunk.tiles[index];  // Store the previous tile value before updating it

	if (prevValue != tileValue) { // Only mark the chunk as dirty if the tile value is actually changing to avoid unnecessary saves
		chunk.tiles[index] = tileValue; // Update the tile value at the specified local coordinates within the chunk
		chunk.dirty = true; // Mark the chunk as dirty since it has been modified and needs to be saved to disk later

		// Move the chunk to the front of the LRU list to mark it as recently used
		m_lruList.remove(key); // Remove the chunk key from its current position in the LRU list
		m_lruList.push_front(key); // Add the chunk key to the front of the LRU list to mark it as recently used

		// Rebuild vertex array for this chunk immediately so changes are visible on next draw
		chunk.vertexArray.clear();
		chunk.vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);
		chunk.vertexTexture.reset();
	// Try to acquire atlas by configured key; if no key configured, fall back to the first loaded atlas
	std::shared_ptr<TextureAtlas> atlasPtr;
	if (!m_tilesetKey.empty()) {
		auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(m_tilesetKey);
		if (atlasOpt.has_value() && *atlasOpt) {
			atlasPtr = *atlasOpt;
			chunk.vertexTexture = atlasPtr->GetTexture();
			if (chunk.vertexTexture) std::cout << "  chunk about to use texture for key='" << m_tilesetKey << "'\n";
		}
	} else {
		// no key; try first available atlas
		auto keys = GameEngine::GetInstance().GetTextureManager().GetAtlasKeys();
		if (!keys.empty()) {
			auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(keys[0]);
			if (atlasOpt.has_value() && *atlasOpt) {
				atlasPtr = *atlasOpt;
				chunk.vertexTexture = atlasPtr->GetTexture();
				if (chunk.vertexTexture) std::cout << "  chunk about to use texture for first-available key='" << keys[0] << "'\n";
			}
		}
	}
		for (int yy = 0; yy < chunk.height; ++yy) {
			for (int xx = 0; xx < chunk.width; ++xx) {
				int tv = chunk.tiles[yy * chunk.width + xx];
				if (tv == 0) continue;
				float px = (chunk.chunkX * chunk.width + xx) * chunk.tileSize;
				float py = (chunk.chunkY * chunk.height + yy) * chunk.tileSize;
				bool usedTexture = false;
				sf::Vector2f uv00(0.f, 0.f), uv11(0.f, 0.f);
				if (atlasPtr) {
				// map stored tile value (1-based) to atlas 0-based index
				size_t atlasIdx = (size_t)(tv - 1);
				auto rectOpt = atlasPtr->GetSfFloatRectForTile(atlasIdx);
				if (rectOpt.has_value()) {
						sf::FloatRect fr = *rectOpt;
						uv00 = sf::Vector2f(fr.position.x, fr.position.y);
						uv11 = sf::Vector2f(fr.position.x + fr.size.x, fr.position.y + fr.size.y);
						usedTexture = true;
					}
				}
				if (usedTexture && chunk.vertexTexture) {
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px, py), sf::Color::White, uv00));
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px + chunk.tileSize, py), sf::Color::White, sf::Vector2f(uv11.x, uv00.y)));
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px + chunk.tileSize, py + chunk.tileSize), sf::Color::White, uv11));
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px, py), sf::Color::White, uv00));
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px + chunk.tileSize, py + chunk.tileSize), sf::Color::White, uv11));
					chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px, py + chunk.tileSize), sf::Color::White, sf::Vector2f(uv00.x, uv11.y)));
				} else {
					sf::Vertex vv0(sf::Vector2f(px, py), sf::Color(120, 120, 120, 200));
					sf::Vertex vv1(sf::Vector2f(px + chunk.tileSize, py), sf::Color(120, 120, 120, 200));
					sf::Vertex vv2(sf::Vector2f(px + chunk.tileSize, py + chunk.tileSize), sf::Color(120, 120, 120, 200));
					sf::Vertex vv3(sf::Vector2f(px, py + chunk.tileSize), sf::Color(120, 120, 120, 200));
					chunk.vertexArray.append(vv0);
					chunk.vertexArray.append(vv1);
					chunk.vertexArray.append(vv2);
					chunk.vertexArray.append(vv0);
					chunk.vertexArray.append(vv2);
					chunk.vertexArray.append(vv3);
				}
			}
		}
	}
}


// EnsureChunksInTileRect - Ensures that all chunks that intersect the specified tile rectangle (tileX0, tileY0, tileX1, tileY1) are loaded and ready for rendering. 
// The marginChunks parameter specifies how many additional chunks to load around the edges of the rectangle to ensure smooth rendering when the player moves.
void ChunkManager::EnsureChunksInTileRect(int tileX0, int tileY0, int tileX1, int tileY1, int marginChunks) {
	// normalize the tile rectangle coordinates to ensure tileX0 <= tileX1 and tileY0 <= tileY1
	if (tileX0 > tileX1) std::swap(tileX0, tileX1); // Swap tileX0 and tileX1 if they are in the wrong order to ensure tileX0 is the minimum X coordinate
	if (tileY0 > tileY1) std::swap(tileY0, tileY1); // Swap tileY0 and tileY1 if they are in the wrong order to ensure tileY0 is the minimum Y coordinate

	int chunkX0 = FloorDiv(tileX0, m_chunkWidth) -	marginChunks; // Calculate the minimum chunk X coordinate that intersects the tile rectangle, including margin chunks
	int chunkY0 = FloorDiv(tileY0, m_chunkHeight) -	marginChunks; // Calculate the minimum chunk Y coordinate that intersects the tile rectangle, including margin chunks
	int chunkX1 = FloorDiv(tileX1, m_chunkWidth) + marginChunks; // Calculate the maximum chunk X coordinate that intersects the tile rectangle, including margin chunks
	int chunkY1 = FloorDiv(tileY1, m_chunkHeight) + marginChunks; // Calculate the maximum chunk Y coordinate that intersects the tile rectangle, including margin chunks

	for (int chunkY = chunkY0; chunkY <= chunkY1; ++chunkY) { // Loop through the range of chunk Y coordinates that intersect the tile rectangle
		for (int chunkX = chunkX0; chunkX <= chunkX1;
			 ++chunkX) { // Loop through the range of chunk X coordinates that intersect the tile rectangle
			long long key =	GetChunkKey(chunkX, chunkY); // Get the unique key for the current chunk based on its coordinates
			{
				std::lock_guard<std::mutex> lock(m_mutex); // Lock the mutex to safely access the chunks map
				if (m_chunks.find(key) != m_chunks.end()) {
					// Move the chunk to the front of the LRU list to mark it as recently used since it's needed for rendering
					m_lruList.remove(key); // Remove the chunk key from its current position in the LRU list
					m_lruList.push_front(
						key); // Add the chunk key to the front of the LRU list to mark it as recently used
					continue; // Chunk is already loaded, move to the next chunk
				}

				// Chunk is not loaded, enqueue it to be loaded in the background thread
				m_chunks.emplace(
					key, Chunk(chunkX, chunkY, m_chunkWidth, m_chunkHeight,
							   m_tileSize)); // Create a new chunk with default tile data and add it to the chunks map
				m_lruList.push_front(key); // Add the new chunk to the front of the LRU list to mark it as recently used
			}

			// start background load for the specific chunk we just inserted
			EnqueueLoadChunk(chunkX, chunkY);
		}
	}
}


// UpdateMainThread - This method should be called from the main thread to perform any necessary updates, such as processing dirty chunks or preparing vertex buffers for rendering.
void ChunkManager::UpdateMainThread() {
	// Process any chunks that have been loaded in the background thread and are pending finalization
	std::vector<std::tuple<int, int, std::vector<int>>>	pendingChunksCopy; // Create a local copy of the pending chunks to minimize time spent holding the mutex lock
	{
		std::lock_guard<std::mutex> lock(s_pendingMutex); // Lock the mutex to safely access the pending chunks queue
		pendingChunksCopy = s_pendingChunks;			  // Copy the pending chunks to a local variable
		s_pendingChunks.clear(); // Clear the original pending chunks queue to free memory and allow new chunks to be added by the background thread
	}
	
	// Finalize each loaded chunk by setting its tile data and marking it as ready for rendering. 
	// This should be done in the main thread to ensure thread safety when modifying the chunks map and to prepare the chunk for rendering.
	for (const auto& [chunkX, chunkY, tileData] : pendingChunksCopy) { // Loop through the copied list of pending chunks
		FinalizeLoadedChunk(chunkX, chunkY,	tileData); // Finalize each loaded chunk by setting its tile data and marking it as ready for rendering
	}
}


// SaveAllChunks - Saves all dirty chunks to disk in the specified directory. Each chunk will be saved as a separate file named "chunk_X_Y.dat" where X and Y are the chunk coordinates.
void ChunkManager::SaveAllChunks() {
	std::lock_guard<std::mutex> lock(m_mutex); // Lock the mutex to safely access the chunks map
	for (auto& pr : m_chunks) {
		Chunk& chunk = pr.second; // Get a reference to the current chunk in the loop
		if (chunk.dirty) {		  // Only save chunks that are marked as dirty to avoid unnecessary disk writes
			std::string filename = m_basePath + "chunk_" + std::to_string(chunk.chunkX) + "_" + std::to_string(chunk.chunkY) + ".dat"; // Construct the filename for the chunk based on its coordinates
			std::ofstream outFile(filename, std::ios::binary); // Open a binary output file stream to save the chunk data
			
			if (outFile) {
				outFile.write(reinterpret_cast<const char*>(chunk.tiles.data()), chunk.tiles.size() * sizeof(int)); // Write the chunk's tile data to the file
				chunk.dirty = false; // Mark the chunk as clean since it has been saved to disk
				std::cout << "Saved chunk to file: " << filename  << std::endl; // Log a message indicating that the chunk was successfully saved
			} else {
				std::cerr << "Error saving chunk to file: " << filename << std::endl; // Log an error if the file could not be opened for writing
			}
		}
	}
}


// EnqueueLoadChunk - Enqueues a chunk to be loaded in the background thread. The chunk will be loaded from disk if it exists, or created with default tile data if it does not.
void ChunkManager::EnqueueLoadChunk(int chunkX, int chunkY) {
	// Start a background thread to load the chunk data from disk. The chunk will be loaded from disk if it exists, or created with default tile data if it does not.
	std::thread([chunkX, chunkY, this]() {
		std::string filename = m_basePath + "chunk_" + std::to_string(chunkX) + "_" + std::to_string(chunkY) + ".dat"; // Construct the filename for the chunk based on its coordinates
		std::vector<int> tileData(m_chunkWidth * m_chunkHeight,	0); // Create a vector to hold the tile data for the chunk, initialized with default values (0 = empty)
		
		if (fs::exists(filename)) {							  // Check if the chunk file exists on disk
			std::ifstream inFile(filename, std::ios::binary); // Open a binary input file stream to read the chunk data
			if (inFile) {
				inFile.read(reinterpret_cast<char*>(tileData.data()), tileData.size() * sizeof(int)); // Read the chunk's tile data from the file into the tileData vector
			} else {
				std::cerr << "Error loading chunk from file: " << filename << std::endl; // Log an error if the file could not be opened for reading
			}
		}
		// After loading the chunk data (or using default data if the file doesn't exist), add it to the pending chunks queue to be finalized in the main thread
		{
			std::lock_guard<std::mutex> lock(s_pendingMutex); // Lock the mutex to safely access the pending chunks queue
			s_pendingChunks.emplace_back(chunkX, chunkY,tileData); // Add the loaded chunk data to the pending chunks queue for finalization in the main thread
		}
	}).detach(); // Detach the thread to allow it to run independently without blocking the main thread
}


// FinalizeLoadedChunk - Finalizes the loading of a chunk by setting its tile data and marking it as ready for rendering. This should be called from the main thread after a chunk has been loaded in the background.
void ChunkManager::FinalizeLoadedChunk(int chunkX, int chunkY, std::vector<int> tileData) {
	long long key = GetChunkKey(chunkX, chunkY); // Get the unique key for the chunk based on its coordinates
	std::lock_guard<std::mutex> lock(m_mutex);	 // Lock the mutex to safely access the chunks map
	auto itr = m_chunks.find(key);				 // Find the chunk in the loaded chunks map
	
	if (itr == m_chunks.end()) {
		// This should not happen since we should have already ensured the chunk exists before enqueuing it for loading, but....
		Chunk newChunk(chunkX, chunkY, m_chunkWidth, m_chunkHeight, m_tileSize); // Create a new chunk with default tile data
		newChunk.tiles = std::move(tileData); // Set the new chunk's tile data to the loaded tile data using move semantics to avoid unnecessary copying
		newChunk.readyForRendering = true; // Mark the new chunk as ready for rendering since it has been loaded and finalized
		newChunk.dirty = false; // Mark the new chunk as clean since it has just been loaded and does not need to be saved to disk yet
		
		m_chunks.emplace(key,std::move(newChunk)); // Add the new chunk to the chunks map using move semantics to avoid unnecessary copying
		m_lruList.push_front(key); // Add the new chunk to the front of the LRU list to mark it as recently used
		return; 
	}

	Chunk& chunk = itr->second; // Get a reference to the found chunk
	// Replace tiles and mark ready
	if ((int)tileData.size() == chunk.width * chunk.height) {
		chunk.tiles = std::move(tileData);
	} else {
		// If incoming data doesn't match, resize and fill zeros
		chunk.tiles.assign(chunk.width * chunk.height, 0);
	}
	chunk.readyForRendering = true;
	chunk.dirty = false;
	// Build GPU vertex array for this chunk on the main thread so rendering can be fast.
	chunk.cpuVertexBuffer.clear();
	chunk.vertexArray.clear();
	chunk.vertexArray.setPrimitiveType(sf::PrimitiveType::Triangles);
	chunk.vertexTexture.reset();
	// If tileset key is configured, try to acquire the texture and atlas pointer
	std::shared_ptr<TextureAtlas> atlasPtr;
	if (!m_tilesetKey.empty()) {
		auto atlasOpt = GameEngine::GetInstance().GetTextureManager().GetAtlas(m_tilesetKey);
		if (atlasOpt.has_value() && *atlasOpt) {
			atlasPtr = *atlasOpt;
			chunk.vertexTexture = atlasPtr->GetTexture();
		}
	}
	for (int y = 0; y < chunk.height; ++y) {
		for (int x = 0; x < chunk.width; ++x) {
			int v = chunk.tiles[y * chunk.width + x];
			if (v == 0) continue;
			float px = (chunk.chunkX * chunk.width + x) * chunk.tileSize;
			float py = (chunk.chunkY * chunk.height + y) * chunk.tileSize;
			// create two triangles for the quad (v0,v1,v2) and (v0,v2,v3)
			bool usedTexture = false;
			sf::Vector2f uv00(0.f, 0.f), uv11(0.f, 0.f);
			if (atlasPtr) {
					// map stored tile value (1-based) to atlas 0-based index
					size_t atlasIdx = (size_t)(v - 1);
					auto rectOpt = atlasPtr->GetSfFloatRectForTile(atlasIdx);
				if (rectOpt.has_value()) {
					sf::FloatRect fr = *rectOpt;
					// Texture atlas FloatRect uses position/size members
					uv00 = sf::Vector2f(fr.position.x, fr.position.y);
					uv11 = sf::Vector2f(fr.position.x + fr.size.x, fr.position.y + fr.size.y);
					usedTexture = true;
				}
			}

			if (usedTexture && chunk.vertexTexture) {
				// textured vertices (texcoords in pixels)
				chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px, py), sf::Color::White, uv00));
				chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px + chunk.tileSize, py), sf::Color::White, sf::Vector2f(uv11.x, uv00.y)));
				chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px + chunk.tileSize, py + chunk.tileSize), sf::Color::White, uv11));
				chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px, py), sf::Color::White, uv00));
				chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px + chunk.tileSize, py + chunk.tileSize), sf::Color::White, uv11));
				chunk.vertexArray.append(sf::Vertex(sf::Vector2f(px, py + chunk.tileSize), sf::Color::White, sf::Vector2f(uv00.x, uv11.y)));
			} else {
				// colored fallback
				sf::Vertex vv0(sf::Vector2f(px, py), sf::Color(120, 120, 120, 200));
				sf::Vertex vv1(sf::Vector2f(px + chunk.tileSize, py), sf::Color(120, 120, 120, 200));
				sf::Vertex vv2(sf::Vector2f(px + chunk.tileSize, py + chunk.tileSize), sf::Color(120, 120, 120, 200));
				sf::Vertex vv3(sf::Vector2f(px, py + chunk.tileSize), sf::Color(120, 120, 120, 200));
				chunk.vertexArray.append(vv0);
				chunk.vertexArray.append(vv1);
				chunk.vertexArray.append(vv2);
				chunk.vertexArray.append(vv0);
				chunk.vertexArray.append(vv2);
				chunk.vertexArray.append(vv3);
			}
		}
	}
	// touch LRU
	m_lruList.remove(key);
	m_lruList.push_front(key);
}


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
				std::string filename = m_basePath + "chunk_" + std::to_string(chunk.chunkX) + "_" + std::to_string(chunk.chunkY) + ".dat";
				std::ofstream outFile(filename, std::ios::binary);
				
				if (outFile) { 
					outFile.write(reinterpret_cast<const char*>(chunk.tiles.data()), chunk.tiles.size() * sizeof(int));
				} else {
					std::cerr << "Error saving chunk to file during eviction: " << filename << std::endl;
				}
			}

			// Erase chunk to free memory
			m_chunks.erase(itr);
		}
		m_lruList.pop_back();
	}
}
