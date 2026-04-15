// TileMapEditorScene.h - Simple interactive tilemap editor scene
#pragma once
#include "Scene.h"
#include "TileMap.h"
#include "Vec2.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include "FileDialog.h"
#include <filesystem>

class TileMapEditorScene : public Scene {
	// Public Methods
public:
	TileMapEditorScene(
		GameEngine& engine, sf::RenderWindow& win,
		EntityManager&
			entityManager); // Constructor - initializes the tile map editor scene with references to the game engine, render window, and entity manager
	~TileMapEditorScene()
		override; // defaulted since we don't have any special cleanup logic, but we could add it if needed in the future

	// Override virtual methods from Scene base class
	void Update(float deltaTime) override;
	void Render() override;
	void DoAction() override;
	void RenderDebugOverlay()
		override; // Optional method to render debug visuals on top of the main render, can be used for things like grid lines, tile highlights, etc.
	bool IsImGuiEnabled() override { return m_enableImGui; } // Allow toggling ImGui on/off for debugging purposes

	// Event handling methods
	void HandleEvent(const std::optional<sf::Event>& event) override;
	void OnEnter() override;
	void OnExit() override;

	// Resource management methods
	void LoadResources() override;
	void UnloadResources() override;
	void InitializeGame(sf::Vector2u windowSize) override;

	// Private methods
private:
	void DrawGrid(); // Draw grid lines based on tile map dimensions and tile size
	void
	ProcessInput(); // Handle user input for editing the tile map, such as mouse clicks for toggling tiles, keyboard shortcuts for saving/loading, etc.
	void ToggleTileAt(
		int tx, int ty,
		bool
			setSolid); // Toggle the tile at the given tile coordinates (tx, ty) to be solid or empty based on setSolid parameter

	// private member variables
	sf::RenderWindow& m_window;
	TileMap m_tileMap; // The tile map data structure
	Vec2 m_mouseWorld; // Current mouse position in world coordinates
	bool m_previewActive =
		false; // Preview state for drag-based raycsting or tile placement (not fully implemented in this snippet, but can be used for showing a preview of the tile being placed or the raycast path)
	int m_brushTileValue = 1;	   // value to paint
	bool m_prevCtrlS = false;	   // track Ctrl+S state for save shortcut
	std::string m_currentFilename; // currently loaded/saved filename (empty if new/unsaved)
	bool m_dirty = false;		   // track whether there are unsaved changes to the tile map
	bool m_imguiOwned =
		false; // track whether ImGui has been initialized and is owned by this scene (used to safely call ImGui functions without checking for initialization in every method)
	bool m_showLoadDialog =
		false; // whether to show the load file dialog (can be triggered by a button in the UI or a keyboard shortcut)
	bool m_showSaveDialog =
		false; // whether to show the save file dialog (can be triggered by a button in the UI or a keyboard shortcut)
	char m_saveFilenameBuffer[260] = {0}; // buffer for save file dialog input (initialized to empty string)
	char m_loadFilenameBuffer[260] = {0}; // buffer for load file dialog input (initialized to empty string)
	bool m_prevLeftMouse = false;  // track previous left mouse button state for edge detection of clicks and drags
	bool m_prevRightMouse = false; // track previous right mouse button state for edge detection of clicks and drags
	bool m_allowSceneInput = true; // when false, UI still works but scene input is ignored
	int m_lastClickedX =
		-1; // track last clicked tile coordinates for potential use in features like click-and-drag to toggle multiple tiles, or for displaying info about the last clicked tile. Initialized to -1 to indicate no tile has been clicked yet.
	int m_lastClickedY =
		-1; // track last clicked tile coordinates for potential use in features like click-and-drag to toggle multiple tiles, or for displaying info about the last clicked tile. Initialized to -1 to indicate no tile has been clicked yet.
	std::vector<std::string>
		m_toggleLog; // log of tile toggles for debugging purposes, can be displayed in an ImGui window or output to console
	Entity* m_tileMapEntity =
		nullptr; // Entity representing the tile map in the entity manager, used for rendering the tile map using the engine's TileSystem. We keep a pointer to it so we can update or replace it when the tile map changes.
	bool m_enableImGui =
		true; // whether to enable ImGui rendering and input for this scene, can be toggled for debugging purposes to isolate scene rendering without UI
	std::filesystem::path m_currentDir; // current folder for file navigator
	float m_fps = 0.0f;					// smoothed FPS value for ImGui display
										// Audio-reactive spawn state
	Entity* m_musicEntity = nullptr;	// music entity created by the UI (if any)
	bool m_audioReactive = false;		// whether to spawn shapes to music
	float m_spawnThreshold = 0.04f;		// RMS threshold to trigger spawn
	float m_spawnCooldown = 0.12f;		// seconds between spawns on peaks
	float m_spawnTimer = 0.0f;			// accumulates delta time
	int m_explosionCount = 0;			// active explosion count for UI
	void UpdateExplosions();			// update explosion lifetimes and visuals
};
