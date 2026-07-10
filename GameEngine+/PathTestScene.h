/////////////////////////////////
// PathTestScene - Pathfinding test scene using chunked editor-created levels.
// Loads the same level format as LevelEditorScene, allowing click-based pathfinding tests.
/////////////////////////////////

#pragma once
#include "Scene.h"
#include "ChunkManager.h"
#include "PathFindingSystem.h"
#include "CameraSystem.h"
#include "RenderQueue.h"
#include <SFML/Graphics.hpp>
#include <filesystem>

class PathTestScene : public Scene {
public:
	PathTestScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager);
	~PathTestScene() override;

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
	sf::RenderWindow& m_window;
	ChunkManager m_chunkManager;
	PathFindingSystem m_pathSystem;
	CameraSystem m_cameraSystem;
	RenderQueue m_renderQueue;
	float m_tileSize = 16.0f;
	int m_nodesPerFrame = 300;

	// Camera entity (like LevelEditor)
	Entity* m_cameraEntity = nullptr;

	// Map bounds for clamping camera
	Vec2 m_mapMin{0, 0};
	Vec2 m_mapMax{0, 0};
	bool m_haveBounds = false;

	// Level management (from LevelEditor design)
	std::string m_currentLevelName;
	std::vector<std::string> m_availableLevels;
	void ScanLevelFiles();
	bool SwitchToLevel(const std::string& name);

	// Camera control
	void ApplyMainCameraView();
	void EnsureVisibleChunks();

	// Pathfinding: click-based tests (left=start, right=goal)
	Vec2 m_manualStart{0, 0};
	Vec2 m_manualGoal{0, 0};
	bool m_manualStartSet = false;
	bool m_manualGoalSet = false;
	std::optional<std::vector<Vec2>> m_manualPath;
	bool m_manualPathComplete = false;
	Entity* m_manualEntity = nullptr;

	// Rendering
	void RenderDebugOverlay();
};