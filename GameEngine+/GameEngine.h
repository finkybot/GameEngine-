/////////////////////////////////
// GameEngine.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the GameEngine class. We include necessary headers for SFML graphics, memory management, and various managers for fonts, textures, entities, and input. We also forward declare the Scene class to avoid circular dependencies with the GameEngine.
#pragma once

#include "ShutdownGuard.h"
#include <SFML/Graphics.hpp>
#include <memory>
//#include "Scene.h"
#include "EntityManager.h"
#include "InputController.h"
#include "FontManager.h"
#include "TextureManager.h"
#include "FileManager.h"
#include "Utils/FPSCounter.h"
#include "CursorSystem.h"
#include "RenderQueue.h"
#include "SoundSystem.h"
#include "ChunkManager.h"
#include "TechRegistry.h"
#include "Fontsystem.h"
#include "RenderSystemGL.h"
#include "WorldDiffusionConfig.h"

#include <map>
#include <string>
#include <vector>

// MovementSystem class definition (inline to avoid include path issues)
class Entity;
class MovementSystem {
public:
	MovementSystem() = default;
	~MovementSystem() = default;
	void Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime);
private:
	static constexpr float WAYPOINT_ARRIVAL_THRESHOLD = 5.0f;
};

class Scene; // Forward declaration of Scene class to avoid circular dependency with GameEngine
/////////////////////////////////



/////////////////////////////////
//	|	GameEngine class definition. This class is responsible for managing the main game loop, handling scenes, and providing access to various managers for fonts, textures, entities, and input. It is implemented as a singleton
//	|_______________________________________________________________________
class GameEngine {

private:
	GameEngine();  // Constructor - initializes the game engine, sets up the window, and prepares for the game loop
	~GameEngine(); // Destructor - cleans up resources and shuts down the game engine


	// Private member variable/s
	InputController m_InputController; // Input controller


public:	
	InputController& GetInputController() { return m_InputController; }	// Access to the engine-wide InputController

	GameEngine(const GameEngine&) = delete;				// Deleted copy constructor and copy assignment operator to prevent copying of the game engine instance, ensuring that only one instance exists throughout the application.
	GameEngine& operator=(const GameEngine&) =	delete; // Operator= is deleted to prevent copying of the game engine instance, ensuring that only one instance exists throughout the application.


	// GetInstance - Singleton pattern implementation to provide a single instance of the GameEngine class. This method returns a reference to the static instance of the GameEngine, ensuring that only one instance exists throughout the application and providing global access to it.
	static GameEngine&	GetInstance() { 
		static GameEngine instance;
		return instance;
	}

	// AddScene - Adds a new scene to the game engine by its name, allowing for scene management and transitions. The method takes a scene name and a shared pointer to the scene instance, storing it in the scenes map for later retrieval and activation.
	void AddScene(const std::string& sceneName,	std::shared_ptr<Scene> scene); 
	
	// ChangeScene - Changes the current active scene to the specified scene by its name, allowing for scene transitions and updates. The method checks if the specified scene exists in the scenes map and sets it as the current active scene, calling the OnExit 
	// method of the previous scene and the OnEnter method of the new scene to handle any necessary cleanup and initialization.
	void ChangeScene(const std::string& sceneName);

	// RemoveScene - Removes a scene from the game engine by its name, allowing for scene management and cleanup. The method checks if the specified scene exists in the scenes map and removes it, freeing any associated resources and ensuring that the scene is no longer accessible or active within the game engine.
	void RemoveScene(const std::string& sceneName);

	// Run - Starts the main game loop, handling events, updating the current scene, and rendering the game. The method continuously processes input events, updates the game state based on the elapsed time since the last frame, and renders the current scene to the window until the game is exited or closed.
	void Run();

	// GetSceneNames - Returns a vector of scene names currently managed by the game engine, allowing for easy retrieval and display of available scenes. The method iterates through the scenes map and collects the names of all registered scenes, returning them as a vector of strings for use in menus, debugging, or other purposes.
	std::vector<std::string> GetSceneNames() const;

