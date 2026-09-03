/////////////////////////////////
// PathFindingSystem.cpp - Implementation of the PathFindingSystem class, responsible for managing pathfinding requests and results using worker threads and a Pathfinder instance.
/////////////////////////////////



/////////////////////////////////
#include "PathFindingSystem.h"
#include "Entity.h"
#include <cmath>
#include <algorithm>
/////////////////////////////////



/////////////////////////////////
// Constructor - Initializes the PathFindingSystem with references to the ChunkManager and EntityManager, and creates a Pathfinder 
// instance for pathfinding operations.
PathFindingSystem::PathFindingSystem(ChunkManager& cm, EntityManager& em): m_chunks(cm), m_entities(em), m_pathfinder(cm) {
	const unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
	for (unsigned int i = 0; i < threadCount; ++i) {
		m_workers.emplace_back(&PathFindingSystem::WorkerThreadMain, this);
	}
}
/////////////////////////////////



/////////////////////////////////
// Destructor - signals shutdown and joins worker threads
PathFindingSystem::~PathFindingSystem() {
	{
		std::lock_guard<std::mutex> lk(m_jobsMutex);
		m_shutdown = true;
	}
	m_jobsCv.notify_all();

	for (auto& t : m_workers) {
		if (t.joinable())
			t.join();
	}
}
/////////////////////////////////



/////////////////////////////////
// Update - Updates the PathFindingSystem by processing pathfinding requests and applying results. This method is called every frame with the delta time since the last update.
void PathFindingSystem::Update(float deltaTime) {
	(void)deltaTime;
	//ProcessRequests();
	EnqueueRequests();
	ApplyResults();
}
/////////////////////////////////



/////////////////////////////////
// FindPathSync - Finds a path synchronously between the specified start and goal tile coordinates using the Pathfinder instance.
std::optional<std::vector<Vec2>> PathFindingSystem::FindPathSync(int startTx, int startTy, int goalTx, int goalTy) {
	return m_pathfinder.FindPath(startTx, startTy, goalTx, goalTy);
}
/////////////////////////////////



/////////////////////////////////
// EnqueueRequests - scan entities and push path jobs to worker queue
void PathFindingSystem::EnqueueRequests() {
	// Get tile size and world offsets for converting world coordinates to tile coordinates
	float ts = m_chunks.GetTileSize();
	int offX = m_chunks.worldOffsetX;
	int offY = m_chunks.worldOffsetY;

	// Iterate through all entities and enqueue pathfinding requests for those that have a CPathRequest component
	for (auto& u : m_entities.GetEntities()) {
		// Get the entity pointer from the unique_ptr
		Entity* e = u.get();

		// Skip entities that are not alive
		if (!e || !e->IsAlive()) continue;

		// Get the CPathRequest component from the entity
		auto req = e->GetComponent<CPathRequest>();

		// Skip entities that do not have a CPathRequest component
		if (!req) continue;

		// Calculate the start tile coordinates based on the entity's transform position
		int startTx = 0, startTy = 0;
		if (auto t = e->GetComponent<CTransform>()) {
			startTx = static_cast<int>(std::floor(t->position.x / ts)) - offX;
			startTy = static_cast<int>(std::floor(t->position.y / ts)) - offY;
		}
		
		// Calculate the goal tile coordinates based on the request's target world position
		int goalTx = static_cast<int>(std::floor(req->targetWorld.x / ts)) - offX;
		int goalTy = static_cast<int>(std::floor(req->targetWorld.y / ts)) - offY;

		// Create a PathJob struct to hold the pathfinding job information
		PathJob job;
		job.entityId = e->GetId();
		job.startTx = startTx;
		job.startTy = startTy;
		job.goalTx = goalTx;
		job.goalTy = goalTy;
		job.requestId = req->requestId;
		job.allowPartial = req->allowPartial;

		// Lock the jobs mutex and push the job onto the pending jobs queue
		{
			std::lock_guard<std::mutex> lk(m_jobsMutex);
			m_pendingJobs.push(std::move(job));
		}

		// Notify one worker thread that a new job is available in the queue
		m_jobsCv.notify_one();

		// Request is now owned by the system; remove component
		e->RemoveComponent<CPathRequest>();
	}
}
/////////////////////////////////



