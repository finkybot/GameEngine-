#include <random>

#include "TestScene.h"
#include "GameEngine.h"
#include "EntityManager.h"
#include <SFML/Window/Event.hpp>
#include "Entity.h"
#include "Vec2.h"

#include "CCircle.h"
#include "CExplosion.h"
#include "EntityType.h"
#include "CShape.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>


TestScene::TestScene(GameEngine& engine, sf::RenderWindow& win) : m_window(win), Scene(engine)
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
	static auto fpsLast = std::chrono::steady_clock::now();
	static int fpsFrames = 0;
	static double fpsSmooth = 0.0;
	static constexpr double alpha = 0.15;

	ReportFPS(fpsFrames, fpsLast, fpsSmooth, alpha);

	sf::Time frameTime = m_gameEngine.m_deltaClock.restart();	// Restart the clock to get the time elapsed since the last frame, which will be used for updating game logic and ensuring smooth movement and animations based on delta time
	float deltaTime = frameTime.asSeconds();					// Convert the frame time to seconds for use in game logic updates, allowing for time-based movement and animations that are independent of frame rate
	ImGui::SFML::Update(m_gameEngine.m_window, frameTime);		// Update ImGui with the current frame time, this should give us a responsive UI

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

			// Randomized leftward movement, no vertical component
			float velocityX = std::uniform_real_distribution<float>(-1820.0f, -560.0f)(m_generator);
			float velocityY = 0.0f;
			float spawnX, spawnY;
			int r = m_redVal(m_generator);
			int g = m_greenVal(m_generator);
			int b = m_blueVal(m_generator);
			int a = m_alphaVal(m_generator);
			float radius = m_radiusDistro(m_generator);
			int direction = m_direction(m_generator);

			if (direction == 1) // Move rightward
			{
				velocityX = velocityX * -1.0f; // Reverse velocity for rightward movement
				spawnX = std::uniform_real_distribution<float>(-100.0f, 0.0f)(m_generator);  // Just off left edge
				spawnY = std::uniform_real_distribution<float>(0.0f, static_cast<float>(m_gameEngine.m_windowSize.y))(m_generator);
			}
			else
			{
				// Spawn off the right edge of screen, move left across screen with randomized leftward velocity
				spawnX = static_cast<float>(m_gameEngine.m_windowSize.x) + std::uniform_real_distribution<float>(0.0f, 100.0f)(m_generator);  // Just off right edge
				spawnY = std::uniform_real_distribution<float>(0.0f, static_cast<float>(m_gameEngine.m_windowSize.y))(m_generator);
			}

			SpawnEntityByType(type, radius, Vec3(r, g, b), Vec2(spawnX, spawnY), Vec2(velocityX, velocityY), a);
		}
	}

	// Update game logic (entities, collisions, rendering)
	m_entityManager->Update(deltaTime);
	UpdateExplosions();
	
	m_entityManager->GetPhysicsSystem().Update(m_entityManager->getEntities(), deltaTime, m_window.getSize().x, m_window.getSize().y); // Do physics and boundary collisions first for spatial hash accuracy.
	m_entityManager->GetCollisionSystem().DetectAndResolve(m_entityManager->getEntities(), m_entityManager->GetSpatialHash(), deltaTime); // Then do collision detection and resolution, which may mark entities as dead and spawn explosions.

	// Render ImGui UI
	RenderGameInfoWindow(m_entityManager->getEntities().size(), m_entityManager->GetDeathCountThisFrame(), m_explosionCount);
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


	// Initialize random number generator once (not every frame)
	m_generator = std::default_random_engine(randDevice());									// Random entity colours, in the brighter colour range
	m_xVelocity = std::uniform_int_distribution<int>(-420, -60);							// Faster movement speed
	m_yVelocity = std::uniform_int_distribution<int>(-150, 150);							// Faster movement speed
	m_xDistro = std::uniform_int_distribution<int>(20, m_gameEngine.m_windowSize.x - 20);	// Spawn within screen bounds, leaving a 20-pixel margin on the left and a 5-pixel margin on the right to prevent immediate off-screen spawning
	m_yDistro = std::uniform_int_distribution<int>(20, m_gameEngine.m_windowSize.y - 20);	// Spawn within screen bounds, leaving a 20-pixel margin on the top and a 5-pixel margin on the bottom to prevent immediate off-screen spawning
	m_redVal = std::uniform_int_distribution<int>(100, 255);								// Brighter reds
	m_greenVal = std::uniform_int_distribution<int>(100, 255);								// Brighter greens
	m_blueVal = std::uniform_int_distribution<int>(100, 255);								// Brighter blues
	m_alphaVal = std::uniform_int_distribution<int>(150, 255);								// More opaque
	m_radiusDistro = std::uniform_real_distribution<float>(1.5f, 2.0f);						// Slightly larger radius for better visibility
	m_entityType = std::uniform_int_distribution<int>(0, 4);								// 5 team types
	m_spawnZone = std::uniform_int_distribution<int>(0, 3);									// 4 quadrants
	m_direction = std::uniform_int_distribution<int>(0, 1);									// 2 movement directions: leftward or rightward

	for (int i = 0; i < maxEntities; ++i)
	{
		unsigned int type = m_entityType(generator);

		float spawnX, spawnY;

		// Randomized leftward movement, no vertical component
		float velocityX = m_xVelocity(generator);
		float velocityY = 0.0f;

		int r = m_redVal(generator);
		int g = m_greenVal(generator);
		int b = m_blueVal(generator);
		int a = m_alphaVal(generator);
		float radius = m_radiusDistro(generator);
		int direction = m_direction(generator); 

		if (direction == 1) // Move rightward
		{
			velocityX = velocityX * -1.0f; // Reverse velocity for rightward movement
			spawnX = std::uniform_real_distribution<float>(-100.0f, 0.0f)(m_generator);  // Just off left edge
			spawnY = std::uniform_real_distribution<float>(0.0f, static_cast<float>(m_gameEngine.m_windowSize.y))(m_generator);
		}
		else
		{
			// Spawn off the right edge of screen, move left across screen with randomized leftward velocity
			spawnX = static_cast<float>(m_gameEngine.m_windowSize.x) + std::uniform_real_distribution<float>(0.0f, 100.0f)(m_generator);  // Just off right edge
			spawnY = std::uniform_real_distribution<float>(0.0f, static_cast<float>(m_gameEngine.m_windowSize.y))(m_generator);
		}

		SpawnEntityByType(type, radius, Vec3(r, g, b), Vec2(spawnX, spawnY), Vec2(velocityX, velocityY), a);
	}
}

