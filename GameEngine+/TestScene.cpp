/////////////////////////////////
// TestScene.cpp - Implementation of the TestScene class, responsible for managing the game logic, entity updates, and rendering for a test scene in the game engine
/////////////////////////////////



/////////////////////////////////
// Include necessary headers for the TestScene implementation
#include <random>
#include <chrono>
#include <cmath>
#include <iostream>
#include <cstdlib>

#include "TestScene.h"
#include "GameEngine.h"
#include "EntityManager.h"

#include "CExplosion.h"
#include "CSoundEffect.h"

#include "Entity.h"
#include "EntityType.h"
#include "GameController.h"

#include "InputAction.h"
#include "InputController.h"
#include "Vec2.h"
#include "SpatialLayerRegistry.h"
#include "SpatialHashGrid.h"
#include "CRenderInstance.h"

#include "CCamera.h"
#include "CTransform.h"
#include "CameraSystem.h"


#include <SFML/Window/Event.hpp>
#include <SFML/System/Vector2.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
using Clock = std::chrono::high_resolution_clock;

/////////////////////////////////



/////////////////////////////////
// Constructor - initializes the TestScene with references to the game engine, render window, and entity manager, and sets up ImGui for UI rendering
TestScene::TestScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager)
	: m_window(win), Scene(engine, entityManager) {
	// Initialize ImGui with SFML backend
	if (!ImGui::SFML::Init(engine.window)) {
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
// Update - updates the game logic for the TestScene, including handling events, managing entity population, updating explosions, and performing physics and collision detection. It also calculates and reports FPS using an exponential moving average for smoothing, and renders the ImGui game information 
// window with current entity count, death count, and explosion count.
void TestScene::Update(float dt) {
	m_dt = dt;
	UpdateFPS(dt);

	// --- Scene growth + spawning (bounded) ---
	m_growthTimer += dt;
	if (m_growthTimer >= 5.0f) {
		m_targetEntityCount += 20;
		m_growthTimer = 0.0f;
	}

	UpdateSpawning(dt);

	auto& entities = m_entityManager.GetEntities();


	// --- Main unified per‑entity pass ---
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	CCamera* cam = camOpt ? *camOpt : nullptr;

	float worldW = cam ? cam->worldWidth : static_cast<float>(m_window.getSize().x);
	float worldH = cam ? cam->worldHeight : static_cast<float>(m_window.getSize().y);

	for (auto& uptr : entities) {
		Entity* e = uptr.get();
		if (!e->IsAlive())
			continue;
		
		UpdateEntity(e, dt, worldW, worldH);
	}

	UpdateExplosions();

	// --- Incremental spatial index update (main thread only) ---
	if (auto* spatialIndex = m_entityManager.GetSpatialIndex()) {
		for (auto& uptr : entities) {
			Entity* e = uptr.get();
			if (!e->IsAlive())
				continue;
			spatialIndex->UpdateEntity(e);
		}
	}


	// --- Broad‑phase collision (optional / throttled) ---
	if (m_spatialCollisionEnabled && m_entityManager.GetSpatialIndex()) {
		m_entityManager.GetCollisionSystem().DetectAndResolveSpatial(entities, m_entityManager.GetSpatialIndex(), dt);
	}


	// --- Audio listener (camera only) ---
	if (cam && m_entityManager.GetSoundSystem()) {
		Vec2 listenerPos(cam->position.x, cam->position.y);
		m_entityManager.GetSoundSystem()->SetListenerPosition(listenerPos);
	}


	// --- Commit pending entities ---
	m_entityManager.ProcessPending();
	// --- Camera movement + view ---
	if (m_cameraEntity) {
		m_cameraSystem.Update(dt, GetEntityManager());
		ApplyCameraMovement(dt);
		ApplyMainCameraView(dt);
	}

	// --- UI (ImGui) ---
	RenderUI();
}
/////////////////////////////////



/////////////////////////////////
// Render - responsible for rendering the scene, including all entities and any scene-specific visuals. The actual rendering of entities is handled by the EntityManager's RenderAll 
// method, which is called by the GameEngine after this method. This method can be used to render any additional scene-specific visuals or effects that are not part of the standard entity rendering process.
void TestScene::Render() {}
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

	if (!event) return;

	// Retrieve main camera (if any)
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	CCamera* cam = camOpt ? *camOpt : nullptr;
	if (!cam) return;

	const sf::Event& evt = *event;

	// --- Mouse wheel zoom ---
	if (evt.is<sf::Event::MouseWheelScrolled>()) {
		auto* mw = evt.getIf<sf::Event::MouseWheelScrolled>();
		cam->zoom -= mw->delta * 0.1f;
		cam->zoom = std::clamp(cam->zoom, 0.25f, 4.0f);
	}

	// --- Begin panning (right mouse button) ---
	if (evt.is<sf::Event::MouseButtonPressed>()) {
		auto* mb = evt.getIf<sf::Event::MouseButtonPressed>();
		if (mb->button == sf::Mouse::Button::Right) {
			m_cameraSystem.BeginPan(*cam, sf::Mouse::getPosition(m_window));
		}
	}

	// --- Update panning ---
	if (evt.is<sf::Event::MouseMoved>()) {
		if (m_cameraSystem.IsPanning(*cam)) {
			m_cameraSystem.UpdatePan(*cam, sf::Mouse::getPosition(m_window));
		}
	}

	// --- End panning ---
	if (evt.is<sf::Event::MouseButtonReleased>()) {
		auto* mb = evt.getIf<sf::Event::MouseButtonReleased>();
		if (mb->button == sf::Mouse::Button::Right) {
			m_cameraSystem.EndPan(*cam);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// OnEnter - called when the scene becomes active, allowing for any necessary setup or initialization that should occur each time the scene is entered. This can include resetting game state, starting timers, or preparing resources specific to this scene.
void TestScene::OnEnter() {
	m_entityManager.SetSFMLRenderingEnabled(false); // Disable SFML rendering for this scene
	m_entityManager.SetGLRenderingEnabled(true);	// Enable OpenGL rendering for this scene
	m_entityManager.ClearAll();

	// Expand world bounds (example: double window size)
	m_mapMin = Vec2(0, 0);
	m_mapMax = Vec2((float)m_window.getSize().x * 2.0f, (float)m_window.getSize().y * 3.0f);
	m_hasMapBounds = true;


	// Create  and setup a camera entity, similar to the LevelEditorScene
	m_cameraEntity = GetEntityManager().AddEntity(EntityType::Camera);
	//m_cameraEntity->AddComponent<CTransform>(Vec2(0, 0), Vec2::Zero);

	Vec2 initialCamPos((m_mapMin.x + m_mapMax.x) * 0.5f, (m_mapMin.y + m_mapMax.y) * 0.5f);
	auto camera = m_cameraEntity->AddComponent<CCamera>(initialCamPos, 1.0f);
	camera->isMainCamera = true;
	camera->isActive = true;
	camera->viewportWidth = (float)m_window.getSize().x;
	camera->viewportHeight = (float)m_window.getSize().y;
	camera->smoothness = 0.0f; // Disable smoothing - camera is controlled directly via panning and bounds clamping
	camera->worldWidth = m_mapMax.x - m_mapMin.x;  // your actual world width
	camera->worldHeight = m_mapMax.y - m_mapMin.y;	   // your actual world height

	SpawnInitialPopulation();
	m_entityManager.ProcessPending();
	auto& entities = m_entityManager.GetEntities();

	for (auto& u : entities) {
		Entity* e = u.get();
		e->currentCellX = INT_MIN;
		e->currentCellY = INT_MIN;
	}
	std::cout << "Initial build of spatial index with " << entities.size() << " entities.\n";
	m_entityManager.GetSpatialIndex()->Build(m_entityManager.GetDynamicEntities());

	std::cout << "Task completed for " << m_entityManager.GetDynamicEntities().size() << " entities.\n";
	
	m_entityManager.GetBVH().ReportBVHSize();
}
/////////////////////////////////



/////////////////////////////////
// OnExit - called when the scene is no longer active, allowing for any necessary cleanup or state management that should occur each time the scene is exited. This can include stopping timers, saving state, or releasing resources specific to this scene.
void TestScene::OnExit() { /* cleanup when scene exits */ }
/////////////////////////////////



/////////////////////////////////
// OnWindowResized - called when the window is resized, allowing for any necessary adjustments to the scene's view or layout based on the new window size. 
// In this implementation, it updates the view to center on the new window size and adjusts the view's size accordingly.
void TestScene::OnWindowResized(sf::Vector2u newSize) {
	sf::View view;
	view.setCenter(sf::Vector2f(newSize.x * 0.5f, newSize.y * 0.5f));
	view.setSize(sf::Vector2f(newSize.x, newSize.y));
	m_window.setView(view);
}
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
void TestScene::InitialiseGame(sf::Vector2u windowSize) {
	InitialiseSpatialLayers();

	// Initialize RNG once
	m_rng.seed(std::random_device{}());

	// Initialize distributions (created once, reused forever)
	m_xVelocity = std::uniform_int_distribution<int>(-8, -4);
	m_yVelocity = std::uniform_int_distribution<int>(-20, 20);

	//m_xDistro = std::uniform_int_distribution<int>(20, m_gameEngine.windowSize.x - 20); // m_xDistro is no longer used, so this line is commented out (for removal later)
	//m_yDistro = std::uniform_int_distribution<int>(20, m_gameEngine.windowSize.y - 20); // m_yDistro is no longer used, so this line is commented out (for removal later)

	m_redVal = std::uniform_int_distribution<int>(100, 255);
	m_greenVal = std::uniform_int_distribution<int>(100, 255);
	m_blueVal = std::uniform_int_distribution<int>(100, 255);
	m_alphaVal = std::uniform_int_distribution<int>(150, 255);

	m_radiusDistro = std::uniform_real_distribution<float>(2.0f, 4.0f);

	m_entityType = std::uniform_int_distribution<int>(0, 1);
	m_direction = std::uniform_int_distribution<int>(0, 1);

	//m_spawnZone = std::uniform_int_distribution<int>(0, 1); // m_spawnZone is no longer used, so this line is commented out (for removal later)

	// Register shape templates here
	m_shapeRegistry.RegisterShapeTemplate(EntityType::TeamEagle, 3.0f, Vec3(255, 200, 200));
	m_shapeRegistry.RegisterShapeTemplate(EntityType::TeamHawk, 3.0f, Vec3(200, 255, 200));
	m_shapeRegistry.RegisterShapeTemplate(EntityType::TeamBoogaloo, 3.0f, Vec3(200, 200, 255));
	m_shapeRegistry.RegisterShapeTemplate(EntityType::TeamRocket, 3.0f, Vec3(255, 255, 200));
	m_shapeRegistry.RegisterShapeTemplate(EntityType::TeamMonkey, 3.0f, Vec3(255, 200, 255));
}
/////////////////////////////////



/////////////////////////////////
// SpawnEntityByType - Spawns an entity of the specified team type with random properties and adds it to the EntityManager. It takes the EntityManager reference, team type (0-4), radius, color, position, 
// velocity, and alpha as parameters. The team type is mapped to a specific EntityType enum value, and the new entity is created and added to the EntityManager using the addEntity method.
void TestScene::SpawnEntityByType(unsigned int teamType, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha) {
	// Map teamType (0-4) to EntityType enum values
	const EntityType teamTypes[] = {EntityType::TeamEagle, EntityType::TeamHawk, EntityType::TeamBoogaloo, EntityType::TeamRocket, EntityType::TeamMonkey};
	
	// Ensure teamType is within bounds (0-4), otherwise default to TeamMonkey
	EntityType type = (teamType < 5) ? teamTypes[teamType] : EntityType::TeamMonkey;

	// Create a new entity of the specified type and add it to the EntityManager
	Entity* en = m_entityManager.AddEntity(type);

	//en->AddComponent<CTransform>(position, velocity); // already added in by the Entity Manager's AddEntity() call, so we don't need to add it again here
	en->AddComponent<CName>();

	// Retrieve the CTransform component from the entity to set its position and velocity
	auto* tform = en->GetComponent<CTransform>();
	
	// Use CTransform's public members (position / velocity)
	tform->position = Vec2(position.x + 0.4f, position.y - 0.5f);
	tform->velocity = Vec2(velocity.x, velocity.y);

	// Add a shape component based on the entity type (Explosion or Circle)
	if (EntityType::Explosion == type) {
		auto explosion = std::make_unique<CExplosion>();
		explosion->SetRadius(radius);
		explosion->SetColor(color.x, color.y, color.z, alpha);
		en->AddComponentPtr<CShape>(std::move(explosion));
	} else {
		const ShapeTemplate& tmpl = m_shapeRegistry.GetShapeTemplate(type);

		auto* inst = en->AddComponent<CRenderInstance>();
		inst->radius = radius; // per‑entity variation
		inst->r = tmpl.color.x;
		inst->g = tmpl.color.y;
		inst->b = tmpl.color.z;
		inst->a = alpha;

		// putting CCircle in the entity's component list allows the collision system to detect collisions with this entity
		auto circle = std::make_unique<CCircle>();
		circle->SetRadius(radius);
		circle->SetColor(inst->r, inst->g, inst->b, inst->a);
		en->AddComponentPtr<CShape>(std::move(circle));
	}

	m_spatialLayers.GetLayer("TestScene").Insert(en); // Insert the entity into the spatial hash layer for collision detection and spatial queries
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
	ImGui::Text("Deaths: %d", deathCount);
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
void TestScene::UpdateEntity(Entity* e, float dt, float worldW, float worldH) {
	// 1. Physics (position + velocity)
	m_entityManager.GetPhysicsSystem().UpdateSingle(e, dt, worldW, worldH);

// --- CORRECT: Sync SoA → AoS for rendering ---
	if (auto* t = e->GetComponent<CTransform>()) {
		auto& T = m_entityManager.GetTransformSoA();
		size_t idx = e->transformIndex;
		if (idx != SIZE_MAX) {
			t->position.x = T.posX[idx];
			t->position.y = T.posY[idx];
			//t->velocity.x = T.velX[idx];
			//t->velocity.y = T.velY[idx];
		}
	}

	// 3. Render instance prep (GPU batching)
	if (auto* inst = e->GetComponent<CRenderInstance>()) {
		if (auto* t = e->GetComponent<CTransform>()) {
			inst->x = t->position.x;
			inst->y = t->position.y;
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// UpdateFPS - Updates the FPS value by calculating the number of frames rendered in the last second and applying an exponential moving average to smooth out fluctuations.
void TestScene::UpdateFPS(float deltaTime) {
	m_frameCount++;
	m_fpsAccumulator += deltaTime;

	// Update once per second
	if (m_fpsAccumulator >= 1.0f) {
		float rawFps = static_cast<float>(m_frameCount) / m_fpsAccumulator;

		// exponential moving average for smoothing
		if (m_fpsSmooth <= 0.0f) {
			m_fpsSmooth = rawFps;
		} else { // Apply exponential moving average for smoothing
			m_fpsSmooth = (m_alpha * rawFps) + ((1.0 - m_alpha) * m_fpsSmooth);
		}

		// Update the public FPS value for display
		m_fps = m_fpsSmooth;

		// Reset counters for the next second
		m_frameCount = 0;
		m_fpsAccumulator = 0.0f;
	}
}
/////////////////////////////////



/////////////////////////////////
void TestScene::ProcessEvents() {
	// Retrieve main camera (if any)
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	CCamera* cam = camOpt ? *camOpt : nullptr;

	// Poll SFML events
	while (auto evt = m_gameEngine.window.pollEvent()) {

		// Always forward events to ImGui first
		ImGui::SFML::ProcessEvent(m_gameEngine.window, *evt);

		// Window closed
		if (evt->is<sf::Event::Closed>()) {
			m_gameEngine.window.close();
			continue;
		}

		// -----------------------------
		// CAMERA INPUT (event-driven)
		// -----------------------------
		if (cam) {

			// --- Mouse wheel zoom ---
			if (evt->is<sf::Event::MouseWheelScrolled>()) {
				auto* mw = evt->getIf<sf::Event::MouseWheelScrolled>();
				cam->zoom -= mw->delta * 0.1f;
				cam->zoom = std::clamp(cam->zoom, 0.25f, 4.0f);
			}

			// --- Begin panning (right mouse button) ---
			if (evt->is<sf::Event::MouseButtonPressed>()) {
				auto* mb = evt->getIf<sf::Event::MouseButtonPressed>();
				if (mb->button == sf::Mouse::Button::Right) {
					m_cameraSystem.BeginPan(*cam, sf::Mouse::getPosition(m_window));
				}
			}

			// --- Update panning ---
			if (evt->is<sf::Event::MouseMoved>()) {
				if (m_cameraSystem.IsPanning(*cam)) {
					m_cameraSystem.UpdatePan(*cam, sf::Mouse::getPosition(m_window));
				}
			}

			// --- End panning ---
			if (evt->is<sf::Event::MouseButtonReleased>()) {
				auto* mb = evt->getIf<sf::Event::MouseButtonReleased>();
				if (mb->button == sf::Mouse::Button::Right) {
					m_cameraSystem.EndPan(*cam);
				}
			}
		}
	}
}
/////////////////////////////////




/////////////////////////////////
void TestScene::UpdateSceneLogic(float dt) {
	m_growthTimer += dt;

	if (m_growthTimer >= 5.0f) {   // every 5 seconds
		m_targetEntityCount += 20; // increase population target
		m_growthTimer = 0.0f;
	}

	UpdateSpawning(dt);
}
/////////////////////////////////



/////////////////////////////////
// RenderUI - Renders the ImGui game information window with current entity count, death count, and explosion count. It calls the RenderGameInfoWindow method to display the relevant information in the UI.
void TestScene::RenderUI() {
	m_deathCount += m_entityManager.GetDeathCountThisFrame();
	RenderGameInfoWindow(m_entityManager.GetEntities().size(), m_deathCount, m_explosionCount);
}
/////////////////////////////////



/////////////////////////////////
// UpdateSpawning - Updates the spawning of entities in the scene based on the current entity count and the target entity count. If the current entity count is less than the target, it calculates how many entities need to be spawned and calls the SpawnReplacementEntities method to spawn them.
void TestScene::UpdateSpawning(float deltaTime) {
	// Check if the current entity count is less than the target entity count
	size_t current = m_entityManager.GetEntities().size();

	// If the current entity count is less than the target, spawn replacement entities to maintain the target population
	if (current < m_targetEntityCount) {
		int toSpawn = std::min(8, m_targetEntityCount - static_cast<int>(current));
		SpawnReplacementEntities(toSpawn);
	}
}
/////////////////////////////////



/////////////////////////////////
void TestScene::SpawnInitialPopulation() {
	const auto windowSize = m_window.getSize();
	for (int i = 0; i < m_targetEntityCount; ++i) {
		// 1. Choose team type (0 or 1)
		unsigned int teamType = static_cast<unsigned int>(m_entityType(m_rng));

		// 2. Sample spawn position within map bounds (with a margin of 50 units)
		float spawnX = std::uniform_real_distribution<float>(m_mapMin.x + 50.0f, m_mapMax.x - 50.0f)(m_rng);

		float spawnY = std::uniform_real_distribution<float>(m_mapMin.y + 50.0f, m_mapMax.y - 50.0f)(m_rng);


		// 3. Initial velocity (full 2D motion)
		float velX = std::uniform_real_distribution<float>(-180.0f, 180.0f)(m_rng);
		float velY = std::uniform_real_distribution<float>(-180.0f, 180.0f)(m_rng);

		//// Clamp minimum speeds to avoid slow movers
		//if (std::abs(velX) < 250.0f)
		//	velX = (velX < 0.0f ? -250.0f : 250.0f);

		//if (std::abs(velY) < 180.0f)
		//	velY = (velY < 0.0f ? -180.0f : 180.0f);

		// 4. Visual properties
		int r = m_redVal(m_rng);
		int g = m_greenVal(m_rng);
		int b = m_blueVal(m_rng);
		int a = m_alphaVal(m_rng);
		float radius = m_radiusDistro(m_rng)*3.0f;

		// 5. Spawn entity
		SpawnEntityByType(teamType, radius, Vec3(r, g, b), Vec2(spawnX, spawnY), Vec2(velX, velY), a);
	}
}
/////////////////////////////////



/////////////////////////////////
void TestScene::SpawnReplacementEntities(int count) {
	for (int i = 0; i < count; ++i) {
		// 1. Choose movement direction (0 = leftward, 1 = rightward)
		int direction = m_direction(m_rng);

		// 2. Map direction to team type
		unsigned int teamType = (direction == 1) ? 0u : 1u;

		// 3. Sample velocity (horizontal only)
		//float velX = m_xVelocity(m_rng);
		float velX = 30.0f;
		//std::cout << "SpawnReplacementEntities: direction=" << direction << ", velX=" << velX << "\n";
		float velY = 0.0f;

		// Reverse velocity if moving rightward
		if (direction == 1)
			velX = -velX;

		// 4. Sample spawn position
		float spawnX;
		const auto windowSize = m_window.getSize();
		float spawnY = std::uniform_real_distribution<float>(m_mapMin.y + 150.0f, m_mapMax.y - 150.0f)(m_rng);

		if (direction == 1) {
			// Spawn just off the left edge
			spawnX = std::uniform_real_distribution<float>(m_mapMin.x - 150.0f, m_mapMin.x)(m_rng);
		} else {
			// Spawn just off the right edge
			spawnX = m_mapMax.x + std::uniform_real_distribution<float>(0.0f, 150.0f)(m_rng);
		}

		// 5. Sample visual properties
		int r = m_redVal(m_rng);
		int g = m_greenVal(m_rng);
		int b = m_blueVal(m_rng);
		int a = m_alphaVal(m_rng);
		float radius = m_radiusDistro(m_rng)*3.0f;

		// 6. Spawn the entity
		SpawnEntityByType(teamType, radius, Vec3(r, g, b), Vec2(spawnX, spawnY), Vec2(velX, velY), a);
	}
}
/////////////////////////////////




/////////////////////////////////
// SpawnExplosion - Spawns an explosion entity at the specified position with the given radius. It creates a new entity of type Explosion, sets up its transform and explosion components, and adds it to the scene's registry of active explosions for tracking and updating.
void TestScene::SpawnExplosion(const Vec2& pos, float radius) {
	// Create explosion entity through ECS
	Entity* e = m_entityManager.AddEntity(EntityType::Explosion);

	// Transform setup
	auto* transform = e->GetComponent<CTransform>();
	transform->position = pos;


	// Explosion component setup
	auto* explosion = e->GetComponent<CExplosion>();
	explosion->age = 0;
	explosion->SetRadius(radius);

	// Add to scene registry
	m_explosions.push_back(e);
}
/////////////////////////////////



/////////////////////////////////
void TestScene::ApplyMainCameraView(float deltaTime) {
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt)
		return;
	CCamera* cam = *camOpt;

	//std::cout << "CAMERA BEFORE CLAMP: " << cam->position.x << ", " << cam->position.y << std::endl;


	sf::View v;
	v.setSize(sf::Vector2f(cam->viewportWidth / cam->zoom, cam->viewportHeight / cam->zoom));

	// Clamp camera to bounds
	float halfW = (cam->viewportWidth / cam->zoom) * 0.5f;
	float halfH = (cam->viewportHeight / cam->zoom) * 0.5f;
	Vec2 newPos = cam->position;

	if (m_hasMapBounds) {
		float minCx = m_mapMin.x + halfW;
		float maxCx = m_mapMax.x - halfW;
		float minCy = m_mapMin.y + halfH;
		float maxCy = m_mapMax.y - halfH;

		if (minCx <= maxCx) {
			newPos.x = std::clamp(newPos.x, minCx, maxCx);
		} else {
			newPos.x = (m_mapMin.x + m_mapMax.x) * 0.5f;
		}

		if (minCy <= maxCy) {
			newPos.y = std::clamp(newPos.y, minCy, maxCy);
		} else {
			newPos.y = (m_mapMin.y + m_mapMax.y) * 0.5f;
		}
	}

	cam->position = newPos;

	//std::cout << "CAMERA AFTER CLAMP: " << newPos.x << ", " << newPos.y << std::endl;

	// Snap camera to sub-pixel grid based on zoom level to reduce shimmer/jitter when rendering
	// At zoom levels 2.0x or higher, snap to tile alignment (32 pixels). Otherwise snap to half-pixel.
	float snapGrid = (cam->zoom >= 2.0f) ? 32.0f : 0.5f; // Snap to tile grid at 2x+ zoom, else half-pixel
	float snapX = std::round(newPos.x / snapGrid) * snapGrid;
	float snapY = std::round(newPos.y / snapGrid) * snapGrid;

	v.setCenter(sf::Vector2f(snapX, snapY));
	m_window.setView(v);
}
/////////////////////////////////



/////////////////////////////////
void TestScene::ApplyCameraMovement(float deltaTime) {
	auto camOpt = m_cameraSystem.GetMainCamera(GetEntityManager());
	if (!camOpt)
		return;

	CCamera* cam = *camOpt;

	Vec2 move(0, 0);

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
		move.y -= 1;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
		move.y += 1;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
		move.x -= 1;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
		move.x += 1;

	if (move.Mag2() > 0.0f) {
		move.Normalize();
		move *= 600.0f * deltaTime;
		cam->position += move;

		//std::cout << "Camera pos (after WASD): " << cam->position.x << ", " << cam->position.y << std::endl;
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

	// Remove dead explosions from our tracking vector
	m_explosions.erase(
		std::remove_if(m_explosions.begin(), m_explosions.end(),
			[](Entity* e) { return !e || !e->IsAlive(); }),
		m_explosions.end()
	);

	for (auto& entity : m_entityManager.GetEntities()) { // Iterate over all entities to find explosions and update their state based on elapsed time since creation
		if (entity->GetType() == EntityType::Explosion) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - entity->m_creationTime);

			// Check if the sound effect is still playing
	/*		CSoundEffect* soundEffect = entity->GetComponent<CSoundEffect>();
			bool soundStillPlaying = soundEffect && soundEffect->m_sound && soundEffect->m_state == CSoundEffect::State::Playing;*/

			// Only destroy the entity if both the visual lifespan is over AND the sound has finished
			if (elapsed.count() > 9000) {
				//std::cout << "Destroying explosion entity after 9000ms" << std::endl;
				entity->Destroy();
			} else {
				m_explosionCount++; // Increment explosion count for active explosions that have not yet expired
				float fadeProgress = static_cast<float>(elapsed.count()) / 9000.0f;
				// Use a higher base alpha so explosions remain more visible as they expand.
				const int maxAlpha = 220; // match CExplosion default alpha
				int newAlpha = static_cast<int>(maxAlpha * (1.0f - fadeProgress));

				auto* explosion = entity->GetComponent<CExplosion>();
				auto* shape = entity->GetComponent<CShape>();

				if (explosion && shape) {
					// Grow radius
					//std::cout << "Grow you little bastard, grow! Explosion radius before: " << explosion->GetRadius() << std::endl;
					float r = shape->GetRadius(); // renderer uses CShape radius
					r *= 1.004f;
					shape->SetRadius(r);

					// Fade alpha on the SHAPE (renderer uses CShape)
					sf::Color currentColor = shape->GetColor();
					shape->SetColor(currentColor.r, currentColor.g, currentColor.b, newAlpha);
				}
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
void TestScene::InitialiseSpatialLayers() {
	// Wire this registry into the EntityManager
	m_entityManager.SetSpatialLayerRegistry(&m_spatialLayers);

	// Create a dedicated layer for TestScene entities
	// Balls are small, so use a small cell size for accurate collisions
	m_spatialLayers.CreateLayer("TestScene", 96.0f);
}
/////////////////////////////////