/////////////////////////////////
// WorkerThreadMain - worker loop that consumes jobs and computes paths
void PathFindingSystem::WorkerThreadMain() {
	// Infinite loop to continuously process pathfinding jobs until shutdown is signaled
	for (;;) {
		// Wait for a job to be available in the pending jobs queue or for shutdown to be signaled
		PathJob job;

		// Lock the jobs mutex and wait for a job to be available or for shutdown to be signaled
		{
			// Use a unique_lock to allow for waiting on the condition variable
			std::unique_lock<std::mutex> lk(m_jobsMutex);

			// Wait until there is a job available in the queue or until shutdown is signaled
			m_jobsCv.wait(lk, [this] { return m_shutdown.load() || !m_pendingJobs.empty(); });

			// If shutdown is signaled and there are no pending jobs, exit the worker thread
			if (m_shutdown.load() && m_pendingJobs.empty())	return;

			// Move the job from the front of the pending jobs queue to the local job variable and pop it from the queue
			job = std::move(m_pendingJobs.front());

			// Remove the job from the pending jobs queue
			m_pendingJobs.pop();
		}

		// Perform pathfinding using the Pathfinder instance with the start and goal tile coordinates from the job
		auto pathOpt = m_pathfinder.FindPath(job.startTx, job.startTy, job.goalTx, job.goalTy);

		// Push the result onto the results queue, indicating success or failure based on whether a path was found
		if (!pathOpt.has_value()) {
			// If no path was found, push a failure result onto the results queue
			PushFailureResult(job.entityId, job.requestId);
		} else {
			// If a path was found, push a success result onto the results queue
			PushSuccessResult(job.entityId, job.requestId, *pathOpt);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// ProcessRequests - Processes pathfinding requests for all entities in the EntityManager. It calculates the start and goal tile coordinates based on the entity's transform position and 
// the request's target world position, then performs pathfinding using the Pathfinder instance. The results are pushed onto the results queue for later application.
//void PathFindingSystem::ProcessRequests() {
//	float ts = m_chunks.GetTileSize();
//	int offX = m_chunks.worldOffsetX;
//	int offY = m_chunks.worldOffsetY;
//
//	// Iterate through all entities and process pathfinding requests
//	for (auto& u : m_entities.GetEntities()) {
//		Entity* e = u.get();
//		
//		// Skip entities that are not alive
//		if (!e)
//			continue;
//
//		// Skip entities that do not have a CPathRequest component
//		auto req = e->GetComponent<CPathRequest>();
//		if (!req)
//			continue;
//
//		// Get the entity ID for result tracking
//		size_t id = e->GetId();
//
//		// Calculate the start tile coordinates based on the entity's transform position
//		int startTx = 0, startTy = 0;
//		if (auto t = e->GetComponent<CTransform>()) {
//			startTx = static_cast<int>(std::floor(t->position.x / ts)) - offX;
//			startTy = static_cast<int>(std::floor(t->position.y / ts)) - offY;
//		}
//
//		//	Calculate the goal tile coordinates based on the request's target world position
//		int goalTx = static_cast<int>(std::floor(req->targetWorld.x / ts)) - offX;
//		int goalTy = static_cast<int>(std::floor(req->targetWorld.y / ts)) - offY;
//
//		// Perform pathfinding using the Pathfinder instance
//		auto pathOpt = m_pathfinder.FindPath(startTx, startTy, goalTx, goalTy);
//		
//		// Push the result onto the results queue, indicating success or failure based on whether a path was found
//		if (!pathOpt.has_value()) {
//			PushFailureResult(id, req->requestId);
//		} else {
//			PushSuccessResult(id, req->requestId, *pathOpt);
//		}
//	}
//}
/////////////////////////////////



/////////////////////////////////
// PushFailureResult - Pushes a failed pathfinding result onto the results queue. This method is called when a pathfinding job fails to find a valid path,
void PathFindingSystem::PushFailureResult(size_t entityId, uint32_t requestId) {
	// Create a PathJobResult with no path and mark it as complete (failure)
	PathJobResult res{entityId, std::nullopt, requestId, true};

	// Lock the results mutex to safely access the results queue and push the failure result
	std::lock_guard<std::mutex> lk(m_resultsMutex);
	m_doneResults.push(res);
}
/////////////////////////////////



/////////////////////////////////
// PushSuccessResult - Pushes a successful pathfinding result onto the results queue. This method is called when a pathfinding job successfully computes a valid path,
void PathFindingSystem::PushSuccessResult(size_t entityId, uint32_t requestId, const std::vector<Vec2>& path) {
	// Create a PathJobResult with the provided path and mark it as complete (success)
	PathJobResult res{entityId, path, requestId, true};

	// Lock the results mutex to safely access the results queue and push the success result
	std::lock_guard<std::mutex> lk(m_resultsMutex);
	m_doneResults.push(res);
}
/////////////////////////////////



/////////////////////////////////
// ApplyResults - apply finished paths back onto entities (main thread only)
void PathFindingSystem::ApplyResults() {
	// Create a local queue to hold the results to be applied
	std::queue<PathJobResult> localQueue;

	// Lock the results mutex to safely access the results queue and swap it with the local queue
	{
		std::lock_guard<std::mutex> lk(m_resultsMutex);
		std::swap(localQueue, m_doneResults);
	}

	// Process each result in the local queue and apply it to the corresponding entity
	while (!localQueue.empty()) {
		// Move the result from the front of the local queue to a local variable and pop it from the queue
		PathJobResult res = std::move(localQueue.front());

		// Remove the result from the local queue
		localQueue.pop();

		// Find the target entity based on the entity ID in the result
		Entity* target = nullptr;

		// Lock the EntityManager's entity list and search for the entity with the matching ID
		for (auto& u : m_entities.GetEntities()) {

			// Check if the entity's ID matches the result's entity ID
			if (u->GetId() == res.entityId) {
				target = u.get();
				break;
			}
		}

		// Skip if the target entity is not found or is not alive
		if (!target || !target->IsAlive()) continue;

		// Get or create the CPath component for the target entity
		CPath* pathComp = nullptr;

		// If the target entity already has a CPath component, retrieve it; otherwise, add a new CPath component to the entity
		if (target->HasComponent<CPath>())
			pathComp = target->GetComponent<CPath>();
		else 
			// If the target entity does not have a CPath component, add a new CPath component to the entity
			pathComp = target->AddComponent<CPath>();

		// Update the CPath component with the result's request ID and path points
		pathComp->requestId = res.requestId;

		// If the result does not contain a valid path, clear the points and mark the path as incomplete; otherwise, move the path points into the CPath component and mark it as complete
		if (!res.path.has_value()) {
			pathComp->points.clear();
			pathComp->complete = false;
		} else {
			// Move the path points from the result into the CPath component and mark it as complete
			pathComp->points = std::move(*res.path);
			pathComp->complete = true;
		}
	}
}
/////////////////////////////////
