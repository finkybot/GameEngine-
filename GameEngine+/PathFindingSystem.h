/////////////////////////////////
// PathFinderSystem.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "ChunkManager.h"
#include "EntityManager.h"
#include "Pathfinder.h" // your hierarchical A* helper
#include "CPathRequest.h"
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <memory>
/////////////////////////////////



/////////////////////////////////
// PathJobResult - Represents the result of a pathfinding job, including the entity ID, request ID, and the resulting path points.
struct PathJobResult {
	size_t entityId;
	std::optional<std::vector<Vec2>> path;
	uint32_t requestId;
	bool complete = false;
};
/////////////////////////////////



/////////////////////////////////
// PathFinderSystem - A system responsible for managing pathfinding requests and results in the game engine. It interacts with the Pathfinder 
// class to find paths in a tile-based world and updates entities with CPathRequest and CPath components accordingly.
//								|
//								|_______________________________________________________________________
class PathFindingSystem {
	/////////////////////////////////
	// Public interface for the PathFindingSystem class, including methods for updating the system and finding paths synchronously.
public:
	/////////////////////////////////
	// Constructor and destructor for the PathFindingSystem class, taking references to ChunkManager and EntityManager instances for managing chunk data and entities.
	PathFindingSystem(ChunkManager& cm, EntityManager& em);
	~PathFindingSystem();
	/////////////////////////////////



	/////////////////////////////////
	// called every frame on main thread
	void Update(float deltaTime);

	// tuning
	void SetNodesPerFrame(int n) { m_nodesPerFrame = n; }
	/////////////////////////////////



	/////////////////////////////////
	// FindPathSync - Finds a path synchronously (blocking) from the start tile to the goal tile using A* algorithm. Returns an optional vector of Vec2 
	// containing world positions if a path is found, or std::nullopt if no path exists; start/end are world tile coordinates; optionally submit a 
	// synchronous path (blocking)
	std::optional<std::vector<Vec2>> FindPathSync(int startTx, int startTy, int goalTx, int goalTy);
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the PathFindingSystem class, including references to ChunkManager and EntityManager instances, a Pathfinder instance for
	// performing pathfinding, and data structures for managing asynchronous pathfinding jobs.
private:
	/////////////////////////////////



	/////////////////////////////////
	// References to ChunkManager and EntityManager instances for managing chunk data and entities.
	ChunkManager& m_chunks;
	EntityManager& m_entities;
	Pathfinder m_pathfinder; // implements hierarchical A*

	const int m_dirs8[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
	/////////////////////////////////



	/////////////////////////////////
	std::vector<std::thread> m_workers; // worker threads for async pathfinding
	std::mutex m_queueMutex;
	std::queue<size_t> m_pendingEntities; // entity ids that requested paths
	std::mutex m_resultsMutex;
	std::queue<PathJobResult> m_doneResults; // completed pathfinding results
	std::atomic<bool> m_shutdown{false};

	// Incremental search state per-entity
	struct ChunkNode { int x,y; float g; long long parent; };
	struct ActiveSearch {
		size_t entityId;
		uint32_t requestId = 0;
		int scx, scy, gcx, gcy; // chunk coords
		std::unordered_map<long long, ChunkNode> nodes;
		std::priority_queue<std::pair<float,long long>, std::vector<std::pair<float,long long>>, std::greater<std::pair<float,long long>>> open; // f, key
		std::unordered_map<long long, bool> closed;
		long long goalKey = -1;
		bool found = false;
	};

	std::unordered_map<size_t, std::unique_ptr<ActiveSearch>> m_activeSearches;
	int m_nodesPerFrame = 500; // budget per entity per frame
	/////////////////////////////////



	/////////////////////////////////
	// WorkerThreadLoop - The main loop for worker threads, responsible for processing pathfinding requests and generating results.
	void WorkerThreadLoop();
	/////////////////////////////////



	/////////////////////////////////
	// ProcessRequests - Polls entities with CPathRequest components and enqueues pathfinding jobs for processing by worker threads.
	void ProcessRequests();
	/////////////////////////////////



	/////////////////////////////////
	// ApplyResults - Pops completed pathfinding results from the queue and attaches CPath components to entities on the main thread.
	void ApplyResults();
	/////////////////////////////////



	/////////////////////////////////
	// Helper methods for refactored Update function
	void PushFailureResult(size_t entityId, uint32_t requestId, std::vector<size_t>& finished);
	void PushSuccessResult(size_t entityId, uint32_t requestId, const std::vector<Vec2>& path, std::vector<size_t>& finished);
	Entity* FindEntityById(size_t entId);
	bool GetChunkCopy(int chunkX, int chunkY, Chunk& outChunk);
	void GetEntityTilePos(Entity* entity, int& outTileX, int& outTileY);
	Vec2 GetEntityGoalWorld(Entity* entity);
	/////////////////////////////////
};
/////////////////////////////////