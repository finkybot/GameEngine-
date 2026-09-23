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
//	|	InputController class declaration, responsible for handling user input events and managing the current GameController reference to trigger appropriate actions based on user input.
//	|___________________________________________________________________________________
class InputController {
private:
	InputAction m_Quit;
	GameController* m_CurrentController;
	sf::RenderWindow* m_window = nullptr;



public:
	InputController();
	void Init(InputAction quitAction, sf::RenderWindow*	window);	//  Initializes the input controller with a quit action callback and a reference to the SFML render window; sets up the necessary state for the input controller to handle quit events and poll for input events from the specified window.
	void Update(uint32_t deltaT);									// Updates the input controller by polling for SFML events and triggering the appropriate actions based on user input, e.g., key presses, mouse movements, window events.
	
	void SetGameController(GameController* controller);				// Sets the current GameController reference for this input controller, allowing it to access the input action mappings defined in the GameController and trigger the appropriate actions based on user input events.
	bool IsKeyboardEnabled() const;									// Returns true when keyboard input should be processed (only while the window has focus).
	bool IsMouseEnabled() const;									// Returns true when mouse input should be processed (window focused or pointer over window).
	bool IsPointerInsideWindow() const;								// Returns true when the pointer is currently inside the window client area.
	
	bool IsMouseButtonDown(sf::Mouse::Button button) const;			// Returns true if the specified mouse button is down and mouse input is enabled.
};
/////////////////////////////////