/////////////////////////////////
// TileMapScene.cpp -	Implementation of the TileMapScene class, which is responsible for rendering a tile map and handling user input for raycasting and 
//						tile editing. The scene allows users to click and drag to perform raycasts against the tile map, visualize the results with debug lines 
//						and points, and toggle tile states with right-clicks. It also includes functionality for toggling visual debug overlays and handling 
//						keyboard input for scene management.
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#include "RayCastScene.h"
#include "GameEngine.h"
#include "EntityManager.h"
#include "CameraSystem.h"
#include <iostream>
#include "Raycast.h"
#include "TileMap.h"
#include <SFML/Graphics/RectangleShape.hpp>
#include <Utils.h>
#include "Vec2.h"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>
#include "Entity.h"
#include "CText.h"
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <limits>
#include <imgui/imgui.h>
#include "CRectangle.h"
/////////////////////////////////



/////////////////////////////////
// Constructor - initializes the ray cast scene with a reference to the game engine and the render window, and sets up the entity manager
RayCastScene::RayCastScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager)
	: Scene(engine, entityManager), m_window(win), m_chunkManager(engine.GetChunkManager()) {
	m_debugLines.reserve(256);
	m_debugLineColors.reserve(256);
	m_debugPoints.reserve(256);
	m_rawHitPoints.reserve(256);
	m_visitedCells.reserve(1024);
}
/////////////////////////////////



/////////////////////////////////
// Destructor - defaulted since we don't have any special cleanup logic, but we could add it if needed in the future
RayCastScene::~RayCastScene() = default;
/////////////////////////////////



/////////////////////////////////
// DrawBVHNode - recursively draws the bounding volume hierarchy (BVH) nodes for visualizing the spatial partitioning of the tile map. Each node is drawn as a rectangle, with leaf nodes in green and internal nodes in blue.
void RayCastScene::DrawBVHNode(sf::RenderWindow& window, BVHNode* node) {
	if (!node)
		return;

	sf::RectangleShape rect;
	rect.setPosition(node->bounds.position);
	rect.setSize(node->bounds.size);
	rect.setFillColor(sf::Color::Transparent);

	if (node->IsLeaf())
		rect.setOutlineColor(sf::Color(0, 255, 0, 120)); // green
	else
		rect.setOutlineColor(sf::Color(0, 100, 255, 120)); // blue

	rect.setOutlineThickness(1.f);
	window.draw(rect);

	DrawBVHNode(window, node->left);
	DrawBVHNode(window, node->right);
}
/////////////////////////////////



/////////////////////////////////
// DrawHighlightedEntity - draws a yellow outline around the currently highlighted entity for visual debugging.
void RayCastScene::DrawHighlightedEntity(sf::RenderWindow& window) {
	if (!m_highlightedEntity)
		return;

	CShape* shape = m_highlightedEntity->GetShape();
	if (!shape)
		return;

	sf::FloatRect bounds = shape->GetShape().getGlobalBounds();

	sf::RectangleShape outline;
	outline.setPosition(bounds.position);
	outline.setSize(bounds.size);
	outline.setFillColor(sf::Color::Transparent);
	outline.setOutlineColor(sf::Color::Yellow);
	outline.setOutlineThickness(2.f);

	window.draw(outline);
}
/////////////////////////////////



///////////////////////////////
// RefreshAvailableLevels - scans the same APPDATA levels directory used by the level editor.
void RayCastScene::RefreshMapBounds() {
	float dMinX, dMinY, dMaxX, dMaxY;
	if (m_chunkManager.GetSavedChunkBounds(dMinX, dMinY, dMaxX, dMaxY)) {
		m_mapMin = Vec2(dMinX, dMinY);
		m_mapMax = Vec2(dMaxX, dMaxY);
		m_haveBounds = true;
	} else {
		m_haveBounds = false;
	}
}
/////////////////////////////////



///////////////////////////////
void RayCastScene::RefreshAvailableLevels() {
	m_availableLevels.clear();
	std::error_code ec;
	std::filesystem::path base;
#ifdef _MSC_VER
	char* envBuf = nullptr;
	size_t len = 0;
	errno_t er = _dupenv_s(&envBuf, &len, "APPDATA");
	if (er == 0 && envBuf && envBuf[0] != '\0') base = std::filesystem::path(envBuf) / "GameEnginePlus" / "levels";
	else base = std::filesystem::path("levels");
	free(envBuf);
#else
	const char* appdata = std::getenv("APPDATA");
	if (appdata && appdata[0] != '\0') base = std::filesystem::path(appdata) / "GameEnginePlus" / "levels";
	else base = std::filesystem::path("levels");
#endif

	if (!std::filesystem::exists(base, ec) || ec) return;
	for (auto it = std::filesystem::directory_iterator(base, ec); it != std::filesystem::directory_iterator(); ++it) {
		if (ec) break;
		try {
			auto& entry = *it;
			if (!entry.is_directory()) continue;
			if (std::filesystem::exists(entry.path() / "chunks")) {
				m_availableLevels.push_back(entry.path().filename().string());
			}
		} catch (...) {}
	}
	std::sort(m_availableLevels.begin(), m_availableLevels.end());
}
/////////////////////////////////



