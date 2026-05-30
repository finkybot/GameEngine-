/////////////////////////////////
// Scene.h - Base class for game scenes, defining the interface and common functionality for all scenes in the game engine. Scenes receive injected references to the GameEngine and EntityManager, allowing them to interact with the engine and manage game entities. 
// This class provides virtual methods for updating, rendering, handling events, and managing scene lifecycle, which derived scene classes must implement to define their specific behavior and content.
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the Scene class. We include necessary headers for SFML graphics and events, as well as the GameEngine and EntityManager classes that will be injected into scenes for managing game state and entities.
#pragma once
#include <SFML/Window/Event.hpp>

#include <SFML/Graphics.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/System/Clock.hpp>
#include "GameEngine.h"

class GameEngine;
class EntityManager;
/////////////////////////////////



/////////////////////////////////
// Base class for scenes. Scenes receive injected references to engine + entity manager.
class Scene {
	/////////////////////////////////
	// Public interface for the Scene class, including virtual destructor and pure virtual methods for updating, rendering, handling events, and managing scene lifecycle. Derived scene classes must implement these methods to define their specific behavior and content.
public:
	virtual ~Scene() = default;

	virtual void Update(float deltaTime) = 0;
	virtual void Render() = 0;
	virtual void DoAction() = 0;
	/////////////////////////////////



	/////////////////////////////////
	// Event / lifecycle
	virtual void HandleEvent(const std::optional<sf::Event>& event) = 0;
	virtual void OnEnter() = 0;
	virtual void OnExit() = 0;
	virtual void LoadResources() = 0;
	virtual void UnloadResources() = 0;
	virtual void InitializeGame(sf::Vector2u windowSize) = 0;
	/////////////////////////////////



	/////////////////////////////////
	// Optional: draw overlays after ImGui/UI has been rendered (engine calls this after ImGui::SFML::Render)
	virtual void RenderDebugOverlay() {}
	/////////////////////////////////
	

	
	/////////////////////////////////
	// Allow scene to control whether ImGui should be updated/rendered for this scene
	virtual bool IsImGuiEnabled() { return true; }
	/////////////////////////////////



	/////////////////////////////////
	// Accessors for injected references (scenes do not own these)
	GameController* GetGameController() { return &m_GameController; }
	/////////////////////////////////



	/////////////////////////////////
	// non-copyable
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;
	/////////////////////////////////



	/////////////////////////////////
	// Scene holds a non-owning reference to the engine's EntityManager
	EntityManager& m_entityManager;
	/////////////////////////////////



	/////////////////////////////////
	// Helper to access the injected entity manager (already a reference)
	EntityManager& GetEntityManager() { return m_entityManager; }
	/////////////////////////////////



	/////////////////////////////////
	// Helper to access the injected game engine (already a reference)
protected:
	/////////////////////////////////
	// Construction contract: derived scenes must initialize these references
	Scene(GameEngine& gameEngine, EntityManager& entityManager);
	/////////////////////////////////



	/////////////////////////////////
	GameController m_GameController;
	/////////////////////////////////



	/////////////////////////////////
	// injected references (Scene does not own these)
	GameEngine& m_gameEngine;
	/////////////////////////////////
	


	/////////////////////////////////
	// scene state
	int m_frameCount = 0;
	int m_currentFrame = 0;
	bool m_isLoaded = false;
	bool m_isActive = false;
	bool m_isPaused = false;
	/////////////////////////////////



	/////////////////////////////////
	sf::Clock deltaClock;
	/////////////////////////////////
};
/////////////////////////////////