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
PathFindingSystem::PathFindingSystem(ChunkManager& cm, EntityManager& em)
	: m_chunks(cm), m_entities(em), m_pathfinder(cm) {}
/////////////////////////////////



/////////////////////////////////
// Update - Updates the PathFindingSystem by processing pathfinding requests and applying results. This method is called every frame with the delta time since the last update.
void PathFindingSystem::Update(float deltaTime) {
	(void)deltaTime;
	ProcessRequests();
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
// ProcessRequests - Processes pathfinding requests for all entities in the EntityManager. It calculates the start and goal tile coordinates based on the entity's transform position and 
// the request's target world position, then performs pathfinding using the Pathfinder instance. The results are pushed onto the results queue for later application.
void PathFindingSystem::ProcessRequests() {
	float ts = m_chunks.GetTileSize();
	int offX = m_chunks.worldOffsetX;
	int offY = m_chunks.worldOffsetY;

	// Iterate through all entities and process pathfinding requests
	for (auto& u : m_entities.GetEntities()) {
		Entity* e = u.get();
		
		// Skip entities that are not alive
		if (!e)
			continue;

		// Skip entities that do not have a CPathRequest component
		auto req = e->GetComponent<CPathRequest>();
		if (!req)
			continue;

		// Get the entity ID for result tracking
		size_t id = e->GetId();

		// Calculate the start tile coordinates based on the entity's transform position
		int startTx = 0, startTy = 0;
		if (auto t = e->GetComponent<CTransform>()) {
			startTx = static_cast<int>(std::floor(t->position.x / ts)) - offX;
			startTy = static_cast<int>(std::floor(t->position.y / ts)) - offY;
		}

		//	Calculate the goal tile coordinates based on the request's target world position
		int goalTx = static_cast<int>(std::floor(req->targetWorld.x / ts)) - offX;
		int goalTy = static_cast<int>(std::floor(req->targetWorld.y / ts)) - offY;

		// Perform pathfinding using the Pathfinder instance
		auto pathOpt = m_pathfinder.FindPath(startTx, startTy, goalTx, goalTy);
		
		// Push the result onto the results queue, indicating success or failure based on whether a path was found
		if (!pathOpt.has_value()) {
			PushFailureResult(id, req->requestId);
		} else {
			PushSuccessResult(id, req->requestId, *pathOpt);
		}
	}
}
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
// ApplyResults - Applies the results of completed pathfinding jobs to the corresponding entities. This method retrieves results from the results queue,
void PathFindingSystem::ApplyResults() {
	// Lock the results mutex to safely access the results queue
	std::lock_guard<std::mutex> lk(m_resultsMutex);
	
	// Process all completed pathfinding results in the queue
	while (!m_doneResults.empty()) {
		PathJobResult res = m_doneResults.front();
		m_doneResults.pop();

		// Find the target entity based on the entity ID in the result and iterate through the entity manager's list of entities to locate it
		Entity* target = nullptr;
		for (auto& u : m_entities.GetEntities()) {
			if (u->GetId() == res.entityId) {
				target = u.get();
				break;
			}
		}

		// If the target entity is not found, skip to the next result
		if (!target)
			continue;
		
		// if the result does not contain a valid path, remove the CPathRequest component from the entity (if it exists) and clear the CPath component (if it exists) 
		if (!res.path.has_value()) {
			if (target->HasComponent<CPathRequest>())
				target->RemoveComponent<CPathRequest>();
			
			// Clear the CPath component if it exists, or add a new CPath component with an empty path and the request ID
			CPath* pathComp = nullptr;

			// If the entity already has a CPath component, retrieve it; otherwise, add a new CPath component to the entity
			if (target->HasComponent<CPath>()) pathComp = target->GetComponent<CPath>();
			else pathComp = target->AddComponent<CPath>();
			
			// Clear the points in the CPath component, set the request ID, and mark it as incomplete
			pathComp->points.clear();
			pathComp->requestId = res.requestId;
			pathComp->complete = false;
			continue;
		}

		// If the result contains a valid path, update the CPath component of the entity with the new path points, request ID, and mark it as complete
		CPath* pathComp = nullptr;
		
		// If the entity already has a CPath component, retrieve it; otherwise, add a new CPath component to the entity
		if (target->HasComponent<CPath>()) pathComp = target->GetComponent<CPath>();
		else pathComp = target->AddComponent<CPath>();

		// Move the path points from the result into the CPath component, set the request ID, and mark it as complete
		pathComp->points = std::move(*res.path);
		pathComp->requestId = res.requestId;
		pathComp->complete = true;

		// Remove the CPathRequest component from the entity, as the pathfinding request has been processed and the result has been applied
		if (target->HasComponent<CPathRequest>())
			target->RemoveComponent<CPathRequest>();
	}
}
/////////////////////////////////
