/////////////////////////////////
// PathTestScene.cpp - Implementation of the PathTestScene class, which is a scene for testing pathfinding in a tile-based game engine. 
// This scene manages the camera, chunk loading, and pathfinding logic, allowing users to interactively test pathfinding algorithms by 
// setting start and goal points on the map. The scene also handles rendering of the map, pathfinding results, and debug overlays using 
// ImGui for user interface elements.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "PathTestScene.h"
#include "GameEngine.h"
#include "CTransform.h"
#include "CPathRequest.h"
#include "CCamera.h"
#include "CRectangle.h"
#include "imgui/imgui.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <Windows.h>
/////////////////////////////////



/////////////////////////////////
// Namespace alias for filesystem
namespace fs = std::filesystem;
/////////////////////////////////



/////////////////////////////////
// PathTestScene implementation
PathTestScene::PathTestScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager)
	: Scene(engine, entityManager), m_window(win), m_chunkManager(32, 32, 32.0f), 
	  m_pathSystem(m_chunkManager, entityManager) {}
/////////////////////////////////



/////////////////////////////////
// Destructor for the PathTestScene class. Ensures that all chunks are saved to disk when the scene is destroyed.
PathTestScene::~PathTestScene() {
	m_chunkManager.SaveAllChunks();
}
/////////////////////////////////



/////////////////////////////////
// InitializeGame - Initializes the game scene, setting up the camera entity, map bounds, and scanning for available levels.
void PathTestScene::InitializeGame(sf::Vector2u /*windowSize*/) {
	// Create  and setup a camera entity, similar to the LevelEditorScene
	m_cameraEntity = GetEntityManager().AddEntity(EntityType::Default);
	m_cameraEntity->AddComponent<CTransform>(Vec2(0, 0), Vec2::Zero);
	
	auto camera = m_cameraEntity->AddComponent<CCamera>(Vec2(0, 0), 1.0f);
	camera->m_isMainCamera = true;
	camera->m_isActive = true;
	camera->m_viewportWidth = (float)m_window.getSize().x;
	camera->m_viewportHeight = (float)m_window.getSize().y;
	camera->m_smoothness = 0.0f; // Disable smoothing - camera is controlled directly via panning and bounds clamping

	// Initialize map bounds
	m_mapMin = Vec2(-512, -512);
	m_mapMax = Vec2(512, 512);
	m_haveBounds = false;

	// Scan available levels
	ScanLevelFiles();

	// Create movement test entity (small red square for path following demonstration)
	m_movementTester = GetEntityManager().AddEntity(EntityType::Default);
	std::cout << "[PathTestScene::InitializeGame] Created movement tester, entity pointer: " << m_movementTester << std::endl;
	
	// Add components to the movement tester entity
	if (m_movementTester) {
		// Place test entity at camera origin (0, 0) so it's always visible initially
		m_movementTester->AddComponent<CTransform>(Vec2(0, 0), Vec2::Zero);
		auto rect = std::make_unique<CRectangle>(16.0f, 16.0f);
		rect->SetColor(255.0f, 0.0f, 0.0f, 200);  // Red square
		m_movementTester->AddComponentPtr<CShape>(std::move(rect));
		m_movementTester->AddComponent<CPathFollower>(300.0f);  // 300 pixels per second
		
		std::cout << "[PathTestScene] Movement tester entity created at (0, 0) - watch the center of the screen!" << std::endl;
		std::cout << "[PathTestScene] Entity alive: " << (m_movementTester->IsAlive() ? "YES" : "NO") << std::endl;
		std::cout << "[PathTestScene] Has CShape: " << (m_movementTester->GetComponent<CShape>() ? "YES" : "NO") << std::endl;
	} else {
		std::cout << "[PathTestScene::InitializeGame] FAILED to create movement tester entity!" << std::endl;
	}

	// Set initial base path (will be overridden when level is loaded)
	m_chunkManager.SetBasePath("levels/chunks");

	// Determine world size from saved chunk bounds BEFORE loading chunks
	float minX, minY, maxX, maxY;
	if (m_chunkManager.GetSavedChunkBounds(minX, minY, maxX, maxY)) {
		//int tileSize = (int)m_chunkManager.GetTileSize();

		//int minTileX = (int)std::floor(minX / tileSize);
		//int minTileY = (int)std::floor(minY / tileSize);
		//int maxTileX = (int)std::ceil(maxX / tileSize);
		//int maxTileY = (int)std::ceil(maxY / tileSize);

		//int worldW = maxTileX - minTileX;
		//int worldH = maxTileY - minTileY;

		//m_chunkManager.SetWorldOffset(minTileX, minTileY); // <-- safe now
		//m_chunkManager.SetWorldSize(worldW, worldH);
	} else {
		// No saved chunks yet — world mask stays empty until chunks load
		//m_chunkManager.SetWorldSize(0, 0);
	}


	m_chunkManager.SetMaxLoadedChunks(256);
}
/////////////////////////////////



