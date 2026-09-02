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
#include "ChunkManager.h"
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



/////////////////////////////////
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



/////////////////////////////////
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



/////////////////////////////////
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



/////////////////////////////////
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



/////////////////////////////////
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
	outEntity = nullptr;
	outHit = RaycastHit{};
	if (maxDistance <= 0.0f)
		return false;

	Vec2 dirN = dir.GetUnitVec();
	if (dirN.Mag2() <= 1e-12f)
		return false;

	// Use unified spatial index instead of chunk BVHs
	auto* si = GetEntityManager().GetSpatialIndex();
	if (!si)
		return false;

	return si->RaycastEntities(origin, dirN, maxDistance, outHit, outEntity);
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
	// --- Handle drag start ---
	if (leftMouseDown && !m_prevLmbMouseDown) {
		m_lmbdragging = true;
		m_lmbDragStart = mouseWorld;
		m_previewActive = true;
	}
	// --- Handle drag release ---
	else if (!leftMouseDown && m_prevLmbMouseDown) {
		if (m_lmbdragging) {
			m_lmbdragging = false;
			m_lmbDragEnd = mouseWorld;

			Vec2 dir = m_lmbDragEnd - m_lmbDragStart;
			float dragLen = dir.Mag();

			if (dragLen <= 0.001f) {
				m_previewActive = false;
				m_prevLmbMouseDown = leftMouseDown;
				return;
			}

			dir = dir.GetUnitVec();

			// --- Clear previous debug ---
			m_debugLines.clear();
			m_debugLineColors.clear();
			m_debugPoints.clear();		// static hits (yellow)
			m_dynamicHitPoints.clear(); // dynamic hits (green)
			m_rawHitPoints.clear();
			m_debugTraversals.clear();
			m_visitedCells.clear();

			const float tileSize = m_chunkManager.GetTileSize();

			// --- World mask refresh ---
			const uint64_t beforeRayRevision = m_chunkManager.GetWorldRevision();
			if (beforeRayRevision != m_lastSeenWorldRevision) {
				m_chunkManager.RefreshWorldBoundsFromLoadedChunks();
				m_chunkManager.BuildWorldMask();
				m_lastSeenWorldRevision = m_chunkManager.GetWorldRevision();
			}

			// --- DDA static raycast ---
			const int startTileX = static_cast<int>(std::floor(m_lmbDragStart.x / tileSize));
			const int startTileY = static_cast<int>(std::floor(m_lmbDragStart.y / tileSize));

			RaycastHit startCellHit = MakeStartCellHit(startTileX, startTileY, m_lmbDragStart);
			const bool startSolid = startCellHit.hit;

			std::vector<std::pair<int, int>> visitedCellsTemp;
			std::vector<std::pair<int, int>>* visitedOut = m_visualDebug ? &visitedCellsTemp : nullptr;

			RaycastHit staticIgnoreHit;
			{
				std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex());
				staticIgnoreHit =
					RaycastWorldMaskDDA(m_lmbDragStart, dir, m_chunkManager.worldMask, m_chunkManager.worldWidth,
										m_chunkManager.worldHeight, m_chunkManager.worldOffsetX,
										m_chunkManager.worldOffsetY, tileSize, dragLen, startSolid, visitedOut);
			}

			if (m_visualDebug) {
				if (visitedCellsTemp.size() > 1024)
					visitedCellsTemp.resize(1024);
				m_visitedCells = std::move(visitedCellsTemp);
			}

			// --- Dynamic raycast using unified spatial index ---
			RaycastHit entityHit;
			Entity* hitEntity = nullptr;

			auto* si = GetEntityManager().GetSpatialIndex();
			bool hitDynamic = false;

			if (si) {
				hitDynamic = si->RaycastEntities(m_lmbDragStart, dir, dragLen, entityHit, hitEntity);
			}

			m_highlightedEntity = hitDynamic ? hitEntity : nullptr;

			// --- Merge static + dynamic ---
			RaycastHit staticHit = startSolid ? startCellHit : staticIgnoreHit;

			RaycastHit finalHit;
			if (hitDynamic && staticHit.hit)
				finalHit = (entityHit.distance <= staticHit.distance) ? entityHit : staticHit;
			else if (hitDynamic)
				finalHit = entityHit;
			else
				finalHit = staticHit;

			// --- Visualize final hit ---
			if (finalHit.hit) {
				Vec2 hitPos = finalHit.position;

				float proj = (hitPos.x - m_lmbDragStart.x) * dir.x + (hitPos.y - m_lmbDragStart.y) * dir.y;

				if (proj > dragLen)
					hitPos = m_lmbDragStart + dir * dragLen;

				// Ray line
				m_debugLines.push_back({m_lmbDragStart, hitPos});
				m_debugLineColors.push_back(sf::Color::Green);

				// STATIC hit (yellow)
				if (staticHit.hit)
					m_debugPoints.push_back(staticHit.position);

				// DYNAMIC hit (green)
				if (hitDynamic)
					m_dynamicHitPoints.push_back(entityHit.position);

				// RAW static hit (blue)
				if (staticHit.hit && !hitDynamic)
					m_rawHitPoints.push_back(staticHit.position);
			} else {
				// Miss line
				m_debugLines.push_back({m_lmbDragStart, m_lmbDragEnd});
				m_debugLineColors.push_back(sf::Color::Red);

				// STATIC miss point (yellow)
				m_debugPoints.push_back(m_lmbDragEnd);
			}

			m_previewActive = false;
		}
	}
	// --- Drag preview update ---
	else if (leftMouseDown && m_lmbdragging) {
		m_lmbDragEnd = mouseWorld;

		if (m_visualDebug)
			m_previewLine = {m_lmbDragStart, m_lmbDragEnd};
	}

	m_prevLmbMouseDown = leftMouseDown;
}
/////////////////////////////////



