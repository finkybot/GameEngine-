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
#include <limits>

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
#include "CRaycast.h"
#include "CTransform.h"
#include "CMouseState.h"
#include "CameraSystem.h"


#include <SFML/Window/Event.hpp>
#include <SFML/System/Vector2.hpp>

#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
using Clock = std::chrono::high_resolution_clock;
/////////////////////////////////



/////////////////////////////////
// Constructor - initializes the TestScene with references to the game engine, render window, and entity manager, and sets up ImGui for UI rendering
TestScene::TestScene(std::string name, sf::RenderWindow& win, World& world)
	: m_window(win), Scene(name, world) {
	// Initialize ImGui with SFML backend
	if (!ImGui::SFML::Init(world.GetWindow())) {
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
	m_mouseSystem->Update(dt);

	// Now we can read the mouse state
	CMouseState* ms = m_mouseEntity->GetComponent<CMouseState>();
	if (!ms)
		return;

	CRaycast* raycast = m_mouseEntity->GetComponent<CRaycast>();

	//
	// --- IMPORTANT: update raycast fields BEFORE running RaycastSystem ---
	//
	raycast->screenPos = ms->screenPos;

	// --- Update raycast world position and direction based on the main camera's position and orientation. This ensures that the raycast is correctly aligned with the camera's view, allowing for accurate detection of entities in the scene based on mouse input.
	auto cCam = m_cameraSystem.GetMainCamera(GetWorld().GetEntityManager());
	if (cCam) {
		// Ray starts at the mouse world position
		raycast->worldPos = ms->worldPos;

		// Ray goes straight up a few pixels
		raycast->direction = Vec2(0.0f, 1.0f);

		// Use a long ray to exercise the grid raycast over multiple cells.
		raycast->maxDistance = 20.0f;
	}

	// --- Update delta time and explosion sound timer, and calculate smoothed FPS using an exponential moving average to reduce fluctuations in the reported FPS. This provides a more stable and accurate representation of the game's performance over time.
	m_dt = dt;
	m_explosionSoundTimer = std::min(1.0f / 12.0f, m_explosionSoundTimer + dt);
	UpdateFPS(dt);

	// --- Get the CollisionSystem from the EntityManager for handling collision detection and resolution ---
	auto& CollisionSystem = GetWorld().GetEntityManager().GetCollisionSystem();

	// --- Scene growth + spawning (bounded) ---
	m_growthTimer += dt;
	if (m_growthTimer >= 10.0f) {
		m_targetEntityCount += 28;
		m_growthTimer = 0.0f;
	}

	// --- Update spawning of entities based on the target entity count and current population. This manages the dynamic growth of the entity population in the scene, ensuring that new entities are spawned as needed to maintain the desired count.
	UpdateSpawning(dt);

	//
	// --- SINGLE declaration of entities ---
	//
	auto& entities = GetWorld().GetEntityManager().GetEntities();

	// --- Main unified per‑entity pass ---
	auto camOpt = m_cameraSystem.GetMainCamera(GetWorld().GetEntityManager());
	CCamera* cam = camOpt ? *camOpt : nullptr;

	// --- Determine world dimensions based on the camera's world width and height, or fallback to the window size if no camera is available. This ensures that entity updates are performed within the correct bounds of the game world.
	float worldW = cam ? cam->worldWidth : static_cast<float>(m_window.getSize().x);
	float worldH = cam ? cam->worldHeight : static_cast<float>(m_window.getSize().y);

	// --- Update each entity in the scene, applying movement, collision detection, and other game logic as needed. This loop iterates through all entities, checking their alive status and updating their state based on the current delta time and world dimensions.
	for (auto& uptr : entities) {
		Entity* e = uptr.get();
		if (!e->IsAlive())
			continue;

		UpdateEntity(e, dt, worldW, worldH);
	}

	// --- Update explosions, handling their lifecycle and any associated effects. This manages the visual and audio feedback for explosions in the scene, ensuring that they are updated correctly based on the current game state and delta time.
	UpdateExplosions();

	// --- Broad‑phase collision (optional / throttled) ---
	if (m_spatialCollisionEnabled && GetWorld().GetEntityManager().GetSpatialIndex()) {
		GetWorld().GetEntityManager().GetCollisionSystem().DetectAndResolveSpatial(entities, GetWorld().GetEntityManager().GetSpatialIndex(), dt);
	}

	// --- Process collision explosion requests and spawn explosions at the collision points. This handles the visual and audio feedback for collisions between entities, creating explosion effects and playing sound effects as appropriate.
	const auto& explosionRequests = CollisionSystem.GetExplosionRequests();
	if (!explosionRequests.empty()) {
		//std::cout << "[TestScene] Collision explosion requests: " << explosionRequests.size() << ", sound entity ID: " << m_explosionSoundID.index << "\n";
	}

	// --- Retrieve the main camera entity and its CCamera component to determine if the collision point is within the camera's view before playing the explosion sound effect. This prevents unnecessary sound effects for off-screen collisions, improving performance and user experience.
	bool queuedExplosionSound = false;
	for (const auto& request : explosionRequests) {
		SpawnExplosion(request.position, request.radius, request.colour);

		// --- Check if the collision point is within the camera's view before playing the explosion sound effect. This prevents unnecessary sound effects for off-screen collisions, improving performance and user experience.
		bool collisionInCameraView = false;

		// Retrieve the main camera entity and its CCamera component to determine if the collision point is within the camera's view before playing the explosion sound effect. This prevents unnecessary sound effects for off-screen collisions, improving performance and user experience.
		if (cam) {
			float halfViewWidth = cam->viewportWidth / (2.0f * cam->zoom);
			float halfViewHeight = cam->viewportHeight / (2.0f * cam->zoom);

			// Check if the collision point is within the camera's view bounds by comparing the absolute difference between the collision position and the camera's position to half of the camera's viewport dimensions, adjusted for zoom. If both the x and y differences are within the respective half-view dimensions, the collision is considered to be in view.
			collisionInCameraView =
				std::abs(request.position.x - cam->position.x) <= halfViewWidth &&
				std::abs(request.position.y - cam->position.y) <= halfViewHeight;
		}

		// --- Play explosion sound effect if the collision is within the camera's view and the sound timer allows for it. This throttles the sound effect to avoid overwhelming the audio system with too many simultaneous sounds, especially during high-collision scenarios.
		if (collisionInCameraView && !queuedExplosionSound && m_explosionSoundTimer >= 1.0f / 12.0f) {
			if (auto* SoundSystem = GetWorld().GetEntityManager().GetSoundSystem()) {
				// Queue the explosion sound effect after collision detection has generated the request.
				SoundSystem->QueueSoundEffect(m_explosionSoundID, request.position);
				m_explosionSoundTimer = 0.0f;
				queuedExplosionSound = true;
			} else {
				std::cerr << "[TestScene] SoundSystem is unavailable\n";
			}
		}
	}

	// Clear the explosion requests after processing to avoid duplicate explosions in the next update cycle. This ensures that each collision event results in a single explosion effect and sound, maintaining consistency in the game's visual and audio feedback.
	CollisionSystem.ClearExplosionRequests();

	// --- Audio listener (camera only) ---
	if (cam && GetWorld().GetEntityManager().GetSoundSystem()) {
		Vec2 listenerPos(cam->position.x, cam->position.y);
		GetWorld().GetEntityManager().GetSoundSystem()->SetListenerPosition(listenerPos);
	}

	// --- Commit pending entities ---
	GetWorld().GetEntityManager().ProcessPending();

	m_raycastSystem->Update(dt);

	//if (raycast->lastHit.hit) {
	//	std::cout << "Mouse worldPos: " << raycast->worldPos.x << ", " << raycast->worldPos.y << "\n";
	//	std::cout << "\x1b[36mRaycast HIT entity ID: " << raycast->lastHit.entity->GetId().index << " at (" << raycast->lastHit.position.x << ", " << raycast->lastHit.position.y << ")\n\x1b[0m";
	//}

	//GetWorld().GetEntityManager().GetBVH().ReportBVHSize();
	// --- Camera movement + view ---
	if (m_cameraEntity) {
		m_cameraSystem.Update(dt, GetWorld().GetEntityManager());
		ApplyCameraMovement(dt);
		ApplyMainCameraView(dt);
	}

	// --- UI (ImGui) ---
	RenderUI();
}
/////////////////////////////////



/////////////////////////////////
// Render - responsible for rendering the scene, including all entities and any scene-specific visuals. The actual rendering of entities is handled by the EntityManager's RenderAll method, which is called by the GameEngine after this method. This method can be used to render any additional scene-specific visuals or effects that are not part of 
// the standard entity rendering process.
void TestScene::Render() {}
/////////////////////////////////



/////////////////////////////////
// DoAction - performs any scene-specific actions or updates that are not covered by the standard update and render methods. This can include things like triggering events, managing timers, or handling specific game mechanics unique to this scene.
void TestScene::DoAction() { /*scene-specific action*/ }
/////////////////////////////////



/////////////////////////////////
// HandleEvent - processes input events for the scene, such as keyboard and mouse input. In this implementation, it checks if the Escape key is pressed and calls the ProcessEscapeKey method to close the window if it is. This method can be expanded to handle additional input events as needed for the scene's functionality.
void TestScene::HandleEvent(const std::optional<sf::Event>& event) {
	bool escapeKeyDown = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape);
	// handle input events
	ProcessEscapeKey(escapeKeyDown);

	if (!event) return;

	// Retrieve main camera (if any)
	auto camOpt = m_cameraSystem.GetMainCamera(GetWorld().GetEntityManager());
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
		if (mb->button == sf::Mouse::Button::Left) {
			const Vec2 mouseWorld = GetMouseWorldPosition(mb->position);
			m_draggedBall = PickBall(mouseWorld);
			if (m_draggedBall) {
				if (auto* transform = m_draggedBall->GetComponent<CTransform>())
					m_dragOffset = transform->position - mouseWorld;

				// --- Reset drag velocity and last drag position for calculating the drag release velocity when the mouse button is released. This ensures that the entity being dragged will have a smooth and responsive movement when released, based on the user's drag input.
				m_dragVelocity = Vec2::Zero;
				m_lastDragWorldPosition = mouseWorld;
				m_hasDragSample = true;
			}
		} else if (mb->button == sf::Mouse::Button::Right) {
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
		if (mb->button == sf::Mouse::Button::Left) {
			// --- Calculate the drag release velocity based on the last sampled drag position and the current mouse world position. This allows for a smooth transition of the dragged entity's movement when the mouse button is released, giving it a natural momentum based on the user's drag input.
			if (m_draggedBall && m_hasDragSample) {
				const Vec2 releaseWorld = GetMouseWorldPosition(mb->position);
				const float sampleTime = std::max(m_dt, 1.0f / 120.0f);
				m_dragVelocity = (releaseWorld - m_lastDragWorldPosition) / sampleTime;

				// Clamp the drag velocity to a maximum speed to prevent excessive movement when releasing the dragged entity. This ensures that the entity's movement remains within reasonable bounds and does not become uncontrollable or unrealistic.
				constexpr float maxDragSpeed = 1200.0f;

				// Clamp the drag velocity to a maximum speed to prevent excessive movement when releasing the dragged entity. This ensures that the entity's movement remains within reasonable bounds and does not become uncontrollable or unrealistic.
				if (m_dragVelocity.Mag() > maxDragSpeed)
					m_dragVelocity = m_dragVelocity.GetUnitVec() * maxDragSpeed;

				// Apply the calculated drag velocity to the dragged entity's transform component, updating its velocity for movement in the next update cycle. This allows the entity to continue moving in the direction of the drag release, simulating momentum and providing a more dynamic interaction with the game world.
				if (auto* transform = m_draggedBall->GetComponent<CTransform>()) {
					transform->velocity = m_dragVelocity;

					// If the dragged ball has a valid transform index, update its velocity in the Transform SoA (Structure of Arrays) for efficient access and processing. This allows for optimized updates of the entity's position and movement in the game world, leveraging the benefits of the SoA data structure for performance.
					if (m_draggedBall->transformIndex != SIZE_MAX) {
						auto& transforms = GetWorld().GetEntityManager().GetTransformSoA();
						transforms.velX[m_draggedBall->transformIndex] = m_dragVelocity.x;
						transforms.velY[m_draggedBall->transformIndex] = m_dragVelocity.y;
					}
				}
			}

			// Reset drag state variables after releasing the dragged entity, ensuring that the next drag operation starts with a clean state. This prevents unintended behavior or carry-over effects from previous drag interactions.
			m_draggedBall = nullptr;
			m_dragOffset = Vec2::Zero;
			m_dragVelocity = Vec2::Zero;
			m_hasDragSample = false;
		} else if (mb->button == sf::Mouse::Button::Right) {
			m_cameraSystem.EndPan(*cam);
		}
	}
}
/////////////////////////////////


