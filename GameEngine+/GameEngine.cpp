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
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Audio/Listener.hpp>
#include <memory>
#include <string>
#include <direct.h>
#include <iostream>
#include "Scene.h"
#include "TestScene.h"
#include "TileMapScene.h"
#include "TileMapEditorScene.h"
#include "MusicVisualizerScene.h"
#include "LevelEditorScene.h"
#include "MainMenuScene.h"
#include "PathTestScene.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
#include <cstdlib>
#include "MainThreadTasks.h"
/////////////////////////////////



/////////////////////////////////
GameEngine::GameEngine() {
	// Setup the SFML window as borderless (fullscreen-windowed) to avoid exclusive fullscreen quirks
	m_windowSize = sf::VideoMode::getDesktopMode().size;
	m_window.create(sf::VideoMode(m_windowSize), "SFML Game Engine", sf::Style::None);
	//m_window.setPosition(sf::Vector2i(0, 0));
	//m_window.create(sf::VideoMode(m_windowSize), "SFML Game Engine", sf::State::Fullscreen);

	m_window.setFramerateLimit(120);
	//m_window.setVerticalSyncEnabled(true);
	m_isRunning = true;

	// Create a single engine-wide EntityManager and bind shared resources
	m_entityManager = std::make_unique<EntityManager>(m_window);
	m_entityManager->GetRenderSystem().SetFontManager(&m_fontManager);

	// Debug: Print current working directory
	char cwd[260];
	if (_getcwd(cwd, sizeof(cwd)) != nullptr) {
		std::cout << "[GameEngine] Current working directory: " << cwd << std::endl;
	}

	// Create and initialize the SoundSystem
	m_soundSystem = std::make_unique<SoundSystem>();
	m_soundSystem->Initialize();  // Check audio device and log diagnostics
	m_soundSystem->InitializePool(*m_entityManager, 64);  // Initialize sound pool with 64 entities
	m_soundSystem->SetMasterVolume(70.0f);  // Set master volume to 70% (0-100 scale)

	// Connect SoundSystem to CollisionSystem for sound limit checking
	m_entityManager->GetCollisionSystem().SetSoundSystem(m_soundSystem.get());

	// Audio listener setup completely disabled - causes music volume issues in SFML 3.x
	// Spatial audio will use SFML defaults without explicit configuration

	std::cout << "[GameEngine] Audio system initialized (using SFML defaults)" << std::endl;

	// Try to load a default font for in-engine text (used by MainMenu and CText). Expect file at assets/fonts/tech.ttf
	if (!m_fontManager.LoadFont("default", "assets/fonts/tech.ttf")) {
		std::cerr << "Warning: failed to load default font at assets/fonts/tech.ttf" << std::endl;
	}

	m_cursorSystem = std::make_unique<CursorSystem>(); // Initialize the cursor system with the game window
	m_cursorSystem->Initialize(&m_window);

	// Initialize FileManager with current working directory for asset loading
	m_fileManager.SetBasePath(".");

	// Do not preload atlases automatically. Atlases should be loaded explicitly via the editor UI so users
	// can choose which atlas to use at runtime.

	// Initialize ImGui-SFML early so scenes can safely call ImGui during Update
	if (!ImGui::SFML::Init(m_window)) {
		std::cerr << "Warning: Failed to initialize ImGui::SFML in GameEngine" << std::endl;
		// continue without ImGui but scenes must tolerate absence
	}
}
/////////////////////////////////



/////////////////////////////////
// Destructor - cleans up resources and shuts down the game engine, including ImGui-SFML shutdown and clearing scenes
GameEngine::~GameEngine() {}
/////////////////////////////////



/////////////////////////////////
// AddScene - Adds a new scene to the game engine with the given name and scene instance, allowing for dynamic scene management. The scene is stored in a map of 
// scene names to scene instances, enabling easy retrieval and switching between scenes during the game loop.
void GameEngine::AddScene(const std::string& sceneName, std::shared_ptr<Scene> scene) {
	m_scenes[sceneName] = scene;
}
/////////////////////////////////



/////////////////////////////////
// GetSceneNames - Return a list of registered scene names (useful for UI like a main menu)
std::vector<std::string> GameEngine::GetSceneNames() const {
	std::vector<std::string> names;
	for (auto const& p : m_scenes) names.push_back(p.first);
	return names;
}
/////////////////////////////////



