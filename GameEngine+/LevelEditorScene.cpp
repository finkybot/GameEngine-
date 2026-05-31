/////////////////////////////////
// LevelEditor.cpp - simple (simple???? wtf is simple about it?????) chunked level editor that uses ChunkManager and CameraSystem
/////////////////////////////////



/////////////////////////////////
// Includes and namespace aliases for the LevelEditorScene implementation. We include necessary headers for the scene, chunk manager, game engine, entity management, and SFML graphics, as well as ImGui for UI rendering and filesystem for file operations.
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
/////////////////////////////////



/////////////////////////////////
// Constructor - initializes the level editor scene with references to the game engine, render window, and entity manager, and sets up the chunk manager with specified chunk dimensions and tile size
LevelEditorScene::LevelEditorScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& em)
	: Scene(engine, em), m_window(win), m_chunkManager(32, 32, 32.0f) {
}
/////////////////////////////////



/////////////////////////////////
// Destructor - ensures that all chunks are saved to disk when the level editor scene is destroyed, preventing data loss and ensuring that any changes made to the level are preserved
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
// InitializeGame - Initialize the level editor scene by creating a camera entity with a transform and camera component, setting it as the main camera, and configuring the chunk manager with the base path for chunk files and the 
// maximum number of loaded chunks allowed in memory at once. This setup allows the level editor to manage the camera view and efficiently load and save chunks of the level as needed.
void LevelEditorScene::InitializeGame(sf::Vector2u /*windowSize*/) {
	// create camera entity
	m_cameraEntity = GetEntityManager().AddEntity(EntityType::Default);
	m_cameraEntity->AddComponent<CTransform>(Vec2(0, 0), Vec2::Zero);
	auto cam = m_cameraEntity->AddComponent<CCamera>(Vec2(0, 0), 1.0f);
	cam->m_isMainCamera = true;
	cam->m_isActive = true;
	cam->m_viewportWidth = (float)m_window.getSize().x;
	cam->m_viewportHeight = (float)m_window.getSize().y;
	cam->m_smoothness = 6.0f; // fairly snappy smoothing

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
	m_chunkManager.SetBasePath("levels/chunks");
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

	// Reset all input/selection state so stale drag positions from a previous session
	// don't cause a huge area fill on the very first click after re-entering the scene.
	// Seed prevLmb/prevRmb from the CURRENT hardware state so that a button which is
	// already physically held when the scene initialises (e.g. the LMB used to click the
	// "Level Editor" main-menu button) is never treated as a fresh press edge.
	m_prevLmb        = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	m_prevRmb        = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
	m_prevDKey       = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
	m_prevMiddleDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle);
	m_panning        = false;
	// If LMB or RMB is already down we must NOT start a selection — wait for a full
	// press/release cycle before allowing any painting.
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
// OnEnter and OnExit methods - called when the level editor scene becomes active or inactive. OnEnter is currently empty, but we could add logic here if needed (e.g., to reset state or load specific resources).
// OnExit ensures that any collider entities generated by the ChunkManager are removed from the EntityManager before the engine clears all entities,
// preventing dangling pointers and crashes when returning to the scene. It then saves chunks to disk.
void LevelEditorScene::OnEnter() {}
void LevelEditorScene::OnExit() {
	// Unregister any chunk-generated collider entities before the global EntityManager is cleared by the engine.
	try {
		m_chunkManager.UnregisterChunkColliders(GetEntityManager());
	} catch (...) {}
	m_chunkManager.SaveAllChunks();
}
/////////////////////////////////



