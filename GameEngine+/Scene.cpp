// Scene.cpp
#include "Scene.h"
#include "GameEngine.h"
#include "EntityManager.h"

Scene::Scene(GameEngine& gameEngine)
	: m_gameEngine(gameEngine)
	, m_entityManager(nullptr)	// Derived scenes must initialize this reference in their constructor after calling the base constructor
	, m_frameCount(0)
	, m_currentFrame(0)
	, m_isLoaded(false)
	, m_isActive(false)
	, m_isPaused(false)
{}
