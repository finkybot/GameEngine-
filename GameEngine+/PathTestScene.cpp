#include "PathTestScene.h"
#include "GameEngine.h"
#include "CTransform.h"
#include "CPathRequest.h"
#include "CCamera.h"
#include "imgui/imgui.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <Windows.h>

namespace fs = std::filesystem;

PathTestScene::PathTestScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager)
	: Scene(engine, entityManager), m_window(win), m_chunkManager(32, 32, 32.0f), 
	  m_pathSystem(m_chunkManager, entityManager) {}

PathTestScene::~PathTestScene() {
	m_chunkManager.SaveAllChunks();
}

void PathTestScene::InitializeGame(sf::Vector2u /*windowSize*/) {
	// Create camera entity (like LevelEditor)
	m_cameraEntity = GetEntityManager().AddEntity(EntityType::Default);
	m_cameraEntity->AddComponent<CTransform>(Vec2(0, 0), Vec2::Zero);
	auto cam = m_cameraEntity->AddComponent<CCamera>(Vec2(0, 0), 1.0f);
	cam->m_isMainCamera = true;
	cam->m_isActive = true;
	cam->m_viewportWidth = (float)m_window.getSize().x;
	cam->m_viewportHeight = (float)m_window.getSize().y;
	cam->m_smoothness = 8.0f;

	// Initialize with a reasonable bounds
	m_mapMin = Vec2(-512, -512);
	m_mapMax = Vec2(512, 512);
	m_haveBounds = false;

	// Scan available levels
	ScanLevelFiles();

	// Set initial base path (will be overridden when level is loaded)
	m_chunkManager.SetBasePath("levels/chunks");
	m_chunkManager.SetMaxLoadedChunks(256);
}

void PathTestScene::ScanLevelFiles() {
	m_availableLevels.clear();

	// Scan APPDATA/GameEnginePlus/levels
	std::error_code ec;
	fs::path base;

#ifdef _MSC_VER
	char* appdata_buf = nullptr;
	size_t len = 0;
	if (_dupenv_s(&appdata_buf, &len, "APPDATA") == 0 && appdata_buf) {
		base = fs::path(appdata_buf) / "GameEnginePlus" / "levels";
		free(appdata_buf);
	} else {
		base = fs::path("levels");
	}
#else
	const char* appdata = std::getenv("APPDATA");
	if (appdata && appdata[0] != '\0') {
		base = fs::path(appdata) / "GameEnginePlus" / "levels";
	} else {
		base = fs::path("levels");
	}
#endif

	try {
		if (fs::exists(base, ec) && !ec) {
			for (auto& entry : fs::directory_iterator(base, ec)) {
				if (entry.is_directory()) {
					// Check if this directory has a "chunks" subdirectory
					auto chunksDir = entry.path() / "chunks";
					if (fs::exists(chunksDir)) {
						m_availableLevels.push_back(entry.path().filename().string());
					}
				}
			}
		}
	} catch (...) {
	}

	std::sort(m_availableLevels.begin(), m_availableLevels.end());
}

