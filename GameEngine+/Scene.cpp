/////////////////////////////////
// Scene.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "Scene.h"
#include "GameEngine.h"
#include "EntityManager.h"
/////////////////////////////////



/////////////////////////////////
// Constructor for the Scene class. Initializes the scene with references to the GameEngine and EntityManager, as well as default values for frame count, loaded state, active state, and paused state. This constructor is protected and can only be called by derived scene classes, 
// which must provide the necessary references to the engine and entity manager.
Scene::Scene(GameEngine& gameEngine, EntityManager& entityManager)
	: m_gameEngine(gameEngine), m_entityManager(entityManager), m_frameCount(0), m_currentFrame(0), m_isLoaded(false),
	  m_isActive(false), m_isPaused(false) {}
/////////////////////////////////



/////////////////////////////////
// GetEngineRenderQueue - Accessor for the engine-wide render queue managed by GameEngine
RenderQueue& Scene::GetEngineRenderQueue() {
	return m_gameEngine.GetRenderQueue();
}
/////////////////////////////////