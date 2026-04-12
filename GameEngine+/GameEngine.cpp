// GameEngine.cpp - Implementation of the GameEngine class, responsible for managing the game loop, scenes, and rendering using SFML
#include "GameEngine.h"
#include "FontManager.h"
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <memory>
#include <string>
#include "Scene.h"
#include "TestScene.h"
#include "TileMapScene.h"
#include "TileMapEditorScene.h"
#include "MusicVisualizerScene.h"
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>
#include <cstdlib>

GameEngine::GameEngine()
{
    // Setup the SFML window as borderless (fullscreen-windowed) to avoid exclusive fullscreen quirks
	m_windowSize = sf::VideoMode::getDesktopMode().size;
	// Create a borderless window sized to the desktop resolution and position it at (0,0)
	m_window.create(sf::VideoMode(m_windowSize), "SFML Game Engine", sf::Style::None);
	m_window.setPosition(sf::Vector2i(0, 0));

	m_window.setFramerateLimit(1000);
	//m_window.setVerticalSyncEnabled(true);	
	m_isRunning = true;

	// Create a single engine-wide EntityManager and bind shared resources
	m_entityManager = std::make_unique<EntityManager>(m_window);
	m_entityManager->GetRenderSystem().SetFontManager(&m_fontManager);

    // Do not preload atlases automatically. Atlases should be loaded explicitly via the editor UI so users
	// can choose which atlas to use at runtime.

	// Initialize ImGui-SFML early so scenes can safely call ImGui during Update
	if (!ImGui::SFML::Init(m_window)) {
		std::cerr << "Warning: Failed to initialize ImGui::SFML in GameEngine" << std::endl;
		// continue without ImGui but scenes must tolerate absence
	}
}

GameEngine::~GameEngine()
{}

void GameEngine::AddScene(const std::string & sceneName, std::shared_ptr<Scene> scene)
{
	m_scenes[sceneName] = scene;
}

void GameEngine::ChangeScene(const std::string & sceneName)
{
	auto it = m_scenes.find(sceneName);
	if (it != m_scenes.end())
	{
		m_currentScene = it->second;
	}
	else
	{
		std::cerr << "Scene '" << sceneName << "' not found!" << std::endl;
	}
}

void GameEngine::RemoveScene(const std::string& sceneName)
{
	auto it = m_scenes.find(sceneName);
	if (it != m_scenes.end())
	{
		m_scenes.erase(it);
	}
}

void GameEngine::Run()
{
	// Setup Event Handler.
	bool running = true; // Create a Boolean variable to manage the engine running state

	// Going to run a test scene for now, will add a main menu and other scenes later once the scene management system is more fleshed out.
    AddScene("TestScene", std::make_shared<TestScene>(*this, m_window, *m_entityManager)); // Adding TestScene
	AddScene("TileMapScene", std::make_shared<TileMapScene>(*this, m_window, *m_entityManager)); // Adding TileMapScene
	AddScene("TileMapEditor", std::make_shared<TileMapEditorScene>(*this, m_window, *m_entityManager)); // Adding TileMapEditor
    AddScene("MusicVisualizer", std::make_shared<MusicVisualizerScene>(*this, m_window, *m_entityManager)); // Adding MusicVisualizer
    //ChangeScene("TestScene");
    ChangeScene("MusicVisualizer");
	m_currentScene->InitializeGame(m_windowSize);

    // FontManager already bound to engine-owned EntityManager in constructor
	m_InputController.SetGameController(m_currentScene->GetGameController());

	m_InputController.Init([&running](uint32_t deltaT, InputState state) { running = false; std::cout << "Quitting" << std::endl; }, &m_window); // The defined function will be called when we quit the game..

	/* Main Loop, game logic is handled in here once per frame */
	while (m_window.isOpen())
	{
		Update(0.016f); // Update the scene with a fixed delta time (16ms for ~60 FPS), I can calculate actual delta time using the deltaClock for variable time steps
	}
}

void GameEngine::Update(float deltaTime)
{
	// Handle events and input before updating the scene.
	while (m_window.isOpen())
	{
        // Clear the window at the start of each frame
		m_window.clear();

		// Restart delta clock and update ImGui once per frame
		sf::Time frameTime = m_deltaClock.restart();
		if (ImGui::GetCurrentContext()) ImGui::SFML::Update(m_window, frameTime);

		// Update shared FPS counter with the real frame time
		m_fpsCounter.Update(frameTime.asSeconds());

		// Poll events (SFML 3: pollEvent returns std::optional<sf::Event>) and forward to current scene
		while (auto eventOpt = m_window.pollEvent())
		{
            // Forward events to ImGui-SFML so UI widgets receive input
			if (ImGui::GetCurrentContext()) ImGui::SFML::ProcessEvent(m_window, *eventOpt);

			if (m_currentScene) m_currentScene->HandleEvent(eventOpt);
			if (eventOpt->is<sf::Event::Closed>()) m_window.close();
		}
		
        // Update method will run  (carry out) these actions.
		m_InputController.Update(deltaTime);

		if (m_currentScene)
		{
			// Let the scene update (handles ImGui update and input)
			// Use the actual frame time measured above so scenes get accurate timing for FPS and logic.
			m_currentScene->Update(frameTime.asSeconds());

			// Ensure the scene's EntityManager processes game logic (tile system, pending entities)
			m_currentScene->GetEntityManager().Update(deltaTime);

			// Engine render pass ordering:
			// 1) Entity shapes
			m_currentScene->GetEntityManager().RenderShapes();
			// 2) Scene overlays
			m_currentScene->Render();
			// 3) Entity text (render on top of overlays)
			m_currentScene->GetEntityManager().RenderText();
		}

        // Draw any debug overlays from the current scene before ImGui so they are visible
		//if (m_currentScene) m_currentScene->RenderDebugOverlay();

        // Render ImGui on top of everything (ImGui::SFML::Render without args uses current target)
		if (ImGui::GetCurrentContext() && (m_currentScene == nullptr || m_currentScene->IsImGuiEnabled())) {
			ImGui::SFML::Render(m_window);
		}

		m_window.display();
	}
}
