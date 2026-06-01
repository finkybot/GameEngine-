/////////////////////////////////
// GameEngine.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the GameEngine class. We include necessary headers for SFML graphics, memory management, and various managers for fonts, textures, entities, and input. We also forward declare the Scene class to avoid circular dependencies with the GameEngine.
#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
//#include "Scene.h"
#include "EntityManager.h"
#include "InputController.h"
#include "FontManager.h"
#include "TextureManager.h"
#include "Utils/FPSCounter.h"
#include "CursorSystem.h"

#include <map>
#include <string>
#include <iostream>

class Scene; // Forward declaration of Scene class to avoid circular dependency with GameEngine
/////////////////////////////////



/////////////////////////////////
// GameEngine class definition. This class is responsible for managing the main game loop, handling scenes, and providing access to various managers for fonts, textures, entities, and input. It is implemented as a singleton
class GameEngine {
	/////////////////////////////////
	// Private constructor and destructor to enforce singleton pattern. The constructor initializes the game engine, sets up the window, and prepares for the game loop, while the destructor cleans up resources and shuts down the game engine.
private:
	GameEngine();  // Constructor - initializes the game engine, sets up the window, and prepares for the game loop
	~GameEngine(); // Destructor - cleans up resources and shuts down the game engine
	/////////////////////////////////



	/////////////////////////////////
	// Private member variable/s
	InputController m_InputController; // Input controller
	/////////////////////////////////



	/////////////////////////////////
	// Public interface for accessing the singleton instance of the GameEngine and managing scenes. The copy constructor and copy assignment operator are deleted to prevent copying of the game engine instance, ensuring that only one instance exists throughout the application. 
	// The GetInstance method provides access to the singleton instance, while AddScene, ChangeScene, RemoveScene, Run, and Update methods provide functionality for scene management and the main game loop.
public:

	/////////////////////////////////
	// Deleted copy constructor and copy assignment operator to prevent copying of the game engine instance, ensuring that only one instance exists throughout the application.
	GameEngine(const GameEngine&) = delete;
	/////////////////////////////////



	//////////////////////////////////
	// Operator= is deleted to prevent copying of the game engine instance, ensuring that only one instance exists throughout the application.
	GameEngine& operator=(const GameEngine&) =	delete; 
	/////////////////////////////////



	/////////////////////////////////
	// GetInstance - Static method to access the singleton instance of the GameEngine, ensuring only one instance exists throughout the application. 
	// The static local variable inside this method is initialized on the first call and destroyed when the program ends, providing a thread-safe and lazy-initialized singleton implementation.
	static GameEngine&	GetInstance() { 
		static GameEngine instance;
		return instance;
	}
	/////////////////////////////////



	/////////////////////////////////
	// AddScene - Adds a new scene to the game engine with the given name and scene instance, allowing for dynamic scene management. The scene is stored in a map of scene names to scene instances, enabling easy retrieval and switching between scenes during the game loop.
	void AddScene(const std::string& sceneName, std::shared_ptr<Scene>	scene);
	/////////////////////////////////



	/////////////////////////////////
	// ChangeScene - Changes the current scene to the specified scene name, allowing for scene management and transitions. The method checks if the specified scene exists in the scenes map and sets it as the current active scene, enabling the game loop to update and render the new scene.
	void ChangeScene(const std::string& sceneName);
	/////////////////////////////////



	/////////////////////////////////
	// RemoveScene - Removes a scene from the game engine by its name, allowing for cleanup and resource management of scenes that are no longer needed. The method checks if the specified scene exists in the scenes map and removes it, freeing up resources associated with that scene and ensuring it is no longer updated or rendered in the game loop.
	void RemoveScene(const std::string& sceneName);
	/////////////////////////////////



	/////////////////////////////////
	// Run - Main game loop that handles events, updates the game state, and renders frames until the window is closed. The loop continuously processes input events, updates the current scene based on the elapsed time since the last frame, and renders the current scene to the window, providing a real-time interactive experience for the player.
	void Run();
	/////////////////////////////////



	/////////////////////////////////
	// GetSceneNames - Return a list of registered scene names (useful for UI like a main menu)
	std::vector<std::string> GetSceneNames() const;
	/////////////////////////////////



	/////////////////////////////////
	// GetCursorSystem - Accessor for the cursor system, allowing scenes and other game components to interact with the cursor system through the game engine's centralized management. This method returns a reference to the CursorSystem instance owned by the engine, enabling scenes to change the cursor mode and update/render the cursor as needed.
	CursorSystem& GetCursorSystem() { return *m_cursorSystem; }
	/////////////////////////////////



	/////////////////////////////////
	// Update - Updates the current scene and game state based on the elapsed time since the last frame, allowing for time-based updates and game logic processing. The method calculates the delta time using the SFML clock and calls the update method of the current active scene, enabling smooth and consistent updates regardless of frame rate variations.
	void Update(float deltaTime);
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for managing scenes, the game window, the current active scene, the game loop state, and various managers for fonts, textures, entities, and input. These variables are used throughout the game engine to manage the game state, handle rendering, and provide access to resources and systems needed for game development.
	std::map<std::string, std::shared_ptr<Scene>> m_scenes; // Map of scene names to scene instances, allowing for easy scene management and switching
	sf::RenderWindow m_window; // SFML RenderWindow for rendering the game, handling events, and managing the main game window
	std::shared_ptr<Scene>	m_currentScene; // Pointer to the current active scene, used to determine which scene to update and render during the game loop
	bool m_isRunning =	false; // Flag to indicate whether the game loop is currently running, used to control the main game loop execution

	sf::Clock m_deltaClock; // SFML Clock to measure the time elapsed between frames, used for calculating delta time for updates and game logic processing
	sf::Vector2u m_windowSize = {0, 0}; // Size of the game window, initialized to zero and set in the constructor based on the desktop mode

	FontManager	m_fontManager; // Font manager instance for managing fonts across the game, allowing for loading, retrieving, and unloading fonts in a centralized manner
	std::unique_ptr<EntityManager>	m_entityManager; // Unique pointer to the central EntityManager owned by the engine, responsible for managing game entities and providing access to the entity system throughout the game
	/////////////////////////////////



	/////////////////////////////////
	// Accessor methods for the various managers and systems. These methods provide access to the font manager, texture manager, entity manager, and FPS counter, allowing scenes and other game components to interact with these systems through the game engine's centralized management.
	FontManager& GetFontManager() { return m_fontManager; } // Accessor for shared font manager
	/////////////////////////////////



	/////////////////////////////////
	// Texture manager for atlases/tilesets
	FPSCounter m_fpsCounter; // Shared FPS counter that scenes and UI can query
	FPSCounter& GetFPSCounter() { return m_fpsCounter; }
	TextureManager m_textureManager;
	TextureManager& GetTextureManager() { return m_textureManager; }



	
	EntityManager& GetEntityManager() const { return *m_entityManager; } // Accessor for central entity manager, returns a reference to the EntityManager instance owned by the engine, allowing scenes and other game components to access and manage entities through the engine's central entity management system
	/////////////////////////////////



	/////////////////////////////////
	// Cursor system for global cursor handling
	std::unique_ptr<CursorSystem> m_cursorSystem;
	/////////////////////////////////
	
};
/////////////////////////////////