/////////////////////////////////
// ScanLevelFiles - Scans the APPDATA/GameEnginePlus/levels directory for available levels, checking for directories that contain a "chunks" subdirectory.
void PathTestScene::ScanLevelFiles() {
	m_availableLevels.clear();

	// Scan APPDATA/GameEnginePlus/levels
	std::error_code ec;
	fs::path base;

	// Determine the base path for levels based on the operating system
#ifdef _MSC_VER
	char* appdata_buf = nullptr; // Create a buffer to hold the APPDATA path
	size_t len = 0;				 // Variable to hold the length of the APPDATA path

	// Use _dupenv_s to safely retrieve the APPDATA environment variable on Windows
	// if found and not empty, _dupenv_s allocates memory for appdata_buf, which must be FREED LATER (REMEMBER TO FREE, NO SERIOUSLY REMEMBER TO FREE)
	if (_dupenv_s(&appdata_buf, &len, "APPDATA") == 0 && appdata_buf) {
		base = fs::path(appdata_buf) / "GameEnginePlus" / "levels";
		free(appdata_buf); // Free the allocated memory for appdata_buf after use to avoid memory leaks (I REMEMBERED, actually AI did it for me... thanks I guess)
	} else {
		base = fs::path("levels");
	}
#else // For non-Windows platforms, use std::getenv to retrieve the APPDATA environment variable
	const char* appdata = std::getenv("APPDATA");
	if (appdata && appdata[0] != '\0') {
		base = fs::path(appdata) / "GameEnginePlus" / "levels";
	} else {
		base = fs::path("levels");
	}
#endif

	// Iterate through the directories in the base path and check for the presence of a "chunks" subdirectory
	try {
		if (fs::exists(base, ec) && !ec) {
			for (auto& entry : fs::directory_iterator(base, ec)) {
				if (entry.is_directory()) {
					// Check if this directory has a "chunks" subdirectory
					auto chunksDir = entry.path() / "chunks";
					if (fs::exists(chunksDir)) {
						m_availableLevels.push_back(entry.path().filename().string()); // Add the level name to the available levels list
					}
				}
			}
		}
	} catch (...) {
	}

	// Sort the available levels alphabetically for easier selection in the UI
	std::sort(m_availableLevels.begin(), m_availableLevels.end());
}



