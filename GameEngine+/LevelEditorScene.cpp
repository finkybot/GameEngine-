/////////////////////////////////
// LevelEditor.cpp - simple (simple???? wtf is simple about it?????) chunked level editor that uses ChunkManager and CameraSystem
/////////////////////////////////



/////////////////////////////////
// Includes and namespace aliases for the LevelEditorScene implementation. We include necessary headers for the scene, chunk manager, game engine, 
// entity management, and SFML graphics, as well as ImGui for UI rendering and filesystem for file operations.
#include "LevelEditorScene.h"
#include "GameEngine.h"
#include "Entity.h"
#include "CTransform.h"
#include "CCamera.h"
#include <iostream>
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
#include <imgui/imgui_internal.h>
#include <filesystem>
#include <mutex>
#include <cstdlib>
#include <fstream>
namespace fs = std::filesystem;
/////////////////////////////////



/////////////////////////////////
// Constructor - initializes the level editor scene with references to the game engine, render window, and entity manager, and sets up the 
// chunk manager with specified chunk dimensions and tile size
LevelEditorScene::LevelEditorScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& em)
	: Scene(engine, em), m_window(win), m_chunkManager(32, 32, 32.0f) {
}
/////////////////////////////////



/////////////////////////////////
// RefreshAvailableLevels - scan "levels" directory for subfolders and populate m_availableLevels
void LevelEditorScene::RefreshAvailableLevels() {
	m_availableLevels.clear();
	std::error_code ec;
	// Use per-user app data folder for installed builds so levels are user-writable and easy to manage
	fs::path base;
	// Use secure getenv alternative on MSVC
#ifdef _MSC_VER
	char* envBuf = nullptr;
	size_t len = 0;
	errno_t er = _dupenv_s(&envBuf, &len, "APPDATA");
	if (er == 0 && envBuf && envBuf[0] != '\0') {
		base = fs::path(envBuf) / "GameEnginePlus" / "levels";
	} else {
		base = fs::path("levels");
	}
	free(envBuf);
#else
	const char* appdata = std::getenv("APPDATA");
	if (appdata && appdata[0] != '\0') base = fs::path(appdata) / "GameEnginePlus" / "levels";
	else base = fs::path("levels");
#endif
	if (!fs::exists(base, ec)) return;
	for (auto it = fs::directory_iterator(base); it != fs::directory_iterator(); ++it) {
		try {
			auto &p = *it;
			if (!p.is_directory()) continue;
			m_availableLevels.push_back(p.path().filename().string());
		} catch(...) { continue; }
	}
}
/////////////////////////////////



/////////////////////////////////
// SwitchToLevel - change the working level folder. Saves current chunks, sets new base path, and reloads chunks for that level.
void LevelEditorScene::SwitchToLevel(const std::string& name) {
	// Save current and clear loaded chunks
	m_chunkManager.SaveAllChunks();
	m_chunkManager.ClearAllLoadedChunks();
	m_currentLevelName = name;
	m_levelSelected = !m_currentLevelName.empty();
	// Update chunk manager path
	// Use same APPDATA-based path as RefreshAvailableLevels
	// Use secure getenv alternative on MSVC
	fs::path base;
#ifdef _MSC_VER
	char* envBuf = nullptr;
	size_t len = 0;
	errno_t er = _dupenv_s(&envBuf, &len, "APPDATA");
	if (er == 0 && envBuf && envBuf[0] != '\0') base = fs::path(envBuf) / "GameEnginePlus" / "levels";
	else base = fs::path("levels");
	free(envBuf);
#else
	const char* appdata = std::getenv("APPDATA");
	if (appdata && appdata[0] != '\0') base = fs::path(appdata) / "GameEnginePlus" / "levels";
	else base = fs::path("levels");
#endif
	if (m_currentLevelName.empty()) {
		m_chunkManager.SetBasePath((base / "chunks").string());
	} else {
		m_chunkManager.SetBasePath((base / m_currentLevelName / "chunks").string());
	}
	// Ensure directory exists
	try {
		if (m_currentLevelName.empty()) fs::create_directories(base / "chunks");
		else fs::create_directories(base / m_currentLevelName / "chunks");
	} catch(...) {}

	// Parse meta.txt to determine available layers and tileset
	std::vector<std::string> parsedLayers = { "background", "main", "upper" };
	{
		auto metaPath = (m_currentLevelName.empty()) ? (base / "meta.txt") : (base / m_currentLevelName / "meta.txt");
		try {
			std::ifstream meta(metaPath.string());
			if (meta) {
				std::string line;
				while (std::getline(meta, line)) {
					// simple key=value parsing
					auto eq = line.find('=');
					if (eq == std::string::npos) continue;
					std::string key = line.substr(0, eq);
					std::string val = line.substr(eq+1);
					if (key == "layers") {
						parsedLayers.clear();
						// comma separated
						size_t start = 0;
						while (start < val.size()) {
							auto comma = val.find(',', start);
							if (comma == std::string::npos) comma = val.size();
							std::string token = val.substr(start, comma - start);
							if (!token.empty()) parsedLayers.push_back(token);
							start = comma + 1;
						}
					}
					else if (key == "tileset") {
						// copy into UI buffer
						ImStrncpy(m_tilesetKeyBuf, val.c_str(), sizeof(m_tilesetKeyBuf));
					}
					else if (key == "tilesetPath") {
						// Restore tileset path for saving later
						m_currentTilesetPath = val;
					}
				}
			}
		} catch(...) {}
	}

	// update editor layer names and inform chunk manager of layer count before loading
	if (!parsedLayers.empty()) {
		m_layerNames = parsedLayers;
		m_activeLayer = std::min(m_activeLayer, (int)m_layerNames.size()-1);
		m_chunkManager.SetNumLayers((int)m_layerNames.size());
	}
	// Keep m_currentDir unchanged so tileset browser still points at assets/user folder.
	fs::path chunkPath = (m_currentLevelName.empty()) ? (base / "chunks") : (base / m_currentLevelName / "chunks");
	std::cout << "SwitchToLevel: '" << name << "' basePath='" << chunkPath.string() << "'\n";
	// Count chunk files for diagnostics
	int fileCount = 0;
	try {
		for (auto it = fs::directory_iterator(chunkPath); it != fs::directory_iterator(); ++it) {
			try {
				auto &e = *it;
				if (!e.is_regular_file()) continue;
				std::string fn = e.path().filename().string();
				if (fn.rfind("chunk_", 0) == 0) ++fileCount;
			} catch(...) { continue; }
		}
	} catch(...) { fileCount = 0; }
	m_exportMessage = std::string("Loading level '") + name + "' - found " + std::to_string(fileCount) + " chunk files in " + chunkPath.string();
	// Load level chunks
	m_chunkManager.LoadAllSavedChunks();
	
	// Shift all chunks so they have positive coordinates (fixes pathfinding issues crossing boundaries)
	m_chunkManager.ShiftChunksToPositiveCoords();
	m_chunkManager.UpdateMainThread();
	m_chunkManager.RebuildAllChunksFromTileset();

	// After loading and shifting, refresh bounds and report
	RefreshMapBounds();
	float dMinX, dMinY, dMaxX, dMaxY;
	if (m_chunkManager.GetSavedChunkBounds(dMinX, dMinY, dMaxX, dMaxY)) {
		int tileSize = (int)m_chunkManager.GetTileSize();

		int minTileX = (int)std::floor(dMinX / tileSize);
		int minTileY = (int)std::floor(dMinY / tileSize);
		int maxTileX = (int)std::ceil(dMaxX / tileSize);
		int maxTileY = (int)std::ceil(dMaxY / tileSize);

		std::cout << "LevelEditorScene::SwitchToLevel: bounds in tiles: minTileX=" << minTileX
				  << " minTileY=" << minTileY << " maxTileX=" << maxTileX << " maxTileY=" << maxTileY << "\n";

		int worldW = maxTileX - minTileX;
		int worldH = maxTileY - minTileY;

		m_chunkManager.SetWorldOffset(minTileX, minTileY); // <-- safe now
		m_chunkManager.SetWorldSize(worldW, worldH); // <-- safe now
	}
}
/////////////////////////////////


/////////////////////////////////
// Destructor - ensures that all chunks are saved to disk when the level editor scene is destroyed, preventing data loss and ensuring that any 
// changes made to the level are preserved
LevelEditorScene::~LevelEditorScene() {
	m_chunkManager.SaveAllChunks();
}
/////////////////////////////////



