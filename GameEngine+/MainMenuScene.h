/////////////////////////////////
// MainMenuScene.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Scene.h"
#include <vector>
#include <string>
/////////////////////////////////



/////////////////////////////////
// MainMenuScene class definition, derived from the base Scene class. This class represents the main menu scene of the game, allowing players to select different scenes to play. It includes methods for updating, rendering, handling events, and managing the scene lifecycle, 
// as well as a reference to the SFML RenderWindow for drawing the menu and a list of available scene names for selection.
class MainMenuScene : public Scene {
	/////////////////////////////////
	// Public interface
public:
	/////////////////////////////////
	// Constructor and destructor for the MainMenuScene class. The constructor initializes the scene with references to the game engine, render window, and entity manager, while the destructor handles any necessary cleanup when the scene is destroyed.
	MainMenuScene(GameEngine& engine, sf::RenderWindow& win, EntityManager& entityManager);
	~MainMenuScene() override;
	/////////////////////////////////



	/////////////////////////////////
	// Override methods from the base Scene class for updating, rendering, handling events, and managing the scene lifecycle. These methods define the specific behavior and content of the main menu scene, allowing players to interact with the menu and select different scenes to play.
	void Update(float deltaTime) override;
	void Render() override;
	void DoAction() override;
	/////////////////////////////////



	/////////////////////////////////
	// Override methods for handling events, entering and exiting the scene, loading and unloading resources, and initializing the game. These methods manage the lifecycle of the main menu scene, allowing it to respond to player input, set up necessary resources, and clean up when the scene is exited.
	void HandleEvent(const std::optional<sf::Event>& event) override;
	void OnEnter() override;
	void OnExit() override;
	void LoadResources() override;
	void UnloadResources() override;
	void InitializeGame(sf::Vector2u windowSize) override;
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables
private:
	/////////////////////////////////
	// Reference to the SFML RenderWindow for drawing the menu and handling events, as well as a list of available scene names for selection and an index to track the currently selected scene in the menu.
	sf::RenderWindow& m_window;
	/////////////////////////////////



	/////////////////////////////////
	// List of available scene names for selection in the main menu, allowing players to choose which scene to play. The selected index tracks the currently highlighted scene in the menu for navigation and selection purposes.
	std::vector<std::string> m_sceneNames;
	int m_selectedIndex = 0;
	// Cooldown after entering the scene so a held Escape from another scene doesn't immediately close the window
	float m_enterCooldown = 0.0f;
	// Only allow Escape-to-close once the key has been physically released after entering
	bool m_escapeArmed = false;
	/////////////////////////////////
};
/////////////////////////////////