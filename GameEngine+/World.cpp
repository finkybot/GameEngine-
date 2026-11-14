/////////////////////////////////
// World.cpp - Implementation of the World class, which manages the game world, including entities and chunks.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "World.h"
#include "GameEngine.h"
#include "Scene.h"
#include "EntityManager.h"
#include "ChunkManager.h"
#include <iostream>
/////////////////////////////////



/////////////////////////////////
World::World(GameEngine& gameEngine, ChunkManager& chunkManager, sf::RenderWindow& window)
	: m_chunkManager(chunkManager), m_gameEngine(&gameEngine), m_entityManager(std::make_unique<EntityManager>(window)) {}
/////////////////////////////////



/////////////////////////////////
void World::Update(float deltaTime) {}
/////////////////////////////////



/////////////////////////////////
// AddScene - Adds a new scene to the world with the given name and scene pointer.
void World::AddScene(const std::string& sceneName, ScenePtr scene) {
	m_scenes[sceneName] = scene;
}
/////////////////////////////////



/////////////////////////////////
// RemoveScene - Removes a scene from the world by its name.
void World::RemoveScene(const std::string& sceneName) {
	// Find the scene in the map and remove it if it exists
	auto it = m_scenes.find(sceneName);

	// If the scene is found, erase it from the map
	if (it != m_scenes.end()) {
		m_scenes.erase(it);
	}
}
/////////////////////////////////



/////////////////////////////////
// ChangeScene - Changes the current scene to the one specified by sceneName.
void World::ChangeScene(const std::string& sceneName) {
	// Check if the scene exists in the map
	auto it = m_scenes.find(sceneName);

	// If the scene is found, set it as the current scene
	if (it != m_scenes.end()) {
		if (m_activeScene == it->second)
			return;

		if (m_activeScene) {
			m_activeScene->OnExit();
			m_activeScene->UnloadResources();
			m_activeScene->ClearRenderQueue();
			ClearAllEntities();
		}

		if (m_window)
			m_window->clear(sf::Color::Black);

		m_activeScene = it->second;
		m_activeScene->Initialise(m_windowSize);
		m_activeScene->LoadResources();
		m_activeScene->OnEnter();
	} else {
		// If the scene is not found, log an error message
		std::cerr << "Scene '" << sceneName << "' not found!" << std::endl;
	}

}
/////////////////////////////////

EntityManager& World::GetEntityManager() { return *m_entityManager; }
const EntityManager& World::GetEntityManager() const { return *m_entityManager; }
GameEngine& World::GetGameEngine() { return *m_gameEngine; }
InputController& World::GetInputController() { return m_gameEngine->GetInputController(); }
CursorSystem& World::GetCursorSystem() { return m_gameEngine->GetCursorSystem(); }
TextureManager& World::GetTextureManager() { return m_gameEngine->GetTextureManager(); }
FontManager& World::GetFontManager() { return m_gameEngine->GetFontManager(); }
FPSCounter& World::GetFPSCounter() { return m_gameEngine->GetFPSCounter(); }
FileManager& World::GetFileManager() { return m_gameEngine->GetFileManager(); }
RenderQueue& World::GetRenderQueue() { return m_gameEngine->GetRenderQueue(); }
TechRegistry& World::GetTechRegistry() { return m_gameEngine->GetTechRegistry(); }
WorldDiffusionConfig& World::GetWorldDiffusionConfig() { return m_gameEngine->GetWorldDiffusionConfig(); }
Entity* World::CreateEntity(EntityType type) { return GetEntityManager().AddEntity(type); }
Entity* World::GetEntity(EntityID id) { return GetEntityManager().GetEntity(id); }
void World::DestroyEntity(EntityID id) { GetEntityManager().DestroyEntity(id); }
EntityVector& World::GetEntities() { return GetEntityManager().GetEntities(); }
std::vector<Entity*>& World::GetEntities(EntityType type) { return GetEntityManager().GetEntities(type); }
void World::AddEntityToBucket(BucketID bucket, EntityID id) { GetEntityManager().AddEntityToBucket(bucket, id); }
EntityBucket* World::GetEntityBucket(BucketID bucketId) { return GetEntityManager().GetEntityBucket(bucketId); }
void World::ClearAllEntities() { GetEntityManager().ClearAll(); }



/////////////////////////////////
// HasScene - Checks if a scene with the given name exists in the world.
bool World::HasScene(const std::string& sceneName) const {
	return m_scenes.find(sceneName) != m_scenes.end();
}
/////////////////////////////////



/////////////////////////////////
// GetSceneNames - Returns a vector of all scene names currently in the world.
std::vector<std::string> World::GetSceneNames() const {
	// Create a vector to hold the names of the scenes
	std::vector<std::string> names;

	for (const auto& p : m_scenes) {
		names.push_back(p.first); // Add the scene name (key) to the vector
	}
	return names;
}
/////////////////////////////////