/////////////////////////////////
// RefreshMapBounds - Recompute m_mapMin/m_mapMax/m_haveBounds from saved chunk files on disk.
// Called after any paint or erase so the camera bounds grow when tiles are added and shrink when they are removed.
void LevelEditorScene::RefreshMapBounds() {
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
// InitializeGame - Initialize the level editor scene by creating a camera entity with a transform and camera component, setting it as the main camera, 
// and configuring the chunk manager with the base path for chunk files and the maximum number of loaded chunks allowed in memory at once. This setup 
// allows the level editor to manage the camera view and efficiently load and save chunks of the level as needed.
void LevelEditorScene::InitializeGame(sf::Vector2u /*windowSize*/) {
	// create camera entity
	m_cameraEntity = GetEntityManager().AddEntity(EntityType::Default);
	m_cameraEntity->AddComponent<CTransform>(Vec2(0, 0), Vec2::Zero);
	auto cam = m_cameraEntity->AddComponent<CCamera>(Vec2(0, 0), 1.0f);
	cam->m_isMainCamera = true;
	cam->m_isActive = true;
	cam->m_viewportWidth = (float)m_window.getSize().x;
	cam->m_viewportHeight = (float)m_window.getSize().y;
	cam->m_smoothness = 0.0f; // Disable smoothing - editor controls camera directly via panning and clamping

	// Ensure initial world area is larger than the screen so the user can pan around.
	// Make the logical map area 3x the screen size centered on the camera.
	{
		float mapPxW = cam->m_viewportWidth * 3.0f;
		float mapPxH = cam->m_viewportHeight * 3.0f;
		
		// store world bounds so camera panning can be clamped
		m_mapMin = Vec2(cam->m_position.x - mapPxW * 0.5f, cam->m_position.y - mapPxH * 0.5f);
		m_mapMax = Vec2(cam->m_position.x + mapPxW * 0.5f, cam->m_position.y + mapPxH * 0.5f);
		int tx0 = (int)std::floor((cam->m_position.x - mapPxW * 0.5f) / m_tileSize);
		int ty0 = (int)std::floor((cam->m_position.y - mapPxH * 0.5f) / m_tileSize);
		int tx1 = (int)std::floor((cam->m_position.x + mapPxW * 0.5f) / m_tileSize);
		int ty1 = (int)std::floor((cam->m_position.y + mapPxH * 0.5f) / m_tileSize);
		m_chunkManager.EnsureChunksInTileRect(tx0, ty0, tx1, ty1, m_marginChunks);
		
		// finalize any background loads immediately so chunks are ready
		m_chunkManager.UpdateMainThread();
		m_chunkManager.RebuildAllChunksFromTileset();
	}

	// set persistence path for chunks
	// Default path points inside the current level folder. If no level selected, do not set a writable path and prevent editing.
	if (!m_levelSelected) {
		// use a dummy path that won't be written to until a level is selected
		m_chunkManager.SetBasePath("");
	} else {
		if (m_currentLevelName.empty()) m_chunkManager.SetBasePath("levels/chunks");
		else m_chunkManager.SetBasePath((fs::path("levels") / m_currentLevelName / "chunks").string());
	}

	m_chunkManager.SetMaxLoadedChunks(256);
	// Load any previously-saved chunk files so saved maps appear on startup
	m_chunkManager.LoadAllSavedChunks();
	// finalize any loads immediately
	m_chunkManager.UpdateMainThread();
	m_chunkManager.RebuildAllChunksFromTileset();

	// Compute fixed map bounds from saved chunk files on disk.
	// These bounds are stable and do not grow as new chunks are loaded at runtime.
	{
		float dMinX, dMinY, dMaxX, dMaxY;
		if (m_chunkManager.GetSavedChunkBounds(dMinX, dMinY, dMaxX, dMaxY)) {
			m_mapMin = Vec2(dMinX, dMinY);
			m_mapMax = Vec2(dMaxX, dMaxY);
			m_haveBounds = true;
		} else {
			// No saved chunks yet — start with the initial 3x screen area as soft bounds.
			m_haveBounds = false;
		}
	}

	// set start folder to assets and do not auto-load any tileset at startup
	std::error_code ec;
	auto assetsPath = std::filesystem::current_path() / std::filesystem::path("assets");
	if (std::filesystem::exists(assetsPath, ec) && !ec) {
		m_currentDir = assetsPath;
	} else {
		m_currentDir = std::filesystem::current_path();
	}

	// ensure no tileset is active at start
	if (sizeof(m_tilesetKeyBuf) > 0) ImStrncpy(m_tilesetKeyBuf, "", sizeof(m_tilesetKeyBuf));
	if (sizeof(m_loadFilenameBuffer) > 0) ImStrncpy(m_loadFilenameBuffer, "", sizeof(m_loadFilenameBuffer));

	// Rebuild any already-loaded chunks so they use the tileset (if available)
	m_chunkManager.RebuildAllChunksFromTileset();

	// Reset all input/selection state so stale drag positions from a previous session don't cause a huge area fill on the very first click after re-entering the scene. 
	// Seed prevLmb/prevRmb from the CURRENT hardware state so that a button which is already physically held when the scene initialises (e.g. the LMB used to click 
	// the "Level Editor" main-menu button) is never treated as a fresh press edge.
	m_prevLmb        = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	m_prevRmb        = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
	m_prevDKey       = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
	m_prevMiddleDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle);
	m_panning        = false;
	
	// If LMB or RMB is already down we must NOT start a selection — wait for a full press/release cycle before allowing any painting.
	m_lmbSelecting   = false;
	m_rmbSelecting   = false;
	sf::Vector2i curMouse = sf::Mouse::getPosition(m_window);
	m_selectLmbStartPx = curMouse;
	m_selectLmbEndPx   = curMouse;
	m_selectRmbStartPx = curMouse;
	m_selectRmbEndPx   = curMouse;
	m_panStart         = curMouse;
	// Block painting until all buttons are physically released after scene entry.
	m_inputReady = false;
}
/////////////////////////////////



/////////////////////////////////
// OnEnter and OnExit methods - called when the level editor scene becomes active or inactive. 
// OnEnter is currently empty, but we could add logic here if needed (e.g., to reset state or load specific resources). 
// OnExit ensures that any collider entities generated by the ChunkManager are removed from the EntityManager before the engine clears all entities,
// preventing dangling pointers and crashes when returning to the scene. It then saves chunks to disk.
void LevelEditorScene::OnEnter() {}
void LevelEditorScene::OnExit() {
	// Unregister any chunk-generated collider entities before the global EntityManager is cleared by the engine.
	try {
		m_chunkManager.UnregisterChunkColliders(GetEntityManager());
	} catch (...) {}
	std::cout << "[LevelEditorScene] OnExit: Saving all chunks...\n";
	m_chunkManager.SaveAllChunks();
	std::cout << "[LevelEditorScene] OnExit: Chunks saved.\n";
}
/////////////////////////////////



/////////////////////////////////
// UnloadResources - free large resources held by the editor when it is no longer active.
// 
void LevelEditorScene::UnloadResources() {
	try {
		// Ensure any collider entities are unregistered
		m_chunkManager.UnregisterChunkColliders(GetEntityManager());
	} catch (...) {}
	try {
		// Persist and then clear chunk data to free memory
		m_chunkManager.SaveAllChunks();
	} catch (...) {}
	try {
		m_chunkManager.ClearAllLoadedChunks();
	} catch (...) {}
	try {
		// Unload tileset atlas if one is set for this level/editor
		std::string key = std::string(m_tilesetKeyBuf);
		if (!key.empty()) {
			m_gameEngine.GetTextureManager().UnloadAtlas(key);
		}
	} catch (...) {}
}
/////////////////////////////////



/////////////////////////////////
// HandleEvent  - currently empty (not anymore), as we will poll input in the Update method instead of relying on event callbacks. This allows for 
// smoother and more responsive input handling in the level editor, especially for continuous actions like dragging to paint tiles.
void LevelEditorScene::HandleEvent(const std::optional<sf::Event>& event) {
	if (!event) return;
	// If ImGui is capturing the mouse (e.g., user is over a UI window), don't change camera zoom
	if (ImGui::GetCurrentContext() && ImGui::GetIO().WantCaptureMouse) return;
	if (const auto* scroll = event->getIf<sf::Event::MouseWheelScrolled>()) {
		auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
		if (!camOpt) return;
		CCamera* cam = *camOpt;
		constexpr int kMaxIndex = static_cast<int>(std::size(k_zoomSteps)) - 1;
		if (scroll->delta > 0.f) {
			// scroll up = zoom in (smaller index = more zoomed in)
			m_zoomIndex = std::max(0, m_zoomIndex - 1);
		} else {
			// scroll down = zoom out (larger index = more zoomed out)
			m_zoomIndex = std::min(kMaxIndex, m_zoomIndex + 1);
		}
		cam->m_zoom = k_zoomSteps[m_zoomIndex];
	}
}
/////////////////////////////////



