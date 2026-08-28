/////////////////////////////////
// GameEngine.cpp - Implementation of the GameEngine class, responsible for managing the game loop, scenes, and rendering using SFML
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the GameEngine implementation.
#include "CursorSystem.h"
#include "GameEngine.h"
#include "FontManager.h"
#include "SoundSystem.h"
#include "Vec2.h"
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <memory>
#include <string>
#include <direct.h>
#include <iostream>
#include "Scene.h"
#include "TestScene.h"
#include "RayCastScene.h"
#include "TileMapEditorScene.h"
#include "TechSimulationScene.h"
#include "MusicVisualizerScene.h"
#include "LevelEditorScene.h"
#include "MainMenuScene.h"
#include "PathTestScene.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
#include "MainThreadTasks.h"
#include "JobSystem.h"
/////////////////////////////////



/////////////////////////////////
GameEngine::GameEngine() {
	// Setup the SFML window as borderless (fullscreen-windowed) to avoid exclusive fullscreen quirks
	windowSize = sf::VideoMode::getDesktopMode().size;
	window.create(sf::VideoMode(windowSize), "SFML Game Engine", sf::Style::None);
	//window.setPosition(sf::Vector2i(0, 0));
	//window.create(sf::VideoMode(windowSize), "SFML Game Engine", sf::Style::Fullscreen);

	window.setFramerateLimit(120);
	//window.setVerticalSyncEnabled(true);
	isRunning = true;

	// *** ENTITY MANAGER *** Create a single engine-wide EntityManager and bind shared resources
	entityManager = std::make_unique<EntityManager>(window);
	entityManager->GetRenderSystem().SetFontManager(&fontManager);

	// Debug: Print current working directory
	char cwd[260];
	if (_getcwd(cwd, sizeof(cwd)) != nullptr) {
		std::cout << "[GameEngine] Current working directory: " << cwd << std::endl;
	}

	// *** AUDIO *** Create and initialize the SoundSystem
	soundSystem = std::make_unique<SoundSystem>();
	soundSystem->Initialize();  // Check audio device and log diagnostics
	soundSystem->InitializePool(*entityManager, 64);  // Initialize sound pool with 64 entities
	soundSystem->SetMasterVolume(70.0f);  // Set master volume to 70% (0-100 scale)

	// Bind the SoundSystem to the CollisionSystem so it can check for concurrent sound limits when playing collision sounds (bit of coupling here, but it's a simple solution for now)
	entityManager->GetCollisionSystem().SetSoundSystem(soundSystem.get());
	std::cout << "[GameEngine] Audio system initialized (using SFML defaults)"  << std::endl; // Log audio system initialization


	// *** FONTS *** Try to load a default font for in-engine text (used by MainMenu and CText). Expect file at assets/fonts/tech.ttf
	if (!fontManager.LoadFont("default", "assets/fonts/tech.ttf")) {
		std::cerr << "Warning: failed to load default font at assets/fonts/tech.ttf" << std::endl;
	}

	// *** CURSOR SYSTEM *** Initialize the cursor system with the game window
	m_cursorSystem = std::make_unique<CursorSystem>();
	m_cursorSystem->Initialize(&window);

	// *** MOVEMENT SYSTEM *** Initialize MovementSystem for path following
	movementSystem = std::make_unique<MovementSystem>();

	// *** FILE MANAGER *** Initialize FileManager with current working directory for asset loading
	m_fileManager.SetBasePath(".");

	// Do not preload atlases automatically. Atlases should be loaded explicitly via the editor UI so users can choose which atlas to use at runtime.
	// *** IMGUI *** Initialize ImGui-SFML early so scenes can safely call ImGui during Update
	if (!ImGui::SFML::Init(window)) {
		std::cerr << "Warning: Failed to initialize ImGui::SFML in GameEngine" << std::endl;
		// continue without ImGui but scenes must tolerate absence, have no idea if they will atm.
	}

	techRegistry.LoadDefaults(); // Load default rendering techniques for the engine
}
/////////////////////////////////



/////////////////////////////////
// Destructor - cleans up resources and shuts down the game engine, including ImGui-SFML shutdown and clearing scenes
GameEngine::~GameEngine() {
	// Mark that we're shutting down BEFORE member destruction begins
	// This prevents destructors from attempting operations on already-destroyed objects
	ShutdownGuard::MarkShuttingDown();
}
/////////////////////////////////



/////////////////////////////////
// AddScene - Adds a new scene to the game engine with the given name and scene instance, allowing for dynamic scene management. The scene is stored in a map of scene names to scene instances, enabling 
// easy retrieval and switching between scenes during the game loop.
void GameEngine::AddScene(const std::string& sceneName, std::shared_ptr<Scene> scene) {
	scenes[sceneName] = scene;
}
/////////////////////////////////



