/////////////////////////////////
// World.h - Header file for the World class
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

class EntityManager;
class ChunkManager;

class Scene;
/////////////////////////////////



/////////////////////////////////
using ScenePtr = std::shared_ptr<Scene>;
using SceneMap = std::unordered_map<std::string, ScenePtr>;
/////////////////////////////////



/////////////////////////////////
//	|	World class - Represents the game world, including entities, tile maps, and other game state. This class is currently a placeholder and can be expanded in the future to include more functionality as needed.
//	|_______________________________________________________________________
class World {
public:

	// Constructor and destructor for the World class. The constructor takes references to the EntityManager and ChunkManager, which are used to manage entities and tile-based chunks in the game world.
	World(EntityManager& entityManager, ChunkManager& chunkManager);
	~World() = default;

	void Update(float deltaTime); // Update the world state, including entities and tile maps, based on the elapsed time since the last update.
	void SetWindowSize(sf::Vector2u windowSize) { m_windowSize = windowSize; }
	void SetRenderWindow(sf::RenderWindow& window) { m_window = &window; }

	EntityManager& GetEntityManager() {	return m_entityManager; } // Accessor for the EntityManager, allowing other parts of the code to interact with entities in the world.

	ChunkManager& GetChunkManager() { return m_chunkManager; } // Accessor for the ChunkManager, allowing other parts of the code to interact with tile-based chunks in the world.

	void AddScene(const std::string& sceneName, ScenePtr scene); // Add a new scene to the world, allowing for scene management and transitions.
	void RemoveScene(const std::string&	sceneName); // Remove a scene from the world by its name, allowing for scene management and cleanup.
	void ChangeScene(const std::string&	sceneName); // Change the current active scene to the specified scene by its name, allowing for scene transitions and updates.
	bool HasScene(const std::string& sceneName)	const; // Check if a scene with the specified name exists in the world, allowing for scene management and validation.

	Scene* GetActiveScene() const {	return m_activeScene.get(); } // Accessor for the current active scene, allowing other parts of the code to interact with the active scene in the world.

	std::vector<std::string> GetSceneNames() const; // Get a list of all scene names in the world, allowing for scene management and UI display.

private:
	EntityManager& m_entityManager;
	ChunkManager& m_chunkManager;


	SceneMap m_scenes; // Map of scene names to scene instances, allowing for easy management and switching between different scenes in the game world.
	ScenePtr m_activeScene; // Pointer to the current active scene, used to determine which scene to update and render during the game loop.
	sf::Vector2u m_windowSize{};
	sf::RenderWindow* m_window = nullptr;
};
