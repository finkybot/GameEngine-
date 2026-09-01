/////////////////////////////////
// TileMapScene.h - Simple scene to test tilemap component and TileSystem
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the TileMapScene implementation.
#pragma once
#include "Scene.h"
#include "Raycast.h"
#include "ChunkManager.h"
#include "CameraSystem.h"
#include <vector>
#include <string>
#include "BVHSystem.h"
/////////////////////////////////



/////////////////////////////////
// TileMapScene class - A simple scene to test the tilemap component and TileSystem. This scene allows for interactive raycasting and tile editing, providing visual debug overlays to help visualize the raycasting process and tile states. 
// It serves as a testing ground for the tilemap functionality and can be used to experiment with different tile configurations and raycasting scenarios. (Note: This scene is not intended to be a full level editor, but rather a simple testbed for tilemap features.)
//								|
//								|_______________________________________________________________________
class RayCastScene : public Scene {
	/////////////////////////////////
	// Public interface for the RayCastScene class
public:
	/////////////////////////////////
	// Constructor and destructor for the RayCastScene class. The constructor initializes the scene with references to the game engine, render window, and entity manager, while the destructor can be used to clean up any resources if needed.
	RayCastScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager);
	~RayCastScene() override;
	/////////////////////////////////



	/////////////////////////////////
	// Overridden virtual methods from the Scene base class. These methods handle updating the scene state, rendering the scene, processing input events, and managing the scene lifecycle (entering and exiting).
