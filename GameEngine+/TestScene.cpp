/////////////////////////////////
// TestScene.cpp - Implementation of the TestScene class, responsible for managing the game logic, entity updates, and rendering for a test scene in the game engine
/////////////////////////////////



/////////////////////////////////
// Include necessary headers for the TestScene implementation
#include <random>

#include "TestScene.h"
#include "GameEngine.h"
#include "EntityManager.h"

#include "CCircle.h"
#include "CShape.h"
#include "CExplosion.h"
#include "CSoundEffect.h"

#include "Entity.h"
#include "EntityType.h"
#include "GameController.h"

#include "InputAction.h"
#include "InputController.h"
#include "Vec2.h"

#include <SFML/Window/Event.hpp>
#include <SFML/System/Vector2.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
/////////////////////////////////



/////////////////////////////////
// Constructor - initializes the TestScene with references to the game engine, render window, and entity manager, and sets up ImGui for UI rendering
TestScene::TestScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager)
	: m_window(win), Scene(engine, entityManager) {
	// Initialize ImGui with SFML backend
	if (!ImGui::SFML::Init(engine.m_window)) {
		std::cerr << "Failed to initialize ImGui::SFML." << std::endl;
		std::exit(EXIT_FAILURE);
	}
}
/////////////////////////////////



/////////////////////////////////
// Destructor - cleans up ImGui resources when the TestScene is destroyed
TestScene::~TestScene() = default;
/////////////////////////////////



/////////////////////////////////
// Update - updates the game logic for the TestScene, including handling events, managing entity population, updating explosions, and performing physics and collision detection. 
// It also calculates and reports FPS using an exponential moving average for smoothing, and renders the ImGui game information window with current entity count, death count, and explosion count.
void TestScene::Update(float deltaTime) {
	// ImGui and event polling are handled centrally by GameEngine; use the deltaTime parameter supplied by the engine.
	static auto fpsLast = std::chrono::steady_clock::now();
	static int fpsFrames = 0;
	static double fpsSmooth = 0.0;
	static constexpr double alpha = 0.15;

	ReportFPS(fpsFrames, fpsLast, fpsSmooth, alpha);

	// Handle events (SFML 3.0: pollEvent returns std::optional<sf::Event>)
	while (auto eventOpt = m_gameEngine.m_window.pollEvent()) {
		ImGui::SFML::ProcessEvent(m_gameEngine.m_window, *eventOpt);

		if (eventOpt->is<sf::Event::Closed>()) {
			m_gameEngine.m_window.close(); // window X button - always close
		}
		// Escape is handled globally by GameEngine before scenes run, so do NOT forward it here
		if (!eventOpt->is<sf::Event::KeyPressed>() || [&]{
				auto kp = eventOpt->getIf<sf::Event::KeyPressed>();
				return !kp || static_cast<sf::Keyboard::Key>(kp->code) != sf::Keyboard::Key::Escape;
			}())
			HandleEvent(eventOpt);
	}

	// scene update logic
	// Dynamic population control: maintain entities by spawning to replace dead ones
	size_t currentEntityCount = m_entityManager.GetEntities().size();
	if (currentEntityCount < m_targetEntityCount) {
		// Spawn entities to maintain target population
		// Spawn up to 4 entities per frame to replace those killed in collisions
		int entitiesToSpawn = std::min(4, m_targetEntityCount - static_cast<int>(currentEntityCount));

		for (int i = 0; i < entitiesToSpawn; ++i) {

			// Determine spawn direction first, then select team based on side
			int direction = m_direction(m_generator);
			// direction==1 => spawned just off the left edge (will move rightward)
			unsigned int type = (direction == 1) ? 0u : 1u;

			// Randomized leftward movement, no vertical component
			float velocityX = std::uniform_real_distribution<float>(-1820.0f, -560.0f)(m_generator);
			float velocityY = 0.0f;
			float spawnX, spawnY;
			int r = m_redVal(m_generator);
			int g = m_greenVal(m_generator);
			int b = m_blueVal(m_generator);
			int a = m_alphaVal(m_generator);
			float radius = m_radiusDistro(m_generator);
			// direction already sampled above

			if (direction == 1) {	// Move rightward
				velocityX = velocityX * -1.0f; // Reverse velocity for rightward movement
				spawnX = std::uniform_real_distribution<float>(-100.0f, 0.0f)(m_generator); // Just off left edge
				spawnY = std::uniform_real_distribution<float>(0.0f, static_cast<float>(m_gameEngine.m_windowSize.y))(
					m_generator);
			} else {
				// Spawn off the right edge of screen, move left across screen with randomized leftward velocity
				spawnX = static_cast<float>(m_gameEngine.m_windowSize.x) +
						 std::uniform_real_distribution<float>(0.0f, 100.0f)(m_generator); // Just off right edge
				spawnY = std::uniform_real_distribution<float>(0.0f, static_cast<float>(m_gameEngine.m_windowSize.y))(
					m_generator);
			}

			SpawnEntityByType(type, radius, Vec3(r, g, b), Vec2(spawnX, spawnY), Vec2(velocityX, velocityY), a);
		}
	}

	// Update game logic (entities, collisions, rendering)
	UpdateExplosions();

	m_entityManager.GetPhysicsSystem().Update(
		m_entityManager.GetEntities(), deltaTime, m_window.getSize().x,
		m_window.getSize().y); // Do physics and boundary collisions first for spatial hash accuracy.

	m_entityManager.GetCollisionSystem().DetectAndResolve(
		m_entityManager.GetEntities(), m_entityManager.GetSpatialHash(),
		deltaTime); // Then do collision detection and resolution, which may mark entities as dead and spawn explosions.

	// Set listener position for 3D spatial audio (at center of screen)
	Vec2 listenerPos(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f);
	if (m_entityManager.GetSoundSystem()) {
		m_entityManager.GetSoundSystem()->SetListenerPosition(listenerPos);
	}

	// Render ImGui UI (actual ImGui::Render called by GameEngine)
	RenderGameInfoWindow(m_entityManager.GetEntities().size(), m_entityManager.GetDeathCountThisFrame(),
						 m_explosionCount);
}
/////////////////////////////////