///////////////////////////////
// SwitchToLevel - configure chunk base path for selected level and rebuild scene data.
bool RayCastScene::SwitchToLevel(const std::string& name) {
	std::filesystem::path base;
#ifdef _MSC_VER
	char* envBuf = nullptr;
	size_t len = 0;
	errno_t er = _dupenv_s(&envBuf, &len, "APPDATA");
	if (er == 0 && envBuf && envBuf[0] != '\0') base = std::filesystem::path(envBuf) / "GameEnginePlus" / "levels";
	else base = std::filesystem::path("levels");
	free(envBuf);
#else
	const char* appdata = std::getenv("APPDATA");
	if (appdata && appdata[0] != '\0') base = std::filesystem::path(appdata) / "GameEnginePlus" / "levels";
	else base = std::filesystem::path("levels");
#endif

	std::filesystem::path chunkPath = name.empty() ? (base / "chunks") : (base / name / "chunks");
	if (!std::filesystem::exists(chunkPath)) return false;

	m_currentLevelName = name;
	m_chunkManager.SetNumLayers(3);
	m_chunkManager.SetActiveLayer(m_collisionLayer);
	m_chunkManager.SetBasePath(chunkPath.string());
	RefreshMapBounds();
	m_hasLastViewCenter = false;
	m_chunkManager.ClearAllLoadedChunks();
	m_chunkManager.LoadAllSavedChunks();
	for (int i = 0; i < 10; ++i) m_chunkManager.UpdateMainThread_NoLock();
	m_chunkManager.RebuildAllChunksFromTileset();
	m_chunkManager.UpdateMainThread_NoLock();
	m_chunkManager.RefreshWorldBoundsFromLoadedChunks();
	m_chunkManager.BuildWorldMask();
	m_lastSeenWorldRevision = m_chunkManager.GetWorldRevision();
	return true;
}
/////////////////////////////////



