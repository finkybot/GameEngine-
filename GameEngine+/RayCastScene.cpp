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
/////////////////////////////////



/////////////////////////////////
// Constructor - initializes the ray cast scene with a reference to the game engine and the render window, and sets up the entity manager
RayCastScene::RayCastScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager)
	: Scene(engine, entityManager), m_window(win), m_chunkManager(32, 32, 32.0f, 3), m_raycastMap(0, 0, 32.0f) {}
/////////////////////////////////



/////////////////////////////////
// Destructor - defaulted since we don't have any special cleanup logic, but we could add it if needed in the future
RayCastScene::~RayCastScene() = default;
/////////////////////////////////



///////////////////////////////
// RefreshAvailableLevels - scans the same APPDATA levels directory used by the level editor.
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
	m_chunkManager.ClearAllLoadedChunks();
	m_chunkManager.LoadAllSavedChunks();
	for (int i = 0; i < 10; ++i) m_chunkManager.UpdateMainThread_NoLock();
	m_chunkManager.RebuildAllChunksFromTileset();
	m_chunkManager.UpdateMainThread_NoLock();
	RebuildRaycastSnapshotFromChunks();
	m_chunkManager.BuildWorldMask();
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
bool RayCastScene::RaycastDynamicEntities(const Vec2& origin, const Vec2& dir, float maxDistance, RaycastHit& outHit,
													  Entity*& outEntity) {
	outEntity = nullptr;
	outHit = RaycastHit{};
	if (maxDistance <= 0.0f)
		return false;

	Vec2 dirN = dir.GetUnitVec();
	if (dirN.Mag2() <= 1e-12f)
		return false;

	std::vector<Entity*> candidates;
	m_entityManager.GetSpatialHash().Query(candidates, origin, maxDistance + m_chunkManager.GetTileSize());
	if (candidates.empty())
		return false;

	float nearest = maxDistance;
	for (Entity* e : candidates) {
		if (!e || !e->IsAlive())
			continue;

		CShape* shape = e->GetShape();
		if (!shape)
			continue;

		sf::Shape& sfShape = shape->GetShape();
		const sf::FloatRect b = sfShape.getGlobalBounds();
		if (b.size.x <= 0.0f || b.size.y <= 0.0f)
			continue;

		float hitDist = 0.0f;
		if (!RayIntersectsAABB(origin, dirN, Vec2(b.position.x, b.position.y),
								 Vec2(b.position.x + b.size.x, b.position.y + b.size.y), hitDist, nearest)) {
			continue;
		}

		if (hitDist < 0.0f || hitDist > nearest)
			continue;

		nearest = hitDist;
		outEntity = e;
		outHit.hit = true;
		outHit.tileX = -1;
		outHit.tileY = -1;
		outHit.tileValue = 0;
		outHit.distance = hitDist;
		outHit.position = Vec2(origin.x + dirN.x * hitDist, origin.y + dirN.y * hitDist);

		Vec2 center(b.position.x + b.size.x * 0.5f, b.position.y + b.size.y * 0.5f);
		Vec2 delta = outHit.position - center;
		float nx = (b.size.x > 0.0f) ? (delta.x / (b.size.x * 0.5f)) : 0.0f;
		float ny = (b.size.y > 0.0f) ? (delta.y / (b.size.y * 0.5f)) : 0.0f;
		if (std::fabs(nx) > std::fabs(ny))
			outHit.normal = Vec2((nx > 0.0f) ? 1.0f : -1.0f, 0.0f);
		else
			outHit.normal = Vec2(0.0f, (ny > 0.0f) ? 1.0f : -1.0f);
	}

	return outEntity != nullptr;
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
		// To prevent unnecessary raycasts, only perform raycast if we were dragging, otherwise it was just a click without movement, and we will handle that case separately to toggle tile state. We get 
		// the magnitude of the drag and if it's very small, we treat it as a click rather than a drag.
		if (m_lmbdragging) {
			m_lmbdragging = false;
			m_lmbDragEnd = mouseWorld;
			Vec2 dir = m_lmbDragEnd - m_lmbDragStart;
			float dragLen = dir.Mag();

			// If the drag length is very small, we can treat it as a click to toggle a tile solid/not solid state.
			// I'm using a small threshold of 0.001 units.
			if (dragLen <= 0.001f) {
				m_previewActive = false;
				m_prevLmbMouseDown = leftMouseDown;
				return;
			}

			// Normalize the direction vector for raycasting. Use the original drag length to clamp the raycast distance, but the direction needs to be a unit vector for the DDA algorithm.
			dir = dir.GetUnitVec();

			// Clear previous debug lines and points to ensure that our debug visualization is accurate and up-to-date with the latest raycast.
			m_debugLines.clear();
			m_debugLineColors.clear();
			m_debugPoints.clear();

			// Raycast directly against chunk world mask (static geometry) and then test dynamic entities; keep nearest hit.
			const float tileSize = m_chunkManager.GetTileSize();
			const int startTileX = static_cast<int>(std::floor(m_lmbDragStart.x / tileSize));
			const int startTileY = static_cast<int>(std::floor(m_lmbDragStart.y / tileSize));

			RaycastHit rayHitStartCell = MakeStartCellHit(startTileX, startTileY, m_lmbDragStart);

			std::vector<uint8_t> worldMaskCopy;
			int worldWidth = 0;
			int worldHeight = 0;
			int worldOffsetX = 0;
			int worldOffsetY = 0;
			{
				std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex());
				worldMaskCopy = m_chunkManager.worldMask;
				worldWidth = m_chunkManager.worldWidth;
				worldHeight = m_chunkManager.worldHeight;
				worldOffsetX = m_chunkManager.worldOffsetX;
				worldOffsetY = m_chunkManager.worldOffsetY;
			}

			const bool startSolid = rayHitStartCell.hit;

			m_visitedCells.clear();
			RaycastHit rayHitIgnore = RaycastWorldMaskDDA(
				m_lmbDragStart, dir, worldMaskCopy, worldWidth, worldHeight, worldOffsetX, worldOffsetY, tileSize, dragLen,
				startSolid, &m_visitedCells);

			RaycastHit entityHit;
			Entity* hitEntity = nullptr;
			const bool hitDynamic = RaycastDynamicEntities(m_lmbDragStart, dir, dragLen, entityHit, hitEntity);

			// m_visitedCells is now produced by RaycastWorldMaskDDA, so no secondary sampling pass is needed.

			// Determine which hit to use for visualization. If the ray starts inside a solid tile, we will use the synthetic hit at the start position for visualization, but we will also check if 
			// the DDA reported a different hit further along the ray. If it did, we will draw a line to that hit as well to show the exit point from the wall. If the ray starts in an empty tile, 
			// we will just use the DDA hit as normal.
			RaycastHit staticHit;
			if (startSolid) {
				staticHit = rayHitStartCell;
				if (rayHitIgnore.hit &&
					(rayHitIgnore.tileX != rayHitStartCell.tileX || rayHitIgnore.tileY != rayHitStartCell.tileY)) {
					m_debugLines.push_back({m_lmbDragStart, rayHitIgnore.position});
					m_debugLineColors.push_back(sf::Color::Green);
					m_rawHitPoints.push_back(rayHitIgnore.position);
				}
			} else {
				staticHit = rayHitIgnore;
			}

			RaycastHit rayHit;
			if (hitDynamic && staticHit.hit)
				rayHit = (entityHit.distance <= staticHit.distance) ? entityHit : staticHit;
			else if (hitDynamic)
				rayHit = entityHit;
			else
				rayHit = staticHit;

			// For visualization, we will draw a line from the drag start to the hit position. If there was a hit, we will clamp the hit position to the ray length in case it exceeds it (which can 
			// happen if the ray starts inside a solid tile and the DDA reports a hit at the boundary). We will also draw a point at the hit position. If there was no hit, we will draw a line to the drag 
			// end position and a point there instead. This allows us to visualize the result of the raycast and see where it hit or where it ended if it didn't hit anything.
			if (rayHit.hit) {
				Vec2 hitPos = rayHit.position;
				float proj = (hitPos.x - m_lmbDragStart.x) * dir.x + (hitPos.y - m_lmbDragStart.y) * dir.y;
				if (proj > dragLen) {
					hitPos = Vec2(m_lmbDragStart.x + dir.x * dragLen, m_lmbDragStart.y + dir.y * dragLen);
				}
				m_debugLines.push_back({m_lmbDragStart, hitPos});
				m_debugLineColors.push_back(sf::Color::Green);
				m_debugPoints.push_back(hitPos);
				m_rawHitPoints.push_back(rayHit.position);
			}
			// Otherwise, if there was no hit, we will draw a line to the drag end position and a point there instead. This allows us to visualize the result of the raycast and see where it hit or where 
			// it ended if it didn't hit anything.
			else {
				m_debugLines.push_back({m_lmbDragStart, m_lmbDragEnd});
				m_debugLineColors.push_back(sf::Color::Red);
				m_debugPoints.push_back(m_lmbDragEnd);
			}

			m_previewActive = false;
		}
	}
	// Only update the preview line if we are currently dragging with the left mouse button. This allows us to show a real-time preview of the raycast as we drag, which can be helpful for aiming and 
	// visualizing where the ray will go before we release the mouse button to perform the actual raycast.
	else if (leftMouseDown && m_lmbdragging) {
		m_lmbDragEnd = mouseWorld;
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
	const bool middleDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle);
	const sf::Vector2i mousePos = sf::Mouse::getPosition(m_window);

	if (middleDown && !m_panning) {
		m_panning = true;
		m_panStart = mousePos;
		m_viewPanStart = Vec2(m_window.getView().getCenter().x, m_window.getView().getCenter().y);
	}
	if (!middleDown && m_panning) {
		m_panning = false;
	}
	if (!m_panning)
		return;

	sf::Vector2i delta = mousePos - m_panStart;
	Vec2 newCenter = m_viewPanStart - Vec2(static_cast<float>(delta.x), static_cast<float>(delta.y));

	// Clamp to currently loaded map bounds if available.
	if (m_raycastMap.width > 0 && m_raycastMap.height > 0) {
		const sf::View view = m_window.getView();
		const float halfW = view.getSize().x * 0.5f;
		const float halfH = view.getSize().y * 0.5f;
		const float tile = m_chunkManager.GetTileSize();
		const float minX = static_cast<float>(m_raycastOffsetX) * tile;
		const float minY = static_cast<float>(m_raycastOffsetY) * tile;
		const float maxX = static_cast<float>(m_raycastOffsetX + m_raycastMap.width) * tile;
		const float maxY = static_cast<float>(m_raycastOffsetY + m_raycastMap.height) * tile;
		const float minCx = minX + halfW;
		const float maxCx = maxX - halfW;
		const float minCy = minY + halfH;
		const float maxCy = maxY - halfH;
		if (minCx <= maxCx)
			newCenter.x = std::clamp(newCenter.x, minCx, maxCx);
		else
			newCenter.x = (minX + maxX) * 0.5f;
		if (minCy <= maxCy)
			newCenter.y = std::clamp(newCenter.y, minCy, maxCy);
		else
			newCenter.y = (minY + maxY) * 0.5f;
	}

	sf::View newView = m_window.getView();
	newView.setCenter(sf::Vector2f(newCenter.x, newCenter.y));
	m_window.setView(newView);
}
/////////////////////////////////