bool PathTestScene::SwitchToLevel(const std::string& name) {
	m_currentLevelName = name;

	fs::path base;
#ifdef _MSC_VER
	char* appdata_buf = nullptr;
	size_t len = 0;
	if (_dupenv_s(&appdata_buf, &len, "APPDATA") == 0 && appdata_buf) {
		base = fs::path(appdata_buf) / "GameEnginePlus" / "levels";
		free(appdata_buf);
	} else {
		base = fs::path("levels");
	}
#else
	const char* appdata = std::getenv("APPDATA");
	if (appdata && appdata[0] != '\0') {
		base = fs::path(appdata) / "GameEnginePlus" / "levels";
	} else {
		base = fs::path("levels");
	}
#endif

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
			if (meta) {
				std::string line;
				while (std::getline(meta, line)) {
					auto eq = line.find('=');
					if (eq == std::string::npos) continue;
					std::string key = line.substr(0, eq);
					std::string val = line.substr(eq + 1);
					if (key == "tileset") {
						tilesetKey = val;
					} else if (key == "tilesetPath") {
						tilesetPath = val;
					} else if (key == "layers") {
						layers.clear();
						// comma separated
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

	if (!tilesetKey.empty()) {
		m_chunkManager.SetTilesetKey(tilesetKey);

		// If we have a tileset path from metadata, load it
		if (!tilesetPath.empty()) {
			auto& tm = GameEngine::GetInstance().GetTextureManager();
			// Check if already loaded
			auto atlasOpt = tm.GetAtlas(tilesetKey);
			if (!atlasOpt.has_value() || !*atlasOpt) {
				std::cout << "PathTestScene: Loading tileset '" << tilesetKey << "' from " << tilesetPath << "\n";
				// Use the stored tile size
				int tileSize = (int)m_chunkManager.GetTileSize();
				tm.LoadAtlas(tilesetKey, tilesetPath, tileSize, tileSize);
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

	// For pathfinding test, render all layers with same opacity
	// Set to render the "main" layer as active (middle layer) for game logic
	m_chunkManager.SetActiveLayer(std::min(1, (int)layers.size() - 1));
	// Render unselected layers at full opacity so all layers are visible
	m_chunkManager.SetUnselectedLayerAlpha(1.0f);

	// Debug: log layer info
	std::cout << "DEBUG: Loaded level '" << name << "' with " << layers.size() << " layers: ";
	for (size_t i = 0; i < layers.size(); ++i) {
		std::cout << layers[i];
		if (i < layers.size() - 1) std::cout << ", ";
	}
	std::cout << std::endl;
	std::cout << "  Tileset key: '" << tilesetKey << "'\n";
	std::cout << "  Active layer: " << m_chunkManager.GetActiveLayer() << std::endl;
	std::cout << "  Unselected alpha: " << m_chunkManager.GetUnselectedLayerAlpha() << std::endl;

	// Clear all previously loaded chunks and flush any pending load jobs
	// We need to process UpdateMainThread several times to drain all pending finalizations
	// from background loader threads BEFORE we clear to avoid evicting chunks with in-flight loads
	for (int i = 0; i < 20; ++i) {
		m_chunkManager.UpdateMainThread();
	}
	m_chunkManager.ClearAllLoadedChunks();

	// Now load the new level
	m_chunkManager.LoadAllSavedChunks();

	// Process pending chunk loads - call UpdateMainThread multiple times to drain the finalization queue
	// This ensures all chunks are loaded and finalized before we proceed
	for (int i = 0; i < 20; ++i) {
		m_chunkManager.UpdateMainThread();
	}

	m_chunkManager.RebuildAllChunksFromTileset();

	// Get bounds and debug info
	float dMinX, dMinY, dMaxX, dMaxY;
	bool hasBounds = m_chunkManager.GetSavedChunkBounds(dMinX, dMinY, dMaxX, dMaxY);
	std::cout << "  Bounds found: " << (hasBounds ? "YES" : "NO");
	if (hasBounds) {
		std::cout << " (" << dMinX << "," << dMinY << ") to (" << dMaxX << "," << dMaxY << ")";
	}
	std::cout << "\n";

	// Log tile counts per layer
	std::cout << "  Chunk/Layer tile count:\n";
	for (int layer = 0; layer < m_chunkManager.GetNumLayers(); ++layer) {
		int totalTiles = 0;
		// We need to count tiles per layer - this would require access to internal chunks
		// For now just log that the layer exists
		std::cout << "    Layer " << layer << ": (loaded)\n";
	}

	if (hasBounds) {
		m_mapMin = Vec2(dMinX, dMinY);
		m_mapMax = Vec2(dMaxX, dMaxY);
		m_haveBounds = true;

		// Center camera on level
		if (m_cameraEntity) {
			if (auto cam = m_cameraEntity->GetComponent<CCamera>()) {
				auto tr = m_cameraEntity->GetComponent<CTransform>();
				Vec2 center = (m_mapMin + m_mapMax) * 0.5f;
				tr->m_position = center;
				cam->m_position = center;
				std::cout << "  Camera centered at (" << center.x << "," << center.y << ")\n";
			}
		}
	}

	return true;
}

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
}

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

	// Snap to half-pixel to reduce shimmer
	float snapX = std::round(newPos.x * 2.0f) * 0.5f;
	float snapY = std::round(newPos.y * 2.0f) * 0.5f;

	v.setCenter(sf::Vector2f(snapX, snapY));
	m_window.setView(v);
}

void PathTestScene::Update(float deltaTime) {
	// Update camera
	if (m_cameraEntity) {
		m_cameraSystem.Update(deltaTime, GetEntityManager());
	}

	// Ensure visible chunks are loaded
	EnsureVisibleChunks();
	m_chunkManager.UpdateMainThread();

	// Apply camera view
	ApplyMainCameraView();

	// Update path system
	m_pathSystem.SetNodesPerFrame(m_nodesPerFrame);
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
}

void PathTestScene::Render() {
	// Enqueue chunks
	m_chunkManager.EnqueueChunks(m_renderQueue, m_window.getView());

	// Draw debug overlay (paths, markers) BEFORE flushing, while world view is active
	RenderDebugOverlay();

	// Flush render queue
	m_renderQueue.Flush(m_window);

	// Reset view for UI
	m_window.setView(m_window.getDefaultView());
}

void PathTestScene::RenderDebugOverlay() {
	// Draw start/goal markers
	if (m_manualStartSet) {
		sf::CircleShape startMarker(8.0f);
		startMarker.setFillColor(sf::Color::Blue);
		startMarker.setOrigin(sf::Vector2f(8.0f, 8.0f));
		startMarker.setPosition(sf::Vector2f(m_manualStart.x, m_manualStart.y));
		m_window.draw(startMarker);
	}

	if (m_manualGoalSet) {
		sf::CircleShape goalMarker(8.0f);
		goalMarker.setFillColor(sf::Color::Magenta);
		goalMarker.setOrigin(sf::Vector2f(8.0f, 8.0f));
		goalMarker.setPosition(sf::Vector2f(m_manualGoal.x, m_manualGoal.y));
		m_window.draw(goalMarker);
	}

	// Draw path - convert from pathfinder coordinates back to world space
	if (m_manualPath.has_value() && !m_manualPath->empty()) {
		sf::Color pathColor = m_manualPathComplete ? sf::Color::Green : sf::Color::Yellow;
		sf::VertexArray va(sf::PrimitiveType::Lines);

		// Get chunk offset
		float chunkMinX = m_mapMin.x;
		float chunkMinY = m_mapMin.y;

		for (size_t i = 1; i < m_manualPath->size(); ++i) {
			// The pathfinder returns coordinates in local tile space, need to add chunk offset
			auto& a = (*m_manualPath)[i - 1];
			auto& b = (*m_manualPath)[i];

			// Adjust back to world space
			Vec2 a_world = a + Vec2(chunkMinX, chunkMinY);
			Vec2 b_world = b + Vec2(chunkMinX, chunkMinY);

			va.append(sf::Vertex(sf::Vector2f(a_world.x, a_world.y), pathColor));
			va.append(sf::Vertex(sf::Vector2f(b_world.x, b_world.y), pathColor));
		}

		m_window.draw(va);
	}
}

void PathTestScene::HandleEvent(const std::optional<sf::Event>& event) {
	if (!event.has_value()) return;

	// Get camera for manual coordinate conversion
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt) return;
	CCamera* cam = *camOpt;

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
	} else if (rightClicked && m_manualStartSet) {
		// Set goal and compute path
		m_manualGoal = mouseWorld;
		m_manualGoalSet = true;

		float ts = m_chunkManager.GetTileSize();

		// Get chunk bounds to determine offset
		float chunkMinX = m_mapMin.x;
		float chunkMinY = m_mapMin.y;

		// Convert world coordinates to tile coordinates, accounting for chunk offset
		int sx = static_cast<int>(std::floor((m_manualStart.x - chunkMinX) / ts));
		int sy = static_cast<int>(std::floor((m_manualStart.y - chunkMinY) / ts));
		int gx = static_cast<int>(std::floor((m_manualGoal.x - chunkMinX) / ts));
		int gy = static_cast<int>(std::floor((m_manualGoal.y - chunkMinY) / ts));

		m_manualPath = m_pathSystem.FindPathSync(sx, sy, gx, gy);
		m_manualPathComplete = m_manualPath.has_value();

		if (m_manualPathComplete) {
			std::cout << "PathTestScene: Path found (" << m_manualPath->size() << " waypoints)\n";
		} else {
			std::cout << "PathTestScene: Path NOT found\n";
		}
	}

	prevLeftDown = leftMouseDown;
	prevRightDown = rightMouseDown;
}

void PathTestScene::OnEnter() {}
void PathTestScene::OnExit() {}