///////////////////////////////
// LevelManagerWindow - read-only level selector matching level-editor flow.
void RayCastScene::LevelManagerWindow() {
	ImGui::SetNextWindowBgAlpha(0.35f);
	if (!ImGui::Begin("RayCastScene Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::End();
		return;
	}

	if (ImGui::Button("Refresh Levels")) {
		RefreshAvailableLevels();
	}

	for (size_t i = 0; i < m_availableLevels.size(); ++i) {
		bool selected = (static_cast<int>(i) == m_selectedLevelIndex);
		if (ImGui::Selectable(m_availableLevels[i].c_str(), selected)) {
			m_selectedLevelIndex = static_cast<int>(i);
		}
	}

	if (m_selectedLevelIndex >= 0 && m_selectedLevelIndex < static_cast<int>(m_availableLevels.size())) {
		if (ImGui::Button("Switch To")) {
			SwitchToLevel(m_availableLevels[m_selectedLevelIndex]);
		}
	}

	ImGui::Separator();
	ImGui::Text("Current: %s", m_currentLevelName.empty() ? "(default)" : m_currentLevelName.c_str());
	ImGui::Checkbox("Clamp Pan To Bounds", &m_clampPanToBounds);
	ImGui::Checkbox("Include Default Entity Hits", &m_includeDefaultEntitiesInRaycast);
	const sf::Vector2i mousePixel = sf::Mouse::getPosition(m_window);
	const sf::Vector2f mouseWorld = m_window.mapPixelToCoords(mousePixel);
	ImGui::Text("Mouse X: %.2f", mouseWorld.x);
	ImGui::Text("Mouse Y: %.2f", mouseWorld.y);
	ImGui::TextUnformatted("Read-only scene; edit in Level Editor.");
	ImGui::End();
}
/////////////////////////////////



///////////////////////////////
// ProcessDebugToggle - handles debug toggle key (D) to show/hide visual debug overlays
void RayCastScene::ProcessDebugToggle(bool debugToggle) {
	if (debugToggle && !m_prevDebugKeyDown)
		m_visualDebug = !m_visualDebug;
	m_prevDebugKeyDown = debugToggle;
}
/////////////////////////////////



/////////////////////////////////
// RaycastDynamicEntities - Query nearby entities from spatial hash and run narrowphase ray-vs-AABB against candidates.
bool RayCastScene::RaycastDynamicEntities(const Vec2& origin, const Vec2& dir, float maxDistance, RaycastHit& outHit, Entity*& outEntity) {
	
	// *** OUTPUT PARAMETER ***
	// outEntity will be set to the nearest hit entity, or nullptr if no hit occurred
	outEntity = nullptr;
	outHit = RaycastHit{}; // Reset outHit to default values
	if (maxDistance <= 0.0f)
		return false;

	// *** DIRECTION NORMALIZATION ***
	Vec2 dirN = dir.GetUnitVec();
	if (dirN.Mag2() <= 1e-12f) return false;

	// *** SPATIAL HASH QUERY ***
	std::vector<Entity*> candidates;
	m_entityManager.GetSpatialHash().Query(candidates, origin, maxDistance + m_chunkManager.GetTileSize());
	if (candidates.empty())	return false;

	// *** NARROWPHASE RAYCAST ***
	float nearest = maxDistance;
	for (Entity* e : candidates) {
		if (!e || !e->IsAlive()) continue;

		const EntityType type = e->GetType();
		if (type == EntityType::Tile || type == EntityType::TileMap || type == EntityType::Chunk) continue;
		if (!m_includeDefaultEntitiesInRaycast && type == EntityType::Default) continue;

		// Check if the entity has a CShape component, which is required for raycasting against its bounding box
		CShape* shape = e->GetShape();
		if (!shape) continue;

		sf::Shape& sfShape = shape->GetShape();
		const sf::FloatRect b = sfShape.getGlobalBounds();
		
		// Skip entities with non-positive size, as they cannot be hit by a ray
		if (b.size.x <= 0.0f || b.size.y <= 0.0f) continue;

		// Perform ray-AABB intersection test. If the ray does not intersect the entity's bounding box, continue to the next candidate.
		float hitDist = 0.0f;
		if (!RayIntersectsAABB(origin, dirN, Vec2(b.position.x, b.position.y), Vec2(b.position.x + b.size.x, b.position.y + b.size.y), hitDist, nearest)) continue;

		// If the hit distance is negative (behind the ray origin) or greater than the nearest hit found so far, skip this entity
		if (hitDist < 0.0f || hitDist > nearest) continue;

		// Update the nearest hit information with the current entity's hit data
		UpdateHitInfo(nearest, hitDist, outEntity, e, outHit, origin, dirN);

		// Calculate the normal of the hit surface based on the entity's bounding box and the hit position. This is done by finding the center of the bounding box and computing the delta from the 
		// hit position to the center.
		Vec2 center(b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f);
		Vec2 delta = outHit.position - center;
		float nx = (b.size.x > 0.0f) ? (delta.x / (b.size.x * 0.5f)) : 0.0f;
		float ny = (b.size.y > 0.0f) ? (delta.y / (b.size.y * 0.5f)) : 0.0f;
		
		// Determine the normal direction based on which axis has the larger absolute value. This helps to identify which side of the bounding box was hit by the ray.
		if (std::fabs(nx) > std::fabs(ny))
			outHit.normal = Vec2((nx > 0.0f) ? 1.0f : -1.0f, 0.0f);
		else
			outHit.normal = Vec2(0.0f, (ny > 0.0f) ? 1.0f : -1.0f);
	}

	return outEntity != nullptr;
}
/////////////////////////////////



/////////////////////////////////
// Update the nearest hit information with the current entity's hit data
void RayCastScene::UpdateHitInfo(float& nearest, float hitDist, Entity*& outEntity, Entity* e, RaycastHit& outHit, const Vec2& origin, Vec2& dirN) {
	nearest = hitDist;
	outEntity = e;
	outHit.hit = true;
	outHit.tileX = -1;
	outHit.tileY = -1;
	outHit.tileValue = 0;
	outHit.distance = hitDist;
	outHit.position = Vec2(origin.x + dirN.x * hitDist, origin.y + dirN.y * hitDist);
}
/////////////////////////////////



/////////////////////////////////
// ProcessMouseDragRaycast - Helper to create a RaycastHit when ray starts inside a solid tile, since DDA will return no hit in this case
void RayCastScene::ProcessMouseDragRaycast(bool leftMouseDown, const Vec2& mouseWorld) {
	// Handle mouse drag state transitions and perform raycast on release
	if (leftMouseDown && !m_prevLmbMouseDown) {
		m_lmbdragging = true;
		m_lmbDragStart = mouseWorld;
		m_previewActive = true;
	}

	// If mouse is up and was down in previous frame, end dragging and perform raycast
	else if (!leftMouseDown && m_prevLmbMouseDown) {
		if (m_lmbdragging) {
			m_lmbdragging = false;
			m_lmbDragEnd = mouseWorld;
			Vec2 dir = m_lmbDragEnd - m_lmbDragStart;
			float dragLen = dir.Mag();

			// If the drag length is too small, cancel the preview and return early
			if (dragLen <= 0.001f) {
				m_previewActive = false;
				m_prevLmbMouseDown = leftMouseDown;
				return;
			}

			dir = dir.GetUnitVec(); // Normalize direction for raycast

			// Clear previous debug lines, colors, and points for the new raycast
			m_debugLines.clear();
			m_debugLineColors.clear();
			m_debugPoints.clear();

			// Clear previous raw hit points and visited cells for the new raycast
			const float tileSize = m_chunkManager.GetTileSize();
			const float rayMinX = std::min(m_lmbDragStart.x, m_lmbDragEnd.x);
			const float rayMinY = std::min(m_lmbDragStart.y, m_lmbDragEnd.y);
			const float rayMaxX = std::max(m_lmbDragStart.x, m_lmbDragEnd.x);
			const float rayMaxY = std::max(m_lmbDragStart.y, m_lmbDragEnd.y);
			const int rayMinTx = static_cast<int>(std::floor(rayMinX / tileSize));
			const int rayMinTy = static_cast<int>(std::floor(rayMinY / tileSize));
			const int rayMaxTx = static_cast<int>(std::floor(rayMaxX / tileSize));
			const int rayMaxTy = static_cast<int>(std::floor(rayMaxY / tileSize));
			(void)rayMinTx;
			(void)rayMinTy;
			(void)rayMaxTx;
			(void)rayMaxTy;

			// Refresh world bounds and world mask if the world revision has changed since the last raycast
			const uint64_t beforeRayRevision = m_chunkManager.GetWorldRevision();
			if (beforeRayRevision != m_lastSeenWorldRevision) {
				m_chunkManager.RefreshWorldBoundsFromLoadedChunks();
				m_chunkManager.BuildWorldMask();
				m_lastSeenWorldRevision = m_chunkManager.GetWorldRevision();
			}

			// Determine the starting tile coordinates for the raycast based on the drag start position and tile size
			const int startTileX = static_cast<int>(std::floor(m_lmbDragStart.x / tileSize));
			const int startTileY = static_cast<int>(std::floor(m_lmbDragStart.y / tileSize));

			// Perform a raycast to check if the starting cell is solid. This is necessary because the DDA algorithm will not return a hit if the ray starts inside a solid tile.
			RaycastHit rayHitStartCell = MakeStartCellHit(startTileX, startTileY, m_lmbDragStart);
			const bool startSolid = rayHitStartCell.hit;

			// Clear visited cells and prepare for visual debug output if enabled
			m_visitedCells.clear();
			std::vector<std::pair<int, int>> visitedCellsTemp;
			std::vector<std::pair<int, int>>* visitedOut = nullptr;
			
			// If visual debug is enabled, reserve space for visited cells and set the output pointer to the temporary vector
			if (m_visualDebug) {
				visitedCellsTemp.reserve(1024);
				visitedOut = &visitedCellsTemp;
			}

			RaycastHit rayHitIgnore; // This will hold the result of the DDA raycast, ignoring the starting cell if it's solid
			{
				std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex());
				rayHitIgnore =
					RaycastWorldMaskDDA(m_lmbDragStart, dir, m_chunkManager.worldMask, m_chunkManager.worldWidth,
										m_chunkManager.worldHeight, m_chunkManager.worldOffsetX,
										m_chunkManager.worldOffsetY, tileSize, dragLen, startSolid, visitedOut);
			}

			// If visual debug is enabled, store the visited cells in the member variable for rendering
			if (m_visualDebug) {
				if (visitedCellsTemp.size() > 1024)
					visitedCellsTemp.resize(1024);
				m_visitedCells = std::move(visitedCellsTemp);
			}

			// BVH DYNAMIC ENTITY RAYCAST (drop‑in replacement)
			RaycastHit entityHit;
			Entity* hitEntity = nullptr;

			auto& bvh = GetEntityManager().GetBVH();
			bool hitDynamic = bvh.Raycast(m_lmbDragStart, dir, dragLen, entityHit, hitEntity);

			// Highlight the nearest hit entity if a dynamic hit occurred; otherwise, clear the highlighted entity
			if (hitDynamic)
				m_highlightedEntity = hitEntity;
			else
				m_highlightedEntity = nullptr;

			// Continue exactly as before — merge static + dynamic hits
			RaycastHit staticHit;
			if (startSolid) {
				staticHit = rayHitStartCell;
				if (rayHitIgnore.hit &&
					(rayHitIgnore.tileX != rayHitStartCell.tileX || rayHitIgnore.tileY != rayHitStartCell.tileY)) {
					if (m_debugLines.size() < 256) {
						m_debugLines.push_back({m_lmbDragStart, rayHitIgnore.position});
						m_debugLineColors.push_back(sf::Color::Green);
					}
					if (m_rawHitPoints.size() < 256)
						m_rawHitPoints.push_back(rayHitIgnore.position);
				}
			} else {
				staticHit = rayHitIgnore;
			}

			// Merge static and dynamic hits to determine the nearest hit along the ray
			RaycastHit rayHit;
			if (hitDynamic && staticHit.hit)
				rayHit = (entityHit.distance <= staticHit.distance) ? entityHit : staticHit;
			else if (hitDynamic)
				rayHit = entityHit;
			else
				rayHit = staticHit;

			// Visual debug output for the raycast result
			if (rayHit.hit) {
				Vec2 hitPos = rayHit.position;
				float proj = (hitPos.x - m_lmbDragStart.x) * dir.x + (hitPos.y - m_lmbDragStart.y) * dir.y;
				
				// Clamp the hit position to the drag length if it exceeds it
				if (proj > dragLen) {
					hitPos = Vec2(m_lmbDragStart.x + dir.x * dragLen, m_lmbDragStart.y + dir.y * dragLen);
				}
				
				// If the hit position is within the drag length, add a green debug line and point to visualize the hit
				if (m_debugLines.size() < 256) {
					m_debugLines.push_back({m_lmbDragStart, hitPos});
					m_debugLineColors.push_back(sf::Color::Green);
				}

				// Add the hit position to the debug points and raw hit points for visualization
				if (m_debugPoints.size() < 256)
					m_debugPoints.push_back(hitPos);

				// Add the hit position to the raw hit points for further analysis or rendering
				if (m_rawHitPoints.size() < 256)
					m_rawHitPoints.push_back(rayHit.position);
			} else {
				// If no hit occurred, add a red debug line and point to visualize the raycast path
				if (m_debugLines.size() < 256) {
					m_debugLines.push_back({m_lmbDragStart, m_lmbDragEnd});
					m_debugLineColors.push_back(sf::Color::Red);
				}
				// Add the drag end position to the debug points for visualization
				if (m_debugPoints.size() < 256)
					m_debugPoints.push_back(m_lmbDragEnd);
			}

			m_previewActive = false;
		}
	} else if (leftMouseDown && m_lmbdragging) { // Update drag end position while dragging
		m_lmbDragEnd = mouseWorld;
		
		// If visual debug is enabled, update the preview line to show the current drag line
		if (m_visualDebug)
			m_previewLine = {m_lmbDragStart, m_lmbDragEnd};
	}

	m_prevLmbMouseDown = leftMouseDown;
}
/////////////////////////////////



