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
PathFindingSystem::PathFindingSystem(ChunkManager& cm, EntityManager& em)
	: m_chunks(cm), m_entities(em), m_pathfinder(cm) {}
/////////////////////////////////



/////////////////////////////////
PathFindingSystem::~PathFindingSystem() {
	m_shutdown.store(true);
	for (auto& worker : m_workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// Update - Called every frame on the main thread to process pathfinding requests and step active incremental searches.
// It processes pending requests, steps through active searches with a per-entity budget, and handles success or failure results.
void PathFindingSystem::Update(float deltaTime) {
	(void) deltaTime; // currently unused, but may be used for future time-based logic, i'll void it for now.

	// Process any new pathfinding requests from entities with CPathRequest components, creating ActiveSearch instances for each new request.
	ProcessRequests();

	// 
	std::vector<size_t> finished; 
	for (auto &searchPair : m_activeSearches) {
		size_t entId = searchPair.first;
		auto &search = *searchPair.second;

		// Get a local copy of the nodes per frame budget, its gonna be modified so we need a local copy
		int steps = m_nodesPerFrame;

		// Step through the search while we have steps remaining and the search has not yet found a path. 
		while (steps-- > 0 && !search.found) {
			// If the open list is empty, we push a failure result and break out of the loop.
			if (search.open.empty()) {
				PushFailureResult(entId, search.requestId, finished);
				break;
			}

			// Get the top node from the open list, pop it, and check if it has already been closed. 
			// If it has, we continue to the next iteration.
			auto top = search.open.top(); 
			search.open.pop();
			uint64_t key = top.second;
			if (search.closed[key]) continue;
			
			// If the node has not been closed, we mark it as closed and retrieve the corresponding 
			// ChunkNode from the nodes map.
			search.closed[key] = true;
			auto nodeIt = search.nodes.find(key);
			
			// If the node is not found in the nodes map, we continue to the next iteration. Otherwise, 
			// we retrieve the current ChunkNode and check if it is the goal node.
			if (nodeIt == search.nodes.end()) continue;
			auto cur = nodeIt->second;
			
			// Check if the current node is the goal node. If it is, we reconstruct the chunk path and stitch 
			// local paths synchronously using the Pathfinder.
			if (key == search.goalKey) {
				// Reconstruct the chunk path from the goal node back to the start node using the parent pointers in the nodes map.
				std::vector<std::pair<int,int>> chunkPath;
				uint64_t currentKey = key;
				while (currentKey != -1) { /* -1 indicates the start node */
					auto cNode = search.nodes.find(currentKey);
					if (cNode == search.nodes.end()) break; // should not happen
					chunkPath.emplace_back(cNode->second.x, cNode->second.y);
					currentKey = cNode->second.parent; // move to parent
				}

				// Reverse the chunk path to get it from start to goal, as we reconstructed it backwards.
				std::reverse(chunkPath.begin(), chunkPath.end());

				// Stitch local paths synchronously using Pathfinder
				std::vector<Vec2> finalPath;
				int chunkW = m_chunks.GetChunkWidth();
				int chunkH = m_chunks.GetChunkHeight();
				
				// If the chunk dimensions are invalid (non-positive), we push a failure result 
				// and break out of the loop.
				if (chunkW <= 0 || chunkH <= 0) {
					PushFailureResult(entId, search.requestId, finished);
					break;
				}

				// Get initial tile position (cached entity pointer)
				Entity* entPtr = FindEntityById(entId);
				int curTx = 0, curTy = 0;
				GetEntityTilePos(entPtr, curTx, curTy);

				for (size_t i = 0; i < chunkPath.size(); ++i) {
					int cx = chunkPath[i].first;
					int cy = chunkPath[i].second;
					int localStartX = curTx - cx * chunkW;
					int localStartY = curTy - cy * chunkH;

					if (i + 1 == chunkPath.size()) {
						// final chunk: goal tile from ActiveSearch request target
						int goalWorldX = search.gcx * chunkW; // placeholder, will compute below
						int goalWorldY = search.gcy * chunkH;
						// compute actual goal tile coords from request later; for now use requestId to find entity
						// locate entity and its CPathRequest target
						Vec2 goalWorld = GetEntityGoalWorld(entPtr);

						float ts = m_chunks.GetTileSize();
						int goalTx = static_cast<int>(std::floor(goalWorld.x / ts));
						int goalTy = static_cast<int>(std::floor(goalWorld.y / ts));
						int localGoalX = goalTx - cx * chunkW;
						int localGoalY = goalTy - cy * chunkH;

						// fetch chunk
						Chunk chunkCopy;
						if (!GetChunkCopy(cx, cy, chunkCopy)) {
							PushFailureResult(entId, search.requestId, finished);
							break;
						}
						auto seg = m_pathfinder.LocalAStar(chunkCopy, localStartX, localStartY, localGoalX, localGoalY);
						if (!seg.has_value()) {
							PushFailureResult(entId, search.requestId, finished);
							break;
						}
						if (!finalPath.empty() && !seg->empty()) seg->erase(seg->begin());
						finalPath.insert(finalPath.end(), seg->begin(), seg->end());
					// finished
					PushSuccessResult(entId, search.requestId, finalPath, finished);
					break;
					} else {
						int nx = chunkPath[i+1].first;
						int ny = chunkPath[i+1].second;
						int dx = nx - cx; int dy = ny - cy;

						// find best portal
						float bestDist = std::numeric_limits<float>::infinity();
						int bestAx=-1,bestAy=-1,bestBx=-1,bestBy=-1;
						Vec2 pref = Vec2(curTx * m_chunks.GetTileSize(), curTy * m_chunks.GetTileSize());

						// Check the shared edge of the two chunks to find the best portal (walkable tile) between them. 
						// We iterate over the tiles along the shared edge,
						if (dx != 0) {
							int edgeAx = cx * chunkW + (dx > 0 ? (chunkW - 1) : 0);
							int edgeBx = nx * chunkW + (dx > 0 ? 0 : (chunkW - 1));
							for (int irow = 0; irow < chunkH; ++irow) {
								int ay = cy * chunkH + irow; int by = ay;
								int va = m_chunks.GetTileAt(edgeAx, ay);
								int vb = m_chunks.GetTileAt(edgeBx, by);

								// If both tiles are walkable (value 0), we calculate the distance from the preferred position 
								// to the portal and update the best portal if this one is closer.
								if (va == 0 && vb == 0) {
									Vec2 ca = Vec2(edgeAx * m_chunks.GetTileSize(), ay * m_chunks.GetTileSize());
									float dxp = ca.x - pref.x; float dyp = ca.y - pref.y; float dist = dxp*dxp + dyp*dyp;

									// If the distance is less than the best distance found so far, we update the best portal 
									// coordinates and distance.
									if (dist < bestDist) { bestDist = dist; bestAx=edgeAx; bestAy=ay; bestBx=edgeBx; bestBy=by; }
								}
							}
						} else {
							// Else the chunks are adjacent vertically, we check the shared edge along the y-axis. We iterate over 
							// the tiles along the shared edge,
							int edgeAy = cy * chunkH + (dy > 0 ? (chunkH - 1) : 0);
							int edgeBy = ny * chunkH + (dy > 0 ? 0 : (chunkH - 1));
							for (int icol = 0; icol < chunkW; ++icol) {
								int ax = cx * chunkW + icol; int bx = nx * chunkW + icol;
								int va = m_chunks.GetTileAt(ax, edgeAy);
								int vb = m_chunks.GetTileAt(bx, edgeBy);

								// If both tiles are walkable (value 0), we calculate the distance from the preferred position
								if (va == 0 && vb == 0) {
									Vec2 ca = Vec2(ax * m_chunks.GetTileSize(), edgeAy * m_chunks.GetTileSize());
									float dxp = ca.x - pref.x; float dyp = ca.y - pref.y; float dist = dxp*dxp + dyp*dyp;
									if (dist < bestDist) { bestDist = dist; bestAx=ax; bestAy=edgeAy; bestBx=bx; bestBy=edgeBy; }
								}
							}
						}

						// If no valid portal is found between the current chunk and the next chunk, we push a failure result and 
						// break out of the loop.
						if (bestAx == -1) { 
							PushFailureResult(entId, search.requestId, finished);
							break;
						}

						int exitLocalX = bestAx - cx * chunkW;
						int exitLocalY = bestAy - cy * chunkH;
						
						// Fetch the chunk copy for the current chunk and perform a local A* search from the current local 
						// position to the exit position.
						Chunk chunkCopy;
						if (!GetChunkCopy(cx, cy, chunkCopy)) {
							PushFailureResult(entId, search.requestId, finished);
							break;
						}
						auto seg = m_pathfinder.LocalAStar(chunkCopy, localStartX, localStartY, exitLocalX, exitLocalY);

						// If the local path segment is not found, we push a failure result and break out of the loop.
						if (!seg.has_value()) { 
							PushFailureResult(entId, search.requestId, finished);
							break;
						}

						// Stitch the local path segment into the final path, ensuring that we do not duplicate the last tile 
						// of the previous segment.
						if (!finalPath.empty() && !seg->empty()) seg->erase(seg->begin());
						finalPath.insert(finalPath.end(), seg->begin(), seg->end());
						curTx = bestBx; curTy = bestBy;
						// continue to next chunk
					}
				}
				break;
			}

			// If the current node is not the goal, we explore its neighbors in 8 directions (including diagonals).			
			for (auto &d : m_dirs8) {
				int nx = cur.x + d[0]; int ny = cur.y + d[1];
				// check adjacency via chunk border test including diagonals
				auto chunkHasConnection = [&](int cx, int cy, int nx2, int ny2) {
					int dx = nx2 - cx; int dy = ny2 - cy;

					// If the neighbor chunk is more than one chunk away in either direction, we return false, indicating 
					// that it is not connected.
					if (std::abs(dx) > 1 || std::abs(dy) > 1) return false;

					// If the neighbor chunk is adjacent (orthogonal), we check the shared edge of the two chunks to ensure 
					// that they are walkable (not blocked).
					int cw2 = m_chunks.GetChunkWidth(); int ch2 = m_chunks.GetChunkHeight();

					// If the neighbor chunk is adjacent (orthogonal), we check the shared edge of the two chunks to ensure 
					// that they are walkable (not blocked).
					if (std::abs(dx) + std::abs(dy) == 1) {

						// Check the tiles along the shared edge of the two chunks to ensure that they are walkable (not blocked).
						if (dx != 0) {
							int edgeX_A = cx * cw2 + (dx > 0 ? (cw2 - 1) : 0);
							int edgeX_B = nx2 * cw2 + (dx > 0 ? 0 : (cw2 - 1));
							for (int irow = 0; irow < ch2; ++irow) {
								int ay = cy * ch2 + irow;
								int va = m_chunks.GetTileAt(edgeX_A, ay);
								int vb = m_chunks.GetTileAt(edgeX_B, ay);

								// If both tiles are walkable (value 0), we return true, indicating that the neighbor chunk is 
								// connected to the current chunk.
								if (va == 0 && vb == 0) return true;
							}
						} else {
							// Else the neighbor chunk is adjacent vertically, we check the shared edge along the y-axis. We iterate over
							int edgeY_A = cy * ch2 + (dy > 0 ? (ch2 - 1) : 0);
							int edgeY_B = ny2 * ch2 + (dy > 0 ? 0 : (ch2 - 1));

							// Check the tiles along the shared edge of the two chunks to ensure that they are walkable (not blocked).
							for (int i = 0; i < cw2; ++i) {
								int ax = cx * cw2 + i;
								int va = m_chunks.GetTileAt(ax, edgeY_A);
								int vb = m_chunks.GetTileAt(cx * cw2 + i, edgeY_B);

								// As above, if both tiles are walkable (value 0), we return true, indicating that the neighbor chunk is 
								// connected to the current chunk.
								if (va == 0 && vb == 0) return true;
							}
						}
						return false;
					}

					// diagonal neighbor: check corner-adjacent tile pair and prevent corner-cut
					if (std::abs(dx) == 1 && std::abs(dy) == 1) {
						int ax = cx * cw2 + (dx > 0 ? (cw2 - 1) : 0);
						int ay = cy * ch2 + (dy > 0 ? (ch2 - 1) : 0);
						int bx = nx2 * cw2 + (dx > 0 ? 0 : (cw2 - 1));
						int by = ny2 * ch2 + (dy > 0 ? 0 : (ch2 - 1));

						// Check the tiles at the corners of the two chunks to ensure that they are walkable (not blocked).
						int va = m_chunks.GetTileAt(ax, ay);
						int vb = m_chunks.GetTileAt(bx, by);

						// If either of the corner tiles is blocked (non-zero), we return false, indicating that the diagonal 
						// neighbor is not connected to the current chunk.
						if (va != 0 || vb != 0) return false;

						// Check the orthogonal tiles adjacent to the diagonal neighbor to prevent corner-cutting.
						int orth1x = ax + dx; int orth1y = ay;
						int orth2x = ax; int orth2y = ay + dy;
						int o1 = m_chunks.GetTileAt(orth1x, orth1y);
						int o2 = m_chunks.GetTileAt(orth2x, orth2y);

						// If either of the orthogonal tiles is walkable (zero), we return true, indicating that the diagonal neighbor
						if (o1 == 0 || o2 == 0) return true;
						return false;
					}
					return false;
				};

				// Check if the neighbor chunk is connected to the current chunk. If not, we skip this neighbor.
				if (!chunkHasConnection(cur.x, cur.y, nx, ny)) continue;
				uint64_t nk = (static_cast<uint64_t>(nx) << 32) | static_cast<unsigned int>(ny);
				auto nit = search.nodes.find(nk);
				float moveCost = (std::abs(d[0]) + std::abs(d[1]) == 2) ? std::sqrt(2.0f) : 1.0f;
				float ng = cur.g + moveCost;

				// If the neighbor node is not in the nodes map or if the new g-cost is lower than the existing g-cost, 
				// we update the neighbor node and push it onto the open list.
				if (nit == search.nodes.end() || ng < nit->second.g) {
					search.nodes[nk] = {nx, ny, ng, key};
					float h = m_pathfinder.HeuristicChunk(nx, ny, search.gcx, search.gcy);
					search.open.push({ng + h, nk});
					// produce partial chunk-path preview: reconstruct current best path to this neighbor
					std::vector<std::pair<int,int>> previewChunks;
					uint64_t curK = nk;

					// Reconstruct the current best path to this neighbor by following parent pointers in the nodes map.
					while (curK != -1) {
						auto itn = search.nodes.find(curK);
						if (itn == search.nodes.end()) break;
						previewChunks.emplace_back(itn->second.x, itn->second.y);
						curK = itn->second.parent;
					}

					// If the previewChunks vector is not empty, we reverse it to get the path from start to the current neighbor.
					if (!previewChunks.empty()) {
						reverse(previewChunks.begin(), previewChunks.end());
						// translate chunk preview into a simple world polyline by using chunk centers
						std::vector<Vec2> previewPath;
						float ts = m_chunks.GetTileSize();
						float cw = (float)m_chunks.GetChunkWidth();
						float ch = (float)m_chunks.GetChunkHeight();

						// For each chunk in the previewChunks vector, we calculate its world position based on its chunk coordinates and the chunk dimensions,
						for (auto &cc : previewChunks) {
							float wx = (cc.first * cw + cw * 0.5f) * ts;
							float wy = (cc.second * ch + ch * 0.5f) * ts;
							previewPath.emplace_back(wx, wy);
						}

						// Push the preview path result onto the results queue, indicating that the pathfinding is not yet complete.
						PathJobResult pres; pres.entityId = entId; pres.requestId = search.requestId; pres.path = previewPath; pres.complete = false;
						{
							std::lock_guard<std::mutex> lk2(m_resultsMutex);
							m_doneResults.push(pres);
						}
					}
				}
			}
		}
	}

	// cleanup finished searches
	for (size_t id : finished) m_activeSearches.erase(id);

	// Finally, we apply the results of the pathfinding operations by processing the results queue and updating the corresponding 
	// entities with the computed paths or failure notifications.
	ApplyResults();
}
/////////////////////////////////



/////////////////////////////////
// FindPathSync - Performs a synchronous pathfinding operation from the start tile coordinates to the goal tile coordinates using the 
// Pathfinder instance.
std::optional<std::vector<Vec2>> PathFindingSystem::FindPathSync(int startTx, int startTy, int goalTx, int goalTy) {
	return m_pathfinder.FindPath(startTx, startTy, goalTx, goalTy);
}
/////////////////////////////////



/////////////////////////////////
void PathFindingSystem::WorkerThreadLoop() {}
/////////////////////////////////



/////////////////////////////////
void PathFindingSystem::ProcessRequests() {
	// Scan entities for CPathRequest components and start incremental searches for new requests
	for (auto &u : m_entities.GetEntities()) {
		Entity* e = u.get();
		if (!e) continue;
		auto req = e->GetComponent<CPathRequest>();
		if (!req) continue;
		size_t id = e->GetId();
		if (m_activeSearches.find(id) != m_activeSearches.end()) continue; // already processing

		// create ActiveSearch
		float ts = m_chunks.GetTileSize();
		int startTx = 0, startTy = 0;
		if (auto t = e->GetComponent<CTransform>()) {
			startTx = static_cast<int>(std::floor(t->m_position.x / ts));
			startTy = static_cast<int>(std::floor(t->m_position.y / ts));
		}
		int goalTx = static_cast<int>(std::floor(req->targetWorld.x / ts));
		int goalTy = static_cast<int>(std::floor(req->targetWorld.y / ts));

		int cw = m_chunks.GetChunkWidth();
		int ch = m_chunks.GetChunkHeight();
		auto floorDiv = [](int a, int b) {
			if (a >= 0) return a / b;
			return -(((-a) + b - 1) / b);
		};
		int scx = floorDiv(startTx, cw);
		int scy = floorDiv(startTy, ch);
		int gcx = floorDiv(goalTx, cw);
		int gcy = floorDiv(goalTy, ch);

		auto as = std::make_unique<ActiveSearch>();
		as->entityId = id;
		as->requestId = req->requestId;
		as->scx = scx; as->scy = scy; as->gcx = gcx; as->gcy = gcy;
		auto keyFor = [](int x, int y) { return (static_cast<uint64_t>(x) << 32) | static_cast<unsigned int>(y); };
		uint64_t k = keyFor(scx, scy);
		as->goalKey = keyFor(gcx, gcy);
		as->nodes[k] = {scx, scy, 0.0f, static_cast<uint64_t>(-1)};
		float h = m_pathfinder.HeuristicChunk(scx, scy, gcx, gcy);
		as->open.push({h, k});
		m_activeSearches[id] = std::move(as);
	}
}
/////////////////////////////////



/////////////////////////////////
// Helper method implementations
/////////////////////////////////



/////////////////////////////////
// PushFailureResult - Pushes a failure result for the specified entity and request ID onto the results queue, 
// indicating that no path could be found.
void PathFindingSystem::PushFailureResult(size_t entityId, uint32_t requestId, std::vector<size_t>& finished) {
	PathJobResult res{ entityId, std::nullopt, requestId };
	{
		std::lock_guard<std::mutex> lk(m_resultsMutex);
		m_doneResults.push(res);
	}
	finished.push_back(entityId);
}
/////////////////////////////////



/////////////////////////////////
// PushSuccessResult - Pushes a success result for the specified entity and request ID onto the results queue, along with the computed path, 
// indicating that a path has been successfully found.
void PathFindingSystem::PushSuccessResult(size_t entityId, uint32_t requestId, const std::vector<Vec2>& path, std::vector<size_t>& finished) {
	PathJobResult res{ entityId, path, requestId, true };
	{
		std::lock_guard<std::mutex> lk(m_resultsMutex);
		m_doneResults.push(res);
	}
	finished.push_back(entityId);
}
/////////////////////////////////



/////////////////////////////////
// FindEntityById - Finds an entity by its ID in the EntityManager and returns a pointer to the entity if found, or nullptr if not found.
Entity* PathFindingSystem::FindEntityById(size_t entId) {
	for (auto& entityUnPtr : m_entities.GetEntities()) {
		if (entityUnPtr->GetId() == entId) {
			return entityUnPtr.get();
		}
	}
	return nullptr;
}
/////////////////////////////////



/////////////////////////////////
// GetChunkCopy - Retrieves a copy of the chunk at the specified chunk coordinates (chunkX, chunkY) and stores it in outChunk. Returns true if 
// the chunk was found and copied, or false if the chunk does not exist.
bool PathFindingSystem::GetChunkCopy(int chunkX, int chunkY, Chunk& outChunk) {
	std::lock_guard<std::mutex> lk(m_chunks.GetMutex());
	auto& chunks = m_chunks.GetChunks();
	uint64_t keyk = (static_cast<uint64_t>(chunkX) << 32) | static_cast<unsigned int>(chunkY);
	auto itc = chunks.find(keyk);
	if (itc == chunks.end()) return false;
	outChunk = itc->second;
	return true;
}
/////////////////////////////////



/////////////////////////////////
// GetEntityTilePos - Retrieves the tile position (tileX, tileY) of the specified entity based on its world position and the tile size.
void PathFindingSystem::GetEntityTilePos(Entity* entity, int& outTileX, int& outTileY) {
	outTileX = outTileY = 0;
	if (!entity) return;
	if (auto t = entity->GetComponent<CTransform>()) {
		float ts = m_chunks.GetTileSize();
		outTileX = static_cast<int>(std::floor(t->m_position.x / ts));
		outTileY = static_cast<int>(std::floor(t->m_position.y / ts));
	}
}
/////////////////////////////////



/////////////////////////////////
// GetEntityGoalWorld - Retrieves the goal world position of the specified entity based on its CPathRequest component. Returns a Vec2 
// epresenting the goal world position, or Vec2::Zero if the entity or request is not found.
Vec2 PathFindingSystem::GetEntityGoalWorld(Entity* entity) {
	Vec2 goalWorld = Vec2::Zero;
	if (!entity) return goalWorld;
	if (auto req = entity->GetComponent<CPathRequest>()) {
		goalWorld = req->targetWorld;
	}
	return goalWorld;
}
/////////////////////////////////



/////////////////////////////////
// ApplyResults - Pops results from the results queue and attaches CPath components to entities on the main thread.
void PathFindingSystem::ApplyResults() {
	// Pop results and attach CPath components to entities on main thread.
	std::lock_guard<std::mutex> lk(m_resultsMutex);
	while (!m_doneResults.empty()) {
		PathJobResult res = m_doneResults.front(); m_doneResults.pop();
		// find entity by id
		Entity* target = nullptr;
		for (auto &u : m_entities.GetEntities()) {
			if (u->GetId() == res.entityId) { target = u.get(); break; }
		}
		if (!target) continue; // entity gone

		// If this is a terminal failure (no path), attach empty CPath and remove request
		if (!res.path.has_value()) {
			if (target->HasComponent<CPathRequest>()) target->RemoveComponent<CPathRequest>();
			CPath* pathComp = nullptr;
			if (target->HasComponent<CPath>()) pathComp = target->GetComponent<CPath>();
			else pathComp = target->AddComponent<CPath>();
			pathComp->points.clear();
			pathComp->requestId = res.requestId;
			pathComp->complete = false;
			continue;
		}

		// res.path has a value: either partial preview (complete==false) or final full path (complete==true)
		CPath* pathComp = nullptr;
		if (target->HasComponent<CPath>()) pathComp = target->GetComponent<CPath>();
		else pathComp = target->AddComponent<CPath>();
		pathComp->points = std::move(*res.path);
		pathComp->requestId = res.requestId;
		pathComp->complete = res.complete;
		if (res.complete) {
			// final result: remove the original request so system stops processing
			if (target->HasComponent<CPathRequest>()) target->RemoveComponent<CPathRequest>();
		}
	}
}
/////////////////////////////////