/////////////////////////////////
// GetMouseWorldPosition - converts the given mouse position in window coordinates to world coordinates based on the camera's position, zoom, and viewport size. This allows for accurate interaction with entities and other game elements in the game's world space.
Vec2 TestScene::GetMouseWorldPosition(const sf::Vector2i& mousePosition) const {
	// Retrieve the main camera entity and its CCamera component to access the camera's position, zoom, and viewport size. If the camera is not available, return a zero vector as a fallback.
	CCamera* cam = m_cameraEntity ? m_cameraEntity->GetComponent<CCamera>() : nullptr;
	if (!cam) {	return Vec2::Zero; }

	// Calculate a safe zoom value to avoid division by zero or extremely small zoom values that could lead to rendering issues. If the camera's zoom is greater than a small threshold (0.0001f), use it; otherwise, default to 1.0f.
	const float safeZoom = (cam->zoom > 0.0001f) ? cam->zoom : 1.0f;
	sf::View view;

	// Set the view's center and size based on the camera's position, viewport dimensions, and safe zoom value. This ensures that the mouse position is correctly mapped to world coordinates relative to the camera's current view.
	view.setCenter(sf::Vector2f(cam->position.x, cam->position.y));
	view.setSize(sf::Vector2f(cam->viewportWidth / safeZoom, cam->viewportHeight / safeZoom));

	// Convert the mouse position from window coordinates to world coordinates using the SFML window's mapPixelToCoords function, which takes into account the current view. The resulting world position is then returned as a Vec2 object for use in game logic and interactions.
	const sf::Vector2f worldPos = m_window.mapPixelToCoords(mousePosition, view);
	return Vec2(worldPos.x, worldPos.y);
}
/////////////////////////////////


