// ***** Scene.h - Base class for game scenes *****
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

// Base class for scenes. Scenes receive injected references to engine + entity manager.
class Scene
{
public:
	virtual ~Scene() = default;

	virtual void Update(float deltaTime) = 0;
	virtual void Render() = 0;
	virtual void DoAction() = 0;

	// Event / lifecycle
	virtual void HandleEvent(const std::optional<sf::Event>& event) = 0;
	virtual void OnEnter() = 0;
	virtual void OnExit() = 0;
	virtual void LoadResources() = 0;
	virtual void UnloadResources() = 0;
	virtual void InitializeGame(sf::Vector2u windowSize) = 0;

	GameController* GetGameController() { return &m_GameController; }

	// non-copyable
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;

	EntityManager* m_entityManager;

protected:
	// Construction contract: derived scenes must initialize these references
	Scene(GameEngine& gameEngine);
	
	GameController m_GameController;

	// injected references (Scene does not own these)
	GameEngine& m_gameEngine;

	// scene state
	int m_frameCount = 0;
	int m_currentFrame = 0;
	bool m_isLoaded = false;
	bool m_isActive = false;
	bool m_isPaused = false;

	sf::Clock deltaClock;
};
