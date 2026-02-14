#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
#include <memory>
#include <optional>
#include <random>

#include "Vec2.h"
#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "SpatialHashGrid.h"

/// <summary>
/// Spawns an entity of the specified team type with given parameters.
/// </summary>
/// <param name="entityManager">Reference to the EntityManager</param>
/// <param name="teamType">Team type (0-4)</param>
/// <param name="radius">Entity radius</param>
/// <param name="color">RGB color</param>
/// <param name="position">Spawn position</param>
/// <param name="velocity">Initial velocity</param>
/// <param name="alpha">Alpha value (0-255)</param>
/// <returns>Pointer to the created entity</returns>
static Entity* SpawnEntityByType(EntityManager& entityManager, unsigned int teamType, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha)
{
	const EntityType teamTypes[] = { EntityType::TeamEagle, EntityType::TeamHawk, EntityType::TeamBoogaloo, EntityType::TeamRocket, EntityType::TeamMonkey };
	EntityType type = (teamType < 5) ? teamTypes[teamType] : EntityType::TeamMonkey;
	
	return entityManager.addEntity(type, radius, color, position, velocity, alpha);
}

/// <summary>
/// Renders the main game info ImGui window displaying entity count and controls.
/// </summary>
/// <param name="entityCount">Number of active entities in the scene</param>
/// <param name="deathCount">Number of entities killed this frame</param>
/// <param name="explosionCount">Number of active explosions</param>
static void RenderGameInfoWindow(size_t entityCount, int deathCount, int explosionCount)
{
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(450, 280), ImGuiCond_FirstUseEver);
	
	ImGui::Begin("Game Info & Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	
	// Entity Statistics
	ImGui::Text("Entity Count: %zu", entityCount);
	ImGui::Text("Deaths This Frame: %d", deathCount);
	ImGui::Text("Active Explosions: %d", explosionCount);
	ImGui::Separator();
	
	// QuadTree Performance Metrics
	ImGui::Text("Spatial Hash Collision Detection");
	ImGui::Spacing();

	ImGui::BulletText("Queries/Frame: %zu", SpatialHashGrid<Entity>::GetQueryCount());
	ImGui::BulletText("Total Objects Checked: %zu", SpatialHashGrid<Entity>::GetTotalObjectsQueried());
	ImGui::BulletText("Avg Objects/Query: %.2f", SpatialHashGrid<Entity>::GetAverageObjectsPerQuery());
	
	ImGui::End();
}

/// <summary>
/// Initializes the game by creating and populating entities.
/// </summary>
/// <param name="entityManager">Reference to the EntityManager</param>
/// <param name="windowSize">Size of the render window</param>
/// <param name="maxEntities">Maximum number of entities to create</param>
static void InitializeGame(EntityManager& entityManager, sf::Vector2u windowSize, int maxEntities)
{
	// Initialize random number generator ONCE (not per entity)
	std::random_device randDevice;
	std::default_random_engine generator(randDevice());

	std::uniform_int_distribution<int> redVal(100, 255);      // Brighter reds
	std::uniform_int_distribution<int> greenVal(100, 255);    // Brighter greens
	std::uniform_int_distribution<int> blueVal(100, 255);     // Brighter blues
	std::uniform_int_distribution<int> alphaVal(150, 255);    // More opaque
	std::uniform_real_distribution<float> radiusDistro(1.5f, 4.0f);
	std::uniform_int_distribution<int> entityType(0, 4);
	std::uniform_real_distribution<float> velocityDistro(-220.0f, -180.0f);  // Randomize leftward velocity

	for (int i = 0; i < maxEntities; ++i)
	{
		unsigned int type = entityType(generator);

		// Spawn anywhere on screen with randomized leftward velocity
		float spawnX = std::uniform_real_distribution<float>(0.0f, static_cast<float>(windowSize.x))(generator);
		float spawnY = std::uniform_real_distribution<float>(0.0f, static_cast<float>(windowSize.y))(generator);

		// Randomized leftward movement, no vertical component
		float velocityX = velocityDistro(generator);
		float velocityY = 0.0f;

		int r = redVal(generator);
		int g = greenVal(generator);
		int b = blueVal(generator);
		int a = alphaVal(generator);
		float radius = radiusDistro(generator);

		SpawnEntityByType(entityManager, type, radius, Vec3(r, g, b), Vec2(spawnX, spawnY), Vec2(velocityX, velocityY), a);
	}
}