/////////////////////////////////
// PickBall - returns the closest entity (ball) to the given world position that is within its radius. It iterates through all entities, checking if they are alive and have a render instance, and calculates the distance from the world position to each entity's position. If the distance is less than or equal to the entity's radius, 
// it considers it a potential pick and keeps track of the closest one found.
Entity* TestScene::PickBall(const Vec2& worldPosition) const {
	// Find the closest entity (ball) to the given world position that is within its radius. This method iterates through all entities, checking if they are alive and have a render instance, and calculates the distance from the world position to each entity's position. If the distance is less than or equal to the entity's radius, it considers it a 
	// potential pick and keeps track of the closest one found.
	Entity* closestBall = nullptr;
	float closestDistanceSquared = std::numeric_limits<float>::max();

	// --- Iterate through all entities in the EntityManager ---
	for (const auto& uptr : GetWorld().GetEntityManager().GetEntities()) {
		Entity* entity = uptr.get();

		// Skip entities that are not alive or do not have a render instance, as they cannot be picked.
		if (!entity || !entity->IsAlive()) continue;
		if (!entity->GetComponent<CRenderInstance>()) continue;

		// Get the entity's transform component to access its position and radius for picking calculations.
		const auto* transform = entity->GetComponent<CTransform>();
		if (!transform)	continue;

		// Calculate the squared distance from the world position to the entity's position. This avoids the need for a square root calculation, which is more expensive and unnecessary for comparison purposes.
		const float radius = entity->GetRadius();
		const float dx = transform->position.x - worldPosition.x;
		const float dy = transform->position.y - worldPosition.y;
		const float distanceSquared = dx * dx + dy * dy;

		// If the distance is less than or equal to the entity's radius and is the closest found so far, update the closest ball and its distance.
		if (distanceSquared <= (radius * radius) && distanceSquared < closestDistanceSquared) {
			closestDistanceSquared = distanceSquared;
			closestBall = entity;
		}
	}

	return closestBall;
}
/////////////////////////////////