public:
	// The Update method will handle logic for processing user input, performing raycasts based on mouse interactions, and updating any necessary state for rendering. The Render method will handle drawing the tilemap and any visual debug overlays to the screen. 
	// The HandleEvent method will process SFML events for user input, while the OnEnter and OnExit methods can be used to set up and clean up the scene as needed.
	void Update(float deltaTime) override; 
	void Render() override;
	void DoAction() override;
	/////////////////////////////////



	/////////////////////////////////
	// Event / lifecycle methods for the RayCastScene class. These methods will handle processing input events, setting up the scene when it becomes active, and cleaning up when it exits. The LoadResources and UnloadResources methods can be used to manage 
	// any resources specific to this scene, while the InitializeGame method can be used to set up the initial game state (e.g., spawning a test tilemap).
	void HandleEvent(const std::optional<sf::Event>& event) override;
	void OnEnter() override;
	void OnExit() override;
	void OnWindowResized(sf::Vector2u newSize) override;
	void LoadResources() override;
	void UnloadResources() override;
	void InitialiseGame(sf::Vector2u windowSize) override;
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods for the TileMapScene class.
private:
	/////////////////////////////////
	// Rendering helpers for drawing the tile grid, debug lines, hit points, visited cells, and preview line. These methods will be called from the Render method to draw the appropriate visuals based on the current state of the scene and any raycasting interactions.
	void DrawTileGrid();
	void DrawDebugLines();
	void DrawBVHNode(sf::RenderWindow& window, BVHNode* node);
	void DrawHighlightedEntity(sf::RenderWindow& window);
	void DrawHitPoints();
	void DrawBVHHitPoint(const Vec2& p);
	void DrawBVHLeafNode(BVHNode* node);
	void DrawRawHitPoints();
	void DrawVisitedCells();
	void DrawPreviewLine();
	void DrawBVHNodeColored(BVHNode* node, const sf::Color& color);
	void DrawBVHTraversal(const BVHDebugTraversal& traversal);
	/////////////////////////////////



	/////////////////////////////////
	// Input processing helpers for handling specific key presses (e.g., Left Ctrl for toggling preview mode, a debug toggle key, Escape for closing the window, and a save key for saving the tilemap). 
	// These methods will be called from the HandleEvent method to process user input and update the scene state accordingly.
	void ProcessDebugToggle(bool keyDown);
	void ProcessEscapeKey(bool keyDown) const;
	void ProcessMouseDragRaycast(bool leftMouseDown, const Vec2& mouseWorld);
	void ProcessMiddleMousePan();
	void ApplyMainCameraView();
	void RefreshMapBounds();
	void RefreshAvailableLevels();
	bool SwitchToLevel(const std::string& name);
	void LevelManagerWindow();
	/////////////////////////////////



	/////////////////////////////////
	// Raycasting helpers for performing DDA raycasts based on mouse interactions and synthesizing a start-cell hit when the ray begins inside a solid tile.
	RaycastHit MakeStartCellHit(int tileX, int tileY, const Vec2& origin);
	bool RaycastDynamicEntities(const Vec2& origin, const Vec2& dir, float maxDistance, RaycastHit& outHit,	Entity*& outEntity);
	void UpdateHitInfo(float& nearest, float hitDist, Entity*& outEntity, Entity* e, RaycastHit& outHit, const Vec2& origin, Vec2& dirN);
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the TileMapScene class
private:
	/////////////////////////////////
	// Reference to the SFML render window for rendering and context and a 2D grid of tile values for rendering. The m_debugLines vector will store pairs of Vec2 
	// representing the start and end points of lines to draw for raycasts, while the m_debugPoints vector will store hit points from raycasts.
	sf::RenderWindow& m_window;
	sf::View m_worldView;
	/////////////////////////////////



	/////////////////////////////////
	// Debug visualization data for raycasts and tile states. The m_debugLines vector will store pairs of Vec2 representing the start and end points of lines to draw for raycasts, while the m_debugPoints vector will store hit points from raycasts.
	std::vector<std::pair<Vec2, Vec2>> m_debugLines;	// lines to draw for raycasts
	std::vector<sf::Color> m_debugLineColors;			// per-line color (contact vs no-contact)
	std::vector<Vec2> m_debugPoints;					// hit points
	std::vector<Vec2> m_dynamicHitPoints;
	std::vector<Vec2> m_rawHitPoints;					// raw hit positions (before clamping) for debug
	Entity* m_highlightedEntity = nullptr;				// currently highlighted entity from raycast
	std::vector<BVHDebugTraversal> m_debugTraversals;	// BVH traversal debug data
	/////////////////////////////////



	/////////////////////////////////
	// Mouse drag state for interactive raycasts
	bool m_lmbdragging = false;
	/////////////////////////////////



	/////////////////////////////////
	// Start and end points for mouse drag raycasts. These will be used to perform raycasts when the user drags the mouse with the left or right button held down, allowing for interactive testing of raycasting and tile editing.
	Vec2 m_lmbDragStart = Vec2(0, 0);
	Vec2 m_lmbDragEnd = Vec2(0, 0);
	/////////////////////////////////



	/////////////////////////////////
	// Preview mode state for showing a preview line while dragging with the left mouse button. m_previewLine stores start/end points for visual guidance.
	bool m_previewActive = false;
	std::pair<Vec2, Vec2> m_previewLine;
	ChunkManager& m_chunkManager;
	CameraSystem m_cameraSystem;
	Entity* m_cameraEntity = nullptr;
	int m_collisionLayer = 1;
	/////////////////////////////////



	/////////////////////////////////
	// Input state tracking for mouse buttons/keys used by the read-only raycast interactions.
	bool m_prevLmbMouseDown = false;
	/////////////////////////////////



	/////////////////////////////////
	// Debug toggle state for showing/hiding visual debug overlays. m_visualDebug will indicate whether the debug overlays should be drawn, while m_prevDebugKeyDown will track the previous state of the debug toggle key for edge-triggered toggling.
	bool m_visualDebug = false;
	bool m_prevDebugKeyDown = false;
	/////////////////////////////////



	/////////////////////////////////
	// Last raycast start tile information for synthesizing a start-cell hit when the ray begins inside a solid tile. These variables will store the coordinates of the last start tile and whether it was solid, allowing for proper handling of raycasts that originate inside solid tiles.
	int m_lastStartTileX = -1;
	int m_lastStartTileY = -1;
	bool m_lastStartSolid = false;
	/////////////////////////////////



	/////////////////////////////////
	// Cells visited by the last DDA raycast, stored as pairs of tile coordinates. This vector will be used for debug visualization to show which cells were visited during the raycasting process.
	std::vector<std::pair<int, int>> m_visitedCells;
	/////////////////////////////////



	/////////////////////////////////
	// Level selection state
	std::vector<std::string> m_availableLevels;
	int m_selectedLevelIndex = -1;
	std::string m_currentLevelName;

	// Persistent map bounds for camera/pan clamping (full saved level bounds, not only currently loaded chunks)
	Vec2 m_mapMin = Vec2(-512.0f, -512.0f);
	Vec2 m_mapMax = Vec2(512.0f, 512.0f);
	bool m_haveBounds = true;
	bool m_clampPanToBounds = true;
	bool m_includeDefaultEntitiesInRaycast = true;

	// Chunk/world refresh tracking for mask rebuilds (revision-based + teleport detection)
	uint64_t m_lastSeenWorldRevision = 0;
	Vec2 m_lastViewCenter = Vec2(0.0f, 0.0f);
	bool m_hasLastViewCenter = false;
	/////////////////////////////////
};