// InputController.cpp - Implementation of the InputController class, responsible for handling user input events using SFML. The class manages a reference to the current GameController, which defines the mapping of input actions to game logic. The Update method polls for SFML events and triggers the appropriate actions based on user input, such as key presses, mouse movements, and window events.

// Includes.
#include "InputController.h"
#include <SFML/Window/Event.hpp>
#include <iostream> // Add this line for std::cout and std::endl

// Method definitions.
InputController::InputController() : m_CurrentController(nullptr), m_Quit(nullptr) {}

void InputController::Init(InputAction quitAction, sf::RenderWindow* window) {
	m_Quit = quitAction;
	m_window = window;
}

void InputController::Update(uint32_t deltaT) {
	while (const std::optional<sf::Event> event = m_window->pollEvent()) {
		// Handle window close event (SFML equivalent of SDL_QUIT)
		if (event->is<sf::Event::Closed>()) {
			if (m_Quit) // Add null check
			{
				m_Quit(deltaT, 1); // 1 for pressed state
			}
		}

		// Handle mouse moved event
		if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
			if (m_CurrentController) {
				if (MouseMovedAction action = m_CurrentController->GetMouseMovedAction()) {
					MousePosition position;
					position.xPos = mouseMoved->position.x;
					position.yPos = mouseMoved->position.y;
					action(position);
				}
			}
		}

		// Handle mouse button pressed
		if (const auto* mouseButton = event->getIf<sf::Event::MouseButtonPressed>()) {
			if (m_CurrentController) {
				MouseInputAction action =
					m_CurrentController->GetActionForMouseButton(static_cast<MouseButton>(mouseButton->button));

				MousePosition position;
				position.xPos = mouseButton->position.x;
				position.yPos = mouseButton->position.y;

				if (action) {
					InputState state = 1;	 // 1 for pressed state
					action(state, position); // 1 for pressed state
				}
			}
		}

		// Handle mouse button released
		if (const auto* mouseButton = event->getIf<sf::Event::MouseButtonReleased>()) {
			if (m_CurrentController) {
				MouseInputAction action =
					m_CurrentController->GetActionForMouseButton(static_cast<MouseButton>(mouseButton->button));

				MousePosition position;
				position.xPos = mouseButton->position.x;
				position.yPos = mouseButton->position.y;

				if (action) {
					InputState state = 0;	 // 0 for released state
					action(state, position); // 0 for released state
				}
			}
		}

		// Handle key pressed
		if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
			if (m_CurrentController) {
				std::cout << "Key Pressed Event Detected: " << static_cast<int>(keyPressed->code)
						  << std::endl; // Debugging output
				InputAction action = m_CurrentController->GetActionForKey(static_cast<InputKey>(keyPressed->code));
				if (action) {
					std::cout << "Key Pressed: " << static_cast<int>(keyPressed->code) << std::endl; // Debugging output
					action(deltaT, 1); // 1 for pressed state
				}
			}
		}

		// Handle key released
		if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
			if (m_CurrentController) {
				InputAction action = m_CurrentController->GetActionForKey(static_cast<InputKey>(keyReleased->code));
				if (action) {
					action(deltaT, 0); // 0 for released state
				}
			}
		}
	}
}

void InputController::SetGameController(GameController* controller) {
	m_CurrentController = controller;
}