/////////////////////////////////
// OnEnter - called when the scene becomes active, allowing for any necessary setup or initialization that should occur each time the scene is entered. This can include resetting game state, starting timers, or preparing resources specific to this scene.
void TestScene::OnEnter() {
	// --- Scene initialization ---
	GetWorld().GetEntityManager().SetSFMLRenderingEnabled(false);
	GetWorld().GetEntityManager().SetGLRenderingEnabled(true);
	GetWorld().GetEntityManager().ClearAll();

	// --- Reset scene state variables ---
	m_explosionSoundEntity = nullptr;

	// --- Initialise sound system with a pool size of 64 for managing sound effects efficiently. This prepares the sound system to handle multiple simultaneous sound effects without performance degradation.
	GetWorld().GetEntityManager().GetSoundSystem()->InitialisePool(GetWorld().GetEntityManager(), 64);

	// --- Create explosion sound entity ---
	Entity* soundEntity = m_world.CreateEntity(EntityType::Default);
	m_explosionSoundEntity = soundEntity;
	auto* soundEffect = soundEntity->AddComponent<CSoundEffect>("Explosion", "assets/sounds/medium-explosion.ogg");
	m_explosionSoundID = soundEntity->GetId();
	m_explosionSoundTimer = 1.0f / 12.0f;

	// --- Configure sound effect properties ---
	soundEffect->volume = 100.0f;
	soundEffect->_3DMinDistance = 1000.0f;
	soundEffect->_3DMaxDistance = 6000.0f;
	soundEffect->PlayOnAwake = false;
	soundEffect->shouldPlay = false;
	soundEffect->loop = false;
	soundEffect->is3D = true;

	// Expand world bounds
	m_mapMin = Vec2(0, 0);
	m_mapMax = Vec2((float)m_window.getSize().x * 15.0f, (float)m_window.getSize().y * 15.0f);
	m_hasMapBounds = true;

	// Camera entity
	m_cameraEntity = m_world.CreateEntity(EntityType::Camera);

	// Calculate the initial camera position to be centered in the map bounds
	Vec2 initialCamPos((m_mapMin.x + m_mapMax.x) * 0.5f, (m_mapMin.y + m_mapMax.y) * 0.5f);

	// Add a CCamera component to the camera entity with the calculated initial position and set it as the main camera. The camera is configured with a zoom level of 1.0f, viewport dimensions based on the window size, and world dimensions based on the map bounds. The smoothness is set to 0.0f for immediate response to camera movements.
	auto camera = m_cameraEntity->AddComponent<CCamera>(initialCamPos, 1.0f);
	camera->isMainCamera = true;
	camera->isActive = true;
	camera->viewportWidth = (float)m_window.getSize().x;
	camera->viewportHeight = (float)m_window.getSize().y;
	camera->smoothness = 0.0f;
	camera->worldWidth = m_mapMax.x - m_mapMin.x;
	camera->worldHeight = m_mapMax.y - m_mapMin.y;

	// Mouse entity
	Entity* mouseEntity = m_world.CreateEntity(EntityType::Mouse);
	mouseEntity->AddComponent<CMouseState>();
	mouseEntity->AddComponent<CRaycast>();

	m_mouseEntity = mouseEntity;
	m_mouseSystem = new MouseSystem(&GetWorld().GetEntityManager(), &m_window);
	m_raycastSystem = new RaycastSystem(&GetWorld().GetEntityManager());


	// Spawn initial population ONCE
	SpawnInitialPopulation();
	GetWorld().GetEntityManager().ProcessPending();


	// Build spatial index ONCE
	auto& entities = GetWorld().GetEntityManager().GetEntities();
	GetWorld().GetEntityManager().GetSpatialIndex()->Build(GetWorld().GetEntityManager().GetDynamicEntities());

	
	m_testBucket = GetWorld().GetEntityManager().CreateEntityBucket("DynamicEntities", 0);

	// Lets test our bucket structures *this is a very early test, and will be expanded later*
	for (size_t i = 0; i < 10 && i < entities.size(); ++i) {
		m_world.AddEntityToBucket(m_testBucket, entities[i]->GetId());
	}

	auto* bucket = GetWorld().GetEntityManager().GetEntityBucket(m_testBucket);

	// Debug: print bucket contents
	if (bucket)
	{
		std::cout << "Bucket: " << bucket->name << " contains " << bucket->entities.size() << " entities.\n";
		std::cout << "Contains " << bucket->entities.size() << " entities:\n";
	}

	// Debug: print dynamic entity count
	std::cout << "Dynamic entities: " << GetWorld().GetEntityManager().GetDynamicEntities().size() << "\n";

	// Reset cell markers
	for (auto& u : entities) {
		Entity* e = u.get();
		e->currentCellX = INT_MIN;
		e->currentCellY = INT_MIN;
	}

	// Set the mouse system to use world coordinates instead of SFML window coordinates. This ensures that mouse input is interpreted correctly in the context of the game's world space, allowing for accurate interaction with entities and other game elements based on their positions in the world rather than their positions on the screen.
	m_mouseSystem->SetUseSFMLCoords(false);

	std::cout << "Initial build of spatial index with " << entities.size() << " entities.\n";
	std::cout << "Task completed for " << GetWorld().GetEntityManager().GetDynamicEntities().size() << " entities.\n";
}
/////////////////////////////////



