#include <random>

#include "TestScene.h"
#include "GameEngine.h"
#include "EntityManager.h"
#include <SFML/Window/Event.hpp>
#include "Entity.h"
#include "Vec2.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>


TestScene::TestScene(GameEngine& engine, sf::RenderWindow& win) : Scene(engine)
{
	EntityManager* entityManager = new EntityManager(win); // Create a new EntityManager instance for this scene, passing the window reference for rendering purposes
	m_entityManager = entityManager;

	// Initialize ImGui with SFML backend
	if (!ImGui::SFML::Init(engine.m_window))
	{
		std::cerr << "Failed to initialize ImGui::SFML." << std::endl;
		std::exit(EXIT_FAILURE);
	}
}

TestScene::~TestScene() = default;

void TestScene::Update(float /*deltaTime*/)
{
	sf::Time frameTime = m_gameEngine.m_deltaClock.restart(); // Restart the clock to get the time elapsed since the last frame, which will be used for updating game logic and ensuring smooth movement and animations based on delta time
	float deltaTime = frameTime.asSeconds();
	ImGui::SFML::Update(m_gameEngine.m_window, frameTime); // Update ImGui with the current frame time, this should give us a responsive UI

	// Handle events
	while (const std::optional event = m_gameEngine.m_window.pollEvent())
	{
		ImGui::SFML::ProcessEvent(m_gameEngine.m_window, *event);

		if (event->is<sf::Event::Closed>())
		{
			m_gameEngine.m_window.close();
		}
	}

	// scene update logic
	// Dynamic population control: maintain entities by spawning to replace dead ones
	size_t currentEntityCount = m_entityManager->getEntities().size();
	if (currentEntityCount < targetEntityCount)
	{
		// Spawn entities to maintain target population
		// Spawn up to 4 entities per frame to replace those killed in collisions
		int entitiesToSpawn = std::min(4, targetEntityCount - static_cast<int>(currentEntityCount));

		for (int i = 0; i < entitiesToSpawn; ++i)
		{
			// Random team selection
			unsigned int type = m_entityType(m_generator);

			// Spawn off the right edge of screen, move left across screen with randomized leftward velocity
			float spawnX = static_cast<float>(m_gameEngine.m_windowSize.x) + std::uniform_real_distribution<float>(0.0f, 100.0f)(m_generator);  // Just off right edge
			float spawnY = std::uniform_real_distribution<float>(0.0f, static_cast<float>(m_gameEngine.m_windowSize.y))(m_generator);

			// Randomized leftward movement, no vertical component
			float velocityX = std::uniform_real_distribution<float>(-1820.0f, -560.0f)(m_generator);
			float velocityY = 0.0f;

			int r = m_redVal(m_generator);
			int g = m_greenVal(m_generator);
			int b = m_blueVal(m_generator);
			int a = m_alphaVal(m_generator);
			float radius = m_radiusDistro(m_generator);
			SpawnEntityByType(type, radius, Vec3(r, g, b), Vec2(spawnX, spawnY), Vec2(velocityX, velocityY), a);
		}
	}

	// Update game logic (entities, collisions, rendering)
	m_entityManager->Update(deltaTime);

	// Render ImGui UI
	RenderGameInfoWindow(m_entityManager->getEntities().size(), m_entityManager->GetDeathCountThisFrame(), m_entityManager->GetExplosionCount());
	// Render ImGui and display
	ImGui::SFML::Render(m_gameEngine.m_window);
}

void TestScene::Render()
{
	// scene render logic
}

void TestScene::DoAction()
{
	// scene-specific action
}

void TestScene::HandleEvent(const sf::Event& /*event*/)
{
	// handle input events
}

void TestScene::OnEnter()
{
	// called when scene becomes active
}

void TestScene::OnExit()
{
	// cleanup when scene exits
}

void TestScene::LoadResources()
{
	m_isLoaded = true;
}

void TestScene::UnloadResources()
{
	// unload resources
}

void TestScene::InitializeGame(sf::Vector2u windowSize)
{
	int maxEntities = 5000; // Target entity population to maintain in the scene (it won't reach this number as to many will be murdered in collisions, but still should be a lively scene)
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

		SpawnEntityByType(type, radius, Vec3(r, g, b), Vec2(spawnX, spawnY), Vec2(velocityX, velocityY), a);
	}

	// Initialize random number generator once (not every frame)
	m_generator = std::default_random_engine(randDevice());									// Random entity colours, in the brighter colour range
	m_xVelocity = std::uniform_int_distribution<int>(-150, 150);							// Faster movement speed
	m_yVelocity = std::uniform_int_distribution<int>(-150, 150);							// Faster movement speed
	m_xDistro = std::uniform_int_distribution<int>(20, m_gameEngine.m_windowSize.x - 5);	// Spawn within screen bounds, leaving a 20-pixel margin on the left and a 5-pixel margin on the right to prevent immediate off-screen spawning
	m_yDistro = std::uniform_int_distribution<int>(20, m_gameEngine.m_windowSize.y - 5);	// Spawn within screen bounds, leaving a 20-pixel margin on the top and a 5-pixel margin on the bottom to prevent immediate off-screen spawning
	m_redVal = std::uniform_int_distribution<int>(100, 255);								// Brighter reds
	m_greenVal = std::uniform_int_distribution<int>(100, 255);								// Brighter greens
	m_blueVal = std::uniform_int_distribution<int>(100, 255);								// Brighter blues
	m_alphaVal = std::uniform_int_distribution<int>(150, 255);								// More opaque
	m_radiusDistro = std::uniform_real_distribution<float>(1.5f, 2.0f);						// Slightly larger radius for better visibility
	m_entityType = std::uniform_int_distribution<int>(0, 4);								// 5 team types
	m_spawnZone = std::uniform_int_distribution<int>(0, 3);									// 4 quadrants
}

void TestScene::SpawnEntityByType(unsigned int teamType, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha)
{
	const EntityType teamTypes[] = { EntityType::TeamEagle, EntityType::TeamHawk, EntityType::TeamBoogaloo, EntityType::TeamRocket, EntityType::TeamMonkey };
	EntityType type = (teamType < 5) ? teamTypes[teamType] : EntityType::TeamMonkey;

	Entity* en = m_entityManager->addEntity(type, radius, color, position, velocity, alpha);
}

// Renders the ImGui window displaying game information and performance metrics. It takes the current entity count, death count for the current frame, and active explosion count as parameters to display in the UI. 
// The window is positioned at (10, 10) and sized to (450, 280) on first use, and it includes sections for entity statistics and spatial hash collision detection performance metrics.
void TestScene::RenderGameInfoWindow(size_t entityCount, int deathCount, int explosionCount)
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