/////////////////////////////////
// Render - responsible for rendering the scene, including all entities and any scene-specific visuals. The actual rendering of entities is handled by the EntityManager's RenderAll 
// method, which is called by the GameEngine after this method. This method can be used to render any additional scene-specific visuals or effects that are not part of the standard entity rendering process.
void TestScene::Render() { /* scene render logic */ }
/////////////////////////////////



/////////////////////////////////
// DoAction - performs any scene-specific actions or updates that are not covered by the standard update and render methods. This can include things like triggering events, managing timers, or handling specific game mechanics unique to this scene.
void TestScene::DoAction() { /*scene-specific action*/ }
/////////////////////////////////



/////////////////////////////////
// HandleEvent - processes input events for the scene, such as keyboard and mouse input. In this implementation, it checks if the Escape key is pressed and calls the ProcessEscapeKey method to 
// close the window if it is. This method can be expanded to handle additional input events as needed for the scene's functionality.
void TestScene::HandleEvent(const std::optional<sf::Event>& event) {
	bool escapeKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
	// handle input events
	ProcessEscapeKey(escapeKeyDown);
}
/////////////////////////////////



/////////////////////////////////
// OnEnter - called when the scene becomes active, allowing for any necessary setup or initialization that should occur each time the scene is entered. This can include resetting game state, starting timers, or preparing resources specific to this scene.
void TestScene::OnEnter() { /* called when scene becomes active */ }
/////////////////////////////////



/////////////////////////////////
// OnExit - called when the scene is no longer active, allowing for any necessary cleanup or state management that should occur each time the scene is exited. This can include stopping timers, saving state, or releasing resources specific to this scene.
void TestScene::OnExit() { /* cleanup when scene exits */ }
/////////////////////////////////



// LoadResources - responsible for loading any resources needed by the scene, such as textures, fonts, or sounds. In this implementation, it simply sets the m_isLoaded flag to true, but in a more complete implementation, it would include actual resource loading logic.
void TestScene::LoadResources() {
	m_isLoaded = true;
}
/////////////////////////////////



/////////////////////////////////
// UnloadResources - responsible for unloading any resources that were loaded for the scene, allowing for cleanup and freeing of memory when the scene is no longer needed. 
// In this implementation, it is a placeholder, but in a more complete implementation, it would include actual resource unloading logic.
void TestScene::UnloadResources() { /* unload resources */ }
/////////////////////////////////