///////////////////////////////
// ProcessEscapeKey - handles input to close the window when any key is pressed
void RayCastScene::ProcessEscapeKey(bool keyDown) const {
	if (keyDown) {
		m_gameEngine.ChangeScene("MainMenu");
	}
}
/////////////////////////////////



///////////////////////////////
// ProcessMiddleMousePan - pan the current window view by dragging with middle mouse button (PathTestScene style).
void RayCastScene::ProcessMiddleMousePan() {
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt)
		return;

	CCamera* cam = *camOpt;
	const bool middleDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle);
	const sf::Vector2i mousePos = sf::Mouse::getPosition(m_window);

	if (middleDown && !m_cameraSystem.IsPanning(*cam))
		m_cameraSystem.BeginPan(*cam, mousePos);
	else if (!middleDown && m_cameraSystem.IsPanning(*cam))
		m_cameraSystem.EndPan(*cam);

	if (!m_cameraSystem.IsPanning(*cam))
		return;

	m_cameraSystem.UpdatePan(*cam, mousePos);
}
/////////////////////////////////



///////////////////////////////
// Update - handles events, updates the entity manager, and prepares debug visualization data for rendering
void RayCastScene::Update(float deltaTime) {
	GetEntityManager().Update(deltaTime);

	if (m_cameraEntity)
		m_cameraSystem.Update(deltaTime, GetEntityManager());
	ApplyMainCameraView();

	// Keep chunk data streaming/ready for rendering using camera-system state as source of truth.
	const float tileSize = m_chunkManager.GetTileSize();
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt)
		return;
	CCamera* cam = *camOpt;
	const Vec2 cameraCenter = cam->position;
	const float safeZoom = (cam->zoom > 0.0001f) ? cam->zoom : 1.0f;
	const float worldW = cam->viewportWidth / safeZoom;
	const float worldH = cam->viewportHeight / safeZoom;
	const float halfW = worldW * 0.5f;
	const float halfH = worldH * 0.5f;

	int tx0 = static_cast<int>(std::floor((cameraCenter.x - halfW) / tileSize));
	int ty0 = static_cast<int>(std::floor((cameraCenter.y - halfH) / tileSize));
	int tx1 = static_cast<int>(std::floor((cameraCenter.x + halfW) / tileSize));
	int ty1 = static_cast<int>(std::floor((cameraCenter.y + halfH) / tileSize));
	m_chunkManager.EnsureChunksInTileRect(tx0, ty0, tx1, ty1, 1);
	m_chunkManager.UpdateMainThread_NoLock();

	// Refresh raycast world data only when chunks changed or camera teleported far enough to imply a new area jump.
	bool needsRayRefresh = false;
	const uint64_t worldRevision = m_chunkManager.GetWorldRevision();
	if (worldRevision != m_lastSeenWorldRevision)
		needsRayRefresh = true;

	const float teleportThreshold = m_chunkManager.GetTileSize() * static_cast<float>(m_chunkManager.GetChunkWidth() * 4);
	if (m_hasLastViewCenter) {
		const float dx = cameraCenter.x - m_lastViewCenter.x;
		const float dy = cameraCenter.y - m_lastViewCenter.y;
		if ((dx * dx + dy * dy) > (teleportThreshold * teleportThreshold))
			needsRayRefresh = true;
	}
	m_lastViewCenter = cameraCenter;
	m_hasLastViewCenter = true;

	if (needsRayRefresh) {
		m_chunkManager.RefreshWorldBoundsFromLoadedChunks();
		m_chunkManager.BuildWorldMask();
		m_lastSeenWorldRevision = m_chunkManager.GetWorldRevision();
	}


	// Handle events (SFML 3.0: pollEvent returns std::optional<sf::Event>)
	while (auto eventOpt = m_gameEngine.window.pollEvent()) {
		if (eventOpt->is<sf::Event::Closed>()) {
			m_gameEngine.window.close(); // window X button - always close
		}
		// Escape is handled globally by GameEngine before scenes run, so do NOT forward it here
		if (!eventOpt->is<sf::Event::KeyPressed>() || [&] {
				auto kp = eventOpt->getIf<sf::Event::KeyPressed>();
				return !kp || static_cast<sf::Keyboard::Key>(kp->code) != sf::Keyboard::Key::Escape;
			}())
			HandleEvent(eventOpt);
	}

	// Poll live input every frame so drag transitions are captured reliably even near window edges.
	const bool escapeKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
	const bool debugToggle = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
	ProcessDebugToggle(debugToggle);
	ProcessEscapeKey(escapeKeyDown);

	// Apply panning before mapping mouse-to-world for raycast input.
	ProcessMiddleMousePan();
	ApplyMainCameraView();

	// Compute mouse world coords AFTER final view is set for this frame.
	sf::Vector2i mousePixelPos = sf::Mouse::getPosition(m_window);
	sf::Vector2f mouseWorldF = m_window.mapPixelToCoords(mousePixelPos);
	Vec2 mouseWorld(static_cast<float>(mouseWorldF.x), static_cast<float>(mouseWorldF.y));
	const bool leftMouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	ProcessMouseDragRaycast(leftMouseDown, mouseWorld);

	// Show level picker UI like the level editor (read-only selection in this scene).
	LevelManagerWindow();
}
/////////////////////////////////