/////////////////////////////////
// Update - Main update loop for the level editor scene. This will be called every frame by the game engine. In this function, we will handle camera updates, 
// ensure the necessary chunks are loaded based on the camera's position, and process user input for editing the level.
void LevelEditorScene::Update(float deltaTime) {
	// update camera logic (smoothing, shake)
	m_cameraSystem.Update(deltaTime, GetEntityManager());

	// apply active camera view to window before rendering world
	ApplyMainCameraView();

	// ensure chunks for current view
	EnsureVisibleChunks();

	// finalize any background-loaded chunks
	m_chunkManager.UpdateMainThread();

	// determine which tile the mouse is over in world coordinates.
	sf::Vector2i mousePos = sf::Mouse::getPosition(m_window);
	sf::Vector2f world = m_window.mapPixelToCoords(mousePos, m_window.getView());
	int tileX = (int)std::floor(world.x / m_tileSize);
	int tileY = (int)std::floor(world.y / m_tileSize);

	// Check and record mouse buttons and keyboard state (engine-wide policy)
	bool lmb = m_gameEngine.GetInputController().IsMouseButtonDown(sf::Mouse::Button::Left);
	bool rmb = m_gameEngine.GetInputController().IsMouseButtonDown(sf::Mouse::Button::Right);

	// If ImGui is capturing the mouse or UI is hovered/active, do not modify the map, allows us to interact with the UI without accidentally editing level or moving the camera.
	bool uiCapturing = ImGui::GetIO().WantCaptureMouse || ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);

		
	
	// Tile pick: hold D to activate eyedropper mode, then press left mouse button to pick a tile. While D is held, the eyedropper cursor is visible. 
	// When LMB is clicked while D is held, the tile at the mouse position is selected and becomes the active brush tile.
	bool dKey = m_gameEngine.GetInputController().IsKeyboardEnabled() && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);

	// Switch to eyedropper cursor when D is pressed, revert when released
	if (dKey && !m_prevDKey) {
		// D key pressed (edge): switch to eyedropper mode
		m_gameEngine.GetCursorSystem().SetMode(CursorSystem::Mode::Pointer);
	} else if (!dKey && m_prevDKey) {
		// D key released (edge): switch back to default mode
		m_gameEngine.GetCursorSystem().SetMode(CursorSystem::Mode::Default);
	}

	// Pick tile on LMB press while D is held (eyedropper mode)
	if (dKey && lmb && !m_prevLmb && !uiCapturing) {
		int layerToPick = m_activeLayer;
		if (layerToPick < 0 || layerToPick >= (int)m_layerNames.size()) layerToPick = 0;
		int val = m_chunkManager.GetTileAt(tileX, tileY, layerToPick);
		if (val > 0) {
			m_selectedTileIndex = val - 1;
			m_brushValue = val;
			// optionally echo to console for feedback
			//std::cout << "Picked tile at (" << tileX << "," << tileY << ") value=" << val << " -> selectedIndex=" << m_selectedTileIndex << std::endl;
		}
	}

	// Camera panning with the middle mouse button; if we press the middle mouse button and it wasn't pressed in the previous frame, then start the panning 
	// process. Begin by recording the initial mouse 'start' position and camera position. When the middle mouse button is released, end the panning process.
	bool isMiddleDown = m_gameEngine.GetInputController().IsMouseButtonDown(sf::Mouse::Button::Middle);
	m_prevMiddleDown = isMiddleDown;
	bool wasPanning = m_panning;
	// allow panning even if ImGui reports capture so middle-click drag still moves camera
	if (isMiddleDown) {
		if (!m_panning) {
			m_panning = true;
			m_panStart = mousePos;
			// record current camera position
			auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
			if (camOpt) {
				m_camPanStart = (*camOpt)->m_position;
			} else {
				// no main camera found
			}
		}
	} else {
		if (m_panning && !isMiddleDown) {
			// panning ended
			auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
			if (camOpt) {
				Vec2 endPos = (*camOpt)->m_position;
				(void)endPos; // suppressed debug log
			} else {
				// no main camera found
			}
		}
		m_panning = false;
	}

	// If we are currently panning, calculate the delta from the initial mouse position and move the camera in the opposite direction of the mouse drag for a 
	// natural panning feel. We also clamp the camera's new position within the bounds of the map to prevent panning into empty space. Finally, we update the 
	// camera entity's transform to ensure that any smoothing logic in the CameraSystem does not pull it back to a previous position.
	if (m_panning) {
		sf::Vector2i delta = mousePos - m_panStart;
		// move camera opposite to mouse drag for natural panning
		auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
		if (camOpt) {
			Vec2 newPos = m_camPanStart - Vec2((float)delta.x, (float)delta.y);
			// Compute current map bounds from loaded chunks so clamping is accurate at panning time
			CCamera* cam = *camOpt;
			float halfW = cam->m_viewportWidth * 0.5f * cam->m_zoom;
			float halfH = cam->m_viewportHeight * 0.5f * cam->m_zoom;
			if (m_haveBounds) {
				float minCx = m_mapMin.x + halfW;
				float maxCx = m_mapMax.x - halfW;
				float minCy = m_mapMin.y + halfH;
				float maxCy = m_mapMax.y - halfH;
				if (minCx <= maxCx) newPos.x = std::clamp(newPos.x, minCx, maxCx);
				else newPos.x = (m_mapMin.x + m_mapMax.x) * 0.5f;
				if (minCy <= maxCy) newPos.y = std::clamp(newPos.y, minCy, maxCy);
				else newPos.y = (m_mapMin.y + m_mapMax.y) * 0.5f;
			}
			(*camOpt)->m_position = newPos;
			// Also update the camera entity's transform so CameraSystem smoothing doesn't pull it back
			if (m_cameraEntity) {
				if (auto t = m_cameraEntity->GetComponent<CTransform>()) {
					t->m_position = newPos;
				}
			}
		}
	}

	// Handle mouse input for painting or erasing tiles; check we are off the UI before modifying the map m_inputReady gates all painting until every button has been physically 
	// released at least once after the scene was entered — this prevents the click that opened the scene from painting.
	if (!m_inputReady) {
		if (!lmb && !rmb) m_inputReady = true;
	}

	// If the UI is not capturing the mouse and input is ready, handle tile painting/erasing based on mouse button states. This includes starting a selection on button down, 
	// updating the selection rectangle while dragging, and applying the brush value to all tiles within the selected area on button release. The selection is defined in 
	// pixel coordinates and converted to world coordinates and then tile coordinates for editing the level.
	if (!uiCapturing && m_inputReady) {
		// start selection on left-button down (edge)
		if (lmb && !m_prevLmb) {
			if (m_levelSelected) {
				m_lmbSelecting = true;
				m_selectLmbStartPx = mousePos;
				m_selectLmbEndPx = mousePos;
			} else {
				m_exportMessage = "Select or create a level before editing.";
			}
		}
		// update drag end while holding
		if (m_lmbSelecting && lmb) {
			m_selectLmbEndPx = mousePos;
		}
		// finish selection on left-button release
		if (m_lmbSelecting && !lmb && m_prevLmb) {
			m_lmbSelecting = false;
			// convert pixel corners -> world coords using current view
			sf::Vector2f w0 = m_window.mapPixelToCoords(m_selectLmbStartPx, m_window.getView());
			sf::Vector2f w1 = m_window.mapPixelToCoords(m_selectLmbEndPx, m_window.getView());
			// tile coords (floor) and normalize min/max
			int tx0 = (int)std::floor(std::min(w0.x, w1.x) / m_tileSize);
			int ty0 = (int)std::floor(std::min(w0.y, w1.y) / m_tileSize);
			int tx1 = (int)std::floor(std::max(w0.x, w1.x) / m_tileSize);
			int ty1 = (int)std::floor(std::max(w0.y, w1.y) / m_tileSize);
			// clamp loop if you want (optional)
			// Safety: avoid processing excessively large selections that could hang the editor.
			const int maxSelectionTiles = 1024 * 1024; // 1M tiles
			uint64_t selW = (uint64_t)tx1 - (uint64_t)tx0 + 1;
			uint64_t selH = (uint64_t)ty1 - (uint64_t)ty0 + 1;
			if (selW <= 0 || selH <= 0) { /* nothing */; }
			else if (selW * selH > maxSelectionTiles) {
				std::cerr << "Selection too large (" << selW*selH << " tiles) - operation ignored" << std::endl;
			} else {
				if (!m_levelSelected) {
					m_exportMessage = "Select or create a level before editing.";
				} else {
					// Silently paint tiles - no debug spam
					for (int ty = ty0; ty <= ty1; ++ty) {
						for (int tx = tx0; tx <= tx1; ++tx) {
							// Prevent placing tiles at negative coordinates
							if (tx < 0 || ty < 0) {
								continue;
							}
							m_chunkManager.SetTileAt(tx, ty, m_brushValue, m_activeLayer); // paint selection on active layer
						}
					}
					// Refresh fixed bounds from disk (covers both expand on paint and shrink on erase)
					RefreshMapBounds();
				}
			}
		}

		// start selection on right-button down (edge)
		if (rmb && !m_prevRmb) {
			if (m_levelSelected) {
				m_rmbSelecting = true;
				m_selectRmbStartPx = mousePos;
				m_selectRmbEndPx = mousePos;
			} else {
				m_exportMessage = "Select or create a level before editing.";
			}
		}
		// update drag end while holding
		if (m_rmbSelecting && rmb) {
			m_selectRmbEndPx = mousePos;
		}
		// finish selection on right-button release
		if (m_rmbSelecting && !rmb && m_prevRmb) {
			m_rmbSelecting = false;
			// convert pixel corners -> world coords using current view
			sf::Vector2f w0 = m_window.mapPixelToCoords(m_selectRmbStartPx, m_window.getView());
			sf::Vector2f w1 = m_window.mapPixelToCoords(m_selectRmbEndPx, m_window.getView());
			// tile coords (floor) and normalize min/max
			int tx0 = (int)std::floor(std::min(w0.x, w1.x) / m_tileSize);
			int ty0 = (int)std::floor(std::min(w0.y, w1.y) / m_tileSize);
			int tx1 = (int)std::floor(std::max(w0.x, w1.x) / m_tileSize);
			int ty1 = (int)std::floor(std::max(w0.y, w1.y) / m_tileSize);
			
			// Safety: avoid processing excessively large selections that could hang the editor.
			const int maxSelectionTiles = 1024 * 1024; // 1M tiles
			uint64_t selW2 = (uint64_t)tx1 - (uint64_t)tx0 + 1;
			uint64_t selH2 = (uint64_t)ty1 - (uint64_t)ty0 + 1;
			
			if (selW2 <= 0 || selH2 <= 0) { /* nothing */; }
			else if (selW2 * selH2 > maxSelectionTiles) {
				std::cerr << "Selection too large (" << selW2*selH2 << " tiles) - operation ignored" << std::endl;
			} else {
				for (int ty = ty0; ty <= ty1; ++ty) {
					for (int tx = tx0; tx <= tx1; ++tx) {
						// Prevent erasing at negative coordinates
						if (tx < 0 || ty < 0) {
							continue;
						}
						m_chunkManager.SetTileAt(tx, ty, 0, m_activeLayer); // erase selection on active layer
					}
				}
			
			// Recompute bounds from disk since erasing may shrink the map
			RefreshMapBounds();
			}
		}
	}
	m_prevLmb = lmb;
	m_prevRmb = rmb;
	m_prevDKey = dKey;

	ProcessInput();
}
/////////////////////////////////



