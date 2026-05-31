/////////////////////////////////
// GameController.cpp - implementation of the GameController class, which manages input actions for keyboard keys and mouse buttons in the game engine. The GameController class provides methods for adding input action mappings, retrieving actions based on input events, and static helper functions for 
// checking input states and defining common input keys.
/////////////////////////////////



/////////////////////////////////
// Includes.
#include "GameController.h"
#include <SFML/Window/Event.hpp>
#include <optional>
/////////////////////////////////



/////////////////////////////////
// GameController implementation. This class manages input actions for keyboard keys and mouse buttons, allowing for dynamic addition of input mappings and retrieval of actions based on user input events. It also provides static helper functions for checking input 
// states and defining common input keys.
GameController::GameController() : m_MouseMovedAction(nullptr) {}
/////////////////////////////////



/////////////////////////////////
// AddInputActionForKey - adds a new input action mapping for a keyboard key by taking a ButtonAction struct that associates an InputKey with its corresponding InputAction callback function, allowing for dynamic addition of keyboard input actions to the GameController. 
// This method will add the provided ButtonAction to the m_ButtonActions vector, enabling the GameController to recognize and execute the associated action when the specified key is pressed or released during user input events.
void GameController::AddInputActionForKey(const ButtonAction& buttionAction) {
	m_ButtonActions.push_back(buttionAction);
}
/////////////////////////////////



/////////////////////////////////
// ClearAll - clears all input action mappings for both keyboard keys and mouse buttons, allowing for resetting the GameController to a clean state and removing all existing input mappings when needed. This method will clear the m_ButtonActions vector,
void GameController::ClearAll() {
	m_ButtonActions.clear();
}
/////////////////////////////////



/////////////////////////////////
// GetActionForKey - retrieves the InputAction callback function associated with the specified InputKey by searching through the stored ButtonAction mappings, allowing for easy retrieval and execution of actions based on user input events involving keyboard keys. 
// If no matching key is found, it returns an empty InputAction.
InputAction GameController::GetActionForKey(InputKey key) {
	for (const auto& buttonAction : m_ButtonActions) {
		if (key == buttonAction.key) {
			return buttonAction.action;
		}
	}
	return InputAction();
}
/////////////////////////////////



/////////////////////////////////
// Static helper functions for checking input states and defining common input keys. These functions provide convenient ways to check if an input is pressed or released, and to define commonly used keys for actions such as movement and interaction.
bool GameController::IsPressed(InputState state) {
	return state != 0;
}
/////////////////////////////////



/////////////////////////////////
// IsReleased - checks if the input state represents a released state (i.e., not pressed). This function returns true if the state is equal to 0, indicating that the input is not currently active, and false otherwise.
bool GameController::IsReleased(InputState state) {
	return state == 0;
}
/////////////////////////////////



/////////////////////////////////
// ActionKey, CancelKey, LeftKey, RightKey, UpKey, DownKey - static helper functions that return the InputKey corresponding to common actions such as interaction (ActionKey), cancellation (CancelKey), and movement in four directions (LeftKey, RightKey, UpKey, DownKey). 
// These functions provide a centralized way to define and access commonly used keys for game actions.
InputKey GameController::ActionKey() {
	return sf::Keyboard::Key::F; // Add ::Key:: to access the enum member
}

InputKey GameController::CancelKey() {
	return sf::Keyboard::Key::Delete;
}

InputKey GameController::LeftKey() {
	return sf::Keyboard::Key::A;
}

InputKey GameController::RightKey() {
	return sf::Keyboard::Key::D;
}

InputKey GameController::UpKey() {
	return sf::Keyboard::Key::W;
}

InputKey GameController::DownKey() {
	return sf::Keyboard::Key::S;
}
/////////////////////////////////



/////////////////////////////////
// GetActionForMouseButton - retrieves the MouseInputAction callback function associated with the specified MouseButton by searching through the stored MouseButtonAction mappings, allowing for easy retrieval and execution of actions based on user input events involving mouse buttons.
MouseInputAction GameController::GetActionForMouseButton(MouseButton button) {
	for (const auto& buttonAction : m_MouseButtonActions) {
		if (button == buttonAction.mouseButton) {
			return buttonAction.mouseInputAction;
		}
	}
	return MouseInputAction();
}
/////////////////////////////////



/////////////////////////////////
// AddMouseButtonAction - adds a new input action mapping for a mouse button by taking a MouseButtonAction struct that associates a MouseButton with its corresponding MouseInputAction callback function, allowing for dynamic addition of mouse button input actions to the GameController.
void GameController::AddMouseButtonAction(const MouseButtonAction& mouseButtonAction) {
	m_MouseButtonActions.push_back(mouseButtonAction);
}
/////////////////////////////////



/////////////////////////////////
// LeftMouseButton and RightMouseButton - static helper functions that return the MouseButton corresponding to the left and right mouse buttons, respectively. These functions provide a centralized way to define and access commonly used mouse buttons for game actions.
MouseButton GameController::LeftMouseButton() {
	return sf::Mouse::Button::Left;
}

MouseButton GameController::RightMouseButton() {
	return sf::Mouse::Button::Right;
}
/////////////////////////////////