/////////////////////////////////
// Render - draws the tile grid and any debug visualization overlays
void RayCastScene::Render() {
	//std::cout << "RayCastScene::Render CALLED\n";
	// Enqueue and flush chunk tiles first so debug ray overlays always render on top.
	GetEntityManager().RenderShapes(); // Render shapes again to ensure they are drawn on top of debug overlays

	DrawDebugLines();
	DrawHitPoints();
	DrawRawHitPoints();
	DrawVisitedCells();
	DrawPreviewLine();

	// Draw BVH nodes for visualizing the spatial partitioning of the tile map
	DrawHighlightedEntity(m_window);

	if (m_visualDebug) {
		auto& bvh = GetEntityManager().GetBVH();
		DrawBVHNode(m_window, bvh.GetRoot());
	}

	// Text is rendered by the RenderSystem during EntityManager::Update; no per-scene text draw here to avoid double-rendering.

	DrawTileGrid();
	GetEngineRenderQueue().Flush(m_window);
}
/////////////////////////////////



/////////////////////////////////
// DoAction - currently empty, but could be used for game logic that needs to run on a fixed timestep or in response to certain conditions
void RayCastScene::DoAction() {}
/////////////////////////////////
 


/////////////////////////////////
// Draw the tile grid from chunked map data
void RayCastScene::DrawTileGrid() {
	m_chunkManager.EnqueueChunks(GetEngineRenderQueue(), m_window.getView());
}
/////////////////////////////////



