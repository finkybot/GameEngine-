/////////////////////////////////
// LevelEditorScene.h - Chunked level editor scene
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the LevelEditorScene class. We include
#pragma once
#include "Scene.h"
#include "ChunkManager.h"
#include "CameraSystem.h"
#include <mutex>
#include <SFML/Graphics.hpp>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
/////////////////////////////////



/////////////////////////////////
// LevelEditorScene class - implements a chunked level editor scene with camera controls, tile editing, and asynchronous chunk loading. 
// The scene manages a grid of chunks that can be edited in real-time, with support for panning the camera and loading/unloading chunks as needed to optimize performance.
class LevelEditorScene : public Scene {
	/////////////////////////////////
	// Public interface for the LevelEditorScene class, including constructor, destructor, and overridden virtual methods from the Scene base class for updating, rendering, handling events, and managing scene lifecycle.
public:
	/////////////////////////////////
	// Constructor and destructor for the LevelEditorScene class. The constructor initializes the scene with references to the game engine, render window, and entity manager, while the destructor ensures that all chunks are saved to disk when the scene is destroyed.
	LevelEditorScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& em);
	~LevelEditorScene() override;		
	/////////////////////////////////



	/////////////////////////////////
	// Overridden virtual methods from the Scene base class. These methods handle updating the scene state, rendering the scene, processing input events, and managing the scene lifecycle (entering and exiting). 
	// The Update method will handle logic for ensuring visible chunks are loaded and applying camera view, while the Render method will handle drawing the chunks and any UI elements.
	void Update(float deltaTime) override;
	void Render() override;
	void DoAction() override {}
	void HandleEvent(const std::optional<sf::Event>& event) override;
	void OnEnter() override;
	void OnExit() override;
	void LoadResources() override {}
	void UnloadResources() override {}
	void InitializeGame(sf::Vector2u windowSize) override;
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods for the LevelEditorScene class. These methods include logic for ensuring that visible chunks are loaded based on the camera view, applying the main camera's view to the render window, and processing user input for camera controls and tile editing.
private:
	/////////////////////////////////
	void EnqueueCheckerboardBackground(); // Enqueue a checkerboard pattern to the render queue for background rendering
	void EnqueueChunkDiag(); // Enqueue chunk diagnostics visualization to the render queue
	void EnqueueDragRect(); // Enqueue the selection rectangle to the render queue
	void EnqueueMapBounds(); // Enqueue map bounds visualization to the render queue
	void EnsureVisibleChunks(); // Calculate which chunks intersect the current camera view plus a margin, and ensure those chunks are loaded and ready for rendering. This method will use the camera's position and viewport size to determine which chunks are needed, and will call the chunk manager to load any missing chunks.
	void ApplyMainCameraView(); //  Apply the main camera's view to the render window before rendering the world. This method will retrieve the main camera's position and viewport settings, and set the SFML view accordingly so that the rendered chunks are displayed in the correct position and scale on the screen.
	void ProcessInput(); // Process user input for camera controls (e.g., panning) and tile editing (e.g., painting/erasing tiles). This method will handle mouse input for painting tiles based on the current brush value, as well as middle mouse dragging for panning the camera. It will also take into account whether ImGui is capturing the mouse to avoid modifying the map when interacting with the UI.
	void RefreshMapBounds(); // Recompute m_mapMin/m_mapMax/m_haveBounds from saved chunk files on disk. Called after any paint or erase operation so bounds grow with new tiles and shrink when tiles are removed.
	void LevelManagerWindow(float x, float y, float	alpha); // Show a window in ImGui for managing levels, including creating new levels, selecting existing levels, and deleting levels. This method will provide a user interface for managing the different levels in the game, allowing users to easily switch between levels and perform level management tasks without leaving the level editor.
	void BoundsInfoWindow(float x, float y, float alpha); // Show map bounds info in an ImGui window for debugging purposes. This method will display the current map bounds (m_mapMin and m_mapMax) in an ImGui window, providing feedback to the user about the extent of the world they are editing and helping with debugging and level design.
	void CameraZoomWindow(); // Show camera zoom info in an ImGui window for debugging purposes. This method will display the current camera zoom level in an ImGui window, providing feedback to the user about the current zoom state of the camera and helping with debugging and level design.
	void TilePreviewWindow(std::optional<std::shared_ptr<TextureAtlas>>&atlasOpt); // Show a tile preview window in ImGui that displays the currently selected tile from the atlas. This method will take an optional shared pointer to the texture atlas, and if available, will render a preview of the currently selected tile based on m_selectedTileIndex. This provides visual feedback to the user about which tile they have selected for painting in the level editor.
	void TilesetKeyWindow(); // Show a window in ImGui for inputting the tileset key, file path, and tile dimensions, and a button to load the atlas. This method will provide a user interface for loading a texture atlas into the level editor, allowing users to specify the necessary information for loading the atlas and providing feedback on the loading process.
	void AtlasBrowserWindow(); // Show a file browser window in ImGui for selecting a texture atlas file to load. This method will allow users to navigate their file system and select a texture atlas file, providing an alternative way to specify the atlas path for loading into the level editor.
	void SelectedBrushWindow(std::optional<std::shared_ptr<TextureAtlas>>&atlasOpt); // Show a window in ImGui for displaying and inputting the currently selected tile index for painting. This method will allow users to see which tile they have currently selected from the atlas, and will provide an input field for changing the selected tile index, giving users control over which tile they are painting with in the level editor.
	/////////////////////////////////


	
	/////////////////////////////////
	// AtlasLoadWorker - Worker function used to load a texture atlas on a background thread. This function takes the key, file path, and tile dimensions for the atlas, and will load the texture and create the necessary sub-rectangles for each tile in the atlas.
	void AtlasLoadWorker(std::string key, std::string path, int w, int h);
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the LevelEditorScene class, including references to the render window, chunk manager, camera system, and state variables for input handling and camera control. These variables will be used to manage the scene's state and behavior during updates and rendering.
	sf::RenderWindow& m_window;
	ChunkManager m_chunkManager;
	CameraSystem m_cameraSystem;
	Entity* m_cameraEntity = nullptr;
	float m_tileSize = 32.0f;
	/////////////////////////////////
	 
	 

	/////////////////////////////////
	// input states
	bool m_prevLmb = false;
	bool m_prevRmb = false;
	bool m_prevDKey = false; // previous state for 'pick tile' key (D)
	int m_brushValue = 1;
	int m_marginChunks = 1;
	/////////////////////////////////



	/////////////////////////////////
	// Guard that blocks any painting until all mouse buttons have been fully released at least once after entering the scene. Prevents the button used to open the scene (e.g. a main-menu click) from immediately starting a selection drag.
	bool m_inputReady = false;
	/////////////////////////////////



	/////////////////////////////////
	// camera pan state
	bool m_panning = false;
	sf::Vector2i m_panStart = sf::Vector2i(0,0);
	Vec2 m_camPanStart = Vec2::Zero;
	/////////////////////////////////



	/////////////////////////////////
	// selection drag state of left button
	bool m_lmbSelecting = false;
	sf::Vector2i m_selectLmbStartPx = sf::Vector2i(0, 0);
	sf::Vector2i m_selectLmbEndPx = sf::Vector2i(0, 0);
	/////////////////////////////////



	/////////////////////////////////
	// selection drag state of right button
	bool m_rmbSelecting = false;
	sf::Vector2i m_selectRmbStartPx = sf::Vector2i(0, 0);
	sf::Vector2i m_selectRmbEndPx = sf::Vector2i(0, 0);
	/////////////////////////////////
	


	/////////////////////////////////
	// logical map bounds in world pixels (min, max) used to clamp camera — computed once from saved chunk files on disk.
	Vec2 m_mapMin = Vec2::Zero;
	Vec2 m_mapMax = Vec2::Zero;
	bool m_haveBounds = false;
	/////////////////////////////////



	/////////////////////////////////
	// Temporary storage for render shapes during Enqueue phase (kept alive until Flush completes)
	// Use a map with stable memory addresses instead of a vector to avoid pointer invalidation
	std::unordered_map<int, std::shared_ptr<sf::Shape>> m_tempRenderShapes;
	std::unordered_map<int, std::shared_ptr<sf::VertexArray>> m_tempRenderVertexArrays;
	int m_nextTempId = 0;
	/////////////////////////////////



	/////////////////////////////////
	// zoom steps: one level in (0.5x), default (1.0x), two levels out (2.0x, 4.0x)
	static constexpr float k_zoomSteps[] = { 0.5f, 1.0f, 2.0f, 4.0f };
	int m_zoomIndex = 1; // index into k_zoomSteps; 1 = default (1.0x)
	/////////////////////////////////



	/////////////////////////////////
	// previous middle mouse state for debug
	bool m_prevMiddleDown = false;
	/////////////////////////////////



	/////////////////////////////////
	// Tileset UI state
	char m_tilesetKeyBuf[64] = "adventure";
	char m_tilesetPathBuf[512] = "";
	int m_tilesetTileW = 32;
	int m_tilesetTileH = 32;
	/////////////////////////////////



	/////////////////////////////////
	// Selected tile in preview (0-based atlas index). m_brushValue == m_selectedTileIndex+1
	int m_selectedTileIndex = 0;
	/////////////////////////////////
	

	
	/////////////////////////////////
	// Async atlas load state
	std::atomic<bool> m_atlasLoading{false};
	std::mutex m_atlasLoadMutex;
	bool m_atlasLoadFinished = false;
	bool m_atlasLoadSuccess = false;
	std::string m_atlasLoadKey;
	std::string m_atlasLoadPath;
	std::string m_atlasLoadMessage;
	/////////////////////////////////



	/////////////////////////////////
	// file browser state for tileset selection
	std::filesystem::path m_currentDir;
	char m_loadFilenameBuffer[512] = "";
	/////////////////////////////////



	/////////////////////////////////
	// Debugging options
	bool m_showChunkDiagnostics = false;
	/////////////////////////////////



	/////////////////////////////////
	// Level management
	char m_levelNameBuf[128] = ""; // input for new level name
	std::vector<std::string> m_availableLevels; // discovered level folders under "levels"
	int m_selectedLevelIndex = -1;
	std::string m_currentLevelName; // empty = default working folder
	bool m_levelSelected = false;
	std::string m_pendingDeleteName;
	std::string m_exportMessage;
	/////////////////////////////////



	/////////////////////////////////
	// Layer editing
	std::vector<std::string> m_layerNames = { "background", "main", "upper" };
	int m_activeLayer = 1; // default to main
	/////////////////////////////////



	/////////////////////////////////
	// Helper methods for level management UI
	void RefreshAvailableLevels();
	void SwitchToLevel(const std::string& name);
	/////////////////////////////////
};
/////////////////////////////////