/////////////////////////////////
// Render - Render the level editor scene. This will be called every frame by the game engine after Update. In this function, we will render the loaded chunks 
// of the level based on the current camera view.
void LevelEditorScene::Render() {
	// Clear temporary render storage from previous frame
	m_tempRenderShapes.clear();
	m_tempRenderVertexArrays.clear();
	m_nextTempId = 0;

	// Enqueue checkerboard background at depth 0 (background layer)
	EnqueueCheckerboardBackground();

	// Enqueue chunks through the render queue with proper per-layer temp storage
	m_chunkManager.EnqueueChunks(m_renderQueue, m_window.getView());

	// Enqueue optional debug visualization at depth 100 (on top of chunks, below UI)
	EnqueueChunkDiag();
	EnqueueDragRect();
	EnqueueMapBounds();

	// Flush the render queue to the window (this renders all enqueued drawables in depth order)
	m_renderQueue.Flush(m_window);

	// reset view to default for UI rendering
	m_window.setView(m_window.getDefaultView());

	
	// Show bounds info in a separate ImGui window
	BoundsInfoWindow(m_window.getSize().x, m_window.getSize().y, 0.35f);

	// Render the separate Level Manager window
	LevelManagerWindow(m_window.getSize().x - 550.0f, 10, 0.35f);

	// ████
	// ████
	// ████
	
	// ImGui Browser Parent window for tileset management and brush preview. This is where we will add the UI for loading a tileset atlas, inputting the tileset key, 
	// and showing a preview of the currently selected tile/brush. We will also include some diagnostics here like FPS and camera info for convenience while editing.
	{
		ImGuiIO& io = ImGui::GetIO();
		float margin = 10.0f;
		float winH = 10.0f;
		// approximate window height for the tileset panel, position so bottom aligns with margin
		float approxHeight = 420.0f;
		ImGui::SetNextWindowPos(ImVec2(10, std::max(margin, winH - approxHeight - margin)), ImGuiCond_Once);
	}
	ImGui::SetNextWindowBgAlpha(0.35f);
	// Allow the tileset window to be movable by giving it a title bar; keep auto-resize
	ImGui::Begin("Tileset Browser", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	// Level management moved to dedicated window

	// Display current mouse world coordinates and tile coordinates
	{
		sf::Vector2i mousePixelPos = sf::Mouse::getPosition(m_window);
		sf::Vector2f mouseWorldPos = m_window.mapPixelToCoords(mousePixelPos, m_window.getView());
		int mouseTileX = (int)std::floor(mouseWorldPos.x / m_tileSize);
		int mouseTileY = (int)std::floor(mouseWorldPos.y / m_tileSize);

		ImGui::SeparatorText("Mouse Position");
		ImGui::Text("World: (%.1f, %.1f)", mouseWorldPos.x, mouseWorldPos.y);
		ImGui::Text("Tile:  (%d, %d)", mouseTileX, mouseTileY);
	}

	ImGui::Separator();

	// Display camera position and zoom level
	{
		auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
		if (camOpt) {
			CCamera* cam = *camOpt;
			ImGui::SeparatorText("Camera");
			ImGui::Text("Position: (%.1f, %.1f)", cam->m_position.x, cam->m_position.y);
			ImGui::Text("Zoom:     %.2fx", cam->m_zoom);
		}
	}

	ImGui::Separator();

	// Fetch atlas once for UI usage (preview + brush). Declared here so later preview code can use it.
	auto atlasOpt = m_gameEngine.GetTextureManager().GetAtlas(std::string(m_tilesetKeyBuf));

	SelectedBrushWindow(atlasOpt);

	// ████

	CameraZoomWindow();
	
	// ████
		
	AtlasBrowserWindow();

	// ████
	
	TilesetKeyWindow(); // UI for inputting the tileset key and dimensions

	// ████

	// Snapshot shared async-load state under lock, then release before any ImGui/chunk work
	std::string atlasMessage;
	bool atlasFinished = false, atlasSuccess = false;
	std::string atlasKey;
	{
		std::lock_guard<std::mutex> lg(m_atlasLoadMutex);
		atlasMessage  = m_atlasLoadMessage;
		if (m_atlasLoadFinished) {
			atlasFinished = true;
			atlasSuccess  = m_atlasLoadSuccess;
			atlasKey      = m_atlasLoadKey;
			m_atlasLoadFinished = false; // clear flag while we still hold the lock
		}
	}

	// ████

	// Display message outside the lock
	if (!atlasMessage.empty()) ImGui::TextUnformatted(atlasMessage.c_str());

	// Apply finished load result outside the lock
	if (atlasFinished) {
		if (atlasSuccess) {
			ImStrncpy(m_tilesetKeyBuf, atlasKey.c_str(), sizeof(m_tilesetKeyBuf));
			m_chunkManager.SetTilesetKey(atlasKey);
			m_chunkManager.RebuildAllChunksFromTileset();
			m_chunkManager.UpdateMainThread();
			m_chunkManager.RebuildAllChunksFromTileset();
			SaveLevelMetadata();  // Save tileset to meta.txt
			m_selectedTileIndex = 0;
		}
	}

	// ████

	TilePreviewWindow(atlasOpt); // Window for previewing the currently loaded tileset atlas and selecting the active brush tile
	ImGui::End();

	// ████
	// ████
	// ████

}
/////////////////////////////////



/////////////////////////////////
// SeclectedTileWindow - Renders a small floating preview of the currently-selected tile/atlas used for painting. This provides visual feedback to the user about which 
// tile is currently active as the brush for painting in the level editor. It checks if an atlas is loaded and if a brush tile is selected, then extracts the 
// corresponding tile from the atlas texture and displays it using ImGui::ImageButton. If no brush is selected, it shows a "(no brush)" message.
void LevelEditorScene::SelectedBrushWindow(std::optional<std::shared_ptr<TextureAtlas>>& atlasOpt) {
	// Small floating preview of the currently-selected tile/atlas used for painting
	ImGui::Separator();
	ImGui::Text("Current Brush");
	if (atlasOpt.has_value() && atlasOpt.value() && m_brushValue > 0) {
		auto atlas = atlasOpt.value();
		// Show whether the atlas image contains any transparent pixels
		ImGui::Text("Atlas has alpha: %s", atlas->HasAlpha() ? "yes" : "no");
		auto tex = atlas->GetTexture();
		if (tex) {
			int tw = atlas->TileWidth();
			int th = atlas->TileHeight();
			int idx = std::max(0, m_brushValue - 1);
			int cols = (int)(tex->getSize().x / tw);
			int r = idx / cols;
			int c = idx % cols;
			// draw a small image using SFML sprite helper via ImGui::Image (imgui-SFML provides overload)
			sf::Sprite spr(*tex);
			sf::IntRect rect(sf::Vector2i(c * tw, r * th), sf::Vector2i(tw, th));
			spr.setTextureRect(rect);
			ImGui::ImageButton("CurrentBrush", spr, sf::Vector2f((float)tw * 2.0f, (float)th * 2.0f));
		}
	} else {
		ImGui::Text("(no brush)");
	}
}
/////////////////////////////////



/////////////////////////////////
// AtlasBrowserWindow - Renders ImGui controls for browsing the filesystem to load a tileset atlas, inputting the tileset key and tile dimensions, 
// and triggering the loading of the atlas. When an atlas is loaded, we also trigger a rebuild of all chunks to update their vertex textures 
// based on the new atlas.
void LevelEditorScene::AtlasBrowserWindow() {
	// File browser: input for load filename, buttons to navigate up and refresh, and list of files in the current directory. Clicking a directory navigates 
	// into it, while clicking a file selects it for loading.
	if (m_currentDir.empty())
		m_currentDir = std::filesystem::current_path();

	// Cache directory entries in a static vector to avoid re-reading the filesystem on every frame. We will refresh this cache when navigating or when the 
	// user clicks the Refresh button.
	static std::vector<std::filesystem::directory_entry> s_entries;
	auto refresh_entries = [&]() {
		s_entries.clear();
		try {
			for (auto& e : std::filesystem::directory_iterator(m_currentDir))
				s_entries.push_back(e);
			std::sort(s_entries.begin(), s_entries.end(), [](const auto& a, const auto& b) {
				bool a_dir = false, b_dir = false;
				try {
					a_dir = a.is_directory();
				} catch (...) {
					a_dir = false;
				}
				try {
					b_dir = b.is_directory();
				} catch (...) {
					b_dir = false;
				}
				if (a_dir != b_dir)
					return a_dir > b_dir;
				return a.path().filename().string() < b.path().filename().string();
			});
		} catch (...) {
			s_entries.clear();
		}
	};

	if (ImGui::Button("Up") && m_currentDir.has_parent_path()) {
		m_currentDir = m_currentDir.parent_path();
		refresh_entries();
	}
	ImGui::SameLine();
	if (ImGui::Button("Refresh")) {
		refresh_entries();
	}
	ImGui::SameLine();
	ImGui::Text("Current folder: %s", m_currentDir.string().c_str());

	if (s_entries.empty())
		refresh_entries();

	// File browser: list files and directories in the current folder. Directories are shown with a trailing slash and sorted before files. Clicking a 
	// directory navigates into it, while clicking a file selects it for loading. Double-clicking a file triggers loading the atlas immediately. 
	// The selected file is also copied into the load filename buffer for convenience if the user wants to edit it before loading.
	ImGui::BeginChild("files_list", ImVec2(0, 200), true);
	static std::string s_selected_file;
	for (size_t i = 0; i < s_entries.size(); ++i) {
		auto& entry = s_entries[i];
		std::string name = entry.path().filename().string();
		bool is_dir = false;
		try {
			is_dir = entry.is_directory();
		} catch (...) {
			is_dir = false;
		}
		std::string label = is_dir ? (name + "/") : name;
		std::string fullpath = (m_currentDir / entry.path().filename()).string();
		bool selected = (!s_selected_file.empty() && s_selected_file == fullpath);
		if (ImGui::Selectable(label.c_str(), selected)) {
			if (is_dir) {
				m_currentDir = entry.path();
				s_selected_file.clear();
				refresh_entries();
			} else {
				ImStrncpy(m_loadFilenameBuffer, fullpath.c_str(), sizeof(m_loadFilenameBuffer));
				s_selected_file = fullpath;
				if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					// load selected atlas. If UI key is empty, derive a key from filename stem so we don't register under an empty key.
					std::string key = std::string(m_tilesetKeyBuf);
					if (key.empty()) {
						try {
							key = std::filesystem::path(s_selected_file).stem().string();
						} catch (...) {
							key = "";
						}
					}
					if (m_gameEngine.GetTextureManager().LoadAtlas(key, s_selected_file, m_tilesetTileW,
																	   m_tilesetTileH)) {
						m_chunkManager.SetTilesetKey(key);
						m_chunkManager.RebuildAllChunksFromTileset();
						SaveLevelMetadata();  // Save tileset to meta.txt
						std::cout << "Loaded atlas: " << s_selected_file << " key='" << key << "'" << std::endl;
						// copy key into ui buffer
						ImStrncpy(m_tilesetKeyBuf, key.c_str(), sizeof(m_tilesetKeyBuf));
					} else
						std::cerr << "Failed to load atlas: " << s_selected_file << std::endl;
				}
			}
		}
	}
	ImGui::EndChild();
}
/////////////////////////////////