/////////////////////////////////
// DrawDebugLines - draws debug lines for raycasts, using red color for visibility. Only draws if visual debug mode is enabled.
void RayCastScene::DrawDebugLines() {
	if (!m_visualDebug)
		return;
	for (size_t i = 0; i < m_debugLines.size(); ++i) {
		const auto& pr = m_debugLines[i];
		const sf::Color lineColor = (i < m_debugLineColors.size()) ? m_debugLineColors[i] : sf::Color::Red;
		sf::Vertex line[] = {sf::Vertex(sf::Vector2f(pr.first.x, pr.first.y), lineColor),
							 sf::Vertex(sf::Vector2f(pr.second.x, pr.second.y), lineColor)};
		m_window.draw(line, 2, sf::PrimitiveType::Lines);
	}
}
/////////////////////////////////



/////////////////////////////////
// DrawHitPoints - draws hit points as yellow circles, only if visual debug mode is enabled. This shows the final hit position after clamping to the ray length.
void RayCastScene::DrawHitPoints() {
	if (!m_visualDebug)
		return;
	for (const auto& p : m_debugPoints) {
		sf::CircleShape dot(4.0f);
		dot.setFillColor(sf::Color::Yellow);
		dot.setOrigin(sf::Vector2f(4.0f, 4.0f));
		dot.setPosition(sf::Vector2f(p.x, p.y));
		m_window.draw(dot);
	}
}
/////////////////////////////////



/////////////////////////////////
// DrawRawHitPoints - draws raw hit points (before clamping) as blue circles, only if visual debug mode is enabled. This can show the actual intersection point with the tile 
// boundary, which may be outside the ray length if the ray starts inside a solid tile.
void RayCastScene::DrawRawHitPoints() {
	if (!m_visualDebug)
		return;
	for (const auto& p : m_rawHitPoints) {
		sf::CircleShape dot(3.0f);
		dot.setFillColor(sf::Color::Blue);
		dot.setOrigin(sf::Vector2f(3.0f, 3.0f));
		dot.setPosition(sf::Vector2f(p.x, p.y));
		m_window.draw(dot);
	}
}
/////////////////////////////////



/////////////////////////////////
// DrawVisitedCells - draws visited cells as semi-transparent blue rectangles, only if visual debug mode is enabled. This shows which cells have been visited during 
// raycasting or other pathfinding operations.
void RayCastScene::DrawVisitedCells() {
	if (!m_visualDebug)
		return;
	const float tileSize = m_chunkManager.GetTileSize();
	for (const auto& cell : m_visitedCells) {
		const int cx = cell.first;
		const int cy = cell.second;
		sf::RectangleShape rect(sf::Vector2f(tileSize, tileSize));
		rect.setPosition(sf::Vector2f(static_cast<float>(cx) * tileSize, static_cast<float>(cy) * tileSize));
		rect.setFillColor(sf::Color(0, 0, 255, 60));
		rect.setOutlineColor(sf::Color::Blue);
		rect.setOutlineThickness(1.0f);
		m_window.draw(rect);
	}
}
/////////////////////////////////



/////////////////////////////////
// DrawPreviewLine - draws a preview line during mouse drag to show the current ray segment being defined by the drag. This is drawn in green and only shown when 
// the preview is active and visual debug mode is enabled.
void RayCastScene::DrawPreviewLine() {
	if (!(m_previewActive && m_visualDebug))
		return;
	sf::Vertex line[] = {sf::Vertex(sf::Vector2f(m_previewLine.first.x, m_previewLine.first.y), sf::Color::Green),
						 sf::Vertex(sf::Vector2f(m_previewLine.second.x, m_previewLine.second.y), sf::Color::Green)};
	m_window.draw(line, 2, sf::PrimitiveType::Lines);
}
/////////////////////////////////



/////////////////////////////////
// HandleEvent - event routing only; live input polling is handled in Update for robust drag state transitions.
void RayCastScene::HandleEvent(const std::optional<sf::Event>& event) {
	(void)event;
}
/////////////////////////////////



