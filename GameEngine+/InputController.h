/////////////////////////////////
// InputController class declaration, responsible for handling user input events and managing the current GameController reference to trigger appropriate actions based on user input.
/////////////////////////////////



/////////////////////////////////
// Includes for the InputController class. We include necessary headers for input actions, game controller management, and SFML graphics for handling window events and input polling.
#pragma once

#include "InputAction.h"
#include "GameController.h"
#include <SFML/Graphics.hpp>
/////////////////////////////////



/////////////////////////////////
// Forward declaration of the GameController class
class GameController;
/////////////////////////////////



/////////////////////////////////
// InputController class declaration, responsible for handling user input events and managing the current GameController reference to trigger appropriate actions based on user input.
class InputController {
	/////////////////////////////////
	// Private member variables
private:
	/////////////////////////////////
	// Member variable to store the quit action callback function, which will be triggered when a quit event is detected (e.g., window close event). This allows the InputController to handle 
	// quit events and perform necessary cleanup or state changes when the user attempts to close the game window.
	InputAction m_Quit;
	GameController* m_CurrentController;
	sf::RenderWindow* m_window = nullptr;
	/////////////////////////////////

	

	/////////////////////////////////
	// Public interface
public:
	/////////////////////////////////
	// Constructor for the InputController class. Initializes the input controller with default values and sets up the quit action and window reference for event handling.
	InputController();
	/////////////////////////////////



	/////////////////////////////////
	// Init - initializes the input controller with a quit action callback and a reference to the SFML render window. This method sets up the necessary state for the input controller to handle quit events and poll for input events from the specified window.
	void Init(InputAction quitAction, sf::RenderWindow*	window); 
	/////////////////////////////////



	/////////////////////////////////
	// Update - updates the input controller by polling for SFML events and triggering the appropriate actions based on user input, such as key presses, mouse movements, and window events. It takes the delta time since the last update as a parameter for potential use in input handling logic (e.g., for timing-based input actions).
	void Update(uint32_t deltaT);
	/////////////////////////////////



	/////////////////////////////////
	// SetGameController - sets the current GameController reference for this input controller, allowing it to access the input action mappings defined in the GameController and trigger the appropriate actions based on user input events.
	void SetGameController(GameController* controller);
	/////////////////////////////////



	/////////////////////////////////
	// IsKeyboardEnabled - returns true when keyboard input should be processed (only while the window has focus).
	bool IsKeyboardEnabled() const;
	/////////////////////////////////



	/////////////////////////////////
	// IsMouseEnabled - returns true when mouse input should be processed (window focused or pointer over window).
	bool IsMouseEnabled() const;
	/////////////////////////////////



	/////////////////////////////////
	// IsPointerInsideWindow - returns true when the pointer is currently inside the window client area.
	bool IsPointerInsideWindow() const;
	/////////////////////////////////



	/////////////////////////////////
	// IsMouseButtonDown - returns true if the specified mouse button is down and mouse input is enabled.
	bool IsMouseButtonDown(sf::Mouse::Button button) const;
	/////////////////////////////////
};
/////////////////////////////////