/////////////////////////////////
// InitializeGame - responsible for initializing the game state for the scene, including spawning entities with random properties and setting up any necessary game logic or mechanics.
void TestScene::InitializeGame(sf::Vector2u windowSize) {
	// Initialize random number generator ONCE (not per entity)
	std::random_device randDevice;
	std::default_random_engine generator(randDevice());

	// Initialize random number generator once (not every frame)
	m_generator		=		std::default_random_engine(randDevice());		// Random entity colours, in the brighter colour range
	m_xVelocity		=		std::uniform_int_distribution<int>(-80, -40);	// Slow movement speed
	m_yVelocity		=		std::uniform_int_distribution<int>(-20, 20);	// Slow vertical speed

	m_xDistro		=		std::uniform_int_distribution<int>(	20,	m_gameEngine.m_windowSize.x - 20); // Spawn within screen bounds, leaving a 20-pixel margin on the left and a 5-pixel margin on the right to prevent immediate off-screen spawning
	m_yDistro		=		std::uniform_int_distribution<int>(	20,	m_gameEngine.m_windowSize.y - 20); // Spawn within screen bounds, leaving a 20-pixel margin on the top and a 5-pixel margin on the bottom to prevent immediate off-screen spawning

	m_redVal		=		std::uniform_int_distribution<int>(100, 255);			// Brighter reds
	m_greenVal		=		std::uniform_int_distribution<int>(100, 255);			// Brighter greens
	m_blueVal		=		std::uniform_int_distribution<int>(100, 255);			// Brighter blues
	m_alphaVal		=		std::uniform_int_distribution<int>(150, 255);			// More opaque
	m_radiusDistro	=		std::uniform_real_distribution<float>(3.5f, 6.0f);		// Slightly larger radius for better visibility
	m_entityType	=		std::uniform_int_distribution<int>(0, 4);				// 5 team types
	m_spawnZone		=		std::uniform_int_distribution<int>(0, 3);				// 4 quadrants
	m_direction		=		std::uniform_int_distribution<int>(0, 1);				// 2 movement directions: leftward or rightward

	// Spawn initial entities using targetEntityCount
	for (int i = 0; i < m_targetEntityCount; ++i) {
		// For bulk initialization, sample direction first then make left/right spawns use consistent teams
		int direction = m_direction(generator);
		unsigned int type = (direction == 1) ? 0u : 1u;

		float spawnX, spawnY;

		// Randomized leftward movement, no vertical component
		float velocityX = m_xVelocity(generator);
		float velocityY = 0.0f;

		int r = m_redVal(generator);
		int g = m_greenVal(generator);
		int b = m_blueVal(generator);
		int a = m_alphaVal(generator);

		float radius = m_radiusDistro(generator);

		if (direction == 1) {			   // Move rightward
			velocityX = velocityX * -1.0f; // Reverse velocity for rightward movement
			spawnX = std::uniform_real_distribution<float>(-100.0f, 0.0f)(m_generator); // Just off left edge
			spawnY = std::uniform_real_distribution<float>(0.0f, static_cast<float>(m_gameEngine.m_windowSize.y))(
				m_generator);
		} else {
			// Spawn off the right edge of screen, move left across screen with randomized leftward velocity
			spawnX = static_cast<float>(m_gameEngine.m_windowSize.x) +
					 std::uniform_real_distribution<float>(0.0f, 100.0f)(m_generator); // Just off right edge
			spawnY = std::uniform_real_distribution<float>(0.0f, static_cast<float>(m_gameEngine.m_windowSize.y))(
				m_generator);
		}

		SpawnEntityByType(type, radius, Vec3(r, g, b), Vec2(spawnX, spawnY), Vec2(velocityX, velocityY), a);
	}
}
/////////////////////////////////



/////////////////////////////////
// SpawnEntityByType - Spawns an entity of the specified team type with random properties and adds it to the EntityManager. It takes the EntityManager reference, team type (0-4), radius, color, position, 
// velocity, and alpha as parameters. The team type is mapped to a specific EntityType enum value, and the new entity is created and added to the EntityManager using the addEntity method.
void TestScene::SpawnEntityByType(unsigned int teamType, float radius, Vec3 color, Vec2 position, Vec2 velocity,
								  int alpha) {
	const EntityType teamTypes[] = {EntityType::TeamEagle, EntityType::TeamHawk, EntityType::TeamBoogaloo,
									EntityType::TeamRocket, EntityType::TeamMonkey};
	EntityType type = (teamType < 5) ? teamTypes[teamType] : EntityType::TeamMonkey;

	Entity* en = m_entityManager.AddEntity(type);

	en->AddComponent<CTransform>(position, velocity);
	en->AddComponent<CName>();

	// Use CTransform's public members (position / velocity)
	en->GetComponent<CTransform>()->m_position = Vec2(position.x + 0.4f, position.y - 0.5f);
	en->GetComponent<CTransform>()->m_velocity = Vec2(velocity.x, velocity.y);

	if (EntityType::Explosion == type) {
		auto explosion = std::make_unique<CExplosion>();
		explosion->SetRadius(radius);
		explosion->SetColor(color.x, color.y, color.z, alpha);
		en->AddComponentPtr<CShape>(std::move(explosion));
	} else {
		auto circle = std::make_unique<CCircle>();
		circle->SetRadius(radius);
		circle->SetColor(color.x, color.y, color.z, alpha);
		en->AddComponentPtr<CShape>(std::move(circle));
	}
}
/////////////////////////////////



