/////////////////////////////////
// PathFinderSystem.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "ChunkManager.h"
#include "EntityManager.h"
#include "Pathfinder.h"
#include "CPathRequest.h"
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <memory>
/////////////////////////////////



/////////////////////////////////
struct PathJob {
	size_t entityId;
	int startTx, startTy;
	int goalTx, goalTy;
	uint32_t requestId;
	bool allowPartial;
};
/////////////////////////////////



/////////////////////////////////
// PathJobResult - Represents the result of a pathfinding job, including the entity ID, request ID, and the resulting path points.
//								|
//								|_______________________________________________________________________
struct PathJobResult {
	size_t entityId;
	std::optional<std::vector<Vec2>> path;
	uint32_t requestId;
	bool complete = true;
};
/////////////////////////////////

/////////////////////////////////
// PathFindingSystem - A system responsible for managing pathfinding requests and results in the game engine. It interacts with the Pathfinder
//								|
//								|_______________________________________________________________________
class PathFindingSystem {
	/////////////////////////////////
	// Public interface for the PathFindingSystem class, including methods for updating the system and finding paths synchronously.
public:
	/////////////////////////////////
	PathFindingSystem(ChunkManager& cm, EntityManager& em);
	~PathFindingSystem();
	void Update(float deltaTime);
	std::optional<std::vector<Vec2>> FindPathSync(int startTx, int startTy, int goalTx, int goalTy);
	/////////////////////////////////
	 
	 
	
	/////////////////////////////////
	// Private member variables and methods for the PathFindingSystem class, including references to the ChunkManager, EntityManager, and Pathfinder,
private:
	/////////////////////////////////
	ChunkManager& m_chunks;
	EntityManager& m_entities;
	Pathfinder m_pathfinder;

	std::queue<PathJobResult> m_doneResults;
	std::mutex m_resultsMutex;

	// --- Pending job queue (worker threads consume these)
	std::queue<PathJob> m_pendingJobs;
	std::mutex m_jobsMutex;
	std::condition_variable m_jobsCv;

	// --- Worker threads
	std::vector<std::thread> m_workers;

	// --- Shutdown flag
	std::atomic<bool> m_shutdown{false};


	// --- Internal methods
	void EnqueueRequests();
	void WorkerThreadMain();
	void ApplyResults();
	//void ProcessRequests();

	void PushFailureResult(size_t entityId, uint32_t requestId);
	void PushSuccessResult(size_t entityId, uint32_t requestId, const std::vector<Vec2>& path);
	/////////////////////////////////
};
/////////////////////////////////


