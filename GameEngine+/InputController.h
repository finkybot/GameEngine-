// ***** InputController.h *****
#pragma once

// Includes.
#include "InputAction.h"
#include "GameController.h"
#include <SFML/Graphics.hpp>

// Forward declarations.
class GameController;

// Class declaration.
class InputController {
	// ***** Private member variables for input handling and state management *****
private:
	InputAction m_Quit;
	GameController* m_CurrentController;
	sf::RenderWindow* m_window = nullptr;

	// ***** Public methods for initializing, updating, and managing input controllers *****
public:
	InputController(); // Constructor - initializes the input controller with default values and sets up the quit action and window reference for event handling

	void Init(
		InputAction quitAction,
		sf::RenderWindow*
			window); // Initializes the input controller with the specified quit action and window reference, allowing it to handle quit events and poll for input events from the given window
	void Update(
		uint32_t
			deltaT); // Updates the input controller by polling for SFML events and triggering the appropriate actions based on user input, such as key presses, mouse movements, and window events. It takes the delta time since the last update as a parameter for potential use in input handling logic (e.g., for timing-based input actions).
	void SetGameController(
		GameController*
			controller); // Sets the current GameController reference for this input controller, allowing it to access the input action mappings defined in the GameController and trigger the appropriate actions based on user input events.
};