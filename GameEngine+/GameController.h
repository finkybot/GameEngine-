/////////////////////////////////
// GameController.h
/////////////////////////////////



/////////////////////////////////
// Include guards and necessary headers for the GameController class. We include the InputAction header for defining input action types, vector for storing input mappings, and SFML Event header for handling input events.
#pragma once
#include "InputAction.h"
#include <vector>

#include <SFML/Window/Event.hpp>
/////////////////////////////////
 
 

/////////////////////////////////
//	|	GameController class - manages input action mappings for keyboard keys and mouse buttons, allowing for dynamic assignment of input actions and retrieval of actions based on user input events. 
//	|	It also provides static helper functions for checking input states and defining common input keys for game actions.
//	|___________________________________________________________________________________
class GameController {
private:
	std::vector<ButtonAction> m_ButtonActions;				// Vector to store ButtonAction mappings, associating InputKeys with their corresponding InputAction callback functions for keyboard input events.			
	std::vector<MouseButtonAction>	m_MouseButtonActions;	// Vector to store MouseButtonAction mappings, associating MouseButtons with their corresponding MouseInputAction callback functions for mouse button input events.
	MouseMovedAction m_MouseMovedAction;					// Member variable to store the callback function for handling mouse movement events, allowing for responsive and interactive input handling based on the user's mouse position when handling mouse movement events.



public:
	GameController();												// Constructor for the GameController class.
	
	void AddInputActionForKey(const ButtonAction& buttonAction);	// Adds a new input action mapping by taking a ButtonAction struct that associates an InputKey with its corresponding InputAction callback function.
	void ClearAll();												// Clears all stored input action mappings for both keyboard keys and mouse buttons, allowing for resetting the GameController to a clean state and removing all previously defined input actions.
	InputAction GetActionForKey(InputKey key);						// Retrieves the InputAction callback function associated with the specified InputKey by searching through the stored ButtonAction mappings, if no mapping is found for the given key, it returns an empty InputAction.

	// IsPressed and IsReleased - static helper functions to check the state of an InputState value, allowing for simplified checks of whether a key or button is currently pressed (non-zero value) or released (zero value) when handling user input events.
	static bool IsPressed(InputState state);	// Check if the input state is pressed (non-zero value)
	static bool IsReleased(InputState state);	// Check if the input state is released (zero value)

	// Static helper functions to define common input keys for game actions, allowing for easy reference to these keys when defining input mappings and handling user input events. These functions return the InputKey representing the specific key commonly used for the
	// associated action in games, such as the action key (e.g., spacebar), cancel key (e.g., escape), and movement keys (e.g., WASD or arrow keys).
	static InputKey ActionKey(); // Define the action key (e.g., spacebar) for game actions
	static InputKey CancelKey(); // Define the cancel key (e.g., escape) for game actions
	static InputKey LeftKey();	 // Define the left movement key (e.g., A or left arrow) for game actions
	static InputKey RightKey();	 // Define the right movement key (e.g., D or right arrow) for game actions
	static InputKey UpKey();	 // Define the up movement key (e.g., W or up arrow) for game actions
	static InputKey DownKey();	 // Define the down movement key (e.g., S or down arrow) for game actions

	// GetMouseMovedAction and SetMouseMovedAction - inline methods to retrieve and set the MouseMovedAction callback function for mouse movement events
	inline const MouseMovedAction& GetMouseMovedAction() { return m_MouseMovedAction; }										// Retrieve the MouseMovedAction callback function for mouse movement events
	inline void SetMouseMovedAction(const MouseMovedAction& mouseMovedAction) {	m_MouseMovedAction = mouseMovedAction; }	// Set the MouseMovedAction callback function for mouse movement events

	MouseInputAction GetActionForMouseButton(MouseButton button);			// Searches MouseButtonAction mappings and retrieves callback function associated with the specified MouseButton, if no mapping is found for the given button, it returns an empty MouseInputAction.
	void AddMouseButtonAction(const MouseButtonAction&mouseButtonAction);	// Adds a new mouse button action mapping by taking a MouseButtonAction struct that associates a MouseButton with its corresponding MouseInputAction callback function.

	static MouseButton LeftMouseButton();	// Define the left mouse button for game actions
	static MouseButton RightMouseButton();	// Define the right mouse button for game actions
};
/////////////////////////////////