/////////////////////////////////
// OnEnter - currently empty, but could be used for setup logic that needs to run when the scene becomes active
void RayCastScene::OnEnter() {
	std::cout << "RayCastScene::OnEnter\n";

	auto& em = GetEntityManager();

	  // -------------------------
	// DynamicBox1 (Red)
	// -------------------------
	{
		Vec2 position(200.f, 200.f);
		Vec2 velocity(0.f, 0.f);

		Entity* e = em.AddEntity(EntityType::DynamicBox1);
		e->AddComponent<CName>("DynamicBox1");
		e->AddComponent<CTransform>(position, velocity);

		auto* tform = e->GetComponent<CTransform>();
		tform->position = Vec2(position.x + 0.4f, position.y - 0.5f);

		// CRectangle
		auto rect = std::make_unique<CRectangle>(40.f, 40.f);
		rect->GetShape().setFillColor(sf::Color(255, 0, 0, 180));
		e->AddComponentPtr<CShape>(std::move(rect));
	}

	// -------------------------
	// DynamicBox2 (Green)
	// -------------------------
	{
		Vec2 position(350.f, 250.f);
		Vec2 velocity(0.f, 0.f);

		Entity* e = em.AddEntity(EntityType::DynamicBox2);
		e->AddComponent<CName>("DynamicBox2");
		e->AddComponent<CTransform>(position, velocity);

		auto* tform = e->GetComponent<CTransform>();
		tform->position = Vec2(position.x + 0.4f, position.y - 0.5f);

		auto rect = std::make_unique<CRectangle>(50.f, 50.f);
		rect->GetShape().setFillColor(sf::Color(0, 255, 0, 180));
		e->AddComponentPtr<CShape>(std::move(rect));
	}

	// -------------------------
	// DynamicBox3 (Blue)
	// -------------------------
	{
		Vec2 position(500.f, 300.f);
		Vec2 velocity(0.f, 0.f);

		Entity* e = em.AddEntity(EntityType::DynamicBox3);
		e->AddComponent<CName>("DynamicBox3");
		e->AddComponent<CTransform>(position, velocity);

		auto* tform = e->GetComponent<CTransform>();
		tform->position = Vec2(position.x + 0.4f, position.y - 0.5f);

		auto rect = std::make_unique<CRectangle>(60.f, 60.f);
		rect->GetShape().setFillColor(sf::Color(0, 128, 255, 180));
		e->AddComponentPtr<CShape>(std::move(rect));
	}
}
/////////////////////////////////



/////////////////////////////////
// OnExit - currently empty, but could be used for cleanup logic that needs to run when the scene is no longer active
void RayCastScene::OnExit() {}
/////////////////////////////////



/////////////////////////////////
void RayCastScene::OnWindowResized(sf::Vector2u newSize) {
	if (m_cameraEntity) {
		if (auto cam = m_cameraEntity->GetComponent<CCamera>()) {
			cam->viewportWidth = static_cast<float>(newSize.x);
			cam->viewportHeight = static_cast<float>(newSize.y);
		}
	}
	ApplyMainCameraView();
}
/////////////////////////////////



/////////////////////////////////
void RayCastScene::ApplyMainCameraView() {
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt)
		return;

	CCamera* cam = *camOpt;
	sf::View v = CameraSystem::BuildViewFromCamera(*cam, m_clampPanToBounds, m_mapMin, m_mapMax, m_haveBounds);
	cam->position = Vec2(v.getCenter().x, v.getCenter().y);
	m_window.setView(v);
}
/////////////////////////////////



/////////////////////////////////
// LoadResources - currently empty, but could be used to load textures, sounds, or other resources needed by the scene. In this example, we load the tile map in 
// InitializeGame instead, but we could also move it here if we wanted to separate resource loading from game initialization.
void RayCastScene::LoadResources() {}
/////////////////////////////////



/////////////////////////////////
// UnloadResources - currently empty, but could be used to free textures, sounds, or other resources when the scene is unloaded
void RayCastScene::UnloadResources() {}
/////////////////////////////////



