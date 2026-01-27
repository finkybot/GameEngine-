#include <SFML/Graphics.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
#include <iostream>
#include <memory>
#include <optional>
#include <random>

#include "Vec2.h"
#include "Entity.h"
#include "EntityManager.h"
#include "QuadTree.h"

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
	ImGui::Text("QuadTree Collision Detection");
	ImGui::Spacing();
	
	ImGui::BulletText("Queries/Frame: %zu", QuadTree<Entity>::GetQueryCount());
	ImGui::BulletText("Total Objects Checked: %zu", QuadTree<Entity>::GetTotalObjectsQueried());
	ImGui::BulletText("Avg Objects/Query: %.2f", QuadTree<Entity>::GetAverageObjectsPerQuery());
	ImGui::BulletText("Nodes Visited: %zu", QuadTree<Entity>::GetTotalNodesVisited());
	
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

	std::uniform_int_distribution<int> xVelocity(-150, 150);      // Faster movement speed
	std::uniform_int_distribution<int> yVelocity(-150, 150);      // Faster movement speed
	std::uniform_int_distribution<int> xDistro(20, windowSize.x - 5);
	std::uniform_int_distribution<int> yDistro(20, windowSize.y - 5);
	std::uniform_int_distribution<int> redVal(100, 255);      // Brighter reds
	std::uniform_int_distribution<int> greenVal(100, 255);    // Brighter greens
	std::uniform_int_distribution<int> blueVal(100, 255);     // Brighter blues
	std::uniform_int_distribution<int> alphaVal(150, 255);    // More opaque
	std::uniform_real_distribution<float> radiusDistro(1.0f, 3.0f);
	std::uniform_int_distribution<int> entityType(0, 4);

	for (int i = 0; i < maxEntities; ++i)
	{
		unsigned int type = entityType(generator);
		int vX = xVelocity(generator);
		int vY = yVelocity(generator);

		if (vX == 0 && vY == 0)
		{
			// Ensure some movement
			vX = 1;
		}

		int x = xDistro(generator);
		int y = yDistro(generator);

		int r = redVal(generator);
		int g = greenVal(generator);
		int b = blueVal(generator);
		int a = alphaVal(generator);

		float radius = radiusDistro(generator);

		if (type == 0)
		{
			entityManager.addEntity("TeamEagle", radius, Vec3(r, g, b), Vec2(x, y), Vec2(vX, vY), a);
		}
		else if (type == 1)
		{
			entityManager.addEntity("TeamHawk", radius, Vec3(r, g, b), Vec2(x, y), Vec2(vX, vY), a);
		}
		else if (type == 2)
		{
			entityManager.addEntity("TeamBoogaloo", radius, Vec3(r, g, b), Vec2(x, y), Vec2(vX, vY), a);
		}
		else if (type == 3)
		{
			entityManager.addEntity("TeamRocket", radius, Vec3(r, g, b), Vec2(x, y), Vec2(vX, vY), a);
		}
		else
		{
			entityManager.addEntity("TeamMonkey", radius, Vec3(r, g, b), Vec2(x, y), Vec2(vX, vY), a);
		}
	}
}

int main(int argc, char* argv[])
{
	// Get desktop resolution
	sf::Vector2u windowSize = sf::VideoMode::getDesktopMode().size;
	// Create the main window
	sf::RenderWindow window(sf::VideoMode({ windowSize }), "SFML Game Engine");
	
	window.setFramerateLimit(240);
	window.setVerticalSyncEnabled(true);

	bool isPressed = false;

	// Initialize ImGui with SFML backend
	if (!ImGui::SFML::Init(window))
	{
		std::cerr << "Failed to initialize ImGui::SFML." << std::endl;
		return EXIT_FAILURE;	
	}

	EntityManager entity_manager(window);

	// Initialize game with 2,000 entities across 5 teams
	// Spawn logic will gradually add Eagles up to target 10,000
	const int initialEntityCount = 2000;
	const int targetEntityCount = 10000;
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
	std::uniform_real_distribution<float> radiusDistro(1.0f, 3.0f);
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
				std::cout << "Closing Application" << std::endl;
				window.close();
			}
		}

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
				// Random team selection (not just Eagles)
				unsigned int type = entityType(generator);
				
				// Safe spawn location: bias toward screen edges to avoid immediate collisions
				int zone = spawnZone(generator);
				
				int x, y;
				int margin = 200;
				int centerX = windowSize.x / 2;
				int centerY = windowSize.y / 2;
				int halfWidth = (windowSize.x / 2) - margin;
				int halfHeight = (windowSize.y / 2) - margin;
				
				
				// Spawn along one of 4 edges for visual "entering screen" effect
				switch (zone)
				{
					case 0:  // Top edge
						x = std::uniform_int_distribution<int>(50, windowSize.x - 50)(generator);
						y = 50;
						break;
					case 1:  // Bottom edge
						x = std::uniform_int_distribution<int>(50, windowSize.x - 50)(generator);
						y = windowSize.y - 50;
						break;
					case 2:  // Left edge
						x = 50;
						y = std::uniform_int_distribution<int>(50, windowSize.y - 50)(generator);
						break;
					case 3:  // Right edge
					default:
						x = windowSize.x - 50;
						y = std::uniform_int_distribution<int>(50, windowSize.y - 50)(generator);
						break;
				}
				
				int vX = xVelocity(generator);
				int vY = yVelocity(generator);
				int r = redVal(generator);
				int g = greenVal(generator);
				int b = blueVal(generator);
				int a = alphaVal(generator);
				float radius = radiusDistro(generator);

				// Spawn with random team
				if (type == 0)
				{
					entity_manager.addEntity("TeamEagle", radius, Vec3(r, g, b), Vec2(x, y), Vec2(vX, vY), a);
				}
				else if (type == 1)
				{
					entity_manager.addEntity("TeamHawk", radius, Vec3(r, g, b), Vec2(x, y), Vec2(vX, vY), a);
				}
				else if (type == 2)
				{
					entity_manager.addEntity("TeamBoogaloo", radius, Vec3(r, g, b), Vec2(x, y), Vec2(vX, vY), a);
				}
				else if (type == 3)
				{
					entity_manager.addEntity("TeamRocket", radius, Vec3(r, g, b), Vec2(x, y), Vec2(vX, vY), a);
				}
				else
				{
					entity_manager.addEntity("TeamMonkey", radius, Vec3(r, g, b), Vec2(x, y), Vec2(vX, vY), a);
				}
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
