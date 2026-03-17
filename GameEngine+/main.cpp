// main.cpp - Entry point of the game engine, responsible for initializing the game, 
// creating the main window, handling the main game loop, and integrating ImGui for UI rendering. 
// It sets up the initial game state by spawning entities, processes user input, updates game logic, 
// and renders the scene each frame. The main loop also includes dynamic population control to maintain 
// a target number of entities on screen and displays performance metrics using ImGui.
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>

#include <SFML/Graphics.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <memory>
#include <optional>
#include <random>
#include <thread>

#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "ImageManagementSystem.h"
#include "SpatialHashGrid.h"
#include "Vec2.h"

// Spawns an entity of the specified team type with random properties and adds it to the EntityManager. It takes the EntityManager reference, team type (0-4), radius, color, position, velocity, and alpha as parameters. 
// The team type is mapped to a specific EntityType enum value, and the new entity is created and added to the EntityManager using the addEntity method.
static Entity* SpawnEntityByType(EntityManager& entityManager, unsigned int teamType, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha)
{
	const EntityType teamTypes[] = { EntityType::TeamEagle, EntityType::TeamHawk, EntityType::TeamBoogaloo, EntityType::TeamRocket, EntityType::TeamMonkey };
	EntityType type = (teamType < 5) ? teamTypes[teamType] : EntityType::TeamMonkey;
	
	return entityManager.addEntity(type, radius, color, position, velocity, alpha);
}

// Renders the ImGui window displaying game information and performance metrics. It takes the current entity count, death count for the current frame, and active explosion count as parameters to display in the UI. 
// The window is positioned at (10, 10) and sized to (450, 280) on first use, and it includes sections for entity statistics and spatial hash collision detection performance metrics.
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

// Game Initialization
static void InitializeGame(EntityManager& entityManager, sf::Vector2u windowSize, int maxEntities)
{
	// Initialize random number generator ONCE (not per entity)
	std::random_device randDevice;
	std::default_random_engine generator(randDevice());

	// Random entity colours, in the brighter colour range 
	std::uniform_int_distribution<int> redVal(100, 255);    
	std::uniform_int_distribution<int> greenVal(100, 255);    
	std::uniform_int_distribution<int> blueVal(100, 255);     
	std::uniform_int_distribution<int> alphaVal(150, 255);    
	std::uniform_real_distribution<float> radiusDistro(1.5f, 2.0f);
	std::uniform_int_distribution<int> entityType(0, 4);
	std::uniform_real_distribution<float> velocityDistro(-420.0f, -60.0f);  // Randomize leftward velocity

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

// Entry point of the game engine, responsible for initializing the game, creating the main window, handling the main game loop, and integrating ImGui for UI rendering. 
// It sets up the initial game state by spawning entities, processes user input, updates game logic, and renders the scene each frame. 
// The main loop also includes dynamic population control to maintain a target number of entities on screen and displays performance metrics using ImGui.
int main(int argc, char* argv[])
{
	// Setup the SFML window
	sf::Vector2u windowSize = sf::VideoMode::getDesktopMode().size;
	sf::RenderWindow window(sf::VideoMode({ windowSize }), "SFML Game Engine");
	
	//window.setFramerateLimit(240);
	window.setVerticalSyncEnabled(true);

	// Load tile map texture (Im working on a tile map rendering system 13/03/2026) 
	sf::Image image;
	std::filesystem::path imagePath = std::filesystem::absolute("assets\\adventure.png");
	std::cout << "Loading image from: " << imagePath << std::endl;
	IMS::LoadImage(imagePath.string(), image);
	std::vector<sf::Texture> textures;
	IMS::CreateTileMap(0, 0, 32, 32, image, textures);


	std::vector<sf::Sprite> sprites;

	for (size_t i = 0; i < textures.size(); ++i)
	{
		sprites.emplace_back(textures[i]);
	}

	// Position sprites in a grid pattern across the window, wrapping to new rows as needed, this is just for demonstration purposes.
	int x = 0;	int y = 0;
	for (size_t i = 0; i < sprites.size(); ++i)
	{
		if (i % 32 == 0) 
		{
			x = 0; // Reset x to start of row
			y = (y + 32) % windowSize.y; // Move down by 32 pixels and wrap around vertically
		}
		sprites[i].setPosition({ static_cast<float>(x), static_cast<float>(y) }); // Example positions, adjust as needed
		x += 32; // Move to the next column
	}
	
	// Flag to track if the spacebar is currently pressed, used to prevent multiple spawns per key pres, this shouldnt be here, I'll move it later
	bool isPressed = false;

	// Initialize ImGui with SFML backend
	if (!ImGui::SFML::Init(window))
	{
		std::cerr << "Failed to initialize ImGui::SFML." << std::endl;
		return EXIT_FAILURE;	
	}

	EntityManager entity_manager(window);

	// Initialize entities
	// Spawn logic will maintain target count
	const int initialEntityCount = 5000; // Start with 5,000 entities, spawn logic will maintain roughtly 4,000-5,000 entities on screen by spawning to replace those killed
	const int targetEntityCount = 5000;
	InitializeGame(entity_manager, windowSize, initialEntityCount);

	sf::Clock deltaClock;

	// Initialize random number generator once (not every frame)
	std::random_device randDevice;										// Random distributions for entity properties, adjusted for faster movement and brighter colors
	std::default_random_engine generator(randDevice());					// Random entity colours, in the brighter colour range
	std::uniform_int_distribution<int> xVelocity(-150, 150);			// Faster movement speed
	std::uniform_int_distribution<int> yVelocity(-150, 150);			// Faster movement speed
	std::uniform_int_distribution<int> xDistro(20, windowSize.x - 5);	// Spawn within screen bounds, leaving a 20-pixel margin on the left and a 5-pixel margin on the right to prevent immediate off-screen spawning
	std::uniform_int_distribution<int> yDistro(20, windowSize.y - 5);	// Spawn within screen bounds, leaving a 20-pixel margin on the top and a 5-pixel margin on the bottom to prevent immediate off-screen spawning
	std::uniform_int_distribution<int> redVal(100, 255);				// Brighter reds
	std::uniform_int_distribution<int> greenVal(100, 255);				// Brighter greens
	std::uniform_int_distribution<int> blueVal(100, 255);				// Brighter blues
	std::uniform_int_distribution<int> alphaVal(150, 255);				// More opaque
	std::uniform_real_distribution<float> radiusDistro(1.5f, 2.0f);		// Slightly larger radius for better visibility
	std::uniform_int_distribution<int> entityType(0, 4);				// 5 team types
	std::uniform_int_distribution<int> spawnZone(0, 3);					// 4 quadrants

	int spriteIndex = 0;	// Example index, adjust as needed

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
				float velocityX = std::uniform_real_distribution<float>(-420.0f, -60.0f)(generator);
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
		for (const auto& sprite : sprites)
		{ 
			window.draw(sprite);
		}

		window.display();

	}

	// Shutdown ImGui
	ImGui::SFML::Shutdown();

	return 0;
}