///////////////////////////////
// InitializeGame - load chunked level data and create a raycast snapshot from collision layer 1.
void RayCastScene::InitializeGame(sf::Vector2u /*windowSize*/) {
	sf::Vector2u windowSize = m_window.getSize();

	m_cameraEntity = GetEntityManager().AddEntity(EntityType::Default);
	auto camera = m_cameraEntity->AddComponent<CCamera>(Vec2(0, 0), 1.0f);
	camera->isMainCamera = true;
	camera->isActive = true;
	camera->viewportWidth = static_cast<float>(windowSize.x);
	camera->viewportHeight = static_cast<float>(windowSize.y);
	camera->smoothness = 0.0f;

	// Refresh available levels and switch to the first one if any are found. This allows us to load a specific level from the APPDATA levels directory, or fall back to the default chunk path if no named level is selected.
	RefreshAvailableLevels();
	if (!m_availableLevels.empty()) {
		m_selectedLevelIndex = 0;
		SwitchToLevel(m_availableLevels[0]);
	}

	// Use the same default level chunks path as the editor when no named level is selected.
	std::filesystem::path base;
#ifdef _MSC_VER
	char* envBuf = nullptr;
	size_t len = 0;
	errno_t er = _dupenv_s(&envBuf, &len, "APPDATA");
	if (er == 0 && envBuf && envBuf[0] != '\0') base = std::filesystem::path(envBuf) / "GameEnginePlus" / "levels";
	else base = std::filesystem::path("levels");
	free(envBuf);
#else
	const char* appdata = std::getenv("APPDATA");
	if (appdata && appdata[0] != '\0') base = std::filesystem::path(appdata) / "GameEnginePlus" / "levels";
	else base = std::filesystem::path("levels");
#endif

	// If no level is selected, use the default chunk path and load all saved chunks. This allows us to have a default level that can be used for testing or demonstration purposes, even if no named level is selected.
	if (m_currentLevelName.empty()) {
		m_chunkManager.SetNumLayers(3);
		m_chunkManager.SetActiveLayer(m_collisionLayer);
		m_chunkManager.SetBasePath((base / "chunks").string());
		m_chunkManager.SetMaxLoadedChunks(256);
		m_chunkManager.LoadAllSavedChunks();
		m_chunkManager.UpdateMainThread_NoLock();
	}

	RefreshMapBounds();

	// Fallback: if no chunks are available yet, migrate legacy testmap.json into collision layer 1.
	bool hasAnyChunks = false;
	{
		std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex());
		hasAnyChunks = !m_chunkManager.GetChunks().empty();
	}
	if (!hasAnyChunks) {
		std::string err;
		auto maybe = m_gameEngine.GetFileManager().LoadTileMap("assets\\testmap.json", &err);
		if (maybe) {
			const TileMap& map = *maybe;
			for (int y = 0; y < map.height; ++y) {
				for (int x = 0; x < map.width; ++x) {
					int v = map.GetTile(x, y);
					if (v != 0) m_chunkManager.SetTileAt(x, y, v, m_collisionLayer);
				}
			}
			m_chunkManager.UpdateMainThread_NoLock();
		}
	}

	// Initialize native chunk world bounds, then build world mask using those bounds.
	m_chunkManager.RefreshWorldBoundsFromLoadedChunks();
	m_chunkManager.BuildWorldMask();
	m_lastSeenWorldRevision = m_chunkManager.GetWorldRevision();
	if (m_cameraEntity) {
		if (auto cam = m_cameraEntity->GetComponent<CCamera>()) {
			Vec2 center = m_haveBounds ? (m_mapMin + m_mapMax) * 0.5f : Vec2(0, 0);
			cam->position = center;
			m_lastViewCenter = center;
			m_hasLastViewCenter = true;
		}
	}
	ApplyMainCameraView();

	// Load fonts for text rendering. We load two different font styles (regular and thin) from the assets/fonts/roboto directory. If the font fails to load, we print an error message to the console.
	if (!m_gameEngine.GetFontManager().LoadFont("regular", "assets\\fonts\\roboto\\Roboto-Regular.ttf"))
		std::cerr << "Error loading font" << std::endl;
	if (!m_gameEngine.GetFontManager().LoadFont("thin", "assets\\fonts\\roboto\\Roboto-Thin.ttf"))
		std::cerr << "Error loading font" << std::endl;

	// Create text entities for the scene title and instructions. We create two entities with CTransform and CText components to display the scene title and instructions on the screen. The title is 
	// displayed in cyan color with a larger font size, while the instructions are displayed in yellow color with a smaller font size. If the font fails to load for either entity, we print an error message to the console.
	Entity* fontEntity = m_entityManager.AddEntity(EntityType::Default);
	fontEntity->AddComponent<CTransform>(Vec2(50, 50), Vec2::Zero);
	if (!fontEntity->AddComponent<CText>("RayCasting Demo (Chunked 3-Layer)", sf::Color::Cyan, "regular", 60))
		std::cerr << "Error loading font for text entity" << std::endl;

	// Instructions text entity
	Entity* instructionsEntity = m_entityManager.AddEntity(EntityType::Default);
	instructionsEntity->AddComponent<CTransform>(Vec2(50, windowSize.y - 150), Vec2::Zero);
	if (!instructionsEntity->AddComponent<CText>("Left Click + Drag: Raycast\nPress 'D' to toggle debug visualization\nRead-only scene: edit in Level Editor",
										 sf::Color::Yellow, "thin", 20))
		std::cerr << "Error loading font for instructions entity" << std::endl;
}
/////////////////////////////////



/////////////////////////////////
// MakeStartCellHit - create a synthetic RaycastHit representing an immediate hit at the start cell
RaycastHit RayCastScene::MakeStartCellHit(int tileX, int tileY, const Vec2& origin) {
	RaycastHit h;
	if (tileX < 0 || tileY < 0)
		return h;

	std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex());
	const int localX = tileX - m_chunkManager.worldOffsetX;
	const int localY = tileY - m_chunkManager.worldOffsetY;
	if (localX < 0 || localY < 0 || localX >= m_chunkManager.worldWidth || localY >= m_chunkManager.worldHeight)
		return h;
	const size_t idx = static_cast<size_t>(localY) * static_cast<size_t>(m_chunkManager.worldWidth) + static_cast<size_t>(localX);
	if (idx >= m_chunkManager.worldMask.size() || m_chunkManager.worldMask[idx] == 0)
		return h;

	h.hit = true;
	h.tileX = tileX;
	h.tileY = tileY;
	h.tileValue = 1;
	h.position = origin;
	h.distance = 0.0f;
	return h;
}
/////////////////////////////////