// GameController.cpp

// Includes.
#include "GameController.h"

#include <SFML/Window/Event.hpp>
#include <optional>

// Method definitions.
GameController::GameController() : m_MouseMovedAction(nullptr) {}

void GameController::AddInputActionForKey(const ButtonAction& buttionAction) {
	m_ButtonActions.push_back(buttionAction);
}

void GameController::ClearAll() {
	m_ButtonActions.clear();
}

InputAction GameController::GetActionForKey(InputKey key) {
	for (const auto& buttonAction : m_ButtonActions) {
		if (key == buttonAction.key) {
			return buttonAction.action;
		}
	}
	return InputAction();
}

bool GameController::IsPressed(InputState state) {
	return state != 0;
}

bool GameController::IsReleased(InputState state) {
	return state == 0;
}

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

MouseInputAction GameController::GetActionForMouseButton(MouseButton button) {
	for (const auto& buttonAction : m_MouseButtonActions) {
		if (button == buttonAction.mouseButton) {
			return buttonAction.mouseInputAction;
		}
	}
	return MouseInputAction();
}

void GameController::AddMouseButtonAction(const MouseButtonAction& mouseButtonAction) {
	m_MouseButtonActions.push_back(mouseButtonAction);
}

MouseButton GameController::LeftMouseButton() {
	return sf::Mouse::Button::Left;
}

MouseButton GameController::RightMouseButton() {
	return sf::Mouse::Button::Right;
}
