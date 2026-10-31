/////////////////////////////////
// World.cpp - Implementation of the World class, which manages the game world, including entities and chunks.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "World.h"
#include "Scene.h"
#include "EntityManager.h"
#include "ChunkManager.h"
#include <iostream>
/////////////////////////////////



/////////////////////////////////
World::World(EntityManager& entityManager, ChunkManager& chunkManager)
	: m_entityManager(entityManager), m_chunkManager(chunkManager) {}
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
		}

		m_entityManager.ClearAll();

		if (m_window)
			m_window->clear(sf::Color::Black);

		m_activeScene = it->second;
		m_activeScene->InitialiseGame(m_windowSize);
		m_activeScene->LoadResources();
		m_activeScene->OnEnter();
	} else {
		// If the scene is not found, log an error message
		std::cerr << "Scene '" << sceneName << "' not found!" << std::endl;
	}

}
/////////////////////////////////



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