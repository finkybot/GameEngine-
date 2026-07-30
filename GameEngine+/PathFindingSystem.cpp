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
// Update - Updates the PathFindingSystem, processing pathfinding requests and applying results. This method is called once per frame, 
// allowing the system to handle pathfinding jobs and update entities with the computed paths.
void PathFindingSystem::Update(float deltaTime) {
	(void)deltaTime;
	ProcessRequests();
	ApplyResults();
}
/////////////////////////////////



/////////////////////////////////
// FindPathSync - Performs synchronous pathfinding from the start tile coordinates to the goal tile coordinates. This method is useful for immediate 
// pathfinding needs, such as when an entity requires a path without waiting for a job to complete. It returns an optional vector of Vec2 points 
// representing the computed path, or std::nullopt if no path was found.
std::optional<std::vector<Vec2>> PathFindingSystem::FindPathSync(int startTx, int startTy, int goalTx, int goalTy) {
	return m_pathfinder.FindPath(startTx, startTy, goalTx, goalTy);
}
/////////////////////////////////



/////////////////////////////////
// ProcessRequests - Processes all entities with a CPathRequest component, performing pathfinding for each request. This method retrieves the 
// start and goal tile coordinates from the entity's transform and the request target,
void PathFindingSystem::ProcessRequests() {
	float ts = m_chunks.GetTileSize();
	int offX = m_chunks.GetWorldOffsetX();
	int offY = m_chunks.GetWorldOffsetY();

	for (auto& u : m_entities.GetEntities()) {
		Entity* e = u.get();
		if (!e)
			continue;

		auto req = e->GetComponent<CPathRequest>();
		if (!req)
			continue;

		size_t id = e->GetId();

		// world → tile → mask coords
		int startTx = 0, startTy = 0;
		if (auto t = e->GetComponent<CTransform>()) {
			startTx = static_cast<int>(std::floor(t->m_position.x / ts)) - offX;
			startTy = static_cast<int>(std::floor(t->m_position.y / ts)) - offY;
		}

		int goalTx = static_cast<int>(std::floor(req->targetWorld.x / ts)) - offX;
		int goalTy = static_cast<int>(std::floor(req->targetWorld.y / ts)) - offY;

		auto pathOpt = m_pathfinder.FindPath(startTx, startTy, goalTx, goalTy);
		if (!pathOpt.has_value()) {
			PushFailureResult(id, req->requestId);
		} else {
			PushSuccessResult(id, req->requestId, *pathOpt);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// PushFailureResult - Pushes a failed pathfinding result onto the results queue. This method is called when a pathfinding job fails to find 
// a path, and it stores the entity ID and request ID for later processing in ApplyResults.
void PathFindingSystem::PushFailureResult(size_t entityId, uint32_t requestId) {
	PathJobResult res{entityId, std::nullopt, requestId, true};
	std::lock_guard<std::mutex> lk(m_resultsMutex);
	m_doneResults.push(res);
}
/////////////////////////////////



/////////////////////////////////
// PushSuccessResult - Pushes a successful pathfinding result onto the results queue. This method is called when a pathfinding 
// job completes successfully, and it stores the computed path along with the entity ID and request ID for later processing in ApplyResults.
void PathFindingSystem::PushSuccessResult(size_t entityId, uint32_t requestId, const std::vector<Vec2>& path) {
	PathJobResult res{entityId, path, requestId, true};
	std::lock_guard<std::mutex> lk(m_resultsMutex);
	m_doneResults.push(res);
}
/////////////////////////////////



/////////////////////////////////
// ApplyResults - Apply the results of completed pathfinding jobs to the corresponding entities. This method processes 
// the results queue, updating each entity's CPath component with the computed path or clearing it if no path was found. 
// It also removes the CPathRequest component from entities that have completed their pathfinding request.
void PathFindingSystem::ApplyResults() {
	std::lock_guard<std::mutex> lk(m_resultsMutex);
	while (!m_doneResults.empty()) {
		PathJobResult res = m_doneResults.front();
		m_doneResults.pop();

		Entity* target = nullptr;
		for (auto& u : m_entities.GetEntities()) {
			if (u->GetId() == res.entityId) {
				target = u.get();
				break;
			}
		}
		if (!target)
			continue;

		if (!res.path.has_value()) {
			if (target->HasComponent<CPathRequest>())
				target->RemoveComponent<CPathRequest>();
			CPath* pathComp = nullptr;
			if (target->HasComponent<CPath>())
				pathComp = target->GetComponent<CPath>();
			else
				pathComp = target->AddComponent<CPath>();
			pathComp->points.clear();
			pathComp->requestId = res.requestId;
			pathComp->complete = false;
			continue;
		}

		CPath* pathComp = nullptr;
		if (target->HasComponent<CPath>())
			pathComp = target->GetComponent<CPath>();
		else
			pathComp = target->AddComponent<CPath>();
		pathComp->points = std::move(*res.path);
		pathComp->requestId = res.requestId;
		pathComp->complete = true;

		if (target->HasComponent<CPathRequest>())
			target->RemoveComponent<CPathRequest>();
	}
}
/////////////////////////////////