/////////////////////////////////
// SwitchToLevel - Switches the current level to the specified level name, loading the appropriate tileset and layers based on metadata in "meta.txt".
// ***** WARNING ***** Ohh my god this is a fucking long function, Its a damn mess, but I guess it will be fine, right??? right??? Future me no 
// fucking clue????? Spend sometime later to refactor this into smaller functions, maybe even a LevelLoader class or something, but for now, 
// let's just get it working.
bool PathTestScene::SwitchToLevel(const std::string& name) {
	std::cout << "[PathTestScene::SwitchToLevel] CALLED with level: " << name << "\n";
	m_currentLevelName = name;

	// Determine the base path for levels based on the operating system
	fs::path base;
	


	// ==========================
	// We need to handle the Windows Enviorment differently, It requires _dupenv_s to safely retrieve the APPDATA environment variable on Windows, 
	// and std::getenv for other platforms, so remember to free the allocated memory for appdata_buf on Windows after use.
#ifdef _MSC_VER
	char* appdata_buf = nullptr; // buffer
	size_t len = 0;				 // length

	// Use _dupenv_s to safely retrieve the APPDATA environment variable on Windows
	if (_dupenv_s(&appdata_buf, &len, "APPDATA") == 0 && appdata_buf) {
		base = fs::path(appdata_buf) / "GameEnginePlus" / "levels";
		free(appdata_buf); // Free the allocated memory for appdata_buf after use - I membered!!! Honestly it wasn't AI, I membered it myself, I swear
	} else {
		base = fs::path("levels");
	}
#else // For non-Windows platforms there is no AppData, so we can just use std::getenv to retrieve the APPDATA environment variable, it wont find it
	  // but we can just fallback to the "levels" directory in the current working directory.
	const char* appdata = std::getenv("APPDATA");
	if (appdata && appdata[0] != '\0') {
		base = fs::path(appdata) / "GameEnginePlus" / "levels";
	} else {
		base = fs::path("levels");
	}
#endif
	// ==========================



	// Set the chunk manager's base path to the "chunks" subdirectory of the specified level
	fs::path chunkPath = base / name / "chunks";
	m_chunkManager.SetBasePath(chunkPath.string());

	// Parse meta.txt to get tileset and layers
	std::string tilesetKey;
	std::string tilesetPath;
	std::vector<std::string> layers = {"background", "main", "upper"};
	{
		auto metaPath = base / name / "meta.txt";
		try {
			std::ifstream meta(metaPath.string());

			// Parse key=value pairs from meta.txt
			if (meta) {
				// Read each line and split on '=' to get key-value pairs
				std::string line;
				while (std::getline(meta, line)) {
					auto eq = line.find('=');
					if (eq == std::string::npos) continue; // Skip lines without '='
					
					// Trim whitespace from key and value
					std::string key = line.substr(0, eq);
					std::string val = line.substr(eq + 1);

					// set tileset and layers based on key-value pairs
					if (key == "tileset") {
						tilesetKey = val;
					} else if (key == "tilesetPath") {
						tilesetPath = val;
					} else if (key == "layers") {
						layers.clear(); // clear default layers and use the ones specified in meta.txt
						// We split the value on commas to get individual layer names and add them to the layers vector
						size_t start = 0;
						while (start < val.size()) {
							auto comma = val.find(',', start);
							if (comma == std::string::npos) comma = val.size();
							std::string token = val.substr(start, comma - start);
							if (!token.empty()) layers.push_back(token);
							start = comma + 1;
						}
					}
				}
			}
		} catch (...) {
		}
	}

	// Set tileset key in chunk manager and load tileset if specified
	if (!tilesetKey.empty()) {
		m_chunkManager.SetTilesetKey(tilesetKey);

		// If we have a tileset path from metadata, load it
		if (!tilesetPath.empty()) {
			auto& textMan = GameEngine::GetInstance().GetTextureManager();
			// Check if already loaded
			auto atlasOpt = textMan.GetAtlas(tilesetKey);
			if (!atlasOpt.has_value() || !*atlasOpt) {
				std::cout << "PathTestScene: Loading tileset '" << tilesetKey << "' from " << tilesetPath << "\n";
				// Use the stored tile size
				int tileSize = (int)m_chunkManager.GetTileSize();
				textMan.LoadAtlas(tilesetKey, tilesetPath, tileSize, tileSize);
			}
		} else {
			// Fallback: Try common locations if no path in metadata
			auto& tm = GameEngine::GetInstance().GetTextureManager();
			auto atlasOpt = tm.GetAtlas(tilesetKey);
			if (!atlasOpt.has_value() || !*atlasOpt) {
				std::vector<std::string> searchPaths = {
					"assets/tilesets/" + tilesetKey + ".png",
					"assets/tilesets/" + tilesetKey + ".jpg",
					"assets/" + tilesetKey + ".png",
					"assets/" + tilesetKey + ".jpg",
					tilesetKey + ".png",
					tilesetKey + ".jpg",
				};

				int tileSize = (int)m_chunkManager.GetTileSize();
				for (const auto& searchPath : searchPaths) {
					if (fs::exists(searchPath)) {
						std::cout << "PathTestScene: Found tileset at " << searchPath << ", loading...\n";
						tm.LoadAtlas(tilesetKey, searchPath, tileSize, tileSize);
						break;
					}
				}
			}
		}
	}

	// Set number of layers for proper rendering
	if (!layers.empty()) {
		m_chunkManager.SetNumLayers((int)layers.size());
	}

	// For pathfinding test, render all layers with same opacity set to render the "main" layer as active (middle layer) for game logic
	m_chunkManager.SetActiveLayer(std::min(1, (int)layers.size() - 1));
	
	// Render unselected layers at full opacity so all layers are visible
	m_chunkManager.SetUnselectedLayerAlpha(1.0f);

	// Clear any previously loaded chunks before loading the new level
	for (int i = 0; i < 20; ++i) {
		m_chunkManager.UpdateMainThread(); // Process any pending chunk finalizations before clearing
	}

	// Clear all loaded chunks to ensure a clean state for the new level
	{
		std::cout << "[PathTestScene::SwitchToLevel] Clearing all loaded chunks before loading new level...\n";
		m_chunkManager.ClearAllLoadedChunks();
		std::cout << "[PathTestScene::SwitchToLevel] All loaded chunks cleared.\n";
	}

	// Now load the new level
	m_chunkManager.LoadAllSavedChunks();

	// Process pending chunk loads - call UpdateMainThread multiple times to drain the finalization queue
	// This ensures all chunks are loaded and finalized before we proceed
	for (int i = 0; i < 20; ++i) {
		m_chunkManager.UpdateMainThread();
	}

	m_chunkManager.RebuildAllChunksFromTileset();

	
	// IMMEDIATE FIX: Shift chunks to positive coordinates to ensure they are visible in the viewport
	m_chunkManager.ShiftChunksToPositiveCoords();
	m_chunkManager.UpdateMainThread();

	// IMMEDIATE FIX: Reposition test entity to a reasonable default level center
	// This ensures the entity is visible regardless of whether GetSavedChunkBounds works
	if (m_movementTester) {
		if (auto testTransform = m_movementTester->GetComponent<CTransform>()) {
			Vec2 quickCenter(0.0f, -1536.0f);  // Default level center for most levels
			testTransform->m_position = quickCenter;
			std::cout << "  [PathTestScene] Repositioned test entity to quick center: (" << quickCenter.x << ", " << quickCenter.y << ")\n";

			// Disable movement
			if (auto follower = m_movementTester->GetComponent<CPathFollower>()) {
				follower->isActive = false;
			}
		}
	}


	// Get bounds and debug info
	float dMinX, dMinY, dMaxX, dMaxY;
	bool hasBounds = m_chunkManager.GetSavedChunkBounds(dMinX, dMinY, dMaxX, dMaxY);

	std::cout << "  Bounds found: " << (hasBounds ? "YES" : "NO");
	if (hasBounds) {
		std::cout << " (" << dMinX << "," << dMinY << ") to (" << dMaxX << "," << dMaxY << ")";
	}
	std::cout << "\n";

	// If bounds are found, calculate world size and offset in tiles
	if (m_chunkManager.GetSavedChunkBounds(dMinX, dMinY, dMaxX, dMaxY)) {
		int tileSize = (int)m_chunkManager.GetTileSize();

		int minTileX = (int)std::floor(dMinX / tileSize);
		int minTileY = (int)std::floor(dMinY / tileSize);
		int maxTileX = (int)std::ceil(dMaxX / tileSize);
		int maxTileY = (int)std::ceil(dMaxY / tileSize);

		int worldW = maxTileX - minTileX;
		int worldH = maxTileY - minTileY;

		m_chunkManager.SetWorldOffset(minTileX, minTileY); // <-- safe now
		m_chunkManager.SetWorldSize(worldW, worldH);	   // <-- safe now
		m_chunkManager.BuildWorldMask();


		//  ===== DEBUG ===== 
		// Dump world mask once
		std::cout << "\n=== WORLD MASK DUMP ===\n";
		for (int y = 0; y < m_chunkManager.worldHeight; ++y) {
			for (int x = 0; x < m_chunkManager.worldWidth; ++x) {
				bool solid = m_chunkManager.worldMask[y * m_chunkManager.worldWidth + x];
				std::cout << (solid ? '1' : '0');
			}
			std::cout << "\n";
		}

	}	

	// Log tile counts per layer
	//std::cout << "  Chunk/Layer tile count:\n";
	for (int layer = 0; layer < m_chunkManager.GetNumLayers(); ++layer) {
		int totalTiles = 0;
		// We need to count tiles per layer - this would require access to internal chunks
		// For now just log that the layer exists
		//std::cout << "    Layer " << layer << ": (loaded)\n";
	}



	if (hasBounds) {
		m_mapMin = Vec2(dMinX, dMinY);
		m_mapMax = Vec2(dMaxX, dMaxY);
		m_haveBounds = true;
		//std::cout << "  [SwitchToLevel] Bounds detected, checking test entity repositioning...\n";

		// Center camera on level
		if (m_cameraEntity) {
			if (auto cam = m_cameraEntity->GetComponent<CCamera>()) {
				auto tr = m_cameraEntity->GetComponent<CTransform>();
				Vec2 center = (m_mapMin + m_mapMax) * 0.5f;
				tr->m_position = center;
				cam->m_position = center;
				std::cout << "  Camera centered at (" << center.x << "," << center.y << ")\n";

				// Also reposition the test entity to the center so it's visible after level load
				if (m_movementTester) {
					//std::cout << "  [PathTestScene] m_movementTester exists, attempting repositioning...\n";
					if (auto testTransform = m_movementTester->GetComponent<CTransform>()) {
						testTransform->m_position = center;
						//std::cout << "  [PathTestScene] SUCCESS: Test entity repositioned to level center (" << center.x << ", " << center.y << ")\n";

						// IMPORTANT: Disable any active movement so it doesn't move away from center
						if (auto follower = m_movementTester->GetComponent<CPathFollower>()) {
							follower->isActive = false;
							//std::cout << "  [PathTestScene] Disabled movement to keep entity at level center\n";
						}
					} else {
						//std::cout << "  [PathTestScene] ERROR: m_movementTester has no CTransform component!\n";
					}
				} else {
					//std::cout << "  [PathTestScene] ERROR: m_movementTester is NULL!\n";
				}

				// Reset manual pathfinding state for the new level
				m_manualStartSet = false;
				m_manualGoalSet = false;
				m_manualPathComplete = false;
				m_manualStart = Vec2(0, 0);
				m_manualGoal = Vec2(0, 0);
				//std::cout << "  [PathTestScene] Manual pathfinding state reset for new level\n";
			}
		}
	}

	return true;
}
/////////////////////////////////



