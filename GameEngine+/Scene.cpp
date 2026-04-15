// Scene.cpp
#include "Scene.h"
#include "GameEngine.h"
#include "EntityManager.h"

Scene::Scene(GameEngine& gameEngine, EntityManager& entityManager)
	: m_gameEngine(gameEngine), m_entityManager(entityManager), m_frameCount(0), m_currentFrame(0), m_isLoaded(false),
	  m_isActive(false), m_isPaused(false) {}