/////////////////////////////////
// ChangeScene - Changes the current scene to the specified scene name, allowing for scene management and transitions. The method checks if the specified scene exists 
// in the scenes map and sets it as the current active scene, enabling the game loop to update and render the new scene. If the scene name is not found, a warning is logged.
void GameEngine::ChangeScene(const std::string& sceneName) {
	auto it = m_scenes.find(sceneName);
	if (it != m_scenes.end()) {
		// notify previous scene it's exiting
		if (m_currentScene) {
			try { m_currentScene->OnExit(); } catch (...) {}
			// Allow the scene to release any loaded resources (textures, atlases, large buffers)
			try { m_currentScene->UnloadResources(); } catch (...) {}
		}

		// Clear all entities from the previous scene before activating the new one
		if (m_entityManager) m_entityManager->ClearAll();

		m_currentScene = it->second;

		// initialize the new scene so it has window size, input controller, and resources set up
		if (m_currentScene) {
			try {
				m_currentScene->InitializeGame(m_windowSize);
			} catch (...) {}
			// update input controller to use the new scene's game controller
			try { m_InputController.SetGameController(m_currentScene->GetGameController()); } catch (...) {}
			try { m_currentScene->OnEnter(); } catch (...) {}
		}
	} else {
		std::cerr << "Scene '" << sceneName << "' not found!" << std::endl;
	}
}
/////////////////////////////////



/////////////////////////////////
// RemoveScene - Removes a scene from the game engine by its name, allowing for cleanup and resource management of scenes that are no longer needed. The method checks if the 
// specified scene exists in the scenes map and removes it, freeing up resources associated with that scene and ensuring it is no longer updated or rendered in the game loop.
void GameEngine::RemoveScene(const std::string& sceneName) {
	auto it = m_scenes.find(sceneName);
	if (it != m_scenes.end()) {
		m_scenes.erase(it);
	}
}
/////////////////////////////////



/////////////////////////////////
// Run - Starts the main game loop, handling scene initialization, event processing, updating, and rendering. The method sets up the initial scene, initializes it, and enters a 
// loop that continues until the window is closed... The loop processes events, updates the current scene with a fixed delta time, and renders the scene to the window.
void GameEngine::Run() {
	// Setup Event Handler.
	bool running = true; // Create a Boolean variable to manage the engine running state

	// Going to run a test scene for now, will add a main menu and other scenes later once the scene management system is more fleshed out.
	AddScene("MainMenu", std::make_shared<MainMenuScene>(*this, m_window, *m_entityManager));					// Adding MainMenuScene
	AddScene("TestScene", std::make_shared<TestScene>(*this, m_window, *m_entityManager));						// Adding TestScene
	AddScene("TileMapScene", std::make_shared<TileMapScene>(*this, m_window, *m_entityManager));				// Adding TileMapScene
	AddScene("TileMapEditor", std::make_shared<TileMapEditorScene>(*this, m_window, *m_entityManager));			// Adding TileMapEditor
	AddScene("MusicVisualizer", std::make_shared<MusicVisualizerScene>(*this, m_window, *m_entityManager));		// Adding MusicVisualizer	
	AddScene("LevelEditor", std::make_shared<LevelEditorScene>(*this, m_window, *m_entityManager));				// Adding LevelEditor
	AddScene("PathTestScene", std::make_shared<PathTestScene>(*this, m_window, *m_entityManager));				// Adding PathTestScene

	ChangeScene("MainMenu");

	// Initialize the chosen scene if it was found. ChangeScene() logs a warning when the scene
	// name is not found; guard against a null current scene to avoid dereferencing a nullptr.
	if (m_currentScene) {
		m_currentScene->InitializeGame(m_windowSize);

		// FontManager already bound to engine-owned EntityManager in constructor
		m_InputController.SetGameController(m_currentScene->GetGameController());

		m_InputController.Init(
			[&running](uint32_t deltaT, InputState state) {
				running = false;
				std::cout << "Quitting" << std::endl;
			},
			&m_window); // The defined function will be called when we quit the game..
	} else {
		std::cerr << "No current scene selected; skipping scene initialization." << std::endl;
	}




	/* Main Loop, game logic is handled in here once per frame */
	while (m_window.isOpen()) {
		Update(0.016f); // Update the scene with a fixed delta time (16ms for ~60 FPS), I can calculate actual delta time using the deltaClock for variable time steps
	}

	// Ensure window closes cleanly when Run exits
	if (m_window.isOpen()) m_window.close();
}
/////////////////////////////////



