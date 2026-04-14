// MusicVisualizerScene.h - Music visualizer scene adapted from TileMapEditorScene
#pragma once
#include "Scene.h"
#include "TileMap.h"
#include "Vec2.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include "FileDialog.h"
#include <filesystem>

// Forward declarations
namespace Spawn { class SpawnSystem; }


// MusicVisualizerScene: A scene that visualizes music by spawning explosion entities based on audio analysis. It features a grid background and an ImGui interface for loading music, controlling playback, and adjusting audio-reactive spawn settings.
class MusicVisualizerScene : public Scene
{
	// Public methods
public:

	// Constructor and destructor
    MusicVisualizerScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager);
    ~MusicVisualizerScene() override;

	// Override virtual methods from Scene
    void Update(float deltaTime) override;
    void Render() override;
    void DoAction() override;
    void RenderDebugOverlay() override;
    bool IsImGuiEnabled() override { return m_enableImGui; }

	// Event handling and lifecycle methods
    void HandleEvent(const std::optional<sf::Event>& event) override;
    void OnEnter() override;
    void OnExit() override;

	// Resource management and initialization
    void LoadResources() override;
    void UnloadResources() override;
    void InitializeGame(sf::Vector2u windowSize) override;

	// Private helper methods for internal logic
private:
	// Helper methods for grid rendering, input processing, tile toggling, and explosion updates
    void DrawGrid();
    void ProcessInput();
    void ToggleTileAt(int tx, int ty, bool setSolid);
    void UpdateExplosions();

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

    // UI helpers extracted from Update()
    void ShowOpenFileBrowser();
    void DrawAudioReactiveWindow();
    void DrawPlaybackControls();
    void LoadMusicFromPath(const std::string& path);

    // Refresh directory listing helper
    bool RefreshDirectoryListing(const std::filesystem::path& dir, std::vector<std::filesystem::directory_entry>& outEntries, std::string& outError, int& outSkipped, bool showNonAudio);

    // Audio-reactive visual effects
    void SpawnAudioReactiveExplosion(bool resetSpawnTimer = true);
    void SpawnCircularExplosion(bool resetSpawnTimer = true);
    void SpawnCircularExplosionByLevel(float level, bool resetSpawnTimer = true);

	// File dialog state
    bool m_showLoadDialog = false;
    bool m_showSaveDialog = false;

	// Buffers for file dialog input (fixed size char arrays for ImGui input fields)
    char m_saveFilenameBuffer[260] = { 0 };
    char m_loadFilenameBuffer[260] = { 0 };

	// Input state tracking for mouse buttons and scene input allowance
    bool m_prevLeftMouse = false;
    bool m_prevRightMouse = false;
    bool m_allowSceneInput = true;
    int m_lastClickedX = -1;
    int m_lastClickedY = -1;

    std::vector<std::string> m_toggleLog;       	// Toggle log for debugging tile toggling actions
	Entity* m_tileMapEntity = nullptr;              // Entity representing the tile map in the scene, used for rendering and potential interactions
	bool m_enableImGui = true;                      // Flag to enable or disable ImGui rendering for this scene, allowing for a cleaner visual if desired
	std::filesystem::path m_currentDir;             // Current directory for file browsing, initialized to the executable's directory for convenience when loading music files
	float m_fps = 0.0f;                             // Variable to store the current frames per second (FPS) for display in the UI, updated each frame in the Update() method

    // Audio-reactive spawn state
    Entity* m_musicEntity = nullptr;
    bool m_audioReactive = false;
    float m_spawnThreshold = 0.02f;
    float m_spawnCooldown = 0.12f;
    float m_spawnTimer = 0.0f;
    int m_explosionCount = 0;

    // Whether spawn functions should reset the spawn timer when called
    bool m_resetTimerOnSpawn = true;

    // Spawn mode: 0 = random, 1 = circular, 2 = level-scaled circular
    int m_spawnMode = 2;

    // Circular spawn state (deterministic circular spawns)
    float m_circularAngle = 0.0f;    // radians
    float m_circularRadius = 250.0f; // pixels from screen center
    float m_circularSpeed = 0.06f;   // radians per spawn call

    // Debug UI: force OS cursor visible
    bool m_forceShowCursor = false;

    // Loop checkbox and playhead state
    bool m_loopEnabled = true;
    float m_playhead = 0.0f; // current playhead position in seconds
    float m_duration = 0.0f; // current track duration in seconds

    // Playhead drag state: pause on drag, resume on release
    bool m_playheadActive = false;
    bool m_wasPlayingBeforeSeek = false;

    // Spawn system for audio-reactive entities
    Spawn::SpawnSystem* m_spawnSystem = nullptr;

    // Request to restart the track from the beginning (used when pressing Play after track ends)
    bool m_requestRestart = false;
};