int main(int argc, char* argv[])
{
	// Get desktop resolution
	sf::Vector2u windowSize = sf::VideoMode::getDesktopMode().size;
	// Create the main window
	sf::RenderWindow window(sf::VideoMode({ windowSize }), "SFML Game Engine");
	
	window.setFramerateLimit(240);
	//window.setVerticalSyncEnabled(true);

	bool isPressed = false;

	// Initialize ImGui with SFML backend
	if (!ImGui::SFML::Init(window))
	{
		std::cerr << "Failed to initialize ImGui::SFML." << std::endl;
		return EXIT_FAILURE;	
	}

	EntityManager entity_manager(window);

	// Initialize game with 3000+ entities
	// Spawn logic will maintain target count
	const int initialEntityCount = 3500;
	const int targetEntityCount = 3500;
	InitializeGame(entity_manager, windowSize, initialEntityCount);

	sf::Clock deltaClock;

	// Initialize random number generator once (not every frame)
	std::random_device randDevice;
	std::default_random_engine generator(randDevice());
	std::uniform_int_distribution<int> xVelocity(-150, 150);      // Faster movement speed
	std::uniform_int_distribution<int> yVelocity(-150, 150);      // Faster movement speed
	std::uniform_int_distribution<int> xDistro(20, windowSize.x - 5);
	std::uniform_int_distribution<int> yDistro(20, windowSize.y - 5);
	std::uniform_int_distribution<int> redVal(100, 255);      // Brighter reds
	std::uniform_int_distribution<int> greenVal(100, 255);    // Brighter greens
	std::uniform_int_distribution<int> blueVal(100, 255);     // Brighter blues
	std::uniform_int_distribution<int> alphaVal(150, 255);    // More opaque
	std::uniform_real_distribution<float> radiusDistro(1.5f, 4.0f);
	std::uniform_int_distribution<int> entityType(0, 4);  // 5 team types
	std::uniform_int_distribution<int> spawnZone(0, 3);   // 4 quadrants

	// Main Loop, game logic is handled in here once per frame 
	while (window.isOpen())
	{
		// Handle events
		while (const std::optional event = window.pollEvent())
		{
			ImGui::SFML::ProcessEvent(window, *event);

			if (event->is<sf::Event::Closed>())
			{
				window.close();
			}
		}

		// Clear the window at the start of each frame
		window.clear();

		// Update ImGui and calculate delta time for this frame
		sf::Time frameTime = deltaClock.restart();
		float deltaTime = frameTime.asSeconds();
		ImGui::SFML::Update(window, frameTime);

		// Dynamic population control: maintain ~10,000 entities by spawning to replace dead ones
		size_t currentEntityCount = entity_manager.getEntities().size();
		if (currentEntityCount < targetEntityCount)
		{
			// Spawn entities to maintain target population
			// Spawn up to 4 entities per frame to replace those killed in collisions
			int entitiesToSpawn = std::min(4, targetEntityCount - static_cast<int>(currentEntityCount));
			
		for (int i = 0; i < entitiesToSpawn; ++i)
		{
			// Random team selection
			unsigned int type = entityType(generator);

			// Spawn off the right edge of screen, move left across screen with randomized leftward velocity
			float spawnX = static_cast<float>(windowSize.x) + 50.0f;  // Just off right edge
			float spawnY = std::uniform_real_distribution<float>(0.0f, static_cast<float>(windowSize.y))(generator);

			// Randomized leftward movement, no vertical component
			float velocityX = std::uniform_real_distribution<float>(-220.0f, -180.0f)(generator);
			float velocityY = 0.0f;

			int r = redVal(generator);
			int g = greenVal(generator);
			int b = blueVal(generator);
			int a = alphaVal(generator);
			float radius = radiusDistro(generator);

			SpawnEntityByType(entity_manager, type, radius, Vec3(r, g, b), Vec2(spawnX, spawnY), Vec2(velocityX, velocityY), a);
		}
		}

		// Update game logic (entities, collisions, rendering)
		entity_manager.update(deltaTime);

		// Render ImGui UI
		RenderGameInfoWindow(entity_manager.getEntities().size(), entity_manager.GetDeathCountThisFrame(), entity_manager.GetExplosionCount());

		// Render ImGui and display
		ImGui::SFML::Render(window);
		window.display();
	}

	// Shutdown ImGui
	ImGui::SFML::Shutdown();

	return 0;
}