/////////////////////////////////
// RenderGameInfoWindow - Renders the ImGui window displaying game information and performance metrics. It takes the current entity count, death count for the current frame, and active explosion count as parameters to display in the UI.
// The window is positioned at (10, 10) and sized to (450, 280) on first use, and it includes sections for entity statistics and spatial hash collision detection performance metrics. As I have move to a 'full screen' window with no borders or title bar,
// I have decided to add the fps to the ImGui window to help track performance over time.... lots of words, why am I writing this much in the comment, I should just write better code and make it self explanatory.....dumbass
void TestScene::RenderGameInfoWindow(size_t entityCount, int deathCount, int explosionCount) {
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
/////////////////////////////////



/////////////////////////////////
// ReportFPS - Report FPS by calculating the number of frames rendered in the last second and applying an exponential moving average to smooth out fluctuations. 
// It takes references to the frame count, last time point, smoothed FPS value, and a smoothing factor alpha as parameters.
void TestScene::ReportFPS(int& fpsFrames, std::chrono::steady_clock::time_point& fpsLast, double& fpsSmooth,
						  const double alpha) {
	++fpsFrames;
	auto fpsNow = std::chrono::steady_clock::now();
	auto fpsElapsed = std::chrono::duration_cast<std::chrono::duration<double>>(fpsNow - fpsLast);
	if (fpsElapsed.count() >= 1.0) {
		double currentFps = static_cast<double>(fpsFrames) / fpsElapsed.count();
		if (fpsSmooth <= 0.0) {
			fpsSmooth = currentFps;
		} else {
			fpsSmooth = (alpha * currentFps) + ((1.0 - alpha) * fpsSmooth);
		}

		fpsFrames = 0;
		fpsLast = fpsNow;
		m_fps = static_cast<float>(fpsSmooth);
	}
}
/////////////////////////////////



/////////////////////////////////
// UpdateExplosions - Updates the state of all active explosions; iterates through the tracked explosion entities, calculates their 
// age based on their creation time, updates their color alpha for fading effect, and removes them if they have exceeded their lifespan.
void TestScene::UpdateExplosions() {
	m_explosionCount = 0; // Reset explosion count and recalculate based on active explosions in the scene
	auto now = std::chrono::high_resolution_clock::now();
	std::vector<size_t> expiredExplosions;

	for (auto& entity :	m_entityManager.GetEntities()) { // Iterate over all entities to find explosions and update their state based on elapsed time since creation
		if (entity->GetType() == EntityType::Explosion) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - entity->m_creationTime);

			// Check if the sound effect is still playing
			CSoundEffect* soundEffect = entity->GetComponent<CSoundEffect>();
			bool soundStillPlaying = soundEffect && soundEffect->m_sound && soundEffect->m_state == CSoundEffect::State::Playing;

			// Only destroy the entity if both the visual lifespan is over AND the sound has finished
			if (elapsed.count() > 4500 && !soundStillPlaying) {
				entity->Destroy();
			} else {
				m_explosionCount++; // Increment explosion count for active explosions that have not yet expired
				float fadeProgress = static_cast<float>(elapsed.count()) / 4500.0f;
				// Use a higher base alpha so explosions remain more visible as they expand.
				const int maxAlpha = 220; // match CExplosion default alpha
				int newAlpha = static_cast<int>(maxAlpha * (1.0f - fadeProgress));

				auto shape = entity->GetComponent<CShape>();
				if (shape) {
					if (auto* explosion = dynamic_cast<CExplosion*>(shape)) {
						explosion->SetRadius(explosion->GetRadius() * 1.004f); // Expand the explosion radius over time
						// Origin is set in SetRadius to center the circle, so no position adjustment needed
						sf::Color currentColor = explosion->GetColor();
						explosion->SetColor(static_cast<float>(currentColor.r), static_cast<float>(currentColor.g),
											static_cast<float>(currentColor.b), newAlpha);
					}
				}
			}
		}
	}
}
/////////////////////////////////