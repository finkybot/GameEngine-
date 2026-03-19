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


/*	Entry point of the game engine, responsible for initializing the game, creating the main window, handling the main game loop, and integrating ImGui for UI rendering.
	I have moved the game initialization logic into the TestScene class, so this main function is now focused on setting up the game engine, creating the main window, and starting the main loop.
	The code is now cleaner and more modular, with the TestScene class responsible for managing the game state and entity initialization and updating, 
	while the main function handles the overall setup and execution of the game engine.	*/
int main(int argc, char* argv[])
{
	// Setup Game Engine and Entity Manager
	GameEngine& gameEngine = GameEngine::GetInstance();
	TestScene testScene(gameEngine, gameEngine.m_window);
	testScene.InitializeGame(gameEngine.m_windowSize, 5000);

	// Load tile map texture (Im working on a tile map rendering system 13/03/2026) 
	sf::Image image;
	std::filesystem::path imagePath = std::filesystem::absolute("assets\\adventure.png");
	image = IMS::LoadImage(imagePath.string());
	std::vector<sf::Texture> textures = IMS::CreateTileMap(0, 0, 32, 32, image);

	std::vector<sf::Sprite> sprites;

	for (size_t i = 0; i < textures.size(); ++i)
	{
		sprites.emplace_back(textures[i]);
	}

	/*	Position sprites in a grid pattern across the window, wrapping to new rows as needed, this is just for demonstration purposes.	*/
	int x = 0;	int y = 0;
	for (size_t i = 0; i < sprites.size(); ++i)
	{
		if (i % 32 == 0) 
		{
			x = 0; // Reset x to start of row
			y = (y + 32) % gameEngine.m_windowSize.y; // Move down by 32 pixels and wrap around vertically
		}
		sprites[i].setPosition({ static_cast<float>(x), static_cast<float>(y) }); // Example positions, adjust as needed
		x += 32; // Move to the next column
	}
	
	bool isPressed = false; 	// Flag to track if the spacebar is currently pressed, used to prevent multiple spawns per key pres, this shouldnt be here, I'll move it later

	/* Main Loop, game logic is handled in here once per frame */
	while (gameEngine.m_window.isOpen())
	{
		testScene.Update(0.016f); // Update the scene with a fixed delta time (16ms for ~60 FPS), I can calculate actual delta time using the deltaClock for variable time steps
	}

	// Shutdown ImGui
	ImGui::SFML::Shutdown();

	return 0;
}
