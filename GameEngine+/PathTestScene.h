/////////////////////////////////
// PathTestScene - Pathfinding test scene using chunked editor-created levels.
// Loads the same level format as LevelEditorScene, allowing click-based pathfinding tests.
// Users can click to set start (left button) and goal (right button) points for pathfinding, and the scene will visualize the computed path.
// middle-mouse dragging allows panning the camera, and the camera is clamped to the bounds of the loaded level.
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Scene.h"
#include "ChunkManager.h"
#include "PathFindingSystem.h"
#include "CameraSystem.h"
#include "RenderQueue.h"
#include <SFML/Graphics.hpp>
#include <filesystem>
/////////////////////////////////



/////////////////////////////////
// PathTestScene class - A scene for testing pathfinding in a tile-based game engine.
//								|
//								|_______________________________________________________________________
class PathTestScene : public Scene {
	/////////////////////////////////
	// Public interface
public:
	/////////////////////////////////
	// Constructor and destructor for the PathTestScene class. Initializes the scene with references to the game engine, render window, and entity manager.
	PathTestScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager);
	~PathTestScene() override;
	/////////////////////////////////



	/////////////////////////////////
	// Overridden methods from the Scene base class for updating, rendering, handling events, entering/exiting the scene, and managing resources. These methods 
	// implement the specific behavior of the PathTestScene.
	void Update(float deltaTime) override;
	void Render() override;
	void DoAction() override {}
	void HandleEvent(const std::optional<sf::Event>& event) override;
	void OnEnter() override;
	void OnExit() override;
	void OnWindowResized(sf::Vector2u newSize) override;
	void LoadResources() override {}
	void UnloadResources() override;
	void InitialiseGame(sf::Vector2u windowSize) override;
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods and member variables
private:
	/////////////////////////////////
	// Member variables for managing the render window, chunk manager, pathfinding system, camera system, render queue, tile size, and nodes processed per frame
	sf::RenderWindow& m_window;
	ChunkManager& m_chunkManager;
	PathFindingSystem m_pathSystem;
	CameraSystem m_cameraSystem;
	RenderQueue m_renderQueue;
	float m_tileSize = 16.0f;
	int m_nodesPerFrame = 300;
	/////////////////////////////////



	/////////////////////////////////
	// Camera entity (like LevelEditor, I've basically nicked it from there)
	Entity* m_cameraEntity = nullptr;
	/////////////////////////////////



	/////////////////////////////////
	// Map bounds for clamping camera
	Vec2 m_mapMin{0, 0};
	Vec2 m_mapMax{0, 0};
	bool m_haveBounds = false; // true if m_mapMin/m_mapMax are valid and should be used to clamp camera position
	/////////////////////////////////
	 
	 

	/////////////////////////////////
	// Level management (from LevelEditor design)
	std::string m_currentLevelName;
	std::vector<std::string> m_availableLevels;
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods for scanning level files and switching to a specific level. These methods handle the loading 
	// and management of levels in the scene.
	void ScanLevelFiles();
	bool SwitchToLevel(const std::string& name);
	/////////////////////////////////



	/////////////////////////////////
	// Camera control
	void ApplyMainCameraView();
	void EnsureVisibleChunks(); // Ensure all chunks that intersect the current camera view are loaded and ready for rendering.
	/////////////////////////////////



	/////////////////////////////////
	// Middle-mouse panning state
	bool m_panning = false;
	bool m_prevMiddleDown = false;
	sf::Vector2i m_panStart = sf::Vector2i(0,0);
	Vec2 m_camPanStart = Vec2::Zero;
	/////////////////////////////////



	/////////////////////////////////
	// Pathfinding: click-based tests (left=start, right=goal)
	Vec2 m_manualStart{0, 0};
	Vec2 m_manualGoal{0, 0};
	bool m_manualStartSet = false;
	bool m_manualGoalSet = false;
	std::optional<std::vector<Vec2>> m_manualPath;
	bool m_manualPathComplete = false;
	Entity* m_manualEntity = nullptr;
	/////////////////////////////////

 

	/////////////////////////////////
	// Rendering
	void RenderDebugOverlay();
	/////////////////////////////////



	/////////////////////////////////
	// Movement system test: small square entity that follows paths on right-click
	Entity* m_movementTester = nullptr;				// Red square entity for testing path following
	bool m_movementTestActive = false;				// Whether movement testing is currently active
	/////////////////////////////////
};