///////////////////////////////
// Update - handles events, updates the entity manager, and prepares debug visualization data for rendering
void RayCastScene::Update(float /*deltaTime*/) {
	// Keep chunk data streaming/ready for rendering.
	const sf::View view = m_window.getView();
	const sf::Vector2f center = view.getCenter();
	const sf::Vector2f size = view.getSize();
	const float halfW = size.x * 0.5f;
	const float halfH = size.y * 0.5f;
	const float tileSize = m_chunkManager.GetTileSize();

	int tx0 = static_cast<int>(std::floor((center.x - halfW) / tileSize));
	int ty0 = static_cast<int>(std::floor((center.y - halfH) / tileSize));
	int tx1 = static_cast<int>(std::floor((center.x + halfW) / tileSize));
	int ty1 = static_cast<int>(std::floor((center.y + halfH) / tileSize));
	m_chunkManager.EnsureChunksInTileRect(tx0, ty0, tx1, ty1, 1);
	m_chunkManager.UpdateMainThread_NoLock();

	// Refresh raycast snapshot/bounds from current chunk data (layer 1 collision layer), then rebuild world mask.
	RebuildRaycastSnapshotFromChunks();
	m_chunkManager.BuildWorldMask();

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

	// Show level picker UI like the level editor (read-only selection in this scene).
	LevelManagerWindow();
	ProcessMiddleMousePan();
}
/////////////////////////////////