/////////////////////////////////
// GetSceneNames - Return a list of registered scene names (useful for UI like a main menu)
std::vector<std::string> GameEngine::GetSceneNames() const {
	std::vector<std::string> names;
	for (auto const& p : scenes) names.push_back(p.first);
	return names;
}
/////////////////////////////////



/////////////////////////////////
// ChangeScene - Changes the current scene to the specified scene name, allowing for scene management and transitions. The method checks if the specified scene exists // in the scenes map and sets it as 
// the current active scene, enabling the game loop to update and render the new scene. If the scene name is not found, a warning is logged.
void GameEngine::ChangeScene(const std::string& sceneName) {
	// Wait for all jobs to finish before changing scenes to avoid dangling references
	JobSystem::WaitIdle();

	// Clear any pending main thread tasks to avoid executing tasks from the previous scene after switching
	auto it = scenes.find(sceneName);
	if (it != scenes.end()) {
		// notify previous scene it's exiting
		if (currentScene) {
			try { currentScene->OnExit(); } catch (...) {}
			// Allow the scene to release any loaded resources (textures, atlases, large buffers)
			try { currentScene->UnloadResources(); } catch (...) {}
		}

		// Clear all entities from the previous scene before activating the new one
		if (entityManager) entityManager->ClearAll();

		// Reset the view to default before switching scenes to avoid any leftover zoom or pan from the previous scene
		window.setView(window.getDefaultView());

		// Set the new scene as the current active scene
		currentScene = it->second;

		// initialize the new scene so it has window size, input controller, and resources set up
		if (currentScene) {
			try {
				currentScene->InitializeGame(windowSize);
			} catch (...) {}
			// update input controller to use the new scene's game controller
			try { m_InputController.SetGameController(currentScene->GetGameController()); } catch (...) {}
			try { currentScene->OnEnter(); } catch (...) {}
		}
	} else {
		std::cerr << "Scene '" << sceneName << "' not found!" << std::endl;
	}
}
/////////////////////////////////



/////////////////////////////////
// RemoveScene - Removes a scene from the game engine by its name, allowing for dynamic scene management. The method checks if the specified scene exists in the scenes map and erases it, freeing up 
// resources associated with that scene. If the scene name is not found, no action is taken.
void GameEngine::RemoveScene(const std::string& sceneName) {
	auto it = scenes.find(sceneName);
	if (it != scenes.end()) {
		scenes.erase(it);
	}
}
/////////////////////////////////



/////////////////////////////////
// Run - Main game loop that handles scene management, input processing, and rendering. The method initializes the chosen scene, sets up the input controller, and enters a loop that updates the current 
// scene, processes events, and renders the scene to the window. The loop continues until the window is closed or the running state is set to false.
void GameEngine::Run() {
	// Initialise job system before any scenes start scheduling jobs
	JobSystem::Init(4); // Initialize the job system with 4 worker threads (or use std::thread::hardware_concurrency() for dynamic thread count)

	// Setup Event Handler.
	bool running = true; // Create a Boolean variable to manage the engine running state

	// Going to run a test scene for now, will add a main menu and other scenes later once the scene management system is more fleshed out.
	AddScene("MainMenu", std::make_shared<MainMenuScene>(*this, window, *entityManager));					// Adding MainMenuScene
	AddScene("TestScene", std::make_shared<TestScene>(*this, window, *entityManager));						// Adding TestScene
	AddScene("RayCastScene", std::make_shared<RayCastScene>(*this, window, *entityManager));				// Adding TileMapScene, old tilemap (deprecated)
	//AddScene("TileMapEditor", std::make_shared<TileMapEditorScene>(*this, window, *entityManager));		// Adding TileMapEditor, will need to revisit this later to implement new tilemap editor with new tilemap system
	AddScene("MusicVisualizer", std::make_shared<MusicVisualizerScene>(*this, window, *entityManager));		// Adding MusicVisualizer	
	AddScene("LevelEditor", std::make_shared<LevelEditorScene>(*this, window, *entityManager));				// Adding LevelEditor
	AddScene("PathTestScene", std::make_shared<PathTestScene>(*this, window, *entityManager));				// Adding PathTestScene
	AddScene("TechSimulationScene",
			 std::make_shared<TechSimulationScene>(*this, window, *entityManager)); // Adding TechSimulationScene

	ChangeScene("MainMenu");

	// Initialize the chosen scene if it was found. ChangeScene() logs a warning when the scene name is not found; guard against a null current scene to avoid dereferencing a nullptr.
	if (currentScene) {
		currentScene->InitializeGame(windowSize);

		// FontManager already bound to engine-owned EntityManager in constructor
		m_InputController.SetGameController(currentScene->GetGameController());

		m_InputController.Init(
			[&running](uint32_t deltaT, InputState state) {
				running = false;
				std::cout << "Quitting" << std::endl;
			},
			&window); // The defined function will be called when we quit the game..
	} else {
		std::cerr << "No current scene selected; skipping scene initialization." << std::endl;
	}




	// *** MAIN LOOP ***
	/* Main Loop, game logic is handled in here once per frame */
	while (window.isOpen()) {
		Update(0.016f); // Update the scene with a fixed delta time (16ms for ~60 FPS), I can calculate actual delta time using the deltaClock for variable time steps
	}
	
	// Shutdown ImGui-SFML before destroying the window to avoid dangling references
	JobSystem::Shutdown();
	// Ensure window closes cleanly when Run exits
	if (window.isOpen()) window.close();
}
/////////////////////////////////