/////////////////////////////////
// OnExit - called when the scene is no longer active, allowing for any necessary cleanup or state management that should occur each time the scene is exited. This can include stopping timers, saving state, or releasing resources specific to this scene.
void TestScene::OnExit() { /* cleanup when scene exits */ 
	std::cout << "Exiting TestScene...\n";
	
	// Clean up the mouse system and raycast system
	if (m_testBucket != INVALID_BUCKET) {
		bool removed = GetWorld().GetEntityManager().DeleteEntityBucket(m_testBucket);

		std::cout << "Bucket deleted: " << removed << "\n";
	}
}
/////////////////////////////////



/////////////////////////////////
// OnWindowResized - called when the window is resized, allowing for any necessary adjustments to the scene's view or layout based on the new window size. In this implementation, it updates the view to center on the new window size and adjusts the view's size accordingly.
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
// UnloadResources - responsible for unloading any resources that were loaded for the scene, allowing for cleanup and freeing of memory when the scene is no longer needed. In this implementation, it is a placeholder, but in a more complete implementation, it would include actual resource unloading logic.
void TestScene::UnloadResources() { /* unload resources */ }
/////////////////////////////////



/////////////////////////////////
// InitializeGame - responsible for initializing the game state for the scene, including spawning entities with random properties and setting up any necessary game logic or mechanics.
void TestScene::Initialise(sf::Vector2u windowSize) {
	InitialiseSpatialLayers();

	// Initialize RNG once
	m_rng.seed(std::random_device{}());

	// Initialize distributions (created once, reused forever)
	m_xVelocity = std::uniform_int_distribution<int>(-8, -4);
	m_yVelocity = std::uniform_int_distribution<int>(-20, 20);

	m_redVal = std::uniform_int_distribution<int>(100, 255);
	m_greenVal = std::uniform_int_distribution<int>(100, 255);
	m_blueVal = std::uniform_int_distribution<int>(100, 255);
	m_alphaVal = std::uniform_int_distribution<int>(150, 255);

	m_radiusDistro = std::uniform_real_distribution<float>(2.0f, 4.0f);

	m_entityType = std::uniform_int_distribution<int>(0, 1);
	m_direction = std::uniform_int_distribution<int>(0, 1);


	// Register shape templates here
	m_shapeRegistry.RegisterShapeTemplate(EntityType::TeamEagle, 3.0f, Vec3(255, 200, 200));
	m_shapeRegistry.RegisterShapeTemplate(EntityType::TeamHawk, 3.0f, Vec3(200, 255, 200));
	m_shapeRegistry.RegisterShapeTemplate(EntityType::TeamBoogaloo, 3.0f, Vec3(200, 200, 255));
	m_shapeRegistry.RegisterShapeTemplate(EntityType::TeamRocket, 3.0f, Vec3(255, 255, 200));
	m_shapeRegistry.RegisterShapeTemplate(EntityType::TeamMonkey, 3.0f, Vec3(255, 200, 255));

}
/////////////////////////////////



