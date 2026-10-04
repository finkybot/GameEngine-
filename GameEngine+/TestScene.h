/////////////////////////////////
// TestScene.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Scene.h"
#include <SFML/Window/Event.hpp>
#include <random>
#include "GPUInstanceStructs.h"
#include "RenderSystemGL.h"
#include "GPURenderSystem.h"
#include <SFML/Graphics.hpp>
#include "ShapeTemplateRegistry.h"
#include "CCamera.h"
#include "CameraSystem.h"
#include "MouseSystem.h"
#include "RaycastSystem.h"

#include "Systems/PhysicsSystem.h"
/////////////////////////////////



/////////////////////////////////
//	|	TestScene class - implements a test scene for the game engine, responsible for managing the game logic, entity updates, and rendering for a simple test scenario. The scene includes functionality for spawning entities with random properties, updating active explosions, and rendering game information using ImGui.
//	|_______________________________________________________________________
class TestScene : public Scene {
public:
	// Constructor and destructor for the TestScene class.  Must call base constructor with injected refs
	TestScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager);
	~TestScene() override;


	// Overridden virtual methods from the Scene base class. These methods handle updating the game logic, rendering the scene, processing input events, and managing the scene lifecycle (entering and exiting).
	void Update(float deltaTime) override;
	void Render() override;
	void DoAction() override;


	// Event handling and lifecycle methods for the TestScene class. These methods will handle processing input events (e.g., keyboard and mouse input), performing any necessary actions when the scene is entered or exited, and managing resources by loading and unloading them as needed. The HandleEvent method will process 
	// input events to allow for interactions such as closing the window when the escape key is pressed, while the OnEnter and OnExit methods can be used to set up or clean up scene-specific state.
	void HandleEvent(const std::optional<sf::Event>& event) override;
	void OnEnter() override;
	void OnExit() override;
	void OnWindowResized(sf::Vector2u newSize) override;
	void LoadResources() override;
	void UnloadResources() override;
	CCamera& GetActiveCamera() override { return *m_cameraEntity->GetComponent<CCamera>(); }


	// GetExplosionCount - returns the number of active explosions currently playing in the scene.
	int GetExplosionCount() const { return m_explosionCount; } 


	// InitialiseGame - responsible for initializing the game state for the scene, including spawning entities with random properties and setting up any necessary game logic or mechanics. This method will be called when the scene is entered to set up the initial state of the game.
	void InitialiseGame(sf::Vector2u windowSize);


	// ProcessEscapeKey - checks if the escape key is pressed and closes the window if it is. This method is called from the HandleEvent method to allow for exiting the game when the escape key is pressed.
	void ProcessEscapeKey(bool keyDown) const {
		if (keyDown) {
			// Return to main menu instead of closing the window directly
			m_gameEngine.ChangeScene("MainMenu");
		}
	}