/////////////////////////////////
// HandleEvent  - currently empty (not anymore), as we will poll input in the Update method instead of relying on event callbacks. This allows for smoother and more 
// responsive input handling in the level editor, especially for continuous actions like dragging to paint tiles.
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

	// Editing input: paint/erase on mouse click
	sf::Vector2i mousePos = sf::Mouse::getPosition(m_window);
	sf::Vector2f world = m_window.mapPixelToCoords(mousePos, m_window.getView());
	int tileX = (int)std::floor(world.x / m_tileSize);
	int tileY = (int)std::floor(world.y / m_tileSize);

	bool lmb = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	bool rmb = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);

	// If ImGui is capturing the mouse or UI is hovered/active, do not modify the map
	bool uiCapturing = ImGui::GetIO().WantCaptureMouse || ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered() || ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow);

	// Tile pick: press D while hovering a tile to pick its texture into the brush
	bool dKey = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
	if (dKey && !m_prevDKey && !uiCapturing) {
		int val = m_chunkManager.GetTileAt(tileX, tileY);
		if (val > 0) {
			m_selectedTileIndex = val - 1;
			m_brushValue = val;
			// optionally echo to console for feedback
			std::cout << "Picked tile at (" << tileX << "," << tileY << ") value=" << val << " -> selectedIndex=" << m_selectedTileIndex << std::endl;
		}
	}
	// Camera panning with middle mouse
	bool isMiddleDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Middle);
	// debug: detect edge transitions of middle mouse to show events
	//if (isMiddleDown && !m_prevMiddleDown) {
	//	// suppressed debug log
	//}

	//// remember D key state for edge detection
	//m_prevDKey = dKey;
	//if (!isMiddleDown && m_prevMiddleDown) {
	//	// suppressed debug log
	//}
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

	// apply panning
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
			// print only when camera position changed by more than 1 unit to reduce spam
			if (std::abs(newPos.x - m_lastCameraPos.x) > 1.0f || std::abs(newPos.y - m_lastCameraPos.y) > 1.0f) {
				std::cout << "Camera moved to=(" << newPos.x << "," << newPos.y << ") delta=(" << delta.x << "," << delta.y << ")" << std::endl;
				m_lastCameraPos = newPos;
			}
		}
	}

	// Handle mouse input for painting or erasing tiles; check we are off the UI before modifying the map
	// m_inputReady gates all painting until every button has been physically released at least once
	// after the scene was entered — this prevents the click that opened the scene from painting.
	if (!m_inputReady) {
		if (!lmb && !rmb) m_inputReady = true;
	}

	if (!uiCapturing && m_inputReady) {
		// start selection on left-button down (edge)
		if (lmb && !m_prevLmb) {
			m_lmbSelecting = true;
			m_selectLmbStartPx = mousePos;
			m_selectLmbEndPx = mousePos;
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
		long long selW = (long long)tx1 - (long long)tx0 + 1;
		long long selH = (long long)ty1 - (long long)ty0 + 1;
		if (selW <= 0 || selH <= 0) { /* nothing */; }
		else if (selW * selH > maxSelectionTiles) {
			std::cerr << "Selection too large (" << selW*selH << " tiles) - operation ignored" << std::endl;
		} else {
			for (int ty = ty0; ty <= ty1; ++ty) {
				for (int tx = tx0; tx <= tx1; ++tx) {
					m_chunkManager.SetTileAt(tx, ty, m_brushValue); // paint selection
				}
			}
			// Refresh fixed bounds from disk (covers both expand on paint and shrink on erase)
			RefreshMapBounds();
		}
		}

		// start selection on right-button down (edge)
		if (rmb && !m_prevRmb) {
			m_rmbSelecting = true;
			m_selectRmbStartPx = mousePos;
			m_selectRmbEndPx = mousePos;
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
		long long selW2 = (long long)tx1 - (long long)tx0 + 1;
		long long selH2 = (long long)ty1 - (long long)ty0 + 1;
		if (selW2 <= 0 || selH2 <= 0) { /* nothing */; }
		else if (selW2 * selH2 > maxSelectionTiles) {
			std::cerr << "Selection too large (" << selW2*selH2 << " tiles) - operation ignored" << std::endl;
		} else {
			for (int ty = ty0; ty <= ty1; ++ty) {
				for (int tx = tx0; tx <= tx1; ++tx) {
					m_chunkManager.SetTileAt(tx, ty, 0); // erase selection
				}
			}
			// Recompute bounds from disk since erasing may shrink the map
			RefreshMapBounds();
		}
		}
	}
	m_prevLmb = lmb;
	m_prevRmb = rmb;

	ProcessInput();
}
/////////////////////////////////



