/////////////////////////////////
// MusicVisualizerScene.h - Music visualizer scene adapted from TileMapEditorScene
/////////////////////////////////



/////////////////////////////////
// Includes for the MusicVisualizerScene class.
#pragma once
#include "Scene.h"
#include "TileMap.h"
#include "Vec2.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include "FileDialog.h"
#include <filesystem>
/////////////////////////////////



/////////////////////////////////
// Forward declarations
namespace Spawn {
class SpawnSystem;
}
/////////////////////////////////



/////////////////////////////////
// MusicVisualizerScene class definition. This class implements a music visualizer scene that inherits from the base Scene class. It manages a tile map for visual effects, handles user input for file browsing and playback controls, 
// and implements audio-reactive visual effects such as explosions and equalizer bars based on the music being played.
class MusicVisualizerScene : public Scene {
	/////////////////////////////////
	// Public methods
public:
	/////////////////////////////////
	// Constructor and destructor
	MusicVisualizerScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager);
	~MusicVisualizerScene() override;
	/////////////////////////////////



	/////////////////////////////////
	// Override virtual methods from Scene. Update will handle the main logic for updating the scene state, including processing input, updating visual effects, and managing music playback. Render will handle drawing the tile map and any UI elements, 
	// while DoAction can be used for any specific actions that need to be performed each frame. RenderDebugOverlay can be used to draw additional debug information on top of the scene, and IsImGuiEnabled allows the scene to control whether ImGui should be rendered for this scene.
	void Update(float deltaTime) override;
	void Render() override;
	void DoAction() override;
	void RenderDebugOverlay() override;
	bool IsImGuiEnabled() override { return m_enableImGui; }
	/////////////////////////////////



	/////////////////////////////////
	// Event handling and lifecycle methods (overrides from Scene). HandleEvent will process SFML events for user input, while OnEnter and OnExit will manage any setup or cleanup needed when the scene becomes active or is exited.
	void HandleEvent(const std::optional<sf::Event>& event) override;
	void OnEnter() override;
	void OnExit() override;
	/////////////////////////////////



	/////////////////////////////////
	// Resource management and initialization (overrides from Scene). LoadResources and UnloadResources will handle loading and freeing any resources needed by the scene, while InitializeGame can be used to set up the initial state of the scene when the game starts.
	void LoadResources() override;
	void UnloadResources() override;
	void InitializeGame(sf::Vector2u windowSize) override;
	/////////////////////////////////



	/////////////////////////////////
	// Deprecated equalizer active state management (commented out for now, as equalizer bars are always active when enabled, but this could be reintroduced if we want to allow toggling the equalizer bars on/off separately from the music playback)
	//bool IsEqualizerActive() const { return m_EqualizerActive; }
	//void SetEqualizerActive(bool active) { m_EqualizerActive = active; }
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods
private:
	/////////////////////////////////
	// Helper methods for grid rendering, input processing, tile toggling, and explosion updates
	void DrawGrid();
	void ProcessInput();
	void ToggleTileAt(int tx, int ty, bool setSolid);
	void UpdateExplosions();
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for scene state management, tagged with m_ prefix to indicate member variables
	sf::RenderWindow& m_window;
	TileMap m_tileMap;
	Vec2 m_mouseWorld;
	bool m_previewActive = false;
	int m_brushTileValue = 1;
	bool m_prevCtrlS = false;
	std::string m_currentFilename;
	bool m_dirty = false;
	bool m_imguiOwned = false;
	bool m_showOpenDialog = false;
	std::string m_musicStatus;
	/////////////////////////////////



	/////////////////////////////////
	// UI helpers extracted from Update()
	void ShowOpenFileBrowser();
	void DrawAudioReactiveWindow();
	void DrawPlaybackControls();
	void LoadMusicFromPath(const std::string& path);
	/////////////////////////////////



	/////////////////////////////////
	// Refresh directory listing helper
	bool RefreshDirectoryListing(const std::filesystem::path& dir,
								 std::vector<std::filesystem::directory_entry>& outEntries, std::string& outError,
								 int& outSkipped, bool showNonAudio);
	/////////////////////////////////



	/////////////////////////////////
    // Audio-reactive visual effects
	void SpawnAudioReactiveExplosion(bool resetSpawnTimer = true);
	void SpawnCircularExplosion(bool resetSpawnTimer = true);
	void SpawnCircularExplosionByLevel(float level, bool resetSpawnTimer = true);
	/////////////////////////////////



	/////////////////////////////////
	// Equalizer bar management: pre-allocated bars that are lit based on spectrum
    // Initialize visualizer bars: visualCount = number of bars drawn; independent from spectrum band count
	void InitializeEqualizerBars(size_t visualCount);
	void UpdateEqualizerBars(const std::vector<float>& bands);
	void HideEqualizerBars();
	/////////////////////////////////



	/////////////////////////////////
	// Visual parameters for equalizer bars: height ratio and gap ratio
	float m_eqHeightRatio = 0.5f; // fraction of window height maximum
	float m_eqBarGap = 0.08f;     // fraction of bar width used as gap



	/////////////////////////////////
	// Per-bar smoothing (attack/release)
	float m_eqAttack = 0.6f;  // how quickly display rises to new value (0..1)
	float m_eqRelease = 0.12f; // how quickly display falls to new value (0..1)



	/////////////////////////////////
	// Display values for each preallocated bar (smoothed)
	std::vector<float> m_eqDisplayValues;



	/////////////////////////////////
	// Number of visual bars (can be larger than spectrum band count)
	int m_visualBarCount = 10;



	/////////////////////////////////
	bool m_EqualizerActive = false; // Whether the equalizer bars should be active and visible



	/////////////////////////////////
	// File dialog state
	bool m_showLoadDialog = false;
	bool m_showSaveDialog = false;
	/////////////////////////////////



	/////////////////////////////////
	// Buffers for file dialog input (fixed size char arrays for ImGui input fields)
	char m_saveFilenameBuffer[260] = {0};
	char m_loadFilenameBuffer[260] = {0};
	/////////////////////////////////



	/////////////////////////////////
	// Input state tracking for mouse buttons and scene input allowance
	bool m_prevLeftMouse = false;
	bool m_prevRightMouse = false;
	bool m_allowSceneInput = true;
	int m_lastClickedX = -1;
	int m_lastClickedY = -1;
	/////////////////////////////////



	/////////////////////////////////
	// Toggle log for debugging tile toggling actions, storing messages about tile changes for later review
	std::vector<std::string> m_toggleLog;
	/////////////////////////////////



	/////////////////////////////////
	// Tile map entity for rendering and interactions
	Entity* m_tileMapEntity = nullptr;
	/////////////////////////////////



	/////////////////////////////////
	// Flag to enable or disable ImGui rendering for this scene, allowing for a cleaner visual if desired
	bool m_enableImGui = true; 
	/////////////////////////////////



	/////////////////////////////////
	// Current directory for file browsing, initialized to the executable's directory for convenience when loading music files
	std::filesystem::path m_currentDir;
	/////////////////////////////////



	/////////////////////////////////
	// FPS tracking variable for display in the UI, updated each frame in the Update() method to show the current frames per second
	float m_fps = 0.0f;
	/////////////////////////////////
	


	/////////////////////////////////
	// Audio-reactive spawn state
	Entity* m_musicEntity = nullptr;
	bool m_audioReactive = false;
	float m_spawnThreshold = 0.02f;
	float m_spawnCooldown = 0.12f;
	float m_spawnTimer = 0.0f;
	int m_explosionCount = 0;
	/////////////////////////////////



	/////////////////////////////////
	// Whether spawn functions should reset the spawn timer when called
	bool m_resetTimerOnSpawn = true;
	/////////////////////////////////



	/////////////////////////////////
	// Spawn mode: 0 = random, 1 = circular, 2 = level-scaled circular
	int m_spawnMode = 2;
	/////////////////////////////////



	/////////////////////////////////
	// Circular spawn state (deterministic circular spawns)
	float m_circularAngle = 0.0f;	 // radians
	float m_circularRadius = 250.0f; // pixels from screen center
	float m_circularSpeed = 0.06f;	 // radians per spawn call
	/////////////////////////////////



	/////////////////////////////////
	// Debug UI: force OS cursor visible
	bool m_forceShowCursor = false;
	/////////////////////////////////



	/////////////////////////////////
	// Loop checkbox and playhead state
	bool m_loopEnabled = true;
	float m_playhead = 0.0f; // current playhead position in seconds
	float m_duration = 0.0f; // current track duration in seconds
	/////////////////////////////////



	/////////////////////////////////
	// Playhead drag state: pause on drag, resume on release
	bool m_playheadActive = false;
	bool m_wasPlayingBeforeSeek = false;
	/////////////////////////////////



	/////////////////////////////////
	// Spawn system for audio-reactive entities
	Spawn::SpawnSystem* m_spawnSystem = nullptr;
	/////////////////////////////////



	/////////////////////////////////
	// Request to restart the track from the beginning (used when pressing Play after track ends)
	bool m_requestRestart = false;
	/////////////////////////////////



	/////////////////////////////////
	// Temporary drawable storage for render queue. These are cleared at the start of Render() 
	// and repopulated with frame-specific drawables for enqueueing to the engine render queue.
	std::vector<std::shared_ptr<sf::RectangleShape>> m_tempGridShapes; // Temporary grid rectangles
	int m_nextTempShapeId = 0;
	/////////////////////////////////
};
/////////////////////////////////
