/////////////////////////////////
// TestScene.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Scene.h"
#include <SFML/Window/Event.hpp>
#include <random>
#include "Systems/PhysicsSystem.h"
/////////////////////////////////



/////////////////////////////////
// TestScene class - implements a test scene for the game engine, responsible for managing the game logic, entity updates, and rendering for a simple test scenario. 
// The scene includes functionality for spawning entities with random properties, updating active explosions, and rendering game information using ImGui.
class TestScene : public Scene {
	/////////////////////////////////
	// Public interface for the TestScene class
public:
	/////////////////////////////////
	// Constructor and destructor for the TestScene class.  Must call base constructor with injected refs
	TestScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager);
	~TestScene() override;
	/////////////////////////////////



	/////////////////////////////////
	// Overridden virtual methods from the Scene base class. These methods handle updating the game logic, rendering the scene, processing input events, and managing the scene lifecycle (entering and exiting).
	void Update(float deltaTime) override;
	void Render() override;
	void DoAction() override;
	/////////////////////////////////



	/////////////////////////////////
	// Event handling and lifecycle methods for the TestScene class. These methods will handle processing input events (e.g., keyboard and mouse input), performing any necessary actions when the scene is entered or exited, and managing resources 
	// by loading and unloading them as needed. The HandleEvent method will process input events to allow for interactions such as closing the window when the escape key is pressed, while the OnEnter and OnExit methods can be used to set up or clean up scene-specific state.
	void HandleEvent(const std::optional<sf::Event>& event) override;
	void OnEnter() override;
	void OnExit() override;
	void LoadResources() override;
	void UnloadResources() override;
	/////////////////////////////////



	/////////////////////////////////
	// GetExplosionCount - returns the number of active explosions currently playing in the scene.
	int GetExplosionCount() const { return 0; } 
	/////////////////////////////////



	/////////////////////////////////
	// InitializeGame - responsible for initializing the game state for the scene, including spawning entities with random properties and setting up any necessary game logic or mechanics. This method will be called when the scene is entered to set up the initial state of the game.
	void InitializeGame(sf::Vector2u windowSize);
	/////////////////////////////////



	/////////////////////////////////
	// ProcessEscapeKey - checks if the escape key is pressed and closes the window if it is. This method is called from the HandleEvent method to allow for exiting the game when the escape key is pressed.
	void ProcessEscapeKey(bool keyDown) const {
		if (keyDown) {
			// Return to main menu instead of closing the window directly
			m_gameEngine.ChangeScene("MainMenu");
		}
	}
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods
private:
	/////////////////////////////////
	// UpdateExplosions - Updates the state of all active explosions in the scene. This method iterates through the tracked explosion entities, calculates their age based on their creation time, updates their color alpha for a fading effect, and removes them if they have exceeded their lifespan.
	void UpdateExplosions();
	/////////////////////////////////



	/////////////////////////////////
	// SpawnEntityByType - Spawns an entity of the specified team type with random properties and adds it to the EntityManager. It takes the EntityManager reference, team type (0-4), radius, color, position, velocity, and alpha as parameters. The team type is mapped to a specific EntityType enum value, 
	// and the new entity is created and added to the EntityManager using the addEntity method.
	void SpawnEntityByType(unsigned int teamType, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha);
	/////////////////////////////////



	/////////////////////////////////
	// RenderGameInfoWindow - Renders the ImGui window displaying game information and performance metrics. It takes the current entity count, death count for the current frame, and active explosion count as parameters to display in the UI. The window is positioned at (10, 10) and sized to (450, 280) 
	// on first use, and it includes sections for entity statistics and spatial hash collision detection performance metrics.
	void RenderGameInfoWindow(size_t entityCount, int deathCount, int explosionCount);
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the TestScene class. These include references to the GameEngine, EntityManager, and SFML window, as well as random distributions for entity properties and tracking variables for explosions and FPS.
	const int m_targetEntityCount = 500;
	sf::Window& m_window; // Reference to the SFML window for rendering and event handling
	int m_explosionCount = 0; // Number of active explosions currently playing, used for tracking and displaying explosion count in the game info window.
	float m_fps = 0.0f; // Current frames per second (FPS).
	/////////////////////////////////



	/////////////////////////////////
	// ReportFPS - Reports the current frames per second (FPS) by calculating the number of frames rendered in the last second and applying an exponential moving average to smooth out fluctuations. This method takes references to the frame count, 
	// last time point, and smoothed FPS value, as well as a smoothing factor alpha for the moving average calculation.
	void ReportFPS(int& fpsFrames, std::chrono::steady_clock::time_point& fpsLast, double& fpsSmooth, const double	alpha);
	/////////////////////////////////



	/////////////////////////////////
	// Random distributions for entity properties
	std::random_device m_randDevice;						// Random distributions for entity properties
	std::default_random_engine m_generator;					// Random number generator for entity properties
	std::uniform_int_distribution<int> m_xVelocity;			// x movement speed
	std::uniform_int_distribution<int> m_yVelocity;			// y movement speed
	std::uniform_int_distribution<int> m_xDistro;			// Spawn x axis distribution across the entire screen width for more even distribution of entities, preventing clustering at the left or right edges
	std::uniform_int_distribution<int> m_yDistro;			// Spawn y axis distribution across the entire screen height for more even distribution of entities, preventing clustering at the top or bottom edges
	std::uniform_int_distribution<int> m_redVal;			// reds
	std::uniform_int_distribution<int> m_greenVal;			// greens
	std::uniform_int_distribution<int> m_blueVal;			// blues
	std::uniform_int_distribution<int> m_alphaVal;			// alpha values for more visible entities
	std::uniform_real_distribution<float> m_radiusDistro;	// random radius between 1.5 and 2.0 for more visible entities
	std::uniform_int_distribution<int> m_entityType;		// entity type (0-4) for team assignment
	std::uniform_int_distribution<int> m_spawnZone;			// spawn zone (0-3) for more even distribution of entities across the screen, preventing clustering in one area
	std::uniform_int_distribution<int> m_direction;			// direction (0-1) for left or right movement
	/////////////////////////////////
};
/////////////////////////////////