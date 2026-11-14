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
#include "IDTypes.h"
#include "EntityType.h"
#include "EntityManager.h"
#include "WorldDiffusionConfig.h"
#include "TechRegistry.h"


class ChunkManager;
class GameEngine;
class InputController;
class CursorSystem;
class TextureManager;
class FontManager;
class FPSCounter;
class FileManager;
class RenderQueue;

class Entity;
class Scene;
/////////////////////////////////



/////////////////////////////////
using ScenePtr = std::shared_ptr<Scene>;
using SceneMap = std::unordered_map<std::string, ScenePtr>;
using EntityVector = std::vector<std::unique_ptr<Entity>>;
/////////////////////////////////



/////////////////////////////////
//	|	World class - Represents the game world, including entities, tile maps, and other game state. This class is currently a placeholder and can be expanded in the future to include more functionality as needed.
//	|_______________________________________________________________________
class World {
public:

	// Constructor and destructor for the World class. The constructor takes references to the EntityManager and ChunkManager, which are used to manage entities and tile-based chunks in the game world.
	World(GameEngine& gameEngine, ChunkManager& chunkManager, sf::RenderWindow& window);
	~World() = default;

	EntityManager& GetEntityManager();
	const EntityManager& GetEntityManager() const;
	GameEngine& GetGameEngine();
	sf::RenderWindow& GetWindow() { return *m_window; }
	sf::Vector2u GetWindowSize() const { return m_windowSize; }
	InputController& GetInputController();
	CursorSystem& GetCursorSystem();
	TextureManager& GetTextureManager();
	FontManager& GetFontManager();
	FPSCounter& GetFPSCounter();
	FileManager& GetFileManager();
	RenderQueue& GetRenderQueue();
	TechRegistry& GetTechRegistry();
	WorldDiffusionConfig& GetWorldDiffusionConfig();

	void Update(float deltaTime); // Update the world state, including entities and tile maps, based on the elapsed time since the last update.
	void SetWindowSize(sf::Vector2u windowSize) { m_windowSize = windowSize; }
	void SetRenderWindow(sf::RenderWindow& window) { m_window = &window; }

	ChunkManager& GetChunkManager() { return m_chunkManager; } // Accessor for the ChunkManager, allowing other parts of the code to interact with tile-based chunks in the world.

	void AddScene(const std::string& sceneName, ScenePtr scene); // Add a new scene to the world, allowing for scene management and transitions.
	void RemoveScene(const std::string&	sceneName); // Remove a scene from the world by its name, allowing for scene management and cleanup.
	void ChangeScene(const std::string&	sceneName); // Change the current active scene to the specified scene by its name, allowing for scene transitions and updates.
	bool HasScene(const std::string& sceneName)	const; // Check if a scene with the specified name exists in the world, allowing for scene management and validation.

	Scene* GetActiveScene() const {	return m_activeScene.get(); } // Accessor for the current active scene, allowing other parts of the code to interact with the active scene in the world.

	std::vector<std::string> GetSceneNames() const; // Get a list of all scene names in the world, allowing for scene management and UI display.

	Entity* CreateEntity(EntityType type);
	Entity* GetEntity(EntityID id);
	void DestroyEntity(EntityID id);
	EntityVector& GetEntities();
	std::vector<Entity*>& GetEntities(EntityType type);
	void AddEntityToBucket(BucketID bucket, EntityID id);
	EntityBucket* GetEntityBucket(BucketID bucketId);
	void ClearAllEntities(); // Clear all entities from the world, allowing for scene resets or cleanup.

private:
	ChunkManager& m_chunkManager;
	GameEngine* m_gameEngine;
	std::unique_ptr<EntityManager> m_entityManager;


	SceneMap m_scenes; // Map of scene names to scene instances, allowing for easy management and switching between different scenes in the game world.
	ScenePtr m_activeScene; // Pointer to the current active scene, used to determine which scene to update and render during the game loop.
	sf::Vector2u m_windowSize{};
	sf::RenderWindow* m_window = nullptr;
};