/////////////////////////////////
// EnsureVisibleChunks - Ensures that all chunks within the camera's view, plus a margin, are loaded. This method calculates the tile coordinates of 
// the visible area based on the camera's position and zoom level, and requests the ChunkManager to load any chunks that fall within this area.
void PathTestScene::EnsureVisibleChunks() {
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt) return;
	CCamera* cam = *camOpt;


	// Ensure chunks within camera view + margin are loaded
	float halfW = cam->m_viewportWidth * 0.5f * cam->m_zoom;
	float halfH = cam->m_viewportHeight * 0.5f * cam->m_zoom;


	int tx0 = (int)std::floor((cam->m_position.x - halfW) / m_tileSize);
	int ty0 = (int)std::floor((cam->m_position.y - halfH) / m_tileSize);
	int tx1 = (int)std::ceil((cam->m_position.x + halfW) / m_tileSize);
	int ty1 = (int)std::ceil((cam->m_position.y + halfH) / m_tileSize);
	m_chunkManager.EnsureChunksInTileRect(tx0, ty0, tx1, ty1, 2);

	// 3. Evict chunks outside radius
	//m_chunkManager.EvictChunksOutsideRadius(tx0, tx1, ty0, ty1);
}
/////////////////////////////////



/////////////////////////////////
// ApplyMainCameraView - Applies the main camera's view to the SFML render window. This method retrieves the main camera's position, zoom, and viewport size,
void PathTestScene::ApplyMainCameraView() {
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt) return;
	CCamera* cam = *camOpt;

	sf::View v;
	v.setSize(sf::Vector2f(cam->m_viewportWidth * cam->m_zoom, cam->m_viewportHeight * cam->m_zoom));

	// Clamp camera to bounds
	float halfW = cam->m_viewportWidth * 0.5f * cam->m_zoom;
	float halfH = cam->m_viewportHeight * 0.5f * cam->m_zoom;
	Vec2 newPos = cam->m_position;

	if (m_haveBounds) {
		float minCx = m_mapMin.x + halfW;
		float maxCx = m_mapMax.x - halfW;
		float minCy = m_mapMin.y + halfH;
		float maxCy = m_mapMax.y - halfH;

		if (minCx <= maxCx) {
			newPos.x = std::clamp(newPos.x, minCx, maxCx);
		} else {
			newPos.x = (m_mapMin.x + m_mapMax.x) * 0.5f;
		}

		if (minCy <= maxCy) {
			newPos.y = std::clamp(newPos.y, minCy, maxCy);
		} else {
			newPos.y = (m_mapMin.y + m_mapMax.y) * 0.5f;
		}
	}

	cam->m_position = newPos;

	// Snap camera to sub-pixel grid based on zoom level to reduce shimmer/jitter when rendering
	// At zoom levels 2.0x or higher, snap to tile alignment (32 pixels). Otherwise snap to half-pixel.
	float snapGrid = (cam->m_zoom >= 2.0f) ? 32.0f : 0.5f;  // Snap to tile grid at 2x+ zoom, else half-pixel
	float snapX = std::round(newPos.x / snapGrid) * snapGrid;
	float snapY = std::round(newPos.y / snapGrid) * snapGrid;

	v.setCenter(sf::Vector2f(snapX, snapY));
	m_window.setView(v);
}
/////////////////////////////////



