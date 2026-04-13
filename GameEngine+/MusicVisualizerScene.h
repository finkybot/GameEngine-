// MusicVisualizerScene.h - Music visualizer scene adapted from TileMapEditorScene
#pragma once
#include "Scene.h"
#include "TileMap.h"
#include "Vec2.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include "FileDialog.h"
#include <filesystem>

class MusicVisualizerScene : public Scene
{
public:
    MusicVisualizerScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager);
    ~MusicVisualizerScene() override;

    void Update(float deltaTime) override;
    void Render() override;
    void DoAction() override;
    void RenderDebugOverlay() override;
    bool IsImGuiEnabled() override { return m_enableImGui; }

    void HandleEvent(const std::optional<sf::Event>& event) override;
    void OnEnter() override;
    void OnExit() override;

    void LoadResources() override;
    void UnloadResources() override;
    void InitializeGame(sf::Vector2u windowSize) override;

private:
    void DrawGrid();
    void ProcessInput();
    void ToggleTileAt(int tx, int ty, bool setSolid);
    void UpdateExplosions();

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
    bool m_showLoadDialog = false;
    bool m_showSaveDialog = false;
    char m_saveFilenameBuffer[260] = { 0 };
    char m_loadFilenameBuffer[260] = { 0 };
    bool m_prevLeftMouse = false;
    bool m_prevRightMouse = false;
    bool m_allowSceneInput = true;
    int m_lastClickedX = -1;
    int m_lastClickedY = -1;
    std::vector<std::string> m_toggleLog;
    Entity* m_tileMapEntity = nullptr;
    bool m_enableImGui = true;
    std::filesystem::path m_currentDir;
    float m_fps = 0.0f;

    // Audio-reactive spawn state
    Entity* m_musicEntity = nullptr;
    bool m_audioReactive = false;
    float m_spawnThreshold = 0.02f;
    float m_spawnCooldown = 0.12f;
    float m_spawnTimer = 0.0f;
    int m_explosionCount = 0;
    // Debug UI: force OS cursor visible
    bool m_forceShowCursor = false;
    // Loop checkbox and playhead state
    bool m_loopEnabled = true;
    float m_playhead = 0.0f; // current playhead position in seconds
    float m_duration = 0.0f; // current track duration in seconds
    // Playhead drag state: pause on drag, resume on release
    bool m_playheadActive = false;
    bool m_wasPlayingBeforeSeek = false;
};
