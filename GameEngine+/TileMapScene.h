/////////////////////////////////
// TileMapScene.h - Simple scene to test tilemap component and TileSystem
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the TileMapScene implementation.
#pragma once
#include "Scene.h"
#include "Raycast.h"
#include <vector>
/////////////////////////////////



/////////////////////////////////
// TileMapScene class - A simple scene to test the tilemap component and TileSystem. This scene allows for interactive raycasting and tile editing, providing visual debug overlays to help visualize the raycasting process and tile states. 
// It serves as a testing ground for the tilemap functionality and can be used to experiment with different tile configurations and raycasting scenarios. (Note: This scene is not intended to be a full level editor, but rather a simple testbed for tilemap features.)
class TileMapScene : public Scene {
	/////////////////////////////////
	// Public interface for the TileMapScene class
public:
	/////////////////////////////////
	// Constructor and destructor for the TileMapScene class. The constructor initializes the scene with references to the game engine, render window, and entity manager, while the destructor can be used to clean up any resources if needed.
	TileMapScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager);
	~TileMapScene() override;
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
	// Event / lifecycle methods for the TileMapScene class. These methods will handle processing input events, setting up the scene when it becomes active, and cleaning up when it exits. The LoadResources and UnloadResources methods can be used to manage 
	// any resources specific to this scene, while the InitializeGame method can be used to set up the initial game state (e.g., spawning a test tilemap).
	void HandleEvent(const std::optional<sf::Event>& event) override;
	void OnEnter() override;
	void OnExit() override;
	void LoadResources() override;
	void UnloadResources() override;
	void InitializeGame(sf::Vector2u windowSize) override;
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods for the TileMapScene class.
private:
	/////////////////////////////////
	// Rendering helpers for drawing the tile grid, debug lines, hit points, visited cells, and preview line. These methods will be called from the Render method to draw the appropriate visuals based on the current state of the scene and any raycasting interactions.
	void DrawTileGrid();
	void DrawDebugLines();
	void DrawHitPoints();
	void DrawRawHitPoints();
	void DrawVisitedCells();
	void DrawPreviewLine();
	/////////////////////////////////



	/////////////////////////////////
	// Input processing helpers for handling specific key presses (e.g., Left Ctrl for toggling preview mode, a debug toggle key, Escape for closing the window, and a save key for saving the tilemap). 
	// These methods will be called from the HandleEvent method to process user input and update the scene state accordingly.
	void ProcessLeftCtrlKey(bool keyDown);
	void ProcessDebugToggle(bool keyDown);
	void ProcessEscapeKey(bool keyDown) const;
	void ProcessSaveKey(bool keyDown) const;
	void ProcessMouseDragRaycast(bool leftMouseDown, const Vec2& mouseWorld);
	void ProcessMouseRightDrag(bool& rightMouseDown, const Vec2& mouseWorld);
	/////////////////////////////////



	/////////////////////////////////
	// SpawnTestTileMap - Helper method to create a test tilemap with some solid and empty tiles for raycasting tests. This method will populate the m_tileMap member variable with a predefined pattern of tiles, allowing for testing of raycasting interactions and visual debug overlays.
	void SpawnTestTileMap();
	/////////////////////////////////



	/////////////////////////////////
	// Raycasting helpers for performing DDA raycasts based on mouse interactions and synthesizing a start-cell hit when the ray begins inside a solid tile. These methods will be called from the Update method to perform raycasts and update the scene state based on the results.
	RaycastHit MakeStartCellHit(int tileX, int tileY,const Vec2& origin);
	/////////////////////////////////



	/////////////////////////////////
	// Tile editing helper for toggling the state of a tile at given tile coordinates. This method will be called from the ProcessMouseDragRaycast method to allow for interactive editing of the tilemap based on mouse input.
	void ToggleTileAt(int tx, int ty);
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the TileMapScene class
private:
	/////////////////////////////////
	// Reference to the SFML render window for rendering and context and a 2D grid of tile values for rendering. The m_debugLines vector will store pairs of Vec2 
	// representing the start and end points of lines to draw for raycasts, while the m_debugPoints vector will store hit points from raycasts.
	sf::RenderWindow& m_window; 
	Vec2 m_currentTile;
	/////////////////////////////////



	/////////////////////////////////
	// Debug visualization data for raycasts and tile states. The m_debugLines vector will store pairs of Vec2 representing the start and end points of lines to draw for raycasts, while the m_debugPoints vector will store hit points from raycasts.
	std::vector<std::pair<Vec2, Vec2>> m_debugLines; // lines to draw for raycasts
	std::vector<Vec2> m_debugPoints;				 // hit points
	std::vector<Vec2> m_rawHitPoints;				 // raw hit positions (before clamping) for debug
	/////////////////////////////////



	/////////////////////////////////
	// Mouse drag state for interactive raycasts
	bool m_lmbdragging = false;
	bool m_rmbdragging = false;
	/////////////////////////////////



	/////////////////////////////////
	// Start and end points for mouse drag raycasts. These will be used to perform raycasts when the user drags the mouse with the left or right button held down, allowing for interactive testing of raycasting and tile editing.
	Vec2 m_lmbDragStart = Vec2(0, 0);
	Vec2 m_lmbDragEnd = Vec2(0, 0);
	Vec2 m_rmbDragStart = Vec2(0, 0);
	Vec2 m_rmbDragEnd = Vec2(0, 0);
	/////////////////////////////////



	/////////////////////////////////
	// Preview mode state for showing a preview line while dragging with the left mouse button. m_previewLine will store the start and end points of the preview line, 
	// while m_previewActive will indicate whether the preview line should be drawn. m_tileMap will store the tilemap data for the scene, allowing for raycasting and tile editing interactions.
	bool m_previewActive = false;
	std::pair<Vec2, Vec2> m_previewLine;
	TileMap m_tileMap;
	/////////////////////////////////



	/////////////////////////////////
	// Input state tracking for mouse buttons and keys. These variables will be used to track the previous state of the left and right mouse buttons, as well as the Left Ctrl key, to allow for edge-triggered input processing (e.g., toggling preview mode on/off when Left Ctrl is pressed).
	bool m_prevLmbMouseDown = false;
	bool m_prevRmbMouseDown = false;
	bool m_leftCtrlKeyDown = false;
	/////////////////////////////////



	/////////////////////////////////
	// Debug toggle state for showing/hiding visual debug overlays. m_visualDebug will indicate whether the debug overlays should be drawn, while m_prevDebugKeyDown will track the previous state of the debug toggle key for edge-triggered toggling.
	bool m_visualDebug = true;
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
	// Deprecated: Font manager for text rendering. This member variable is no longer used, as scenes should use the engine's shared FontManager via m_gameEngine.GetFontManager() instead of maintaining their own instance.
	// FontManager m_fontManager; // Font manager for text rendering
	/////////////////////////////////
};
/////////////////////////////////