/////////////////////////////////
// Update - Updates the current scene, processes input, and handles rendering. The method is called once per frame and performs the following actions:
//			1) Clears the window and render queue, 
//			2) Updates ImGui and FPS counter, 
//			3) Polls events and forwards them to the current scene, 
//			4) Updates the input controller, 
//			5) Updates the current scene and its entity manager,
void GameEngine::Update(float deltaTime) {
	// *** MAIN LOOP *** Main loop, game logic is handled in here once per frame, runs while the window is open and handles events, updates, and rendering for the current scene.
	//while (window.isOpen()) {
		// *** CLEAR WINDOW *** Clear the window at the start of each frame. Use an explicit clear color so fully transparent tiles in the editor reveal the intended background instead of an unintended 
		// grey fallback.
		window.clear(sf::Color::Transparent);

		// *** RENDER QUEUE *** Clear the engine-wide render queue from the previous frame
		m_renderQueue.Clear();

		// *** DELTA TIME CLOCK *** Restart delta clock and update ImGui once per frame
		sf::Time frameTime = deltaClock.restart();
		if (ImGui::GetCurrentContext())
			ImGui::SFML::Update(window, frameTime);

		// *** FPS COUNTER *** Update shared FPS counter with the real frame time
		m_fpsCounter.Update(frameTime.asSeconds());
	
		// *** EVENT POLLING *** Poll events (SFML 3: pollEvent returns std::optional<sf::Event>) and forward to current scene
		while (auto eventOpt = window.pollEvent()) {
			const auto& resizeEvent = eventOpt->getIf<sf::Event::Resized>();
			// Forward events to ImGui-SFML so UI widgets receive input
			if (ImGui::GetCurrentContext())
				ImGui::SFML::ProcessEvent(window, *eventOpt);

			// Detect if the window is resized and update the current scene's view accordingly
			if (eventOpt->is<sf::Event::Resized>()) {
				if (currentScene) {
					currentScene->OnWindowResized({resizeEvent->size.x, resizeEvent->size.y});
					windowSize = {resizeEvent->size.x, resizeEvent->size.y}; // set the new window size in GameEngine.
				}
			}

			// *** GLOBAL ESCAPE HANDLING *** Intercept Escape before forwarding to the scene so scene-level handlers that close the window won't run. If Escape is pressed and we're not already on 
			// MainMenu, switch to it.
			if (eventOpt->is<sf::Event::KeyPressed>()) {
				if (auto kp = eventOpt->getIf<sf::Event::KeyPressed>()) {
					if (static_cast<sf::Keyboard::Key>(kp->code) == sf::Keyboard::Key::Escape) {
						auto it = scenes.find("MainMenu");
						if (it != scenes.end() && currentScene && currentScene != it->second) {
							ChangeScene("MainMenu");
							// consume the event (do not forward to the current scene)
							continue;
						}
					}
				}
			}

			// Forward the event to the current scene for handling (if any)
			if (currentScene)
				currentScene->HandleEvent(eventOpt);
			if (eventOpt->is<sf::Event::Closed>())
				window.close();
		}

		// *** INPUT CONTROLLER UPDATE *** Update method will run  (carry out) these actions.
		m_InputController.Update(deltaTime);

		// *** GLOBAL ESCAPE HANDLING *** Global keyboard poll: if Escape is held, switch back to MainMenu before any scene Update runs... This prevents scenes that poll sf::Keyboard::isKeyPressed(Escape) 
		// from closing the window directly.
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
			auto it = scenes.find("MainMenu");
			if (it != scenes.end() && currentScene && currentScene != it->second) {
				ChangeScene("MainMenu");
				// Reset input controller to new scene controller
				m_InputController.SetGameController(currentScene->GetGameController());
				// Skip running the previous scene's Update this frame
				//continue;
			}
		}

		// *** SCENE UPDATE *** Update the current scene and its entity manager, movement system, and sound system. The scene's Update method handles ImGui updates and input, while the movement system 
		// updates entity positions based on their paths. The entity manager processes game logic, and the sound system handles sound effects for entities in the current scene.
		if (currentScene) {
			// Let the scene update (handles ImGui update and input)... Use the actual frame time measured above so scenes get accurate timing for FPS and logic.
			currentScene->Update(frameTime.asSeconds());

			// Update movement BEFORE entity manager so new paths can be used immediately
			movementSystem->Update(currentScene->GetEntityManager().GetEntities(), deltaTime);

			// Ensure the scene's EntityManager processes game logic (tile system, pending entities)
			// and rebuilds spatial structures (including BVH).
			currentScene->GetEntityManager().Update(deltaTime);

			// Process sound effects using the SCENE's EntityManager (not global)
			// This ensures we process sounds for entities in the current active scene
			soundSystem->Process(currentScene->GetEntityManager(), deltaTime);
			soundSystem->Update(deltaTime);

			// Update the global cursor system
			m_cursorSystem->Update(deltaTime);

			// *** MAIN-THREAD GL/UPLOAD TASKS *** Execute any main-thread GL/upload tasks queued by worker threads. These must run while the main thread's OpenGL context is current to avoid context 
			// activation errors.
			std::vector<std::function<void()>> _mainThreadTasks;
			MainThreadTaskQueue::Instance().Drain(_mainThreadTasks); // Drain queued tasks into a local vector to avoid holding the mutex while executing tasks
			
			// Execute each task in the drained vector, catching any exceptions to prevent a single task failure from crashing the engine
			for (auto &t : _mainThreadTasks) {
				try { t(); } catch (...) {}
			}

			// *** ENGINE RENDER PASS ORDERING *** 
			// 1) Scene overlays (render chunks and world elements first)
			currentScene->Render();
			// 2) Entity shapes (on top of scene overlays)
			currentScene->GetEntityManager().RenderShapes();
			// 3) Flush queued overlays and shapes
			m_renderQueue.Flush(window);
			// 4) Entity text (direct render after queue flush so text appears on top)
			currentScene->GetEntityManager().RenderText();
			// 5) Custom cursor (on top of scene, below ImGui)
			m_cursorSystem->Render();
		}

		// Draw any debug overlays from the current scene before ImGui so they are visible
		//if (m_currentScene) m_currentScene->RenderDebugOverlay();

		// *** ENGINE RENDER QUEUE FLUSH *** Flush the engine-wide render queue to the window before ImGui rendering
		// This ensures all scene and entity draws appear behind the UI
		m_renderQueue.Flush(window);

		// *** IMGUI RENDERING *** (Should be done last) Render ImGui on top of everything (ImGui::SFML::Render without args uses current target)
		if (ImGui::GetCurrentContext() && (currentScene == nullptr || currentScene->IsImGuiEnabled())) ImGui::SFML::Render(window);

		// *** DISPLAY FRAME *** Display the rendered frame to the window
		window.display();
	//}
}
/////////////////////////////////