/////////////////////////////////
// ProcessEscapeKey - handles input to close the window when any key is pressed
void RayCastScene::ProcessEscapeKey(bool keyDown) const {
	if (keyDown) {
		m_gameEngine.ChangeScene("MainMenu");
	}
}
/////////////////////////////////



/////////////////////////////////
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



/////////////////////////////////
// Update - handles events, updates the entity manager, and prepares debug visualization data for rendering
void RayCastScene::Update(float deltaTime) {
	GetEntityManager().Update(deltaTime);
	// Update camera system and apply main camera view
	if (m_cameraEntity)
		m_cameraSystem.Update(deltaTime, GetEntityManager());
	ApplyMainCameraView();

	// Ensure chunks are loaded for the current camera view
	const float tileSize = m_chunkManager.GetTileSize();
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt)
		return;
	CCamera* cam = *camOpt;
	
	// Compute camera bounds in world space
	const Vec2 cameraCenter = cam->position;
	const float safeZoom = (cam->zoom > 0.0001f) ? cam->zoom : 1.0f;
	const float worldW = cam->viewportWidth / safeZoom;
	const float worldH = cam->viewportHeight / safeZoom;
	const float halfW = worldW * 0.5f;
	const float halfH = worldH * 0.5f;

	// Compute tile coordinates for the camera bounds
	int tx0 = static_cast<int>(std::floor((cameraCenter.x - halfW) / tileSize));
	int ty0 = static_cast<int>(std::floor((cameraCenter.y - halfH) / tileSize));
	int tx1 = static_cast<int>(std::floor((cameraCenter.x + halfW) / tileSize));
	int ty1 = static_cast<int>(std::floor((cameraCenter.y + halfH) / tileSize));
	
	// Ensure chunks are loaded for the camera bounds with a margin of 1 chunk
	m_chunkManager.EnsureChunksInTileRect(tx0, ty0, tx1, ty1, 1);
	m_chunkManager.UpdateMainThread_NoLock();

	// Refresh raycast world data only when chunks changed or camera teleported far enough to imply a new area jump.
	bool needsRayRefresh = false;
	const uint64_t worldRevision = m_chunkManager.GetWorldRevision();
	if (worldRevision != m_lastSeenWorldRevision)
		needsRayRefresh = true;

	// Check if camera has moved far enough to require a raycast refresh (beyond 4 chunks away).
	// If we have a last view center, check if the camera has moved beyond the teleport threshold.
	const float teleportThreshold = m_chunkManager.GetTileSize() * static_cast<float>(m_chunkManager.GetChunkWidth() * 4);
	if (m_hasLastViewCenter) {
		const float dx = cameraCenter.x - m_lastViewCenter.x;
		const float dy = cameraCenter.y - m_lastViewCenter.y;
		if ((dx * dx + dy * dy) > (teleportThreshold * teleportThreshold))
			needsRayRefresh = true;
	}

	// Update last view center and seen world revision
	m_lastViewCenter = cameraCenter;
	m_hasLastViewCenter = true;


	// If needed, refresh the world bounds and world mask for raycasting.
	if (needsRayRefresh) {
		m_chunkManager.RefreshWorldBoundsFromLoadedChunks();
		m_chunkManager.BuildWorldMask();
		m_lastSeenWorldRevision = m_chunkManager.GetWorldRevision();
	}

	// Event Handling (SFML 3.0: pollEvent returns std::optional<sf::Event>)
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
	
	// Convert to Vec2 for our internal representation
	Vec2 mouseWorld(static_cast<float>(mouseWorldF.x), static_cast<float>(mouseWorldF.y));
	const bool leftMouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	
	// Handle left mouse drag for raycasting and debug visualization
	ProcessMouseDragRaycast(leftMouseDown, mouseWorld);

	// Show level picker UI like the level editor (read-only selection in this scene).
	LevelManagerWindow();
}
/////////////////////////////////



