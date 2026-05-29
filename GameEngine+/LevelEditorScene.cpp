// LevelEditor.cpp - simple chunked level editor that uses ChunkManager and CameraSystem
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


// Constructor - initializes the level editor scene with references to the game engine, render window, and entity manager, and sets up the chunk manager with specified chunk dimensions and tile size
LevelEditorScene::LevelEditorScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& em)
	: Scene(engine, em), m_window(win), m_chunkManager(32, 32, 32.0f) {
}


// Destructor - ensures that all chunks are saved to disk when the level editor scene is destroyed, preventing data loss and ensuring that any changes made to the level are preserved
LevelEditorScene::~LevelEditorScene() {
	m_chunkManager.SaveAllChunks();
}


// Initialize the level editor scene by creating a camera entity with a transform and camera component, setting it as the main camera, and configuring the chunk manager with the base path for chunk files and the 
// maximum number of loaded chunks allowed in memory at once. This setup allows the level editor to manage the camera view and efficiently load and save chunks of the level as needed.
void LevelEditorScene::InitializeGame(sf::Vector2u /*windowSize*/) {
	// create camera entity
	m_cameraEntity = GetEntityManager().addEntity(EntityType::Default);
	m_cameraEntity->AddComponent<CTransform>(Vec2(0, 0), Vec2::Zero);
	auto cam = m_cameraEntity->AddComponent<CCamera>(Vec2(0, 0), 1.0f);
	cam->m_isMainCamera = true;
	cam->m_isActive = true;
	cam->m_viewportWidth = (float)m_window.getSize().x;
	cam->m_viewportHeight = (float)m_window.getSize().y;
	cam->m_smoothness = 6.0f; // fairly snappy smoothing

	// set persistence path for chunks
	m_chunkManager.SetBasePath("levels/chunks");
	m_chunkManager.SetMaxLoadedChunks(256);
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
}


// OnEnter and OnExit methods - called when the level editor scene becomes active or inactive. OnEnter is currently empty, but we could add logic here if needed (e.g., to reset state or load specific resources). 
// OnExit ensures that all chunks are saved to disk when the scene is exited, preventing data loss and ensuring that any changes made to the level are preserved.
void LevelEditorScene::OnEnter() {}
void LevelEditorScene::OnExit() { m_chunkManager.SaveAllChunks(); }


// HandleEvent method - currently empty, as we will poll input in the Update method instead of relying on event callbacks. This allows for smoother and more 
// responsive input handling in the level editor, especially for continuous actions like dragging to paint tiles.
void LevelEditorScene::HandleEvent(const std::optional<sf::Event>& /*event*/) {
	// we poll input in Update
}


// Main update loop for the level editor scene. This will be called every frame by the game engine. In this function, we will handle camera updates, 
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

	// Editing input: paint/erase on mouse click
	sf::Vector2i mp = sf::Mouse::getPosition(m_window);
	sf::Vector2f world = m_window.mapPixelToCoords(mp, m_window.getView());
	int tileX = (int)std::floor(world.x / m_tileSize);
	int tileY = (int)std::floor(world.y / m_tileSize);

	bool lmb = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	bool rmb = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

	// If ImGui is capturing the mouse or UI is hovered/active, do not modify the map
	bool uiCapturing = ImGui::GetIO().WantCaptureMouse || ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);
	if (!uiCapturing) {
		if (lmb && !m_prevLmb) {
			m_chunkManager.SetTileAt(tileX, tileY, m_brushValue);
			// debug logging removed
			int stored = m_chunkManager.GetTileAt(tileX, tileY);
			// debug logging removed
		}
		if (rmb && !m_prevRmb) {
			m_chunkManager.SetTileAt(tileX, tileY, 0);
			int stored2 = m_chunkManager.GetTileAt(tileX, tileY);
			// debug logging removed
		}
	}
	m_prevLmb = lmb;
	m_prevRmb = rmb;

	ProcessInput();
}


