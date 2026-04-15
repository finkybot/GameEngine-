// ***** GameController.h *****
#pragma once

// Includes.
#include "InputAction.h"
#include <vector>

#include <SFML/Window/Event.hpp>

// Game Controller is responsible for managing the mapping of input keys and mouse buttons to their corresponding actions.
// It allows for adding new input actions, retrieving actions based on input keys or mouse buttons, and clearing all input mappings.
// The GameController also provides static methods for checking input states (pressed or released) and defines commonly used input keys and mouse buttons for game actions.
class GameController {
	// ***** Private member variables for input action mappings *****
private:
	std::vector<ButtonAction>
		m_ButtonActions; // Vector to store mappings of keyboard keys to their corresponding input actions, allowing for easy retrieval and execution of actions based on user input events involving keyboard keys.
	std::vector<MouseButtonAction>
		m_MouseButtonActions; // Vector to store mappings of mouse buttons to their corresponding mouse input actions, allowing for easy retrieval and execution of actions based on user input events involving mouse buttons.
	MouseMovedAction
		m_MouseMovedAction; // Member variable to store the action callback for mouse movement events, allowing for responsive and interactive input handling based on the user's mouse position when handling mouse movement events.

	// ***** Public methods for managing input action mappings and retrieving actions based on input events *****
public:
	GameController(); // Constructor - initializes the GameController with default values and prepares it for managing input action mappings.

	void AddInputActionForKey(
		const ButtonAction&
			buttionAction); // Adds a new input action mapping for a keyboard key by taking a ButtonAction struct that associates an InputKey with its corresponding InputAction callback function, allowing for dynamic addition of keyboard input actions to the GameController.
	void
	ClearAll(); // Clears all input action mappings for both keyboard keys and mouse buttons, allowing for resetting the GameController to a clean state and removing all existing input mappings when needed.

	InputAction GetActionForKey(
		InputKey
			key); // Retrieves the InputAction callback function associated with the specified InputKey by searching through the stored ButtonAction mappings, allowing for easy retrieval and execution of actions based on user input events involving keyboard keys. If no mapping is found for the given key, it returns an empty InputAction.

	static bool IsPressed(
		InputState
			state); // Static helper function to check if the given InputState represents a pressed state (non-zero value), returning true if the input is currently active and false otherwise. This function can be used throughout the game engine to simplify input state checks and improve code readability when handling user input events.
	static bool IsReleased(
		InputState
			state); // Static helper function to check if the given InputState represents a released state (zero value), returning true if the input is currently inactive and false otherwise. This function can be used throughout the game engine to simplify input state checks and improve code readability when handling user input events.

	static InputKey
	ActionKey(); // Static method to return the InputKey representing the action key (e.g., spacebar) commonly used for performing actions in the game, allowing for easy reference to this key when defining input mappings and handling user input events.
	static InputKey
	CancelKey(); // Static method to return the InputKey representing the cancel key (e.g., escape) commonly used for canceling actions or opening menus in the game, allowing for easy reference to this key when defining input mappings and handling user input events.

	static InputKey
	LeftKey(); // Static method to return the InputKey representing the left movement key (e.g., A or left arrow) commonly used for moving left in the game, allowing for easy reference to this key when defining input mappings and handling user input events.
	static InputKey
	RightKey(); // Static method to return the InputKey representing the right movement key (e.g., D or right arrow) commonly used for moving right in the game, allowing for easy reference to this key when defining input mappings and handling user input events.
	static InputKey
	UpKey(); // Static method to return the InputKey representing the up movement key (e.g., W or up arrow) commonly used for moving up in the game, allowing for easy reference to this key when defining input mappings and handling user input events.
	static InputKey
	DownKey(); // Static method to return the InputKey representing the down movement key (e.g., S or down arrow) commonly used for moving down in the game, allowing for easy reference to this key when defining input mappings and handling user input events.

	inline const MouseMovedAction& GetMouseMovedAction() {
		return m_MouseMovedAction;
	} // Inline method to retrieve the MouseMovedAction callback function for mouse movement events, allowing for easy access to this action when handling mouse movement events in the game engine. It returns a const reference to the stored MouseMovedAction, which can be used to trigger the appropriate response based on the user's mouse position when handling mouse movement events.
	inline void SetMouseMovedAction(const MouseMovedAction& mouseMovedAction) {
		m_MouseMovedAction = mouseMovedAction;
	} // Inline method to set the MouseMovedAction callback function for mouse movement events, allowing for dynamic assignment of this action based on game logic or user input. It takes a const reference to a MouseMovedAction and assigns it to the member variable m_MouseMovedAction, enabling responsive and interactive input handling based on the user's mouse position when handling mouse movement events.

	MouseInputAction GetActionForMouseButton(
		MouseButton
			button); // Retrieves the MouseInputAction callback function associated with the specified MouseButton by searching through the stored MouseButtonAction mappings, allowing for easy retrieval and execution of actions based on user input events involving mouse buttons. If no mapping is found for the given button, it returns an empty MouseInputAction.
	void AddMouseButtonAction(
		const MouseButtonAction&
			mouseButtonAction); // Adds a new mouse button action mapping by taking a MouseButtonAction struct that associates a MouseButton with its corresponding MouseInputAction callback function, allowing for dynamic addition of mouse button input actions to the GameController.

	static MouseButton
	LeftMouseButton(); // Static method to return the MouseButton representing the left mouse button, commonly used for primary actions in the game, allowing for easy reference to this button when defining input mappings and handling user input events involving mouse buttons.
	static MouseButton
	RightMouseButton(); // Static method to return the MouseButton representing the right mouse button, commonly used for secondary actions in the game, allowing for easy reference to this button when defining input mappings and handling user input events involving mouse buttons.
};