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
#include <cstdlib>

GameEngine::GameEngine()
{
	// Setup the SFML window
	m_windowSize = sf::VideoMode::getDesktopMode().size;
	m_window.create(sf::VideoMode(m_windowSize), "SFML Game Engine", sf::State::Fullscreen);

	m_window.setFramerateLimit(1000);
	//m_window.setVerticalSyncEnabled(true);	
	m_isRunning = true;

	// Create a single engine-wide EntityManager and bind shared resources
	m_entityManager = std::make_unique<EntityManager>(m_window);
	m_entityManager->GetRenderSystem().SetFontManager(&m_fontManager);

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
	//ChangeScene("TestScene");
	ChangeScene("TileMapScene");
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

		// Poll events (SFML 3: pollEvent returns std::optional<sf::Event>) and forward to current scene
		while (auto eventOpt = m_window.pollEvent())
		{
			if (m_currentScene) m_currentScene->HandleEvent(eventOpt);
			if (eventOpt->is<sf::Event::Closed>()) m_window.close();
		}
		
		// Update method will run  (carry out) these actions.
		m_InputController.Update(deltaTime);

		if (m_currentScene)
		{
          m_currentScene->Update(deltaTime);
			// Engine render pass ordering:
			// 1) Entity shapes
			m_currentScene->GetEntityManager().RenderShapes();
			// 2) Scene overlays
			m_currentScene->Render();
			// 3) Entity text (render on top of overlays)
			m_currentScene->GetEntityManager().RenderText();
		}

		m_window.display();
	}
}