void TestScene::SpawnEntityByType(unsigned int teamType, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha)
{
	const EntityType teamTypes[] = { EntityType::TeamEagle, EntityType::TeamHawk, EntityType::TeamBoogaloo, EntityType::TeamRocket, EntityType::TeamMonkey };
	EntityType type = (teamType < 5) ? teamTypes[teamType] : EntityType::TeamMonkey;

	Entity* en = m_entityManager->addEntity(type, radius, color, position, velocity, alpha);
}

// Renders the ImGui window displaying game information and performance metrics. It takes the current entity count, death count for the current frame, and active explosion count as parameters to display in the UI. 
// The window is positioned at (10, 10) and sized to (450, 280) on first use, and it includes sections for entity statistics and spatial hash collision detection performance metrics. As I have move to a 'full screen' window with no borders or title bar, 
// I have decided to add the fps to the ImGui window to help track performance over time.... lots of words, why am I writing this much in the comment, I should just write better code and make it self explanatory.....dumbass
void TestScene::RenderGameInfoWindow(size_t entityCount, int deathCount, int explosionCount)
{
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(450, 280), ImGuiCond_FirstUseEver);

	ImGui::Begin("Game Info & Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	// Entity Statistics
	ImGui::Text("Entity Count: %zu", entityCount);
	ImGui::Text("Deaths This Frame: %d", deathCount);
	ImGui::Text("Active Explosions: %d", explosionCount);
	ImGui::Text("FPS: %.1f", m_fps);
	ImGui::Separator();

	// QuadTree Performance Metrics
	ImGui::Text("Spatial Hash Collision Detection");
	ImGui::Spacing();

	ImGui::BulletText("Queries/Frame: %zu", SpatialHashGrid<Entity>::GetQueryCount());
	ImGui::BulletText("Total Objects Checked: %zu", SpatialHashGrid<Entity>::GetTotalObjectsQueried());
	ImGui::BulletText("Avg Objects/Query: %.2f", SpatialHashGrid<Entity>::GetAverageObjectsPerQuery());

	ImGui::End();
}

void TestScene::ReportFPS(int& fpsFrames, std::chrono::steady_clock::time_point& fpsLast, double& fpsSmooth, const double alpha)
{
	++fpsFrames;
	auto fpsNow = std::chrono::steady_clock::now();
	auto fpsElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(fpsNow - fpsLast);
	if (fpsElapsed.count() >= 1.0)
	{
		double currentFps = static_cast<double>(fpsFrames) / fpsElapsed.count();
		if (fpsSmooth <= 0.0)
		{
			fpsSmooth = currentFps;
		}
		else
		{
			fpsSmooth = (alpha * currentFps) + ((1.0 - alpha) * fpsSmooth);
		}

		fpsFrames = 0;
		fpsLast = fpsNow;
		m_fps = static_cast<float>(fpsSmooth);
	}
}

void TestScene::UpdateExplosions()
{
	auto now = std::chrono::high_resolution_clock::now();
	std::vector<size_t> expiredExplosions;

	for (auto& entity : m_entityManager->getEntities()) // Iterate over all entities to find explosions and update their state based on elapsed time since creation
	{
		if (entity->GetType() == EntityType::Explosion)
		{
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - entity->m_creationTime);
			if (elapsed.count() > 600)
			{
				entity->Destroy();
			}
			else
			{
				float fadeProgress = static_cast<float>(elapsed.count()) / 600.0f;
				int newAlpha = static_cast<int>(80 * (1.0f - fadeProgress));

				auto shape = entity->GetComponent<CShape>();
				if (shape)
				{
					if (auto* explosion = dynamic_cast<CExplosion*>(shape))
					{
						explosion->SetRadius(explosion->GetRadius() + 0.5f); // Expand the explosion radius over time
						Vec2 explosionPosition = explosion->GetPosition();
						explosion->SetPosition(explosionPosition.x + 0.4f, explosionPosition.y - 0.5f); // Keep the explosion centered as it expands, adding a little drift for visual interest
						sf::Color currentColor = explosion->GetColor();
						explosion->SetColor(
							static_cast<float>(currentColor.r),
							static_cast<float>(currentColor.g),
							static_cast<float>(currentColor.b),
							newAlpha
						);
					}
				}
			}
		}
	}
}