private:
	// UpdateExplosions - Updates the state of all active explosions in the scene. This method iterates through the tracked explosion entities, calculates their age based on their creation time, updates their color alpha for a fading effect, and removes them if they have exceeded their lifespan.
	void UpdateExplosions();
	void InitialiseSpatialLayers();


	// SpawnEntityByType - Spawns an entity of the specified team type with random properties and adds it to the EntityManager. It takes the EntityManager reference, team type (0-4), radius, color, position, velocity, and alpha as parameters. The team type is mapped to a specific EntityType enum value, 
	// and the new entity is created and added to the EntityManager using the addEntity method.
	void SpawnEntityByType(unsigned int teamType, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha);


	// RenderGameInfoWindow - Renders the ImGui window displaying game information and performance metrics. It takes the current entity count, death count for the current frame, and active explosion count as parameters to display in the UI. The window is positioned at (10, 10) and sized to (450, 280) 
	// on first use, and it includes sections for entity statistics and spatial hash collision detection performance metrics.
	void RenderGameInfoWindow(size_t entityCount, int deathCount, int explosionCount);

	void UpdateEntity(Entity* entity, float deltaTime, float worldWidth, float worldHeight);
	void UpdateFPS(float deltaTime);
	void ProcessEvents();
	void UpdateSceneLogic(float deltaTime);
	void RenderUI();

	void UpdateSpawning(float deltaTime);
	void SpawnInitialPopulation();
	void SpawnReplacementEntities(int count);

	void SpawnExplosion(const Vec2& position, float radius);
	void ApplyMainCameraView(float deltaTime);
	void ApplyCameraMovement(float deltaTime);

	// Private member variables for the TestScene class. These include references to the GameEngine, EntityManager, and SFML window, as well as random distributions for entity properties and tracking variables for explosions and FPS

	int m_targetEntityCount = 3200;
	sf::RenderWindow& m_window;							// Reference to the SFML render window for rendering the scene
	int m_explosionCount = 0;							// Number of active explosions currently playing, used for tracking and displaying explosion count in the game info window.
	int m_deathCount =	0;								// Number of entities that have died, used for tracking and displaying death count in the game info window.
	

	std::vector<Entity*> m_explosions;					// scene‑local explosion registry (currently unused, but will be used for tracking active explosions in the scene at a later date)

	Entity* m_cameraEntity =	nullptr;				// Pointer to the camera entity in the scene, used for managing camera movement and view transformations.
	CameraSystem m_cameraSystem;						// Camera system for updating and managing camera entities.

	Vec2 m_mapMin = Vec2::Zero;							// Minimum bounds of the map for clamping camera movement and ensuring the camera stays within the defined area.
	Vec2 m_mapMax = Vec2::Zero;							// Maximum bounds of the map for clamping camera movement and ensuring the camera stays within the defined area.
	bool m_hasMapBounds = false;						// Flag indicating whether the map has defined bounds for camera clamping.

	float m_dt = 0.0f;									// Delta time for the current frame, used for time-based calculations and updates in the scene.
	float m_fps = 0.0f;									// Current frames per second (FPS).
	float m_fpsAccumulator = 0.0f;						// Accumulator for calculating FPS over time.
	float m_fpsSmooth =	0.0f;							// Smoothed FPS value using an exponential moving average to reduce fluctuations in the reported FPS.
	const double m_alpha = 0.15;						// smoothing factor


	float m_growthTimer = 0.0f;							// Timer for controlling the growth of the entity population over time.
	bool m_spatialCollisionEnabled = true;				// Flag indicating whether spatial collision detection is enabled.
	
	MouseSystem* m_mouseSystem = nullptr;				// Pointer to the mouse system for updating and managing mouse input.
	Entity* m_mouseEntity =	nullptr;					// Pointer to the mouse entity in the scene, used for tracking mouse position and interactions with other entities.
	RaycastSystem* m_raycastSystem = nullptr;			// Raycast system for performing raycasting operations in the scene, allowing for line-of-sight checks and interactions with entities based on mouse input or other criteria.



	// SpatialLayerRegistry member variable for managing spatial layers in the TechSimulationScene class. This registry allows for efficient management and retrieval of spatial layers based on their names, enabling the organization of entities into different layers for spatial queries and interactions.
	SpatialLayerRegistry m_spatialLayers;

	// ShapeTemplateRegistry member variable for managing shape templates associated with different entity types in the TechSimulationScene class. This registry allows for the registration and retrieval of shape templates based on entity types, enabling the creation of entities with specific shapes and visual representations.
	ShapeTemplateRegistry m_shapeRegistry;


	// Random distributions for entity properties
	std::default_random_engine m_rng;

	//std::random_device m_randDevice;						// Random distributions for entity properties (commented out for now, as it may not be necessary with the current random number generation logic)
	//std::default_random_engine m_generator;				// Random number generator for entity properties (commented out for now, as it may not be necessary with the current random number generation logic)

	std::uniform_int_distribution<int> m_xVelocity;			// x movement speed
	std::uniform_int_distribution<int> m_yVelocity;			// y movement speed

	//std::uniform_int_distribution<int> m_xDistro;			// Spawn x axis distribution across the entire screen width for more even distribution of entities, preventing clustering at the left or right edges (commented out for now, as it may not be necessary with the current spawn logic)
	//std::uniform_int_distribution<int> m_yDistro;			// Spawn y axis distribution across the entire screen height for more even distribution of entities, preventing clustering at the top or bottom edges (commented out for now, as it may not be necessary with the current spawn logic)

	std::uniform_int_distribution<int> m_redVal;			// reds
	std::uniform_int_distribution<int> m_greenVal;			// greens
	std::uniform_int_distribution<int> m_blueVal;			// blues
	std::uniform_int_distribution<int> m_alphaVal;			// alpha values for more visible entities
	std::uniform_real_distribution<float> m_radiusDistro;	// random radius between 1.5 and 2.0 for more visible entities
	std::uniform_int_distribution<int> m_entityType;		// entity type (0-4) for team assignment
	std::uniform_int_distribution<int> m_direction;			// direction (0-1) for left or right movement	
	
	//std::uniform_int_distribution<int> m_spawnZone;			// spawn zone (0-3) for more even distribution of entities across the screen, preventing clustering in one area (commented out for now, as it may not be necessary with the current spawn logic)
};
/////////////////////////////////