/////////////////////////////////
// Update - Updates the current scene and game state based on the elapsed time since the last frame, allowing for time-based updates and game logic processing. 
// The method calculates the delta time using the SFML clock and calls the update method of the current active scene, enabling smooth and consistent updates 
// regardless of frame rate variations. It also handles event polling and forwarding to ImGui and the current scene, as well as rendering the scene and ImGui.
void GameEngine::Update(float deltaTime) {
	// Handle events and input before updating the scene.
	while (m_window.isOpen()) {
		// Clear the window at the start of each frame. Use an explicit clear color so fully transparent tiles in the editor reveal the intended background instead
		// of an unintended grey fallback.
		m_window.clear(sf::Color::Transparent);

		// Clear the engine-wide render queue from the previous frame
		m_renderQueue.Clear();

		// Restart delta clock and update ImGui once per frame
		sf::Time frameTime = m_deltaClock.restart();
		if (ImGui::GetCurrentContext())
			ImGui::SFML::Update(m_window, frameTime);

		// Update shared FPS counter with the real frame time
		m_fpsCounter.Update(frameTime.asSeconds());

		// Poll events (SFML 3: pollEvent returns std::optional<sf::Event>) and forward to current scene
		while (auto eventOpt = m_window.pollEvent()) {
			// Forward events to ImGui-SFML so UI widgets receive input
			if (ImGui::GetCurrentContext())
				ImGui::SFML::ProcessEvent(m_window, *eventOpt);

			// Global Escape handling: intercept Escape before forwarding to the scene so scene-level handlers that close the window won't run. If Escape is 
			// pressed and we're not already on MainMenu, switch to it
			if (eventOpt->is<sf::Event::KeyPressed>()) {
				if (auto kp = eventOpt->getIf<sf::Event::KeyPressed>()) {
					if (static_cast<sf::Keyboard::Key>(kp->code) == sf::Keyboard::Key::Escape) {
						auto it = m_scenes.find("MainMenu");
						if (it != m_scenes.end() && m_currentScene && m_currentScene != it->second) {
							ChangeScene("MainMenu");
							// consume the event (do not forward to the current scene)
							continue;
						}
					}
				}
			}

			if (m_currentScene)
				m_currentScene->HandleEvent(eventOpt);
			if (eventOpt->is<sf::Event::Closed>())
				m_window.close();
		}

		// Update method will run  (carry out) these actions.
		m_InputController.Update(deltaTime);

		// Global keyboard poll: if Escape is held, switch back to MainMenu before any scene Update runs... This prevents scenes that poll sf::Keyboard::isKeyPressed(Escape) 
		// from closing the window directly.
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
			auto it = m_scenes.find("MainMenu");
			if (it != m_scenes.end() && m_currentScene && m_currentScene != it->second) {
				ChangeScene("MainMenu");
				// Reset input controller to new scene controller
				m_InputController.SetGameController(m_currentScene->GetGameController());
				// Skip running the previous scene's Update this frame
				continue;
			}
		}
		if (m_currentScene) {
			// Let the scene update (handles ImGui update and input)... Use the actual frame time measured above so scenes get accurate timing for FPS and logic.
			m_currentScene->Update(frameTime.asSeconds());

			// Ensure the scene's EntityManager processes game logic (tile system, pending entities)
			m_currentScene->GetEntityManager().Update(deltaTime);

			// Process sound effects using the SCENE's EntityManager (not global)
			// This ensures we process sounds for entities in the current active scene
			m_soundSystem->Process(m_currentScene->GetEntityManager(), deltaTime);
			m_soundSystem->Update(deltaTime);

			// Update the global cursor system
			m_cursorSystem->Update(deltaTime);

			// Execute any main-thread GL/upload tasks queued by worker threads. These must run while the
			// main thread's OpenGL context is current to avoid context activation errors.
			std::vector<std::function<void()>> _mainThreadTasks;
			MainThreadTaskQueue::Instance().Drain(_mainThreadTasks);
			for (auto &t : _mainThreadTasks) {
				try { t(); } catch (...) {}
			}

			// Engine render pass ordering:
			// 1) Entity shapes (direct render for now to avoid queue lifetime issues with sprites)
			m_currentScene->GetEntityManager().RenderShapes();
			// 2) Scene overlays
			m_currentScene->Render();
			// 3) Flush queued overlays and shapes
			m_renderQueue.Flush(m_window);
			// 4) Entity text (direct render after queue flush so text appears on top)
			m_currentScene->GetEntityManager().RenderText();
			// 5) Custom cursor (on top of scene, below ImGui)
			m_cursorSystem->Render();
		}

		// Draw any debug overlays from the current scene before ImGui so they are visible
		//if (m_currentScene) m_currentScene->RenderDebugOverlay();

		// Flush the engine-wide render queue to the window before ImGui rendering
		// This ensures all scene and entity draws appear behind the UI
		m_renderQueue.Flush(m_window);

		// Render ImGui on top of everything (ImGui::SFML::Render without args uses current target)
		if (ImGui::GetCurrentContext() && (m_currentScene == nullptr || m_currentScene->IsImGuiEnabled())) {
			ImGui::SFML::Render(m_window);
		}

		m_window.display();
	}
}
/////////////////////////////////
