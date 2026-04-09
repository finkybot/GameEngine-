// Main.cpp - Entry point of the game engine, responsible for initializing the game, creating the main window, handling the main game loop, and integrating ImGui for UI rendering.
#include <imgui/imgui.h>
#include <imgui/backends/imgui-SFML.h>

#include <SFML/Graphics.hpp>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/VideoMode.hpp>

#include <memory>
#include <optional>
#include <random>
#include <thread>

#include "Entity.h"
#include "EntityManager.h"
#include "EntityType.h"
#include "GameEngine.h"
#include "ImageManagementSystem.h"
#include "SpatialHashGrid.h"
#include "TestScene.h"
#include "Vec2.h"


/*	Entry point of the game engine, responsible for initializing the game, creating the main window, handling the main game loop, and integrating ImGui for UI rendering. I have moved the game initialization logic into the TestScene class, so this main function is now focused on setting up 
	the game engine, creating the main window, and starting the main loop. The code is now cleaner and more modular, with the TestScene class responsible for managing the game state and entity initialization and updating, while the main function handles the overall setup and execution 
	of the game engine.	*/
int main(int argc, char* argv[])
{
	// Create the game engine instance (singleton)
	GameEngine& gameEngine = GameEngine::GetInstance();

	gameEngine.Run(); // Start the main game loop.

	// Cleanup ImGui resources before exiting the application
	ImGui::SFML::Shutdown(); // Shutdown ImGui.
	return 0;
}
