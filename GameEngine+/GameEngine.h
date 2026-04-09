// ***** GameEngine.h - Game engine manager *****
#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
//#include "Scene.h"
#include "EntityManager.h"
#include "InputController.h"
#include "FontManager.h"
#include "TextureManager.h"

#include <map>
#include <string>
#include <iostream>


class Scene; // Forward declaration of Scene class to avoid circular dependency with GameEngine

class GameEngine
{
private:
	GameEngine();																		// Constructor - initializes the game engine, sets up the window, and prepares for the game loop
	~GameEngine();																		// Destructor - cleans up resources and shuts down the game engine


	InputController	m_InputController;													// Input controller

public:
	GameEngine(const GameEngine&) = delete;												// Deleted copy constructor to prevent copying of the game engine instance
	GameEngine& operator=(const GameEngine&) = delete;									// Deleted copy assignment operator to prevent copying of the game engine instance
	

	static GameEngine& GetInstance()													// Static method to access the singleton instance of the GameEngine, ensuring only one instance exists throughout the application, guranteed by the static local variable inside this method which is initialized on first call and destroyed when the program ends
	{
		static GameEngine instance;														
		return instance;
	}

	void AddScene(const std::string& sceneName, std::shared_ptr<Scene> scene);			// Adds a new scene to the game engine with the given name and scene instance, allowing for dynamic scene management
	void ChangeScene(const std::string& sceneName);										// Changes the current scene to the specified scene name, allowing for scene management and transitions
	void RemoveScene(const std::string& sceneName);										// Removes a scene from the game engine by its name, allowing for cleanup and resource management of scenes that are no longer needed
	void Run();																			// Main game loop - handles events, updates game state, and renders frames until the window is closed
	void Update(float deltaTime);														// Updates the current scene and game state based on the elapsed time since the last frame, allowing for time-based updates and game logic processing

	std::map<std::string, std::shared_ptr<Scene>> m_scenes;								// Map of scene names to scene instances, allowing for easy scene management and switching
	sf::RenderWindow m_window;															// SFML RenderWindow for rendering the game, handling events, and managing the main game window
	std::shared_ptr<Scene> m_currentScene;												// Pointer to the current active scene, used to determine which scene to update and render during the game loop
	bool m_isRunning = false;															// Flag to indicate whether the game loop is currently running, used to control the main game loop execution

	sf::Clock m_deltaClock;																// SFML Clock to measure the time elapsed between frames, used for calculating delta time for updates and game logic processing
	sf::Vector2u m_windowSize = { 0, 0 };												// Size of the game window, initialized to zero and set in the constructor based on the desktop mode

	FontManager m_fontManager;															// Font manager instance for managing fonts across the game, allowing for loading, retrieving, and unloading fonts in a centralized manner
	std::unique_ptr<EntityManager> m_entityManager;										// Unique pointer to the central EntityManager owned by the engine, responsible for managing game entities and providing access to the entity system throughout the game

	FontManager& GetFontManager() { return m_fontManager; }								// Accessor for shared font manager

	// Texture manager for atlases/tilesets
	TextureManager m_textureManager;
	TextureManager& GetTextureManager() { return m_textureManager; }
	EntityManager& GetEntityManager() const { return *m_entityManager; }						// Accessor for central entity manager, returns a reference to the EntityManager instance owned by the engine, allowing scenes and other game components to access and manage entities through the engine's central entity management system
};

