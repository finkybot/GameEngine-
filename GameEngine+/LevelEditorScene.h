// LevelEditorScene.h - Chunked level editor scene
#pragma once
#include "Scene.h"
#include "ChunkManager.h"
#include "CameraSystem.h"
#include <mutex>
#include <SFML/Graphics.hpp>
#include <atomic>
#include <mutex>
#include <string>

class LevelEditorScene : public Scene {
public:
	LevelEditorScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& em);
	~LevelEditorScene() override;		

	void Update(float deltaTime) override;
	void Render() override;
	void DoAction() override {}
	void HandleEvent(const std::optional<sf::Event>& event) override;
	void OnEnter() override;
	void OnExit() override;
	void LoadResources() override {}
	void UnloadResources() override {}
	void InitializeGame(sf::Vector2u windowSize) override;

private:
	void EnsureVisibleChunks();
	void ApplyMainCameraView();
	void ProcessInput();

	sf::RenderWindow& m_window;
	ChunkManager m_chunkManager;
	CameraSystem m_cameraSystem;
	Entity* m_cameraEntity = nullptr;
	float m_tileSize = 32.0f;

	// input state
	bool m_prevLmb = false;
	bool m_prevRmb = false;
	int m_brushValue = 1;
	int m_marginChunks = 1;

	// camera pan state
	bool m_panning = false;
	sf::Vector2i m_panStart = sf::Vector2i(0,0);
	Vec2 m_camPanStart = Vec2::Zero;

	// last known camera position for console updates
	Vec2 m_lastCameraPos = Vec2::Zero;

	// previous middle mouse state for debug
	bool m_prevMiddleDown = false;

	// Tileset UI state
	char m_tilesetKeyBuf[64] = "adventure";
	char m_tilesetPathBuf[512] = "";
	int m_tilesetTileW = 32;
	int m_tilesetTileH = 32;

	// Selected tile in preview (0-based atlas index). m_brushValue == m_selectedTileIndex+1
	int m_selectedTileIndex = 0;

	// Async atlas load state
	std::atomic<bool> m_atlasLoading{false};
	std::mutex m_atlasLoadMutex;
	bool m_atlasLoadFinished = false;
	bool m_atlasLoadSuccess = false;
	std::string m_atlasLoadKey;
	std::string m_atlasLoadPath;
	std::string m_atlasLoadMessage;



	// file browser state for tileset selection
	std::filesystem::path m_currentDir;
	char m_loadFilenameBuffer[512] = "";
};