/////////////////////////////////
// Render - draws the tile grid and any debug visualization overlays
void RayCastScene::Render() {
	// draw tile grid (solid tiles)
	DrawTileGrid();
	DrawDebugLines();
	DrawHitPoints();
	DrawRawHitPoints();
	DrawVisitedCells();
	DrawPreviewLine();

	// Text is rendered by the RenderSystem during EntityManager::Update; no per-scene text draw here to avoid double-rendering.
}



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
// HandleEvent - now delegates to input helpers to keep logic centralized
void RayCastScene::HandleEvent(const std::optional<sf::Event>& event) {
	//(void)event;
	// get mouse pixel coords then convert to world coords using the window's view
	sf::Vector2i mousePixelPos = sf::Mouse::getPosition(m_window);
	sf::Vector2f mouseWorldF = m_window.mapPixelToCoords(mousePixelPos);
	Vec2 mouseWorld(static_cast<float>(mouseWorldF.x), static_cast<float>(mouseWorldF.y));

	bool leftMouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	bool escapeKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
	bool debugToggle = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);

	ProcessDebugToggle(debugToggle);
	ProcessMouseDragRaycast(leftMouseDown, mouseWorld);
	ProcessEscapeKey(escapeKeyDown);
}
/////////////////////////////////



/////////////////////////////////
// OnEnter - currently empty, but could be used for setup logic that needs to run when the scene becomes active
void RayCastScene::OnEnter() {}
/////////////////////////////////