// Render the level editor scene. This will be called every frame by the game engine after Update. In this function, we will render the loaded chunks of the level based on the current camera view.
void LevelEditorScene::Render() {
	// Delegate drawing of visible chunks to the ChunkManager which performs culling and draws without holding the internal lock
	m_chunkManager.DrawChunks(m_window, m_window.getView());
	// reset view to default for UI rendering
	m_window.setView(m_window.getDefaultView());

	// Mirror TileMapEditor tileset panel
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
	ImGui::SetNextWindowBgAlpha(0.35f);
	// Allow the tileset window to be movable by giving it a title bar; keep auto-resize
	ImGui::Begin("Tileset Browser", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	if (m_currentDir.empty()) m_currentDir = std::filesystem::current_path();

	static std::vector<std::filesystem::directory_entry> s_entries;
	auto refresh_entries = [&]() {
		s_entries.clear();
		try {
			for (auto &e : std::filesystem::directory_iterator(m_currentDir)) s_entries.push_back(e);
			std::sort(s_entries.begin(), s_entries.end(), [](const auto &a, const auto &b) {
				bool a_dir=false,b_dir=false; try{a_dir=a.is_directory();}catch(...){a_dir=false;} try{b_dir=b.is_directory();}catch(...){b_dir=false;} if (a_dir!=b_dir) return a_dir>b_dir; return a.path().filename().string()<b.path().filename().string();
			});
		} catch(...) { s_entries.clear(); }
	};

	if (ImGui::Button("Up") && m_currentDir.has_parent_path()) { m_currentDir = m_currentDir.parent_path(); refresh_entries(); }
	ImGui::SameLine();
	if (ImGui::Button("Refresh")) { refresh_entries(); }
	ImGui::SameLine(); ImGui::Text("Current folder: %s", m_currentDir.string().c_str());

	if (s_entries.empty()) refresh_entries();

	ImGui::BeginChild("files_list", ImVec2(0,200), true);
	static std::string s_selected_file;
	for (size_t i=0;i<s_entries.size();++i) {
		auto &entry = s_entries[i];
		std::string name = entry.path().filename().string();
		bool is_dir=false; try{is_dir=entry.is_directory();}catch(...){is_dir=false;}
		std::string label = is_dir ? (name + "/") : name;
		std::string fullpath = (m_currentDir / entry.path().filename()).string();
		bool selected = (!s_selected_file.empty() && s_selected_file==fullpath);
				if (ImGui::Selectable(label.c_str(), selected)) {
					if (is_dir) { m_currentDir = entry.path(); s_selected_file.clear(); refresh_entries(); }
						else { ImStrncpy(m_loadFilenameBuffer, fullpath.c_str(), sizeof(m_loadFilenameBuffer)); s_selected_file = fullpath; if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					// load selected atlas. If UI key is empty, derive a key from filename stem so we don't register under an empty key.
					std::string key = std::string(m_tilesetKeyBuf);
					if (key.empty()) {
						try { key = std::filesystem::path(s_selected_file).stem().string(); } catch(...) { key = ""; }
					}
					if (m_gameEngine.GetTextureManager().LoadAtlas(key, s_selected_file, m_tilesetTileW, m_tilesetTileH)) {
						m_chunkManager.SetTilesetKey(key);
						m_chunkManager.RebuildAllChunksFromTileset();
						std::cout << "Loaded atlas: " << s_selected_file << " key='" << key << "'" << std::endl;
						// copy key into ui buffer
						ImStrncpy(m_tilesetKeyBuf, key.c_str(), sizeof(m_tilesetKeyBuf));
					} else std::cerr << "Failed to load atlas: " << s_selected_file << std::endl;
				} }
		}

	}
	ImGui::EndChild();

	ImGui::Separator();
	ImGui::Text("Tileset key: %s", m_tilesetKeyBuf);
	ImGui::SameLine();
	if (ImGui::Button("Set Key")) { m_chunkManager.SetTilesetKey(std::string(m_tilesetKeyBuf)); m_chunkManager.RebuildAllChunksFromTileset(); }

	ImGui::Separator();
	if (ImGui::Button("Load Selected")) {
		std::string key = std::string(m_tilesetKeyBuf);
		std::string path = std::string(m_loadFilenameBuffer);
		if (!path.empty()) {
			// Load atlas asynchronously to avoid blocking UI
			if (!m_atlasLoading.exchange(true)) {
				// if UI key is empty, derive a key from filename stem so we don't register under an empty key
				if (key.empty()) {
					try { key = std::filesystem::path(path).stem().string(); } catch(...) { key = ""; }
				}
				m_atlasLoadKey = key;
				m_atlasLoadPath = path;
				m_atlasLoadFinished = false;
				m_atlasLoadSuccess = false;
				m_atlasLoadMessage.clear();
				std::thread([this, key, path, w = m_tilesetTileW, h = m_tilesetTileH]() {
					bool ok = m_gameEngine.GetTextureManager().LoadAtlas(key, path, w, h);
					{
						std::lock_guard<std::mutex> lg(m_atlasLoadMutex);
						m_atlasLoadSuccess = ok;
						m_atlasLoadFinished = true;
						m_atlasLoadMessage = ok ? std::string("Loaded: ") + path : std::string("Failed: ") + path;
					}
					m_atlasLoading = false;
				}).detach();
			}
		}
	}

	// Show async load message if present
	{
		std::lock_guard<std::mutex> lg(m_atlasLoadMutex);
		if (!m_atlasLoadMessage.empty()) ImGui::TextUnformatted(m_atlasLoadMessage.c_str());
	}

	// If an async load finished, apply its result on the main thread: set tileset key and rebuild previews/chunks
	{
		std::lock_guard<std::mutex> lg(m_atlasLoadMutex);
		if (m_atlasLoadFinished) {
			if (m_atlasLoadSuccess) {
				// adopt the loaded key into the UI buffer so preview uses it
				ImStrncpy(m_tilesetKeyBuf, m_atlasLoadKey.c_str(), sizeof(m_tilesetKeyBuf));
				m_chunkManager.SetTilesetKey(m_atlasLoadKey);
				// Rebuild all currently-loaded chunks to use the new tileset. Also process any pending background-loaded chunks
				// so they finalize with the new tileset and then rebuild again to ensure textured geometry is prepared.
				m_chunkManager.RebuildAllChunksFromTileset();
				m_chunkManager.UpdateMainThread();
				m_chunkManager.RebuildAllChunksFromTileset();
				m_selectedTileIndex = 0; // reset selection
			} else {
				// keep key but show message already handled above
			}
			m_atlasLoadFinished = false;
		}
	}

	// Preview loaded atlas tiles if available
	ImGui::Separator();
	auto atlasOpt = m_gameEngine.GetTextureManager().GetAtlas(std::string(m_tilesetKeyBuf));
	if (atlasOpt.has_value() && *atlasOpt) {
		auto atlas = *atlasOpt;
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
					if (ImGui::ImageButton(id.c_str(), spr, ImVec2((float)tw, (float)th), bg, sf::Color::White)) {
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


	ImGui::End();
}


// Ensure that the chunks covering the current camera view are loaded. This function calculates the bounds of the camera's viewport in world coordinates, converts those to tile coordinates, 
// and then tells the chunk manager to ensure that all chunks intersecting that tile rectangle are loaded. The marginChunks parameter allows for loading additional chunks around the edges of the view to prevent pop-in when moving the camera.
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


// Apply the main camera's view to the render window. This function retrieves the main camera from the camera system, calculates the appropriate view based on the camera's position, zoom, and viewport size,
void LevelEditorScene::ApplyMainCameraView() {
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt) return;
	CCamera* cam = *camOpt;
	sf::View v;
	v.setSize(sf::Vector2f(cam->m_viewportWidth * cam->m_zoom, cam->m_viewportHeight * cam->m_zoom));
	v.setCenter(sf::Vector2f(cam->m_position.x, cam->m_position.y));
	m_window.setView(v);
}


// Process user input for the level editor scene. This function checks for specific key presses (e.g., Escape to close the window) and can be expanded in the future to handle additional input for camera movement, zooming, or other editing actions.
void LevelEditorScene::ProcessInput() {
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		m_gameEngine.m_window.close();
	}
}
