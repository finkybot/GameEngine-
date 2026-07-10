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
void PathFindingSystem::Update(float deltaTime) {
	(void)deltaTime;
	ProcessRequests();
	// Step active incremental searches with a per-entity budget
	std::vector<size_t> finished; // collect finished entity ids to cleanup after iteration
	for (auto &pr : m_activeSearches) {
		size_t entId = pr.first;
		auto &search = *pr.second;

		int steps = m_nodesPerFrame;
		while (steps-- > 0 && !search.found) {
			if (search.open.empty()) {
				// failed
				PathJobResult res; res.entityId = entId; res.path = std::nullopt; res.requestId = search.requestId;
				{
					std::lock_guard<std::mutex> lk(m_resultsMutex);
					m_doneResults.push(res);
				}
				finished.push_back(entId);
				break;
			}
			auto top = search.open.top(); search.open.pop();
			long long key = top.second;
			if (search.closed[key]) continue;
			search.closed[key] = true;
			auto it = search.nodes.find(key);
			if (it == search.nodes.end()) continue;
			auto cur = it->second;
			if (key == search.goalKey) {
				// reconstruct chunk path
				std::vector<std::pair<int,int>> chunkPath;
				long long curK = key;
				while (curK != -1) {
					auto nit = search.nodes.find(curK);
					if (nit == search.nodes.end()) break;
					chunkPath.emplace_back(nit->second.x, nit->second.y);
					curK = nit->second.parent;
				}
				std::reverse(chunkPath.begin(), chunkPath.end());

					// stitch local paths synchronously using Pathfinder (complete)
					std::vector<Vec2> finalPath;
				int cw = m_chunks.GetChunkWidth();
				int ch = m_chunks.GetChunkHeight();
				if (cw <= 0 || ch <= 0) {
					PathJobResult res; res.entityId = entId; res.path = std::nullopt; res.requestId = search.requestId;
					{
						std::lock_guard<std::mutex> lk(m_resultsMutex);
						m_doneResults.push(res);
					}
					finished.push_back(entId);
					break;
				}

				// current world tile start from entity's transform
				Entity* entPtr = nullptr;
				for (auto &u : m_entities.GetEntities()) { if (u->GetId() == entId) { entPtr = u.get(); break; } }
				int curTx = 0, curTy = 0;
				if (entPtr) {
					if (auto t = entPtr->GetComponent<CTransform>()) {
						float ts = m_chunks.GetTileSize();
						curTx = static_cast<int>(std::floor(t->m_position.x / ts));
						curTy = static_cast<int>(std::floor(t->m_position.y / ts));
					}
				}

				for (size_t i = 0; i < chunkPath.size(); ++i) {
					int cx = chunkPath[i].first;
					int cy = chunkPath[i].second;
					int localStartX = curTx - cx * cw;
					int localStartY = curTy - cy * ch;

					if (i + 1 == chunkPath.size()) {
						// final chunk: goal tile from ActiveSearch request target
						int goalWorldX = search.gcx * cw; // placeholder, will compute below
						int goalWorldY = search.gcy * ch;
						// compute actual goal tile coords from request later; for now use requestId to find entity
						// locate entity and its CPathRequest target
						Vec2 goalWorld = Vec2::Zero;
						if (entPtr) {
							if (auto req = entPtr->GetComponent<CPathRequest>()) goalWorld = req->targetWorld;
							if (auto t = entPtr->GetComponent<CTransform>()) {
								// nothing
							}
						}
						float ts = m_chunks.GetTileSize();
						int goalTx = static_cast<int>(std::floor(goalWorld.x / ts));
						int goalTy = static_cast<int>(std::floor(goalWorld.y / ts));
						int localGoalX = goalTx - cx * cw;
						int localGoalY = goalTy - cy * ch;

						// fetch chunk
						Chunk chunkCopy;
						{
							std::lock_guard<std::mutex> lk(m_chunks.GetMutex());
							auto &chunks = m_chunks.GetChunks();
							long long keyk = (static_cast<long long>(cx) << 32) | static_cast<unsigned int>(cy);
							auto itc = chunks.find(keyk);
							if (itc == chunks.end()) {
								PathJobResult res; res.entityId = entId; res.path = std::nullopt; res.requestId = search.requestId;
								{
									std::lock_guard<std::mutex> lk2(m_resultsMutex);
									m_doneResults.push(res);
								}
								finished.push_back(entId);
								break;
							}
							chunkCopy = itc->second;
						}
						auto seg = m_pathfinder.LocalAStar(chunkCopy, localStartX, localStartY, localGoalX, localGoalY);
						if (!seg.has_value()) {
							PathJobResult res; res.entityId = entId; res.path = std::nullopt; res.requestId = search.requestId;
							{
								std::lock_guard<std::mutex> lk2(m_resultsMutex);
								m_doneResults.push(res);
							}
							finished.push_back(entId);
							break;
						}
						if (!finalPath.empty() && !seg->empty()) seg->erase(seg->begin());
						finalPath.insert(finalPath.end(), seg->begin(), seg->end());
					// finished
					PathJobResult res;
					res.entityId = entId;
					res.path = finalPath;
					res.requestId = search.requestId;
					res.complete = true;
						{
							std::lock_guard<std::mutex> lk2(m_resultsMutex);
							m_doneResults.push(res);
						}
						finished.push_back(entId);
						break;
					} else {
						int nx = chunkPath[i+1].first;
						int ny = chunkPath[i+1].second;
						int dx = nx - cx; int dy = ny - cy;
						// find best portal
						float bestDist = std::numeric_limits<float>::infinity();
						int bestAx=-1,bestAy=-1,bestBx=-1,bestBy=-1;
						Vec2 pref = Vec2(curTx * m_chunks.GetTileSize(), curTy * m_chunks.GetTileSize());
						if (dx != 0) {
							int edgeAx = cx * cw + (dx > 0 ? (cw - 1) : 0);
							int edgeBx = nx * cw + (dx > 0 ? 0 : (cw - 1));
							for (int irow = 0; irow < ch; ++irow) {
								int ay = cy * ch + irow; int by = ay;
								int va = m_chunks.GetTileAt(edgeAx, ay);
								int vb = m_chunks.GetTileAt(edgeBx, by);
								if (va == 0 && vb == 0) {
									Vec2 ca = Vec2(edgeAx * m_chunks.GetTileSize(), ay * m_chunks.GetTileSize());
									float dxp = ca.x - pref.x; float dyp = ca.y - pref.y; float dist = dxp*dxp + dyp*dyp;
									if (dist < bestDist) { bestDist = dist; bestAx=edgeAx; bestAy=ay; bestBx=edgeBx; bestBy=by; }
								}
							}
						} else {
							int edgeAy = cy * ch + (dy > 0 ? (ch - 1) : 0);
							int edgeBy = ny * ch + (dy > 0 ? 0 : (ch - 1));
							for (int icol = 0; icol < cw; ++icol) {
								int ax = cx * cw + icol; int bx = nx * cw + icol;
								int va = m_chunks.GetTileAt(ax, edgeAy);
								int vb = m_chunks.GetTileAt(bx, edgeBy);
								if (va == 0 && vb == 0) {
									Vec2 ca = Vec2(ax * m_chunks.GetTileSize(), edgeAy * m_chunks.GetTileSize());
									float dxp = ca.x - pref.x; float dyp = ca.y - pref.y; float dist = dxp*dxp + dyp*dyp;
									if (dist < bestDist) { bestDist = dist; bestAx=ax; bestAy=edgeAy; bestBx=bx; bestBy=edgeBy; }
								}
							}
						}
						if (bestAx == -1) { PathJobResult res; res.entityId = entId; res.path = std::nullopt; res.requestId = search.requestId; { std::lock_guard<std::mutex> lk2(m_resultsMutex); m_doneResults.push(res); } finished.push_back(entId); break; }

						int exitLocalX = bestAx - cx * cw;
						int exitLocalY = bestAy - cy * ch;
						Chunk chunkCopy;
						{
							std::lock_guard<std::mutex> lk(m_chunks.GetMutex());
							auto &chunks = m_chunks.GetChunks();
							long long keyk = (static_cast<long long>(cx) << 32) | static_cast<unsigned int>(cy);
							auto itc = chunks.find(keyk);
							if (itc == chunks.end()) { PathJobResult res; res.entityId = entId; res.path = std::nullopt; res.requestId = search.requestId; { std::lock_guard<std::mutex> lk2(m_resultsMutex); m_doneResults.push(res); } finished.push_back(entId); break; }
							chunkCopy = itc->second;
						}
						auto seg = m_pathfinder.LocalAStar(chunkCopy, localStartX, localStartY, exitLocalX, exitLocalY);
						if (!seg.has_value()) { PathJobResult res; res.entityId = entId; res.path = std::nullopt; res.requestId = search.requestId; { std::lock_guard<std::mutex> lk2(m_resultsMutex); m_doneResults.push(res); } finished.push_back(entId); break; }
						if (!finalPath.empty() && !seg->empty()) seg->erase(seg->begin());
						finalPath.insert(finalPath.end(), seg->begin(), seg->end());
						curTx = bestBx; curTy = bestBy;
						// continue to next chunk
					}
				}
				break;
			}

			// expand neighbours (8-directional)
			const int dirs8[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
			for (auto &d : dirs8) {
				int nx = cur.x + d[0]; int ny = cur.y + d[1];
				// check adjacency via chunk border test including diagonals
				auto chunkHasConnection = [&](int cx, int cy, int nx2, int ny2) {
					int dx = nx2 - cx; int dy = ny2 - cy;
					if (std::abs(dx) > 1 || std::abs(dy) > 1) return false;
					int cw2 = m_chunks.GetChunkWidth(); int ch2 = m_chunks.GetChunkHeight();
					if (std::abs(dx) + std::abs(dy) == 1) {
						if (dx != 0) {
							int edgeX_A = cx * cw2 + (dx > 0 ? (cw2 - 1) : 0);
							int edgeX_B = nx2 * cw2 + (dx > 0 ? 0 : (cw2 - 1));
							for (int irow = 0; irow < ch2; ++irow) {
								int ay = cy * ch2 + irow;
								int va = m_chunks.GetTileAt(edgeX_A, ay);
								int vb = m_chunks.GetTileAt(edgeX_B, ay);
								if (va == 0 && vb == 0) return true;
							}
						} else {
							int edgeY_A = cy * ch2 + (dy > 0 ? (ch2 - 1) : 0);
							int edgeY_B = ny2 * ch2 + (dy > 0 ? 0 : (ch2 - 1));
							for (int i = 0; i < cw2; ++i) {
								int ax = cx * cw2 + i;
								int va = m_chunks.GetTileAt(ax, edgeY_A);
								int vb = m_chunks.GetTileAt(cx * cw2 + i, edgeY_B);
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
						int va = m_chunks.GetTileAt(ax, ay);
						int vb = m_chunks.GetTileAt(bx, by);
						if (va != 0 || vb != 0) return false;
						int orth1x = ax + dx; int orth1y = ay;
						int orth2x = ax; int orth2y = ay + dy;
						int o1 = m_chunks.GetTileAt(orth1x, orth1y);
						int o2 = m_chunks.GetTileAt(orth2x, orth2y);
						if (o1 == 0 || o2 == 0) return true;
						return false;
					}
					return false;
				};
				if (!chunkHasConnection(cur.x, cur.y, nx, ny)) continue;
				long long nk = (static_cast<long long>(nx) << 32) | static_cast<unsigned int>(ny);
				auto nit = search.nodes.find(nk);
				float moveCost = (std::abs(d[0]) + std::abs(d[1]) == 2) ? std::sqrt(2.0f) : 1.0f;
				float ng = cur.g + moveCost;
				if (nit == search.nodes.end() || ng < nit->second.g) {
					search.nodes[nk] = {nx, ny, ng, key};
					float h = m_pathfinder.HeuristicChunk(nx, ny, search.gcx, search.gcy);
					search.open.push({ng + h, nk});
					// produce partial chunk-path preview: reconstruct current best path to this neighbor
					std::vector<std::pair<int,int>> previewChunks;
					long long curK = nk;
					while (curK != -1) {
						auto itn = search.nodes.find(curK);
						if (itn == search.nodes.end()) break;
						previewChunks.emplace_back(itn->second.x, itn->second.y);
						curK = itn->second.parent;
					}
					if (!previewChunks.empty()) {
						reverse(previewChunks.begin(), previewChunks.end());
						// translate chunk preview into a simple world polyline by using chunk centers
						std::vector<Vec2> previewPath;
						float ts = m_chunks.GetTileSize();
						float cw = (float)m_chunks.GetChunkWidth();
						float ch = (float)m_chunks.GetChunkHeight();
						for (auto &cc : previewChunks) {
							float wx = (cc.first * cw + cw * 0.5f) * ts;
							float wy = (cc.second * ch + ch * 0.5f) * ts;
							previewPath.emplace_back(wx, wy);
						}
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

	ApplyResults();
}
/////////////////////////////////



/////////////////////////////////
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
		auto keyFor = [](int x, int y) { return (static_cast<long long>(x) << 32) | static_cast<unsigned int>(y); };
		long long k = keyFor(scx, scy);
		as->goalKey = keyFor(gcx, gcy);
		as->nodes[k] = {scx, scy, 0.0f, -1};
		float h = m_pathfinder.HeuristicChunk(scx, scy, gcx, gcy);
		as->open.push({h, k});
		m_activeSearches[id] = std::move(as);
	}
}
/////////////////////////////////



/////////////////////////////////
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