/////////////////////////////////
// Render - Render the level editor scene. This will be called every frame by the game engine after Update. In this function, we will render the loaded chunks of the level based on the current camera view.
void LevelEditorScene::Render() {
	// Delegate drawing of visible chunks to the ChunkManager which performs culling and draws without holding the internal lock
	m_chunkManager.DrawChunks(m_window, m_window.getView());

	// Draw selection rectangle in world space while dragging (left or right button)
	if (m_lmbSelecting || m_rmbSelecting) {
		// Map the pixel selection corners into world coordinates using the active view
		sf::Vector2f w0 = m_window.mapPixelToCoords(m_lmbSelecting ? m_selectLmbStartPx : m_selectRmbStartPx, m_window.getView());
		sf::Vector2f w1 = m_window.mapPixelToCoords(m_lmbSelecting ? m_selectLmbEndPx : m_selectRmbEndPx, m_window.getView());
		float left = std::min(w0.x, w1.x);
		float top  = std::min(w0.y, w1.y);
		float width = std::abs(w1.x - w0.x);
		float height = std::abs(w1.y - w0.y);
		sf::RectangleShape rect(sf::Vector2f(width, height));
		rect.setPosition(sf::Vector2f(left, top));
		rect.setFillColor(sf::Color::Transparent);
		if (m_lmbSelecting) rect.setOutlineColor(sf::Color(100, 150, 255, 200)); else rect.setOutlineColor(sf::Color(255, 100, 100, 200));
		rect.setOutlineThickness(2.0f);
		m_window.draw(rect);
	}

	// Draw map bounds rectangle (if available)
	if (m_haveBounds) {
		float left = m_mapMin.x;
		float top = m_mapMin.y;
		float width = m_mapMax.x - m_mapMin.x;
		float height = m_mapMax.y - m_mapMin.y;
		if (width > 0 && height > 0) {
			sf::RectangleShape boundsRect(sf::Vector2f(width, height));
			boundsRect.setPosition(sf::Vector2f(left, top));
			boundsRect.setFillColor(sf::Color::Transparent);
			boundsRect.setOutlineColor(sf::Color(0, 200, 0, 180));
			boundsRect.setOutlineThickness(2.0f);
			m_window.draw(boundsRect);

			// Draw small corner markers so the bounds are visible even when outline is thin or offscreen
			const float markerSize = std::max(8.0f, std::min(width, height) * 0.02f);
			sf::RectangleShape corner(sf::Vector2f(markerSize, markerSize));
			corner.setFillColor(sf::Color::Red);
			corner.setPosition(sf::Vector2f(left - markerSize * 0.5f, top - markerSize * 0.5f));
			m_window.draw(corner);
			corner.setPosition(sf::Vector2f(left + width - markerSize * 0.5f, top - markerSize * 0.5f));
			m_window.draw(corner);
			corner.setPosition(sf::Vector2f(left - markerSize * 0.5f, top + height - markerSize * 0.5f));
			m_window.draw(corner);
			corner.setPosition(sf::Vector2f(left + width - markerSize * 0.5f, top + height - markerSize * 0.5f));
			m_window.draw(corner);
		}
	}
	// reset view to default for UI rendering
	m_window.setView(m_window.getDefaultView());

	// Debug: show computed bounds and camera info to help diagnose missing bounds rectangle
	{
		auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
		int chunkCount = 0;
		{
			std::lock_guard<std::mutex> lg(m_chunkManager.GetMutex());
			chunkCount = (int)m_chunkManager.GetChunks().size();
		}
		ImGui::Begin("Bounds Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::Text("Loaded chunks: %d  haveBounds: %s", chunkCount, m_haveBounds ? "yes" : "no");
		ImGui::Text("m_mapMin=(%.1f, %.1f)", m_mapMin.x, m_mapMin.y);
		ImGui::Text("m_mapMax=(%.1f, %.1f)", m_mapMax.x, m_mapMax.y);
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

	// Mirror TileMapEditor tileset panel
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
	ImGui::SetNextWindowBgAlpha(0.35f);
	// Allow the tileset window to be movable by giving it a title bar; keep auto-resize
	ImGui::Begin("Tileset Browser", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	// Fetch atlas once for UI usage (preview + brush). Declared here so later preview code can use it.
	auto atlasOpt = m_gameEngine.GetTextureManager().GetAtlas(std::string(m_tilesetKeyBuf));

	// Small floating preview of the currently-selected tile/atlas used for painting
	ImGui::Separator();
	ImGui::Text("Current Brush");
	if (atlasOpt.has_value() && atlasOpt.value() && m_brushValue > 0) {
		auto atlas = atlasOpt.value();
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


	// Small diagnostics: FPS + camera position
	{
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
			m_selectedTileIndex = 0;
		}
	}

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


	ImGui::End();
}
/////////////////////////////////



/////////////////////////////////
// EnsureVisibleChunks - Ensure that the chunks covering the current camera view are loaded. This function calculates the bounds of the camera's viewport in world coordinates, converts those to tile coordinates, 
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
/////////////////////////////////



/////////////////////////////////
// AtlasLoadWorker - runs on a background thread. Performs atlas load and writes results into guarded members using m_atlasLoadMutex.
void LevelEditorScene::AtlasLoadWorker(std::string key, std::string path, int w, int h) {
	bool ok = m_gameEngine.GetTextureManager().LoadAtlas(key, path, w, h);
	{
		std::lock_guard<std::mutex> threadLock(m_atlasLoadMutex);
		m_atlasLoadSuccess = ok;
		m_atlasLoadFinished = true;
		m_atlasLoadMessage = ok ? std::string("Loaded: ") + path : std::string("Failed: ") + path;
	}
	m_atlasLoading = false;
}
/////////////////////////////////



/////////////////////////////////
// ApplyMainCameraView - Apply the main camera's view to the render window. This function retrieves the main camera from the camera system, calculates the appropriate view based on the camera's position, zoom, and viewport size,
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
	v.setCenter(sf::Vector2f(newPos.x, newPos.y));
	m_window.setView(v);
}
/////////////////////////////////



/////////////////////////////////
// Process user input for the level editor scene. This function checks for specific key presses (e.g., Escape to close the window) and can be expanded in the future to handle additional input for camera movement, zooming, or other editing actions.
void LevelEditorScene::ProcessInput() {
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
		m_gameEngine.ChangeScene("MainMenu");
	}
}
/////////////////////////////////