/////////////////////////////////
// Update - Updates the scene state, including camera movement, chunk loading, and pathfinding. This method is called every frame with the elapsed time since the last update.
void PathTestScene::Update(float deltaTime) {
	// Update camera
	if (m_cameraEntity) {
		m_cameraSystem.Update(deltaTime, GetEntityManager());
	}

	// Ensure visible chunks are loaded
	EnsureVisibleChunks();
	//m_chunkManager.UpdateStreaming(m_cameraEntity);
	m_chunkManager.UpdateMainThread();

	// Apply camera view
	ApplyMainCameraView();

	// Update path system
	//m_pathSystem.SetNodesPerFrame(m_nodesPerFrame);
	m_pathSystem.Update(deltaTime);

	// ImGui UI for level selection
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
	ImGui::Begin("PathTest Level Selector");

	ImGui::Text("Available Levels:");
	for (const auto& level : m_availableLevels) {
		if (ImGui::Button(level.c_str(), ImVec2(-1, 0))) {
			SwitchToLevel(level);
		}
	}

	ImGui::Separator();
	ImGui::SliderInt("Nodes/Frame", &m_nodesPerFrame, 10, 2000);

	if (m_manualStartSet && m_manualGoalSet) {
		ImGui::Text("Path Found: %s", m_manualPathComplete ? "YES" : "NO");
		if (m_manualPath.has_value()) {
			ImGui::Text("Path Length: %zu", m_manualPath->size());
		}
	}

	ImGui::Text("Click: LMB=start, RMB=goal");
	ImGui::End();

	// Handle middle-mouse panning similar to LevelEditorScene
	{
		// Poll mouse/button states via InputController
		bool isMiddleDown = m_gameEngine.GetInputController().IsMouseButtonDown(sf::Mouse::Button::Middle);
		sf::Vector2i mousePos = sf::Mouse::getPosition(m_window);
		if (isMiddleDown && !m_panning) {
			m_panning = true;
			m_panStart = mousePos;
			// record current camera position
			auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
			if (camOpt) {
				m_camPanStart = (*camOpt)->m_position;
			}
		}
		if (!isMiddleDown && m_panning) {
			m_panning = false; // end panning
		}
		if (m_panning) {
			sf::Vector2i delta = mousePos - m_panStart;
			auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
			if (camOpt) {
				Vec2 newPos = m_camPanStart - Vec2((float)delta.x, (float)delta.y);
				// clamp to bounds if available
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
				// Update camera entity transform too
				if (m_cameraEntity) {
					if (auto t = m_cameraEntity->GetComponent<CTransform>()) {
						t->m_position = newPos;
					}
				}
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// Render - Renders the scene, including the loaded chunks and any debug overlays such as start/goal markers and paths. This method is called every frame after Update.
void PathTestScene::Render() {
	// Save the world view
	sf::View worldView = m_window.getView();

	// Enqueue chunks
	m_chunkManager.EnqueueChunks(m_renderQueue, worldView);

	// Flush render queue first so world tiles are drawn
	m_renderQueue.Flush(m_window);

	// Draw debug overlay (paths, markers) in world view
	m_window.setView(worldView);
	RenderDebugOverlay();

	// NOTE: Do NOT reset view here - entity shapes need to be rendered in world view too!
	// The view will be reset by the engine after all rendering is complete
}
/////////////////////////////////



/////////////////////////////////
// RenderDebugOverlay - Renders debug overlays for the pathfinding test, including start and goal markers, as well as the computed path if available.
void PathTestScene::RenderDebugOverlay() {
	const float outerRadius = 12.0f;
	const float innerRadius = 8.0f;
	const float pathOutlineThickness = 6.0f;
	const float pathCoreThickness = 3.0f;

	// Draw path as thick outlined segments
	if (m_manualPath.has_value() && !m_manualPath->empty()) {
		sf::Color coreColor = m_manualPathComplete ? sf::Color::Green : sf::Color::Yellow;
		sf::Color outlineColor = sf::Color::Black;

		// Path waypoints are already in world pixel coordinates
		for (size_t i = 1; i < m_manualPath->size(); ++i) {
			auto& a = (*m_manualPath)[i - 1];
			auto& b = (*m_manualPath)[i];

			// a and b are already in world pixel coordinates from PathFinder
			sf::Vector2f pa(a.x, a.y);
			sf::Vector2f pb(b.x, b.y);
			sf::Vector2f diff(pb.x - pa.x, pb.y - pa.y);
			float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);
			if (length <= 0.001f) continue;

			float angle = std::atan2(diff.y, diff.x) * 180.0f / 3.14159265f;
			sf::Vector2f mid((pa.x + pb.x) * 0.5f, (pa.y + pb.y) * 0.5f);

			sf::RectangleShape outlineSeg(sf::Vector2f(length, pathOutlineThickness));
			outlineSeg.setOrigin(sf::Vector2f(length * 0.5f, pathOutlineThickness * 0.5f));
			outlineSeg.setPosition(mid);
			outlineSeg.setRotation(sf::degrees(angle));
			outlineSeg.setFillColor(outlineColor);
			m_window.draw(outlineSeg);

			sf::RectangleShape coreSeg(sf::Vector2f(length, pathCoreThickness));
			coreSeg.setOrigin(sf::Vector2f(length * 0.5f, pathCoreThickness * 0.5f));
			coreSeg.setPosition(mid);
			coreSeg.setRotation(sf::degrees(angle));
			coreSeg.setFillColor(coreColor);
			m_window.draw(coreSeg);
		}
	}

	// Draw start/goal markers with outlines
	if (m_manualStartSet) {
		sf::CircleShape startOutline(outerRadius);
		startOutline.setFillColor(sf::Color::Black);
		startOutline.setOrigin(sf::Vector2f(outerRadius, outerRadius));
		startOutline.setPosition(sf::Vector2f(m_manualStart.x, m_manualStart.y));
		m_window.draw(startOutline);

		sf::CircleShape startCore(innerRadius);
		startCore.setFillColor(sf::Color::Blue);
		startCore.setOrigin(sf::Vector2f(innerRadius, innerRadius));
		startCore.setPosition(sf::Vector2f(m_manualStart.x, m_manualStart.y));
		m_window.draw(startCore);
	}

	if (m_manualGoalSet) {
		sf::CircleShape goalOutline(outerRadius);
		goalOutline.setFillColor(sf::Color::Black);
		goalOutline.setOrigin(sf::Vector2f(outerRadius, outerRadius));
		goalOutline.setPosition(sf::Vector2f(m_manualGoal.x, m_manualGoal.y));
		m_window.draw(goalOutline);

		sf::CircleShape goalCore(innerRadius);
		goalCore.setFillColor(sf::Color::Magenta);
		goalCore.setOrigin(sf::Vector2f(innerRadius, innerRadius));
		goalCore.setPosition(sf::Vector2f(m_manualGoal.x, m_manualGoal.y));
		m_window.draw(goalCore);
	}
}
/////////////////////////////////



/////////////////////////////////
// HandleEvent - Handles input events for the scene, including mouse clicks to set start and goal points for pathfinding. This method converts 
// screen coordinates to world coordinates based on the camera's position and zoom level.
void PathTestScene::HandleEvent(const std::optional<sf::Event>& event) {
	if (!event.has_value()) return;

	// Get camera for manual coordinate conversion
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt) return;
	CCamera* cam = *camOpt;

	//std::cout << "[PathTestScene::HandleEvent] Processing event\n";

	// Use polling-style input like TileMapScene does
	sf::Vector2i mousePixelPos = sf::Mouse::getPosition(m_window);

	// Manually convert screen pixel to world coordinates using camera
	// screen coords are relative to viewport, need to transform through camera view
	float halfW = cam->m_viewportWidth * 0.5f * cam->m_zoom;
	float halfH = cam->m_viewportHeight * 0.5f * cam->m_zoom;

	// Normalize screen coordinates to [-1, 1] range relative to viewport center
	float normX = (2.0f * mousePixelPos.x / cam->m_viewportWidth) - 1.0f;
	float normY = (2.0f * mousePixelPos.y / cam->m_viewportHeight) - 1.0f;

	// Map to world space using camera position and zoom
	float worldX = cam->m_position.x + normX * halfW;
	float worldY = cam->m_position.y + normY * halfH;
	Vec2 mouseWorld(worldX, worldY);

	bool leftMouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	bool rightMouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

	// Single click detection: track if we just pressed
	static bool prevLeftDown = false;
	static bool prevRightDown = false;

	bool leftClicked = leftMouseDown && !prevLeftDown;
	bool rightClicked = rightMouseDown && !prevRightDown;

	if (leftClicked) {
		// Set start point
		m_manualStart = mouseWorld;
		m_manualStartSet = true;

	} else if (leftClicked && m_manualStartSet) {
		// Left click with start already set: Set goal and compute path
		m_manualGoal = mouseWorld;
		m_manualGoalSet = true;
		//std::cout << "[PathTestScene::HandleEvent] Left-click path computation triggered\n";

		float tileSize = m_chunkManager.GetTileSize();

		// Get chunk bounds to determine offset
		float chunkMinX = m_mapMin.x;
		float chunkMinY = m_mapMin.y;

		// Convert world coordinates to tile coordinates, accounting for chunk offset
		// Convert world coordinates to absolute world tile coordinates
		// Do NOT subtract chunkMin - pathfinder expects absolute world tiles

		//int startX = static_cast<int>(std::floor(m_manualStart.x / tileSize));
		//int startY = static_cast<int>(std::floor(m_manualStart.y / tileSize));
		//int goalX = static_cast<int>(std::floor(m_manualGoal.x / tileSize));
		//int goalY = static_cast<int>(std::floor(m_manualGoal.y / tileSize));


		int offX = m_chunkManager.GetWorldOffsetX();
		int offY = m_chunkManager.GetWorldOffsetY();

		int startX = static_cast<int>(std::floor(m_manualStart.x / tileSize)) - offX;
		int startY = static_cast<int>(std::floor(m_manualStart.y / tileSize)) - offY;

		int goalX = static_cast<int>(std::floor(m_manualGoal.x / tileSize)) - offX;
		int goalY = static_cast<int>(std::floor(m_manualGoal.y / tileSize)) - offY;

		m_manualPath = m_pathSystem.FindPathSync(startX, startY, goalX, goalY);
		m_manualPathComplete = m_manualPath.has_value();
		//std::cout << "[PathTestScene::HandleEvent] Path computed: complete=" << m_manualPathComplete << " waypoints=" << (m_manualPath.has_value() ? m_manualPath->size() : 0) << "\n";
	}
	if (rightClicked && m_manualStartSet) {
		// Set goal and compute path
		m_manualGoal = mouseWorld;
		m_manualGoalSet = true;

		float tileSize = m_chunkManager.GetTileSize();

		// Get chunk bounds to determine offset
		float chunkMinX = m_mapMin.x;
		float chunkMinY = m_mapMin.y;

		// Convert world coordinates to tile coordinates, accounting for chunk offset
		// Convert world coordinates to absolute world tile coordinates
		// Do NOT subtract chunkMin - pathfinder expects absolute world tiles

		int startX	= static_cast<int>(std::floor(m_manualStart.x	/ tileSize));
		int startY	= static_cast<int>(std::floor(m_manualStart.y	/ tileSize));
		int goalX	= static_cast<int>(std::floor(m_manualGoal.x	/ tileSize));
		int goalY	= static_cast<int>(std::floor(m_manualGoal.y	/ tileSize));

		m_manualPath = m_pathSystem.FindPathSync(startX, startY, goalX, goalY);
		m_manualPathComplete = m_manualPath.has_value();

		// Activate movement testing with the computed path
		if (m_manualPathComplete && m_movementTester) {
			// The pathfinder returns Vec2 positions that are already in world space
			// NO need to multiply by tileSize again
			std::vector<Vec2> worldPath = *m_manualPath;

			// Place tester at the FIRST WAYPOINT (not the click position)
			// The pathfinder returns waypoints at tile centers, so start from there for alignment
			if (worldPath.size() > 0) {
				if (auto* transform = m_movementTester->GetComponent<CTransform>()) {
					transform->m_position = worldPath[0];
					//std::cout << "[PathTestScene] Moving test entity to first waypoint: (" << worldPath[0].x << ", " << worldPath[0].y << ")" << std::endl;
				}
			}

			// Assign the path and activate movement
			auto* path = m_movementTester->GetComponent<CPath>();
			if (!path) {
				path = m_movementTester->AddComponent<CPath>();
			}
			path->points = worldPath;
			path->complete = true;
			//std::cout << "[PathTestScene] Path assigned with " << worldPath.size() << " waypoints" << std::endl;

			// Reset and activate the follower
			if (auto* follower = m_movementTester->GetComponent<CPathFollower>()) {
				follower->currentWaypointIndex = 0;
				follower->isActive = true;
				m_movementTestActive = true;
				//std::cout << "[PathTestScene] Movement activated! Entity will travel to goal." << std::endl;
			}
		}
	}

	prevLeftDown = leftMouseDown;
	prevRightDown = rightMouseDown;
}
/////////////////////////////////



/////////////////////////////////
void PathTestScene::OnEnter() {}
void PathTestScene::OnExit() {
	// Unload resources when exiting the scene
	UnloadResources();
	std::cout << "[PathTestScene] OnExit called, resources unloaded." << std::endl;
	m_cameraEntity->GetComponent<CTransform>()->m_position = Vec2::Zero;
}
/////////////////////////////////
 
 

/////////////////////////////////
// UnloadResources - free large resources held by the path test scene when it is no longer active.
void PathTestScene::UnloadResources() {
	try {
		m_chunkManager.UnregisterChunkColliders(GetEntityManager());
	} catch (...) {}
	try {
		m_chunkManager.SaveAllChunks();
	} catch (...) {}
	try {
		m_chunkManager.ClearAllLoadedChunks();
	} catch (...) {}
	try {
		// Unload any atlas used by the chunk manager
		std::string key = m_chunkManager.GetTilesetKey();
		if (!key.empty()) {
			m_gameEngine.GetTextureManager().UnloadAtlas(key);
		}
	} catch (...) {}
}
/////////////////////////////////