/////////////////////////////////
// *** MOVEMENT SYSTEM UPDATE ***
void MovementSystem::Update(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime) {
	// Iterate through all entities in the scene
	for (const auto& ent : entities) {
		if (!ent || !ent->IsAlive()) continue;

		// Skip if entity doesn't have a path follower or it's not active
		auto* follower = ent->GetComponent<CPathFollower>();
		if (!follower || !follower->isActive) continue;

		// Skip if entity doesn't have a path with waypoints
		auto* path = ent->GetComponent<CPath>();
		if (!path || path->points.empty()) {
			follower->isActive = false;
			continue;
		}

		// Skip if entity doesn't have a transform.
		auto* transform = ent->GetComponent<CTransform>();
		if (!transform) continue;

		// Ensure the current waypoint index is within bounds.
		if (follower->currentWaypointIndex >= (int)path->points.size()) {
			follower->currentWaypointIndex = (int)path->points.size() - 1;
		}

		// Get the current waypoint target
		const Vec2& currentWaypoint = path->points[follower->currentWaypointIndex];
		Vec2 direction = (currentWaypoint - transform->position);
		float distanceToWaypoint = direction.Mag();

		// Check if we've arrived at the current waypoint
		if (distanceToWaypoint < WAYPOINT_ARRIVAL_THRESHOLD) {
			// Move to next waypoint
			follower->currentWaypointIndex++;

			// Check if we've reached the end of the path
			if (follower->currentWaypointIndex >= (int)path->points.size()) {
				// Path complete: mark as inactive but keep components for reuse
				follower->isActive = false;
				continue;
			}
		}

		// Move towards the current waypoint if there's distance to cover
		if (distanceToWaypoint > 0.001f) {
			// Normalize direction and apply speed
			direction.Normalize();
			float moveDistance = follower->speed * deltaTime;
			transform->position = transform->position + (direction * moveDistance);
		}
	}
}
/////////////////////////////////