	// GetCursorSystem - Returns a reference to the engine-owned CursorSystem, allowing scenes and systems to access and manage the cursor's appearance and behavior. The method provides centralized control over the cursor, enabling consistent handling of cursor states and interactions across different scenes and game components.
	CursorSystem& GetCursorSystem() { return *m_cursorSystem; }

	// Update - Updates the game engine state based on the elapsed time since the last frame, allowing for time-based updates and game logic processing. The method calculates the delta time, updates the current scene, and handles any necessary state changes or interactions within the game engine.
	void Update(float deltaTime);


	std::map<std::string, std::shared_ptr<Scene>> scenes;	// Map of scene names to scene instances, allowing for easy scene management and switching
	sf::RenderWindow window;								// SFML RenderWindow for rendering the game, handling events, and managing the main game window
	std::shared_ptr<Scene>	currentScene;					// Pointer to the current active scene, used to determine which scene to update and render during the game loop
	bool isRunning =	false;								// Flag to indicate whether the game loop is currently running, used to control the main game loop execution

	sf::Clock deltaClock;									// SFML Clock to measure the time elapsed between frames, used for calculating delta time for updates and game logic processing
	sf::Vector2u windowSize = {0, 0};						// Size of the game window, initialized to zero and set in the constructor based on the desktop mode

	FontManager	fontManager;								// Font manager instance for managing fonts across the game, allowing for loading, retrieving, and unloading fonts in a centralized manner
	Fontsystem	fontsystem;									// Fontsystem instance for managing font rendering and text display in the game, providing functionality for rendering text with various fonts and styles
	std::unique_ptr<EntityManager> entityManager;			// Unique pointer to the central EntityManager owned by the engine, responsible for managing game entities and providing access to the entity system throughout the game
	std::unique_ptr<SoundSystem> soundSystem;				// Unique pointer to the SoundSystem owned by the engine, responsible for managing sound effects and audio playback
	std::unique_ptr<MovementSystem> movementSystem;			// Unique pointer to the MovementSystem owned by the engine, responsible for moving entities along computed paths

	TechRegistry techRegistry;								// TechRegistry instance for managing technology-related entities and interactions in the game, allowing for simulation of technology diffusion, evolution, and unlocking
	WorldDiffusionConfig worldDiffusionConfig;				// WorldDiffusionConfig instance for managing configuration related to technology diffusion in the game, allowing for customization of diffusion parameters and behavior
	FPSCounter m_fpsCounter;								// FPSCounter instance for tracking and calculating the frames per second (FPS) of the game, providing performance metrics and allowing for optimization and debugging of the game loop		
	TextureManager m_textureManager;						// Texture manager instance for managing textures across the game, allowing for loading, retrieving, and unloading textures in a centralized manner	
	std::unique_ptr<CursorSystem> m_cursorSystem;			// Unique pointer to the CursorSystem owned by the engine, responsible for managing the cursor's appearance and behavior across different scenes and game components
	RenderQueue	m_renderQueue;								// RenderQueue instance for managing the rendering order of drawables in the game, allowing for efficient rendering of entities and other visual elements based on depth and priority
	FileManager m_fileManager;								// FileManager instance for managing file operations, allowing for loading, saving, and organizing game assets and data
	ChunkManager m_chunkManager;							// ChunkManager instance for managing chunks in the game world, responsible for loading, unloading, and updating chunks as needed


	FontManager& GetFontManager() { return fontManager; }				// Accessor for shared font manager
	FPSCounter& GetFPSCounter() { return m_fpsCounter; }				// Accessor for shared FPS counter
	
	TextureManager& GetTextureManager() { return m_textureManager; }	// Accessor for shared texture manager
	EntityManager& GetEntityManager() const { return *entityManager; }	// Accessor for engine-owned shared EntityManager
	SoundSystem& GetSoundSystem() const { return *soundSystem; }		// Accessor for engine-owned shared SoundSystem
	MovementSystem& GetMovementSystem() { return *movementSystem; }		// Accessor for engine-owned shared MovementSystem
	RenderQueue& GetRenderQueue() { return m_renderQueue; }				// Accessor for engine-owned shared RenderQueue
	FileManager& GetFileManager() { return m_fileManager; }
	ChunkManager& GetChunkManager() { return m_chunkManager; }	
};
/////////////////////////////////