// GameEngine.cpp - Implementation of the GameEngine class, responsible for managing the game loop, scenes, and rendering using SFML
#include "GameEngine.h"
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <memory>
#include <string>
#include "Scene.h"
#include <cstdlib>

GameEngine::GameEngine()
{
	// Setup the SFML window
	m_windowSize = sf::VideoMode::getDesktopMode().size;
	m_window.create(sf::VideoMode(m_windowSize), "SFML Game Engine");

	//m_window->setFramerateLimit(240);
	m_window.setVerticalSyncEnabled(true);	
	m_isRunning = true;

	// Initialize ImGui with SFML backend
	if (!ImGui::SFML::Init(m_window))
	{
		std::cerr << "Failed to initialize ImGui::SFML." << std::endl;
		std::exit(EXIT_FAILURE);
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
{}

void GameEngine::update(float deltaTime)
{}