/////////////////////////////////
// OnExit - currently empty, but could be used for cleanup logic that needs to run when the scene is no longer active
void RayCastScene::OnExit() {}
/////////////////////////////////



/////////////////////////////////
void RayCastScene::OnWindowResized(sf::Vector2u newSize) {
	sf::View view;
	view.setCenter(sf::Vector2f(newSize.x * 0.5f, newSize.y * 0.5f));
	view.setSize(sf::Vector2f(newSize.x, newSize.y));
	m_window.setView(view);
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

	// Build temporary dense TileMap snapshot from currently loaded chunks, then build world mask using those bounds.
	RebuildRaycastSnapshotFromChunks();
	m_chunkManager.BuildWorldMask();

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
// Build a temporary dense TileMap snapshot from currently loaded chunks for DDA raycast compatibility.
void RayCastScene::RebuildRaycastSnapshotFromChunks() {
	std::lock_guard<std::mutex> lock(m_chunkManager.GetMutex());
	auto& chunks = m_chunkManager.GetChunks();
	if (chunks.empty()) {
		m_raycastMap = TileMap(0, 0, m_chunkManager.GetTileSize());
		m_raycastOffsetX = 0;
		m_raycastOffsetY = 0;
		m_chunkManager.SetWorldOffset(0, 0);
		m_chunkManager.SetWorldSize(0, 0);
		return;
	}

	int minTx = std::numeric_limits<int>::max();
	int minTy = std::numeric_limits<int>::max();
	int maxTx = std::numeric_limits<int>::min();
	int maxTy = std::numeric_limits<int>::min();

	for (const auto& kv : chunks) {
		const Chunk& c = kv.second;
		const int x0 = c.chunkX * c.width;
		const int y0 = c.chunkY * c.height;
		const int x1 = x0 + c.width - 1;
		const int y1 = y0 + c.height - 1;
		minTx = std::min(minTx, x0);
		minTy = std::min(minTy, y0);
		maxTx = std::max(maxTx, x1);
		maxTy = std::max(maxTy, y1);
	}

	if (minTx > maxTx || minTy > maxTy) {
		m_raycastMap = TileMap(0, 0, m_chunkManager.GetTileSize());
		m_raycastOffsetX = 0;
		m_raycastOffsetY = 0;
		m_chunkManager.SetWorldOffset(0, 0);
		m_chunkManager.SetWorldSize(0, 0);
		return;
	}

	const int width = maxTx - minTx + 1;
	const int height = maxTy - minTy + 1;
	m_raycastOffsetX = minTx;
	m_raycastOffsetY = minTy;
	m_chunkManager.SetWorldOffset(minTx, minTy);
	m_chunkManager.SetWorldSize(width, height);
	m_raycastMap = TileMap(width, height, m_chunkManager.GetTileSize());

	for (const auto& kv : chunks) {
		const Chunk& c = kv.second;
		if (m_collisionLayer < 0 || m_collisionLayer >= c.numLayers) continue;
		const int baseX = c.chunkX * c.width;
		const int baseY = c.chunkY * c.height;
		for (int ly = 0; ly < c.height; ++ly) {
			for (int lx = 0; lx < c.width; ++lx) {
				const int localIdx = ly * c.width + lx;
				const int val = c.tilesPerLayer[m_collisionLayer][localIdx];
				if (val == 0) continue;
				const int sx = baseX + lx - minTx;
				const int sy = baseY + ly - minTy;
				if (m_raycastMap.InBounds(sx, sy)) m_raycastMap.SetTile(sx, sy, val);
			}
		}
	}
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