/////////////////////////////////
// TilesetKeyWindow - Renders ImGui controls for inputting the tileset key and tile dimensions, plus a button to trigger loading the atlas based on the 
// current UI state. When an atlas is loaded, we trigger a rebuild of all chunks to update their vertex textures based on the new atlas.
void LevelEditorScene::TilesetKeyWindow() {
	// Input for tileset key and tile dimensions, plus button to trigger loading the atlas based on the current UI state. When an atlas is loaded, 
	// we trigger a rebuild of all chunks to update their vertex textures based on the new atlas.
	ImGui::Separator();
	ImGui::Text("Tileset key: %s", m_tilesetKeyBuf);
	ImGui::SameLine();
	if (ImGui::Button("Set Key")) {
		m_chunkManager.SetTilesetKey(std::string(m_tilesetKeyBuf));
		m_chunkManager.RebuildAllChunksFromTileset();
		SaveLevelMetadata();  // Save tileset to meta.txt
		std::cout << "LevelEditorScene: Set tileset to '" << m_tilesetKeyBuf << "' and saved metadata\n";
	}

	ImGui::Separator();
	if (ImGui::Button("Load Selected")) {
		std::string key = std::string(m_tilesetKeyBuf);
		std::string path = std::string(m_loadFilenameBuffer);
		if (!path.empty()) {
			// Load atlas asynchronously to avoid blocking UI
			if (!m_atlasLoading.exchange(true)) {
				// if UI key is empty, derive a key from filename stem so we don't register under an empty key
				if (key.empty()) {
					try {
						key = std::filesystem::path(path).stem().string();
					} catch (...) {
						key = "";
					}
				}
				{
					std::lock_guard<std::mutex> lg(m_atlasLoadMutex);
					m_atlasLoadKey = key;
					m_atlasLoadPath = path;
					m_atlasLoadFinished = false;
					m_atlasLoadSuccess = false;
					m_atlasLoadMessage.clear();
				}
				std::thread([this, key, path, w = m_tilesetTileW, h = m_tilesetTileH]() {
					AtlasLoadWorker(key, path, w, h);
				}).detach();
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// TilePreviewWindow - Renders a preview of the currently loaded tileset atlas, showing all the individual tiles in a grid. The user can click 
// on any tile in the preview to select it as the active brush tile for painting. The selected tile is highlighted with a background color. 
// This provides a convenient visual reference for the user to see all available tiles in the atlas and quickly select one for editing the level.
void LevelEditorScene::TilePreviewWindow(std::optional<std::shared_ptr<TextureAtlas>>& atlasOpt) {
	// Preview loaded atlas tiles if available
	ImGui::Separator();
	// atlasOpt was fetched above once for UI usage
	if (atlasOpt.has_value() && atlasOpt.value()) {
		auto atlas = atlasOpt.value();
		std::shared_ptr<sf::Texture> tex = atlas->GetTexture();
		if (tex) {
			int tw = atlas->TileWidth();
			int th = atlas->TileHeight();
			int cols = (int)(tex->getSize().x / tw);
			int rows = (int)(tex->getSize().y / th);
			ImGui::Text("Preview: %d x %d tiles", cols, rows);
			ImGui::BeginChild("atlas_preview", ImVec2(0, 200), true);
			for (int r = 0; r < rows; ++r) {
				ImGui::BeginGroup();
				for (int c = 0; c < cols; ++c) {
					int idx = r * cols + c;
					// build sprite for the tile region
					sf::Sprite spr(*tex);
					sf::IntRect rect(sf::Vector2i(c * tw, r * th), sf::Vector2i(tw, th));
					spr.setTextureRect(rect);
					std::string id = std::string("tile_") + std::to_string(idx);
					// highlight selected tile
					sf::Color bg = (idx == m_selectedTileIndex) ? sf::Color(100, 150, 255) : sf::Color::Transparent;
					if (ImGui::ImageButton(id.c_str(), spr, sf::Vector2f((float)tw, (float)th), bg, sf::Color::White)) {
						m_selectedTileIndex = idx;
						m_brushValue = idx + 1; // Tile values are 1-based
					}
					ImGui::SameLine();
				}
				ImGui::EndGroup();
				ImGui::NewLine();
			}
			ImGui::EndChild();
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// ShowCameraZoomWindow - Renders a small ImGui window with diagnostics about the camera and controls to adjust the zoom level. This allows the 
// user to see the current FPS, camera position, and zoom level at a glance while editing the level, and provides convenient buttons to quickly 
// adjust the zoom without needing to use the mouse wheel.
void LevelEditorScene::CameraZoomWindow() {
	// Separator and diagnostics window to group related info and controls together. This includes FPS, camera position, zoom level, and buttons
	// to control zoom. Small diagnostics: FPS + camera position and zoom, plus buttons to zoom in/out and reset zoom to default. This is useful 
	// for testing and debugging the camera system while we are editing levels, and also serves as a reminder of the current zoom level which 
	// affects how the level will look in-game.
	float fps = m_gameEngine.GetFPSCounter().GetFPS();
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (camOpt) {
		CCamera* cam = *camOpt;
		ImGui::Text("FPS: %.1f  Camera: (%.1f, %.1f)  Zoom: %.2fx", fps, cam->m_position.x, cam->m_position.y,
					cam->m_zoom);
		constexpr int kMaxIndex = static_cast<int>(std::size(k_zoomSteps)) - 1;
		if (ImGui::Button("Zoom In") && m_zoomIndex > 0) {
			m_zoomIndex--;
			cam->m_zoom = k_zoomSteps[m_zoomIndex];
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset Zoom")) {
			m_zoomIndex = 1;
			cam->m_zoom = k_zoomSteps[m_zoomIndex];
		}
		ImGui::SameLine();
		if (ImGui::Button("Zoom Out") && m_zoomIndex < kMaxIndex) {
			m_zoomIndex++;
			cam->m_zoom = k_zoomSteps[m_zoomIndex];
		}
	} else {
		ImGui::Text("FPS: %.1f  Camera: (no main camera)", fps);
	}
	ImGui::Separator();
}
/////////////////////////////////



/////////////////////////////////
// EnqueueCheckerboardBackground - Generates checkerboard pattern and enqueues it to the render queue
// This is the new ECS-aligned way to render the editor background
void LevelEditorScene::EnqueueCheckerboardBackground() {
	const float checkSize = m_tileSize;
	sf::View view = m_window.getView();
	sf::Vector2f center = view.getCenter();
	sf::Vector2f size = view.getSize();

	float left = center.x - size.x * 0.5f;
	float top = center.y - size.y * 0.5f;

	int x0 = (int)std::floor(left / checkSize) - 1;
	int y0 = (int)std::floor(top / checkSize) - 1;
	int x1 = (int)std::ceil((left + size.x) / checkSize) + 1;
	int y1 = (int)std::ceil((top + size.y) / checkSize) + 1;

	auto va = std::make_shared<sf::VertexArray>(sf::PrimitiveType::Triangles);

	for (int y = y0; y <= y1; ++y) {
		for (int x = x0; x <= x1; ++x) {
			bool even = ((x + y) % 2) == 0;
			sf::Color color = even ? sf::Color(34, 34, 34) : sf::Color(18, 18, 18);

			float px = x * checkSize;
			float py = y * checkSize;
			float px2 = px + checkSize;
			float py2 = py + checkSize;

			// Quad as two triangles: TL, TR, BR then TL, BR, BL
			va->append(sf::Vertex(sf::Vector2f(px, py), color));
			va->append(sf::Vertex(sf::Vector2f(px2, py), color));
			va->append(sf::Vertex(sf::Vector2f(px2, py2), color));
			va->append(sf::Vertex(sf::Vector2f(px, py), color));
			va->append(sf::Vertex(sf::Vector2f(px2, py2), color));
			va->append(sf::Vertex(sf::Vector2f(px, py2), color));
		}
	}

	m_tempRenderVertexArrays[m_nextTempId] = va;
	m_renderQueue.Enqueue(va.get(), 0); // depth 0: background
	m_nextTempId++;
}
/////////////////////////////////



/////////////////////////////////
// ShowsBoundsInfo - Debug function to display the current map bounds and camera info in an ImGui window. This is useful for diagnosing issues
// with the bounds calculation and ensuring that the camera clamping logic has the correct values to work with. It also includes a checkbox to 
// toggle additional chunk diagnostics rendering.
void LevelEditorScene::BoundsInfoWindow(float x, float y, float alpha) {
	// Debug: show computed bounds and camera info to help diagnose missing bounds rectangle
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	int chunkCount = 0;
	{
		std::lock_guard<std::mutex> lg(m_chunkManager.GetMutex());
		chunkCount = (int)m_chunkManager.GetChunks().size();
	}
	// Place the bounds/debug window at the bottom-right on first show

	float margin = 10.0f;
	float approxW = 255.0f;
	float approxH = 140.0f;
	ImGui::SetNextWindowPos(ImVec2(std::max(0.0f, x - approxW - margin),
									std::max(0.0f, y - approxH - margin)),
							ImGuiCond_Once);

	ImGui::SetNextWindowBgAlpha(alpha);

	ImGui::Begin("Bounds Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
	ImGui::Text("Loaded chunks: %d  haveBounds: %s", chunkCount, m_haveBounds ? "yes" : "no");
	ImGui::Text("m_mapMin=(%.1f, %.1f)", m_mapMin.x, m_mapMin.y);
	ImGui::Text("m_mapMax=(%.1f, %.1f)", m_mapMax.x, m_mapMax.y);
	ImGui::Checkbox("Show Chunk Diagnostics", &m_showChunkDiagnostics);
	if (camOpt) {
		CCamera* cam = *camOpt;
		float halfW = cam->m_viewportWidth * 0.5f * cam->m_zoom;
		float halfH = cam->m_viewportHeight * 0.5f * cam->m_zoom;
		ImGui::Text("Camera pos=(%.1f, %.1f) zoom=%.2f", cam->m_position.x, cam->m_position.y, cam->m_zoom);
		ImGui::Text("Half view=(%.1f, %.1f)", halfW, halfH);
		if (m_haveBounds) {
			ImGui::Text("minCx=%.1f maxCx=%.1f", m_mapMin.x + halfW, m_mapMax.x - halfW);
			ImGui::Text("minCy=%.1f maxCy=%.1f", m_mapMin.y + halfH, m_mapMax.y - halfH);
		}
	}
	ImGui::End();
}
/////////////////////////////////



/////////////////////////////////
// EnqueueDragRect - Enqueue the selection rectangle to the render queue
void LevelEditorScene::EnqueueDragRect() {
	// Enqueue selection rectangle in world space while dragging (left or right button)
	if (m_lmbSelecting || m_rmbSelecting) {
		// Map the pixel selection corners into world coordinates using the active view
		sf::Vector2f w0 =
			m_window.mapPixelToCoords(m_lmbSelecting ? m_selectLmbStartPx : m_selectRmbStartPx, m_window.getView());
		sf::Vector2f w1 =
			m_window.mapPixelToCoords(m_lmbSelecting ? m_selectLmbEndPx : m_selectRmbEndPx, m_window.getView());
		float left = std::min(w0.x, w1.x);
		float top = std::min(w0.y, w1.y);
		float width = std::abs(w1.x - w0.x);
		float height = std::abs(w1.y - w0.y);

		auto rect = std::make_shared<sf::RectangleShape>(sf::Vector2f(width, height));
		rect->setPosition(sf::Vector2f(left, top));
		rect->setFillColor(sf::Color::Transparent);
		if (m_lmbSelecting)
			rect->setOutlineColor(sf::Color(100, 150, 255, 200));
		else
			rect->setOutlineColor(sf::Color(255, 100, 100, 200));
		rect->setOutlineThickness(2.0f);

		m_tempRenderShapes[m_nextTempId] = rect;
		m_renderQueue.Enqueue(rect.get(), 100); // depth 100: debug overlay
		m_nextTempId++;
	}
}
/////////////////////////////////



/////////////////////////////////
// EnqueueChunkDiag - Enqueue debug chunk diagnostics visualization
void LevelEditorScene::EnqueueChunkDiag() {
	if (!m_showChunkDiagnostics) return;

	std::lock_guard<std::mutex> lg(m_chunkManager.GetMutex());
	for (const auto& pr : m_chunkManager.GetChunks()) {
		const Chunk& c = pr.second;
		const float wx = (float)(c.chunkX * c.width) * c.tileSize;
		const float wy = (float)(c.chunkY * c.height) * c.tileSize;
		const float w = (float)c.width * c.tileSize;
		const float h = (float)c.height * c.tileSize;
		bool any = false;
		for (int L = 0; L < c.numLayers; ++L) { for (int t : c.tilesPerLayer[L]) { if (t != 0) { any = true; break; } } if (any) break; }

		auto r = std::make_shared<sf::RectangleShape>(sf::Vector2f(w, h));
		r->setPosition(sf::Vector2f(wx, wy));
		r->setFillColor(sf::Color::Transparent);
		if (!any) {
			r->setOutlineColor(sf::Color(200, 0, 0, 180)); // red = empty
		} else if (c.vertexTexture) {
			r->setOutlineColor(sf::Color(0, 200, 0, 180)); // green = has tiles + texture
		} else {
			r->setOutlineColor(sf::Color(200, 200, 0, 180)); // yellow = has tiles but no texture
		}
		r->setOutlineThickness(2.0f);
		m_tempRenderShapes[m_nextTempId] = r;
		m_renderQueue.Enqueue(r.get(), 100); // depth 100: debug overlay
		m_nextTempId++;
	}
}
/////////////////////////////////



/////////////////////////////////
// EnqueueMapBounds - Enqueue map bounds visualization
void LevelEditorScene::EnqueueMapBounds() {
	if (!m_haveBounds) return;

	float width = m_mapMax.x - m_mapMin.x;
	float height = m_mapMax.y - m_mapMin.y;

	//std::cout << "DEBUG: EnqueueMapBounds: min=(" << m_mapMin.x << "," << m_mapMin.y 
	//		  << ") max=(" << m_mapMax.x << "," << m_mapMax.y 
	//		  << ") width=" << width << " height=" << height << std::endl;

	// Get outline thickness based on zoom
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	float outlineThickness = 2.0f;
	
	// If we have a main camera, adjust the outline thickness based on the zoom level to keep it 
	// visually consistent regardless of zoom.
	if (camOpt) {
		float zoom = (*camOpt)->m_zoom;
		outlineThickness = 2.0f / zoom;
	}

	// Instead of using RectangleShape outline (which has rendering issues with large bounds),
	// draw the four edges as separate line segments using a vertex array
	auto va = std::make_shared<sf::VertexArray>();
	va->setPrimitiveType(sf::PrimitiveType::Lines);

	sf::Color boundsColor(255, 255, 255, 200);
	float x0 = m_mapMin.x;
	float y0 = m_mapMin.y;
	float x1 = m_mapMax.x;
	float y1 = m_mapMax.y;

	// Top edge
	va->append(sf::Vertex(sf::Vector2f(x0, y0), boundsColor));
	va->append(sf::Vertex(sf::Vector2f(x1, y0), boundsColor));

	// Bottom edge
	va->append(sf::Vertex(sf::Vector2f(x0, y1), boundsColor));
	va->append(sf::Vertex(sf::Vector2f(x1, y1), boundsColor));

	// Left edge
	va->append(sf::Vertex(sf::Vector2f(x0, y0), boundsColor));
	va->append(sf::Vertex(sf::Vector2f(x0, y1), boundsColor));

	// Right edge
	va->append(sf::Vertex(sf::Vector2f(x1, y0), boundsColor));
	va->append(sf::Vertex(sf::Vector2f(x1, y1), boundsColor));

	m_tempRenderVertexArrays[m_nextTempId] = va;
	m_renderQueue.Enqueue(va.get(), 100); // depth 100: debug overlay
	m_nextTempId++;
}
/////////////////////////////////






/////////////////////////////////
// Separate Level Manager window so selection/creation/deletion is distinct from Tileset browser
void LevelEditorScene::LevelManagerWindow(float x, float y, float alpha) {
	// Level manager near top-left (avoid overlapping tileset browser)
	ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Once);
	ImGui::SetNextWindowBgAlpha(alpha);

	// Lets get ImGui to create a simple UI for managing levels. This will include: 
		// 1) a list of available levels (fetched from disk based on the same base path used for loading/saving levels), 
		// 2) a button to refresh the list of levels, 
		// 3) an input box and button to create a new level, 
		// 4) buttons to switch to the selected level, export it, or delete it.

	if (!ImGui::Begin("Level Manager", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::End();
		return;
	} // Begin by creating a new ImGui window; if it fails to initialize, we end and return early
	
	if (ImGui::Button("Refresh Levels"))
		RefreshAvailableLevels(); // Create a button to refesh level directory when clicked

	ImGui::SameLine(); // stay on the same line for the next input
	ImGui::InputText("New Level Name", m_levelNameBuf, sizeof(m_levelNameBuf)); // Create an input box for the new level name, storing the result in m_levelNameBuf
	ImGui::SameLine(); // stay on the same line for the button
	if (ImGui::Button("Create Level")) { 
		std::string name = std::string(m_levelNameBuf); // copy the new level name to std::string for easier handling
		
		// Check we have a valid name (non-empty, no path separators, etc) before trying to create directories and files. We will just ignore invalid names for simplicity, 
		// but in a real application (what ya mean a real application?) we might want to show an error message to the user.
		if (!name.empty()) {
			fs::path base; // should be %APPDATA%/GameEnginePlus/levels on Windows or $XDG_DATA_HOME/GameEnginePlus/levels on Linux, but fallback to ./levels if env var is not set for some reason

// Microsoft's C runtime doesn't have a secure way to get env vars without risking buffer overflow, so we have to use _dupenv_s to safely fetch the APPDATA variable. 
// On other platforms we can just use std::getenv.	
#ifdef _MSC_VER
			char* envBuf = nullptr; size_t len = 0; errno_t er = _dupenv_s(&envBuf, &len, "APPDATA");
			if (er == 0 && envBuf && envBuf[0] != '\0') base = fs::path(envBuf) / "GameEnginePlus" / "levels";
			else base = fs::path("levels");
			free(envBuf);
// On non-Windows platforms, we can just use std::getenv to fetch the APPDATA variable, which is typically used for application data storage. We check if it's set and non-empty, 
// and if so we use it as the base path for levels. If it's not set, we fall back to a local "levels" directory.
#else
			const char* appdata = std::getenv("APPDATA");
			if (appdata && appdata[0] != '\0') base = fs::path(appdata) / "GameEnginePlus" / "levels";
			else base = fs::path("levels");
// In both cases, we end up with a base path where levels are stored. We will create a new directory under this base path with the name of the new level, and also 
// create a "chunks" subdirectory and a "meta.txt" file to store metadata about the level such as the tileset key and layer names.
#endif
			try {
				fs::create_directories(base / name);
				fs::create_directories(base / name / "chunks");
				std::ofstream meta((base / name / "meta.txt").string(), std::ios::trunc);
				if (meta) {
					meta << "tileset=" << m_tilesetKeyBuf << "\n";
					meta << "layers=background,main,upper\n";
				}
			} catch(...) {}
			RefreshAvailableLevels();
		}
	}

	// If we have a level selected, show the active layer selection combo box and unselected layer opacity slider. The active layer selection allows the user 
	// to choose which layer they are currently editing, while the opacity slider lets them adjust how transparent the unselected layers appear, which can 
	// help with visibility when working on a specific layer.
	if (m_levelSelected) {
		ImGui::Text("Active layer:");
		// ImGui::Combo overload for vector of strings requires char* const* or use manual combo
		if (ImGui::BeginCombo("Layer", m_layerNames[m_activeLayer].c_str())) {
			for (int i = 0; i < (int)m_layerNames.size(); ++i) {
				bool selected = (m_activeLayer == i);
				if (ImGui::Selectable(m_layerNames[i].c_str(), selected)) m_activeLayer = i;
				if (selected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		// clamp active layer
		if (m_activeLayer < 0) m_activeLayer = 0;
		if (m_activeLayer >= (int)m_layerNames.size()) m_activeLayer = (int)m_layerNames.size() - 1;
		m_chunkManager.SetActiveLayer(m_activeLayer);
	}

	// Slider to control transparency of unselected layers
	float alphaVal = m_chunkManager.GetUnselectedLayerAlpha();
	if (ImGui::SliderFloat("Unselected layer opacity", &alphaVal, 0.0f, 1.0f)) {
		m_chunkManager.SetUnselectedLayerAlpha(alphaVal);
	}

	// List available levels
	ImGui::Separator();
	if (m_availableLevels.empty())	RefreshAvailableLevels(); // if we don't have any levels loaded yet, try to load them from disk
	for (size_t i = 0; i < m_availableLevels.size(); ++i) {
		bool sel = (int)i == m_selectedLevelIndex;
		if (ImGui::Selectable(m_availableLevels[i].c_str(), sel)) m_selectedLevelIndex = (int)i;
	}

	// If we have a valid level selected, show buttons to switch to it, export it, or delete it. The export button will copy the entire level 
	// directory to a new location with "_export" appended to the name, which allows the user to easily access the raw level data for use in 
	// the game or for sharing with others.	
	if (m_selectedLevelIndex >= 0 && m_selectedLevelIndex < (int)m_availableLevels.size()) {
	if (ImGui::Button("Switch To")) SwitchToLevel(m_availableLevels[m_selectedLevelIndex]);
		ImGui::SameLine();
	if (ImGui::Button("Export Level")) {
		std::string name = m_availableLevels[m_selectedLevelIndex];
		// use same base as other operations
		fs::path base;
#ifdef _MSC_VER
			char* envBuf = nullptr; size_t len = 0; errno_t er = _dupenv_s(&envBuf, &len, "APPDATA");
			if (er == 0 && envBuf && envBuf[0] != '\0') base = fs::path(envBuf) / "GameEnginePlus" / "levels";
			else base = fs::path("levels");
			free(envBuf);
#else
			const char* appdata = std::getenv("APPDATA");
			if (appdata && appdata[0] != '\0') base = fs::path(appdata) / "GameEnginePlus" / "levels";
			else base = fs::path("levels");
#endif
			fs::path src = base / name;
			fs::path dst = base / (name + "_export");
			try { fs::remove_all(dst); fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing); m_exportMessage = std::string("Exported to ") + dst.string(); } catch(...) { m_exportMessage = "Export failed"; }
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete Level")) { m_pendingDeleteName = m_availableLevels[m_selectedLevelIndex]; ImGui::OpenPopup("Confirm Delete"); }
	}

	if (ImGui::BeginPopupModal("Confirm Delete", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Delete level '%s' and all its data?", m_pendingDeleteName.c_str());
		if (ImGui::Button("Yes")) {
			if (!m_pendingDeleteName.empty()) {
				if (m_currentLevelName == m_pendingDeleteName) SwitchToLevel("");
#ifdef _MSC_VER
				char* envBuf = nullptr; size_t len = 0; errno_t er = _dupenv_s(&envBuf, &len, "APPDATA");
				if (er == 0 && envBuf && envBuf[0] != '\0') {
					try { fs::remove_all(fs::path(envBuf) / "GameEnginePlus" / "levels" / m_pendingDeleteName); } catch(...) {}
				} else {
					try { fs::remove_all(fs::path("levels") / m_pendingDeleteName); } catch(...) {}
				}
				free(envBuf);
#else
				const char* appdata = std::getenv("APPDATA");
				if (appdata && appdata[0] != '\0') {
					try { fs::remove_all(fs::path(appdata) / "GameEnginePlus" / "levels" / m_pendingDeleteName); } catch(...) {}
				} else {
					try { fs::remove_all(fs::path("levels") / m_pendingDeleteName); } catch(...) {}
				}
#endif
				RefreshAvailableLevels();
				m_selectedLevelIndex = -1;
			}
			m_pendingDeleteName.clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("No")) { m_pendingDeleteName.clear(); ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}

	if (!m_exportMessage.empty()) ImGui::TextUnformatted(m_exportMessage.c_str());

	ImGui::End();
}
/////////////////////////////////



/////////////////////////////////
// EnsureVisibleChunks - Ensure that the chunks covering the current camera view are loaded. This function calculates the bounds of the camera's viewport 
// in world coordinates, converts those to tile coordinates, and then tells the chunk manager to ensure that all chunks intersecting that tile rectangle 
// are loaded. The marginChunks parameter allows for loading additional chunks around the edges of the view to prevent pop-in when moving the camera.
void LevelEditorScene::EnsureVisibleChunks() {
	// compute view bounds in world coords from active camera
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt) return;
	CCamera* cam = *camOpt;
	float halfW = cam->m_viewportWidth * 0.5f * cam->m_zoom;
	float halfH = cam->m_viewportHeight * 0.5f * cam->m_zoom;
	int tx0 = (int)std::floor((cam->m_position.x - halfW) / m_tileSize);
	int ty0 = (int)std::floor((cam->m_position.y - halfH) / m_tileSize);
	int tx1 = (int)std::floor((cam->m_position.x + halfW) / m_tileSize);
	int ty1 = (int)std::floor((cam->m_position.y + halfH) / m_tileSize);
	m_chunkManager.EnsureChunksInTileRect(tx0, ty0, tx1, ty1, m_marginChunks);
}
/////////////////////////////////



/////////////////////////////////
// AtlasLoadWorker - runs on a background thread. Performs atlas load and writes results into guarded members using m_atlasLoadMutex. Note: the code is thread safe 
// so we can ignore the warnings about the caller failing to hold lock - the caller just needs to ensure it doesn't read the results until m_atlasLoadFinished is true, 
// which is guaranteed by the logic in Update() that checks m_atlasLoadFinished before reading the results.
void LevelEditorScene::AtlasLoadWorker(std::string key, std::string path, int w, int h) {
	bool ok = m_gameEngine.GetTextureManager().LoadAtlas(key, path, w, h);
	{
		std::lock_guard<std::mutex> threadLock(m_atlasLoadMutex);
		m_atlasLoadSuccess = ok;
		m_atlasLoadFinished = true;
		if (ok) {
			m_currentTilesetPath = path;  // Save the path for metadata
		}
		m_atlasLoadMessage = ok ? std::string("Loaded: ") + path : std::string("Failed: ") + path;
	}
	m_atlasLoading = false;
}
/////////////////////////////////



/////////////////////////////////
// ApplyMainCameraView - Apply the main camera's view to the render window. This function retrieves the main camera from the camera system, calculates the appropriate 
// view based on the camera's position, zoom, and viewport size, and then sets that view on the SFML render window. It also includes logic to clamp the camera's position 
// within the bounds of the map if those bounds are available, to prevent the user from panning the camera outside the edges of the level. The clamping is based on the 
// fixed bounds derived from the disk data, which are updated when tiles are painted, ensuring that the camera always has valid bounds to work with. 
// 
// Note as above the code is thread safe. So we can ignore warnings about caller not holding locks - the caller just needs to ensure it doesn't read camera position until 
// after m_haveBounds is set to true, which is guaranteed by the logic in Update() that checks m_haveBounds before calling this function.
void LevelEditorScene::ApplyMainCameraView() {
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt) return;
	CCamera* cam = *camOpt;
	sf::View v;
	v.setSize(sf::Vector2f(cam->m_viewportWidth * cam->m_zoom, cam->m_viewportHeight * cam->m_zoom));

	// Clamp camera using the fixed disk-derived bounds (set once in InitializeGame, updated when tiles are painted).
	float halfW = cam->m_viewportWidth * 0.5f * cam->m_zoom;
	float halfH = cam->m_viewportHeight * 0.5f * cam->m_zoom;
	Vec2 newPos = cam->m_position;
	if (m_haveBounds) {
		float minCx = m_mapMin.x + halfW;
		float maxCx = m_mapMax.x - halfW;
		float minCy = m_mapMin.y + halfH;
		float maxCy = m_mapMax.y - halfH;
		if (minCx <= maxCx) newPos.x = std::clamp(newPos.x, minCx, maxCx);
		else newPos.x = (m_mapMin.x + m_mapMax.x) * 0.5f;
		if (minCy <= maxCy) newPos.y = std::clamp(newPos.y, minCy, maxCy);
		else newPos.y = (m_mapMin.y + m_mapMax.y) * 0.5f;
	}
	cam->m_position = newPos;

	// Snap camera to sub-pixel grid based on zoom level to reduce shimmer/jitter when rendering
	// At zoom levels 2.0x or higher, snap to tile alignment (32 pixels). Otherwise snap to half-pixel.
	// This ensures that the view center aligns with rasterization boundaries and remains stable during camera movement.
	float snapGrid = (cam->m_zoom >= 2.0f) ? 32.0f : 0.5f;  // Snap to tile grid at 2x+ zoom, else half-pixel
	float snapX = std::round(newPos.x / snapGrid) * snapGrid;
	float snapY = std::round(newPos.y / snapGrid) * snapGrid;

	v.setCenter(sf::Vector2f(snapX, snapY));
	m_window.setView(v);
}
/////////////////////////////////



/////////////////////////////////
// Process user input for the level editor scene. This function checks for specific key presses (e.g., Escape to close the window) and can be expanded in the future to handle 
// additional input for camera movement, zooming, or other editing actions.
void LevelEditorScene::ProcessInput() {
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		m_gameEngine.ChangeScene("MainMenu");
	}
}

/////////////////////////////////
// SaveLevelMetadata - Saves the current level's metadata (tileset, layers, etc.) to meta.txt
void LevelEditorScene::SaveLevelMetadata() {
	if (m_currentLevelName.empty()) return;  // No level loaded, can't save

	namespace fs = std::filesystem;
	std::error_code ec;
	fs::path base;
#ifdef _MSC_VER
	char* envBuf = nullptr; size_t len = 0;
	if (_dupenv_s(&envBuf, &len, "APPDATA") == 0 && envBuf && envBuf[0] != '\0') {
		base = fs::path(envBuf) / "GameEnginePlus" / "levels";
	} else {
		base = fs::path("levels");
	}
	free(envBuf);
#else
	const char* appdata = std::getenv("APPDATA");
	if (appdata && appdata[0] != '\0') base = fs::path(appdata) / "GameEnginePlus" / "levels";
	else base = fs::path("levels");
#endif

	try {
		fs::path metaPath = base / m_currentLevelName / "meta.txt";
		std::ofstream meta(metaPath.string(), std::ios::trunc);
		if (meta) {
			meta << "tileset=" << m_tilesetKeyBuf << "\n";
			if (!m_currentTilesetPath.empty()) {
				meta << "tilesetPath=" << m_currentTilesetPath << "\n";
			}
			// Write layer names
			meta << "layers=";
			for (size_t i = 0; i < m_layerNames.size(); ++i) {
				if (i > 0) meta << ",";
				meta << m_layerNames[i];
			}
			meta << "\n";
			std::cout << "LevelEditorScene: Saved metadata to " << metaPath.string() << "\n";
		}
	} catch(...) {
		std::cerr << "LevelEditorScene: Failed to save metadata\n";
	}
}
/////////////////////////////////