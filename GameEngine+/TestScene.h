// ***** TestScene.h *****
#pragma once
#include "Scene.h"

#include <SFML/Window/Event.hpp>
#include <random>

#include "Systems/PhysicsSystem.h"

class TestScene : public Scene
{
public:
	// Must call base constructor with injected refs
	TestScene(GameEngine& engine, sf::RenderWindow& win);
	~TestScene() override;

	void Update(float deltaTime) override;
	void Render() override;
	void DoAction() override;

	// Scene interface
	void HandleEvent(const sf::Event& event) override;
	void OnEnter() override;
	void OnExit() override;
	void LoadResources() override;
	void UnloadResources() override;

	int GetExplosionCount() const { return 0; }																				// Returns the number of active explosions currently playing.

	// Game Initialization
	void InitializeGame(sf::Vector2u windowSize);

private:
	void UpdateExplosions();																								// Updates the state of all active explosions; iterates through the tracked explosion entities, calculates their age based on their creation time, updates their color alpha for fading effect, and removes them if they have exceeded their lifespan; dont like this being here.
	void SpawnEntityByType(unsigned int teamType, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha); 		// Spawns an entity of the specified team type with random properties and adds it to the EntityManager. It takes the EntityManager reference, team type (0-4), radius, color, position, velocity, and alpha as parameters. The team type is mapped to a specific EntityType enum value, and the new entity is created and added to the EntityManager using the addEntity method.
	void RenderGameInfoWindow(size_t entityCount, int deathCount, int explosionCount);										// Renders the ImGui window displaying game information and performance metrics. It takes the current entity count, death count for the current frame, and active explosion count as parameters to display in the UI. The window is positioned at (10, 10) and sized to (450, 280) on first use, and it includes sections for entity statistics and spatial hash collision detection performance metrics.
	const int targetEntityCount = 5000;
	sf::Window& m_window;																									// Reference to the SFML window for rendering and event handling
	
	int m_explosionCount = 0;	// Number of active explosions currently playing, used for tracking and displaying explosion count in the game info window.

	float m_fps = 0.0f;	// Current frames per second (FPS).
	void ReportFPS(int& fpsFrames, std::chrono::steady_clock::time_point& fpsLast, double& fpsSmooth, const double alpha);	// Report FPS by calculating the number of frames rendered in the last second and applying an exponential moving average to smooth out fluctuations.
	
	// Random distributions for entity properties
	std::random_device m_randDevice;						// Random distributions for entity properties
	std::default_random_engine m_generator;					// Random entity colours
	std::uniform_int_distribution<int> m_xVelocity;			// x movement speed
	std::uniform_int_distribution<int> m_yVelocity;			// y movement speed
	std::uniform_int_distribution<int> m_xDistro;			// Spawn x axis distribution across the entire screen width for more even distribution of entities, preventing clustering at the left or right edges
	std::uniform_int_distribution<int> m_yDistro;			// Spawn y axis distroribution across the entire screen height for more even distribution of entities, preventing clustering at the top or bottom edges
	std::uniform_int_distribution<int> m_redVal;			// reds
	std::uniform_int_distribution<int> m_greenVal;			// greens
	std::uniform_int_distribution<int> m_blueVal;			// blues
	std::uniform_int_distribution<int> m_alphaVal;			// alpha values for more visible entities
	std::uniform_real_distribution<float> m_radiusDistro;	// random radius between 1.5 and 2.0 for more visible entities
	std::uniform_int_distribution<int> m_entityType;		// entity type (0-4) for team assignment
	std::uniform_int_distribution<int> m_spawnZone;			// spawn zone (0-3) for more even distribution of entities across the screen, preventing clustering in one area
	std::uniform_int_distribution<int> m_direction;				// direction (0-1) for left or right movement,
};	