/////////////////////////////////
// Render - draws the tile grid and any debug visualization overlays
void RayCastScene::Render() {

		//
		// ────────────────────────────────
		// 1. SET WORLD VIEW
		// ────────────────────────────────
		//
		// Update m_worldView from camera (but don't apply yet)
		ApplyMainCameraView();
		// Now apply the world view for all world-space rendering
		m_window.setView(m_worldView);

		//
		// ────────────────────────────────
		// 2. WORLD SPACE RENDERING
		// ────────────────────────────────
		//

		// Dynamic + static entity shapes
		GetEntityManager().RenderShapes();

		// Raycast debug
		DrawDebugLines();
		DrawHitPoints();
		DrawRawHitPoints();
		DrawVisitedCells();
		DrawPreviewLine();
		DrawHighlightedEntity(m_window);

		// BVH debug
		if (m_visualDebug) {
			for (auto& [key, chunk] : m_chunkManager.GetChunks()) {
				if (chunk.dynamicBVH.GetRoot())
					DrawBVHNode(m_window, chunk.dynamicBVH.GetRoot());
			}

			for (const auto& traversal : m_debugTraversals)
				DrawBVHTraversal(traversal);

			for (const auto& p : m_dynamicHitPoints)
				DrawBVHHitPoint(p);
		}

		// Tile grid
		DrawTileGrid();

		// Flush world-space queue
		GetEngineRenderQueue().Flush(m_window);

		//
		// ────────────────────────────────
		// 3. SWITCH TO UI VIEW
		// ────────────────────────────────
		//
		m_window.setView(m_window.getDefaultView());

		//
		// ────────────────────────────────
		// 4. UI SPACE RENDERING
		// ────────────────────────────────
		//
		GetEntityManager().RenderText();

		//
		// ────────────────────────────────
		// 5. RESTORE WORLD VIEW FOR GAMEENGINE
		// ────────────────────────────────
		//
		// Restore world view so GameEngine's RenderShapes() call uses world space
		m_window.setView(m_worldView);

		// Clear traversal debug
		m_debugTraversals.clear();
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
// DrawBVHHitPoint - draws a green circle at the hit point of a BVH traversal, only if visual debug mode is enabled. This helps visualize where the ray intersected with the BVH.
void RayCastScene::DrawBVHHitPoint(const Vec2& p) {
	sf::CircleShape dot;
	dot.setRadius(4.f);
	dot.setOrigin({4.f, 4.f});
	dot.setPosition({p.x, p.y});
	dot.setFillColor(sf::Color::Green);

	m_window.draw(dot);
}
/////////////////////////////////



/////////////////////////////////
// DrawBVHLeafNode - draws a green rectangle around the bounding box of a BVH leaf node, only if visual debug mode is enabled. This helps visualize which leaf node was hit during a raycast traversal.
void RayCastScene::DrawBVHLeafNode(BVHNode* node) {
	if (!node)
		return;

	sf::RectangleShape rect;
	rect.setPosition({node->bounds.position.x, node->bounds.position.y});
	rect.setSize(sf::Vector2f(node->bounds.size.x, node->bounds.size.y));
	rect.setFillColor(sf::Color::Transparent);
	rect.setOutlineColor(sf::Color::Green);
	rect.setOutlineThickness(2.f);

	m_window.draw(rect);
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
void RayCastScene::DrawBVHNodeColored(BVHNode* node, const sf::Color& color) {
	sf::RectangleShape box;

	box.setPosition(node->bounds.position);	
	box.setSize(sf::Vector2f(node->bounds.size.x, node->bounds.size.y));

	box.setFillColor(sf::Color(0, 0, 0, 0)); // Transparent fill
	box.setOutlineColor(color);				 // Outline color based on the parameter
	box.setOutlineThickness(1.f);


	m_window.draw(box);
}
/////////////////////////////////



/////////////////////////////////
// DrawBVHTraversal - draws the path of BVH nodes visited during a raycast traversal. Visited nodes are drawn in yellow, and the final hit leaf node (if any) is drawn in green.
void RayCastScene::DrawBVHTraversal(const BVHDebugTraversal& traversal) {
	// Draw the traversal path in yellow
	for (BVHNode* node : traversal.visited) {
		DrawBVHNodeColored(node, sf::Color::Yellow);
	}

	// Draw the final hit node in green
	if (traversal.hitLeaf) {
		DrawBVHNodeColored(traversal.hitLeaf, sf::Color::Green);
	}
}
/////////////////////////////////



/////////////////////////////////
// OnEnter - currently empty, but could be used for setup logic that needs to run when the scene becomes active
void RayCastScene::OnEnter() {
	std::cout << "RayCastScene::OnEnter\n";

	auto& em = GetEntityManager();

	// -------------------------
	// Recreate camera (GameEngine clears all entities on scene switch)
	// -------------------------
	sf::Vector2u windowSize = m_window.getSize();
	m_cameraEntity = em.AddEntity(EntityType::Default);
	auto camera = m_cameraEntity->AddComponent<CCamera>(Vec2(0, 0), 1.0f);
	camera->isMainCamera = true;
	camera->isActive = true;
	camera->viewportWidth = static_cast<float>(windowSize.x);
	camera->viewportHeight = static_cast<float>(windowSize.y);
	camera->smoothness = 0.0f;

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

	em.ProcessPending(); // ensure entities are in m_entities
	//em.UpdateBVH();		 // build BVH from current entities
	//std::cout << "EntityManager BVH root: " << em.GetBVH().GetRoot() << "\n";

	// -------------------------
	// Refresh map bounds from loaded chunks
	// -------------------------
	std::cout << "Calling RefreshMapBounds()...\n";
	RefreshMapBounds();
	std::cout << "m_haveBounds: " << m_haveBounds << "\n";
	std::cout << "Map bounds: (" << m_mapMin.x << ", " << m_mapMin.y << ") to (" << m_mapMax.x << ", " << m_mapMax.y << ")\n";

	// -------------------------
	// Position camera to top-left of level
	// -------------------------
	if (m_cameraEntity) {
		if (auto cam = m_cameraEntity->GetComponent<CCamera>()) {
			// Get viewport size for proper offset
			float halfWidth = cam->viewportWidth * 0.5f;
			float halfHeight = cam->viewportHeight * 0.5f;

			// Position camera so top-left of viewport is at top-left of map bounds
			Vec2 topLeft = m_haveBounds ? m_mapMin : Vec2(0, 0);
			cam->position = Vec2(topLeft.x + halfWidth, topLeft.y + halfHeight);

			std::cout << "Viewport size: " << cam->viewportWidth << " x " << cam->viewportHeight << "\n";
			std::cout << "Top-left target: (" << topLeft.x << ", " << topLeft.y << ")\n";
			std::cout << "Camera positioned to: (" << cam->position.x << ", " << cam->position.y << ")\n";
		}
	} else {
		std::cout << "WARNING: m_cameraEntity is null!\n";
	}
}
/////////////////////////////////



/////////////////////////////////
// OnExit - cleanup logic that runs when the scene is no longer active
void RayCastScene::OnExit() {
	std::cout << "RayCastScene::OnExit - Cleaning up resources\n";

	// Clear debug visualization data
	m_debugLines.clear();
	m_debugLineColors.clear();
	m_debugPoints.clear();
	m_dynamicHitPoints.clear();
	m_rawHitPoints.clear();
	m_debugTraversals.clear();
	m_visitedCells.clear();

	// Clear entity references
	m_highlightedEntity = nullptr;
	m_cameraEntity = nullptr;

	// Clear level data
	m_availableLevels.clear();
	m_currentLevelName.clear();
	m_selectedLevelIndex = -1;

	// Reset state flags
	m_lmbdragging = false;
	m_previewActive = false;
	m_visualDebug = false;
	m_prevDebugKeyDown = false;
	m_prevLmbMouseDown = false;
	m_hasLastViewCenter = false;

	std::cout << "RayCastScene::OnExit - Cleanup complete\n";
}
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

    sf::View v;
    v.setSize({cam->viewportWidth, cam->viewportHeight});
	v.setCenter({cam->position.x, cam->position.y});

    // Store it, but DO NOT apply it here
    m_worldView = v;
}
/////////////////////////////////



/////////////////////////////////
// LoadResources - currently empty, but could be used to load textures, sounds, or other resources needed by the scene. In this example, we load the tile map in 
// InitializeGame instead, but we could also move it here if we wanted to separate resource loading from game initialization.
void RayCastScene::LoadResources() {}
/////////////////////////////////



/////////////////////////////////
// UnloadResources - free textures, sounds, fonts, or other resources when the scene is unloaded
void RayCastScene::UnloadResources() {
	std::cout << "RayCastScene::UnloadResources - Unloading fonts\n";

	// Unload fonts that were loaded for this scene
	// Note: FontManager is managed by GameEngine, but we can remove our specific fonts
	// if FontManager has an UnloadFont method. For now, just log the intent.
	// The fonts will be cleaned up when switching scenes or when the engine shuts down.

	std::cout << "RayCastScene::UnloadResources - Complete\n";
}
/////////////////////////////////



///////////////////////////////
// InitializeGame - load chunked level data and create a raycast snapshot from collision layer 1.
void RayCastScene::InitialiseGame(sf::Vector2u /*windowSize*/) {
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
			// Position camera to top-left of level (same as OnEnter)
			float halfWidth = cam->viewportWidth * 0.5f;
			float halfHeight = cam->viewportHeight * 0.5f;
			Vec2 topLeft = m_haveBounds ? m_mapMin : Vec2(0, 0);
			cam->position = Vec2(topLeft.x + halfWidth, topLeft.y + halfHeight);
			m_lastViewCenter = cam->position;
			m_hasLastViewCenter = true;
			std::cout << "InitializeGame: Camera positioned to top-left: (" << cam->position.x << ", " << cam->position.y << ")\n";
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
	fontEntity->AddComponent<CTransform>(Vec2(50.0f, 50.0f), Vec2::Zero);
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