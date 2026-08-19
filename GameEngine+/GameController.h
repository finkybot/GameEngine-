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
// GameController class - manages input action mappings for keyboard keys and mouse buttons, allowing for dynamic assignment of input actions and retrieval of actions based on user input events. 
// It also provides static helper functions for checking input states and defining common input keys for game actions.
//					|
//					|___________________________________________________________________________________
class GameController {
	/////////////////////////////////
	// Private member variables for storing input action mappings and mouse movement action.
private:
	/////////////////////////////////
	// Vector to store mappings of keyboard and mouse button inputs to their corresponding action callbacks, allowing for easy retrieval and execution of actions based on user input events.
	// The m_ButtonActions vector stores mappings for keyboard keys, while the m_MouseButtonActions vector stores mappings for mouse buttons. The m_MouseMovedAction member variable stores
	// the callback function for handling mouse movement events, allowing for responsive and interactive input handling based on the user's mouse position when handling mouse movement events.
	std::vector<ButtonAction> m_ButtonActions;
	std::vector<MouseButtonAction> m_MouseButtonActions;
	MouseMovedAction m_MouseMovedAction;
	/////////////////////////////////



	/////////////////////////////////
	// Public interface for the GameController class, including constructor and methods for managing input action mappings, retrieving actions based on input events, and static helper functions for checking input states and defining common input keys.
public:
	/////////////////////////////////
	// Constructor for the GameController class. Initializes the GameController with default values and prepares it for managing input action mappings.
	GameController();
	/////////////////////////////////



	/////////////////////////////////
	// AddInputActionForKey - adds a new input action mapping for a keyboard key by taking a ButtonAction struct that associates an InputKey with its corresponding InputAction callback function, allowing for dynamic addition of keyboard input actions to the GameController.
	// This method will add the provided ButtonAction to the m_ButtonActions vector, enabling the GameController to recognize and execute the associated action when the specified key is pressed or released during user input events.
	void AddInputActionForKey(const ButtonAction& buttonAction);
	/////////////////////////////////



	/////////////////////////////////
	// ClearAll - clears all input action mappings for both keyboard keys and mouse buttons, allowing for resetting the GameController to a clean state and removing all existing input mappings when needed. This method will clear the m_ButtonActions vector,
	// the m_MouseButtonActions vector, and reset the m_MouseMovedAction to an empty state, effectively removing all input mappings from the GameController.
	void ClearAll();
	/////////////////////////////////



	/////////////////////////////////
	// GetActionForKey - retrieves the InputAction callback function associated with the specified InputKey by searching through the stored ButtonAction mappings, allowing for easy retrieval and execution of actions based on user input events involving keyboard keys.
	InputAction GetActionForKey(InputKey key);
	/////////////////////////////////



	/////////////////////////////////
	// IsPressed and IsReleased - static helper functions to check the state of an InputState value, allowing for simplified checks of whether a key or button is currently pressed (non-zero value) or released (zero value) when handling user input events.
	static bool IsPressed(InputState state);
	static bool IsReleased(InputState state);
	/////////////////////////////////



	/////////////////////////////////
	// Static helper functions to define common input keys for game actions, allowing for easy reference to these keys when defining input mappings and handling user input events. These functions return the InputKey representing the specific key commonly used for the
	// associated action in games, such as the action key (e.g., spacebar), cancel key (e.g., escape), and movement keys (e.g., WASD or arrow keys).
	static InputKey ActionKey();
	static InputKey CancelKey();
	static InputKey LeftKey();
	static InputKey RightKey();
	static InputKey UpKey();
	static InputKey DownKey();
	/////////////////////////////////



	/////////////////////////////////
	// GetMouseMovedAction and SetMouseMovedAction - inline methods to retrieve and set the MouseMovedAction callback function for mouse movement events, allowing for easy access and dynamic assignment of this action based on game logic or user input.
	inline const MouseMovedAction& GetMouseMovedAction() { return m_MouseMovedAction; }
	inline void SetMouseMovedAction(const MouseMovedAction& mouseMovedAction) { m_MouseMovedAction = mouseMovedAction; }
	/////////////////////////////////



	/////////////////////////////////
	// GetActionForMouseButton - retrieves the MouseInputAction callback function associated with the specified MouseButton by searching through the stored MouseButtonAction mappings, allowing for easy retrieval and execution of actions based on user
	// input events involving mouse buttons. If no mapping is found for the given button, it returns an empty MouseInputAction.
	MouseInputAction GetActionForMouseButton(MouseButton button);
	/////////////////////////////////



	/////////////////////////////////
	// AddMouseButtonAction - adds a new mouse button action mapping by taking a MouseButtonAction struct that associates a MouseButton with its corresponding MouseInputAction callback function, allowing for dynamic addition of mouse button input actions to the GameController.
	void AddMouseButtonAction(const MouseButtonAction& mouseButtonAction);
	/////////////////////////////////



	/////////////////////////////////
	// Static helper functions to define common mouse buttons for game actions, allowing for easy reference to these buttons when defining input mappings and handling user input events involving mouse buttons. These functions return the MouseButton representing the specific button
	// commonly used for the associated action in games, such as the left mouse button for primary actions and the right mouse button for secondary actions.
	static MouseButton LeftMouseButton();
	static MouseButton RightMouseButton();
	/////////////////////////////////
};
/////////////////////////////////