/////////////////////////////////
// SpawnEntityByType - Spawns an entity of the specified team type with random properties and adds it to the EntityManager. It takes the EntityManager reference, team type (0-4), radius, color, position, velocity, and alpha as parameters. The team type is mapped to a specific EntityType enum value, and the new entity is created and added to the EntityManager using the addEntity method.
void TestScene::SpawnEntityByType(unsigned int teamType, float radius, Vec3 color, Vec2 position, Vec2 velocity, int alpha) {
	// Map teamType (0-4) to EntityType enum values
	const EntityType teamTypes[] = {EntityType::TeamEagle, EntityType::TeamHawk, EntityType::TeamBoogaloo, EntityType::TeamRocket, EntityType::TeamMonkey};
	
	// Ensure teamType is within bounds (0-4), otherwise default to TeamMonkey
	EntityType type = (teamType < 5) ? teamTypes[teamType] : EntityType::TeamMonkey;

	// Create a new entity of the specified type and add it to the EntityManager
	Entity* en = m_world.CreateEntity(type);

	//en->AddComponent<CTransform>(position, velocity); // already added in by the Entity Manager's AddEntity() call, so we don't need to add it again here
	en->AddComponent<CName>();

	// Retrieve the CTransform component from the entity to set its position and velocity
	auto* tform = en->GetComponent<CTransform>();
	
	// Use CTransform's public members (position / velocity)
	position = ClampBallToMap(position, radius);
	tform->position = position;
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
// RenderGameInfoWindow - Renders the ImGui window displaying game information and performance metrics. It takes the current entity count, death count for the current frame, and active explosion count as parameters to display in the UI. The window is positioned at (10, 10) and sized to (450, 280) on first use, and it includes sections for entity statistics and spatial hash collision detection performance metrics.
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
// UpdateEntity - Updates the state of a single entity based on its components and the current game state. It checks if the entity has a CSoundEffect component, and if so, it skips further updates. If the entity has a CTransform component, it updates its position based on whether it is being dragged or moving normally, 
// and synchronizes the transform with the SoA data.
void TestScene::UpdateEntity(Entity* e, float dt, float worldW, float worldH) {
	(void)worldW;
	(void)worldH;
	if (e->HasComponent<CSoundEffect>())
		return;

	CTransform* transform = e->GetComponent<CTransform>();
	if (!transform || e->transformIndex == SIZE_MAX)
		return;

	const size_t transformIndex = e->transformIndex;
	if (UpdateDraggedBall(e, transformIndex, *transform, dt)) {
		SyncEntityTransform(e, transformIndex, *transform);
		return;
	}

	UpdateBallMovement(e, transformIndex, *transform, dt);
	SyncEntityTransform(e, transformIndex, *transform);
}
/////////////////////////////////



/////////////////////////////////
// ClampBallToMap - Clamps the position of a ball entity to ensure it stays within the defined map bounds, taking into account the ball's radius. It returns a new Vec2 representing the clamped position, ensuring that the ball does not go outside the map boundaries.
Vec2 TestScene::ClampBallToMap(const Vec2& position, float radius) const {
	return Vec2(
		std::clamp(position.x, m_mapMin.x + radius, m_mapMax.x - radius),
		std::clamp(position.y, m_mapMin.y + radius, m_mapMax.y - radius));
}
/////////////////////////////////



/////////////////////////////////
// UpdateDraggedBall - Updates the position of a dragged ball entity based on the current mouse position and drag offset. It checks if the entity being updated is the one currently being dragged, and if so, it calculates the new position by adding the drag offset to the mouse world position. The new position is then clamped to ensure it stays within the 
// defined map bounds, taking into account the ball's radius. The updated position is stored back in the SoA (Structure of Arrays) data and the entity's transform component.
bool TestScene::UpdateDraggedBall(Entity* entity, size_t transformIndex, CTransform& transform, float deltaTime) {
	if (entity != m_draggedBall) return false;

	// Retrieve the current mouse world position from the CMouseState component
	const Vec2 mouseWorld = m_mouseEntity->GetComponent<CMouseState>()->worldPos;
	const Vec2 position = ClampBallToMap(mouseWorld + m_dragOffset, entity->GetRadius());
	if (m_hasDragSample && deltaTime > 0.0f)
		m_dragVelocity = (mouseWorld - m_lastDragWorldPosition) / deltaTime;
	m_lastDragWorldPosition = mouseWorld;

	// Update the SoA (Structure of Arrays) data with the new position
	auto& transforms = GetWorld().GetEntityManager().GetTransformSoA();
	transforms.SetPosition(transformIndex, position.x, position.y);
	transform.position = position;
	return true;
}
/////////////////////////////////



/////////////////////////////////
// UpdateBallMovement - Updates the movement of a ball entity based on its current velocity and position, applying clamping to ensure it stays within the defined map bounds. It calculates the new position based on the velocity and delta time, checks for collisions with the map boundaries, and adjusts the velocity accordingly. The updated position 
// and velocity are then stored back in the SoA (Structure of Arrays) data and the entity's transform component.
void TestScene::UpdateBallMovement(Entity* entity, size_t transformIndex, CTransform& transform, float deltaTime) {
	constexpr float ballSpeed = 18.0f;
	constexpr float speedConvergence = 3.0f;
	auto& transforms = GetWorld().GetEntityManager().GetTransformSoA();
	Vec2 velocity(transforms.velX[transformIndex], transforms.velY[transformIndex]);
	if (velocity.Mag2() <= 0.0001f) {
		velocity = Vec2::RandomDirection() * ballSpeed;
	} else {
		float speed = velocity.Mag();
		float blend = std::min(1.0f, speedConvergence * deltaTime);
		speed += (ballSpeed - speed) * blend;
		velocity = velocity.GetUnitVec() * speed;
	}
	
	// Retrieve the current position from the SoA data
	Vec2 position(transforms.posX[transformIndex], transforms.posY[transformIndex]);
	position += velocity * deltaTime;

	// Clamp the position to ensure the ball stays within the map bounds, taking into account its radius
	const Vec2 clampedPosition = ClampBallToMap(position, entity->GetRadius());
	if (clampedPosition.x != position.x)
		velocity.x = position.x < clampedPosition.x ? std::abs(velocity.x) : -std::abs(velocity.x);
	if (clampedPosition.y != position.y)
		velocity.y = position.y < clampedPosition.y ? std::abs(velocity.y) : -std::abs(velocity.y);

	// Update the SoA data with the new position and velocity
	transforms.SetPosition(transformIndex, clampedPosition.x, clampedPosition.y);
	transforms.velX[transformIndex] = velocity.x;
	transforms.velY[transformIndex] = velocity.y;
	transform.position = clampedPosition;
	transform.velocity = velocity;
}
/////////////////////////////////



/////////////////////////////////
// SyncEntityTransform - Synchronizes the entity's transform component with the SoA (Structure of Arrays) data. It updates the position of the entity based on the values stored in the SoA and also updates the CRenderInstance component if it exists, ensuring that the visual representation of the entity is consistent with its logical position in the game world.
void TestScene::SyncEntityTransform(Entity* entity, size_t transformIndex, CTransform& transform) {
	(void)entity; // unused parameter warning

	// Sync the entity's transform component with the SoA data
	auto& transforms = GetWorld().GetEntityManager().GetTransformSoA();

	// Update the entity's position based on the SoA data
	transform.position = Vec2(transforms.posX[transformIndex], transforms.posY[transformIndex]);
	
	// Update the CRenderInstance component if it exists, ensuring that the visual representation of the entity is consistent with its logical position in the game world
	if (auto* instance = entity->GetComponent<CRenderInstance>()) {
		instance->x = transform.position.x;
		instance->y = transform.position.y;
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
// ProcessEvents - Processes input events for the scene, including window events and camera input. It retrieves the main camera (if any) and polls SFML events, forwarding them to ImGui first. It handles window close events, mouse wheel zoom for the camera, and mouse button events for panning the camera. It also updates the camera's position based 
// on mouse movement while panning.
void TestScene::ProcessEvents() {
	// Retrieve main camera (if any)
	auto camOpt = m_cameraSystem.GetMainCamera(GetWorld().GetEntityManager());
	CCamera* cam = camOpt ? *camOpt : nullptr;

	// Poll SFML events
	while (auto evt = GetWorld().GetWindow().pollEvent()) {

		// Always forward events to ImGui first
		ImGui::SFML::ProcessEvent(GetWorld().GetWindow(), *evt);

		// Window closed
		if (evt->is<sf::Event::Closed>()) {
			GetWorld().GetWindow().close();
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
// UpdateSceneLogic - Updates the scene's logic, including entity growth and spawning. It increments the growth timer and checks if it has reached 5 seconds, at which point it increases the target entity count by 20 and resets the timer. It then calls UpdateSpawning to handle spawning new entities based on the current entity count and target.
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
	m_deathCount += GetWorld().GetEntityManager().GetDeathCountThisFrame();
	RenderGameInfoWindow(GetWorld().GetEntityManager().GetEntities().size(), m_deathCount, m_explosionCount);
}
/////////////////////////////////



/////////////////////////////////
// UpdateSpawning - Updates the spawning of entities in the scene based on the current entity count and the target entity count. If the current entity count is less than the target, it calculates how many entities need to be spawned and calls the SpawnReplacementEntities method to spawn them.
void TestScene::UpdateSpawning(float deltaTime) {
	// Check if the current entity count is less than the target entity count
	size_t current = GetWorld().GetEntityManager().GetEntities().size();

	// If the current entity count is less than the target, spawn replacement entities to maintain the target population
	if (current < m_targetEntityCount) {
		int toSpawn = std::min(8, m_targetEntityCount - static_cast<int>(current));
		SpawnReplacementEntities(toSpawn);
	}
}
/////////////////////////////////



/////////////////////////////////
// SpawnInitialPopulation - Spawns the initial population of entities in the scene based on the target entity count. It randomly generates properties for each entity, including team type, spawn position, velocity, color, and radius, and calls SpawnEntityByType to create and add the entities to the scene.
void TestScene::SpawnInitialPopulation() {
	const auto windowSize = m_window.getSize();
	for (int i = 0; i < m_targetEntityCount; ++i) {
		// 1. Choose team type (0 or 1)
		unsigned int teamType = static_cast<unsigned int>(m_entityType(m_rng));

		// 2. Sample spawn position within map bounds (with a margin of 50 units)
		float spawnX = std::uniform_real_distribution<float>(m_mapMin.x + 50.0f, m_mapMax.x - 50.0f)(m_rng);
		float spawnY = std::uniform_real_distribution<float>(m_mapMin.y + 50.0f, m_mapMax.y - 50.0f)(m_rng);

		// 3. Initial velocity (full 2D motion)
		float velX = std::uniform_real_distribution<float>(-18.0f, 18.0f)(m_rng);
		float velY = std::uniform_real_distribution<float>(-18.0f, 18.0f)(m_rng);

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
// SpawnReplacementEntities - Spawns a specified number of replacement entities in the scene when the current entity count is below the target. It randomly generates properties for each entity, including movement direction, team type, spawn position, velocity, color, and radius, and calls SpawnEntityByType to create and add the entities to the scene.
void TestScene::SpawnReplacementEntities(int count) {
	for (int i = 0; i < count; ++i) {
		// 1. Choose movement direction (0 = leftward, 1 = rightward)
		int direction = m_direction(m_rng);

		// 2. Map direction to team type
		unsigned int teamType = (direction == 1) ? 0u : 1u;

		// 3. Sample velocity (horizontal only)
		float velX = 30.0f;
		float velY = std::uniform_real_distribution<float>(-30.0f, 30.0f)(m_rng);
		
		// Invert velocity for rightward movement
		if (direction == 1)
			velX = -velX;

		// 4. Sample spawn position
		float spawnX;
		float spawnY = std::uniform_real_distribution<float>(m_mapMin.y + 150.0f, m_mapMax.y - 150.0f)(m_rng);

		// Spawn off-screen based on direction
		if (direction == 1) {
			spawnX = std::uniform_real_distribution<float>(m_mapMin.x - 150.0f, m_mapMin.x)(m_rng);
		} else {
			spawnX = m_mapMax.x + std::uniform_real_distribution<float>(0.0f, 150.0f)(m_rng);
		}

		// 5. Visual properties
		int r = m_redVal(m_rng);
		int g = m_greenVal(m_rng);
		int b = m_blueVal(m_rng);
		int a = m_alphaVal(m_rng);
		float radius = m_radiusDistro(m_rng) * 3.0f;

		// 6. Queue the entity (don’t commit yet)
		SpawnEntityByType(teamType, radius, Vec3(r, g, b), Vec2(spawnX, spawnY), Vec2(velX, velY), a);
	}
}
/////////////////////////////////



/////////////////////////////////
// SpawnExplosion - Spawns an explosion entity at the specified position with the given radius and color. It creates a new entity of type Explosion, sets up its transform and explosion components, and adds it to the scene's registry of active explosions for tracking and updating.
void TestScene::SpawnExplosion(const Vec2& pos, float radius, const Vec3& colour) {
	// Create explosion entity through ECS
	Entity* e = m_world.CreateEntity(EntityType::Explosion);

	// Transform setup
	auto* transform = e->GetComponent<CTransform>();
	transform->position = pos;


	// Explosion component setup
	auto* explosion = e->GetComponent<CExplosion>();
	explosion->age = 0;
	explosion->SetRadius(radius);
	explosion->SetColor(colour.x, colour.y, colour.z, 200);

	// Add to scene registry
	m_explosions.push_back(e);
}
/////////////////////////////////



/////////////////////////////////
// ApplyMainCameraView - Applies the main camera's view to the SFML window, adjusting for zoom and clamping the camera position within the defined map bounds. It also snaps the camera position to a sub-pixel grid based on the zoom level to reduce visual jitter when rendering.
void TestScene::ApplyMainCameraView(float deltaTime) {
	// Retrieve the main camera from the camera system
	auto camOpt = m_cameraSystem.GetMainCamera(GetWorld().GetEntityManager());
	
	// If no main camera is found, exit early
	if (!camOpt) return;

	// Retrieve the main camera component
	CCamera* cam = *camOpt;

	// Set up the SFML view based on the camera's position and zoom level
	sf::View v;
	v.setSize(sf::Vector2f(cam->viewportWidth / cam->zoom, cam->viewportHeight / cam->zoom));

	// Clamp camera to bounds
	float halfW = (cam->viewportWidth / cam->zoom) * 0.5f;
	float halfH = (cam->viewportHeight / cam->zoom) * 0.5f;
	Vec2 newPos = cam->position;

	// Clamp camera position to map bounds if they are defined
	if (m_hasMapBounds) {
		float minCx = m_mapMin.x + halfW;
		float maxCx = m_mapMax.x - halfW;
		float minCy = m_mapMin.y + halfH;
		float maxCy = m_mapMax.y - halfH;

		// If the camera's viewport is larger than the map bounds, center the camera instead of clamping
		if (minCx <= maxCx) {
			newPos.x = std::clamp(newPos.x, minCx, maxCx);
		} else {
			newPos.x = (m_mapMin.x + m_mapMax.x) * 0.5f;
		}

		// If the camera's viewport is larger than the map bounds, center the camera instead of clamping
		if (minCy <= maxCy) {
			newPos.y = std::clamp(newPos.y, minCy, maxCy);
		} else {
			newPos.y = (m_mapMin.y + m_mapMax.y) * 0.5f;
		}
	}

	// Update the camera's position after clamping
	cam->position = newPos;


	// Snap camera to sub-pixel grid based on zoom level to reduce shimmer/jitter when rendering At zoom levels 2.0x or higher, snap to tile alignment (32 pixels). Otherwise snap to half-pixel.
	float snapGrid = (cam->zoom >= 2.0f) ? 32.0f : 0.5f; // Snap to tile grid at 2x+ zoom, else half-pixel
	float snapX = std::round(newPos.x / snapGrid) * snapGrid;
	float snapY = std::round(newPos.y / snapGrid) * snapGrid;

	// Set the view's center to the snapped position and apply it to the SFML window
	v.setCenter(sf::Vector2f(snapX, snapY));
	m_window.setView(v);
}
/////////////////////////////////



/////////////////////////////////
// ApplyCameraMovement - Applies camera movement based on keyboard input (WASD) and delta time. It retrieves the main camera and adjusts its position based on the pressed keys, normalizing the movement vector to ensure consistent speed regardless of direction. The camera's position is updated accordingly.
void TestScene::ApplyCameraMovement(float deltaTime) {
	auto camOpt = m_cameraSystem.GetMainCamera(GetWorld().GetEntityManager());
	if (!camOpt)
		return;

	CCamera* cam = *camOpt;

	Vec2 move(0, 0);

	// Keyboard input for camera movement (WASD)
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
		move.y -= 1;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
		move.y += 1;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
		move.x -= 1;
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
		move.x += 1;

	// Normalize movement vector and apply speed based on delta time
	if (move.Mag2() > 0.0f) {
		move.Normalise();
		move *= 600.0f * deltaTime;
		cam->position += move;
	}
}
/////////////////////////////////



/////////////////////////////////
// UpdateExplosions - Updates the state of all active explosions; iterates through the tracked explosion entities, calculates their age based on their creation time, updates their color alpha for fading effect, and removes them if they have exceeded their lifespan.
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

	// Iterate over all entities in the world to find explosions and update their state based on elapsed time since creation
	for (auto& entity : GetWorld().GetEntityManager().GetEntities()) { // Iterate over all entities to find explosions and update their state based on elapsed time since creation
		if (entity->GetType() == EntityType::Explosion) {
			auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - entity->GetCreationTime());

			// Only destroy the entity if both the visual lifespan is over AND the sound has finished
			if (elapsed.count() > 9000) {
				entity->Destroy();
			} else {
				// Increment explosion count for active explosions that have not yet expired
				m_explosionCount++; 
				float fadeProgress = static_cast<float>(elapsed.count()) / 9000.0f;
				
				// Use a higher base alpha so explosions remain more visible as they expand.
				const int maxAlpha = 220; // match CExplosion default alpha
				int newAlpha = static_cast<int>(maxAlpha * (1.0f - fadeProgress));

				// Update the explosion's visual representation (CShape) to grow in size and fade out over time
				auto* explosion = entity->GetComponent<CExplosion>();
				auto* shape = entity->GetComponent<CShape>();

				// Ensure both components exist before attempting to modify them
				if (explosion && shape) {
					// Grow radius
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
// InitialiseSpatialLayers - Initializes the spatial layers for the scene, creating a dedicated layer for TestScene entities with a specified cell size for accurate collision detection. It wires the spatial layer registry into the EntityManager to enable spatial queries and collision handling.
void TestScene::InitialiseSpatialLayers() {
	// Wire this registry into the EntityManager
	GetWorld().GetEntityManager().SetSpatialLayerRegistry(&m_spatialLayers);

	// Create a dedicated layer for TestScene entities
	// Balls are small, so use a small cell size for accurate collisions
	m_spatialLayers.CreateLayer("TestScene", 96.0f);
}
/////////////////////////////////