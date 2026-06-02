/////////////////////////////////
// InputController.cpp - Implementation of the InputController class, responsible for handling user input events using SFML. The class manages a reference to the current GameController, which defines the mapping of input actions to game logic. 
// The Update method polls for SFML events and triggers the appropriate actions based on user input, such as key presses, mouse movements, and window events.
/////////////////////////////////



/////////////////////////////////
// Includes.
#include "InputController.h"
#include <SFML/Window/Event.hpp>
#include <iostream> // Add this line for std::cout and std::endl
/////////////////////////////////



/////////////////////////////////
// Constructor for the InputController class. Initializes member variables to default values, preparing the input controller for handling user input events.
InputController::InputController() : m_CurrentController(nullptr), m_Quit(nullptr) {}
/////////////////////////////////



/////////////////////////////////
// Init - initializes the input controller with a quit action callback and a reference to the SFML render window. This method sets up the necessary state for the input controller to handle quit events and poll for input events from the specified window.
void InputController::Init(InputAction quitAction, sf::RenderWindow* window) {
	m_Quit = quitAction;
	m_window = window;
}
/////////////////////////////////



/////////////////////////////////
// IsKeyboardEnabled - returns true when keyboard input should be processed (only while the window has focus). This method checks if the window reference is valid and if the window currently has focus, indicating that keyboard input should be accepted.
bool InputController::IsKeyboardEnabled() const {
	if (!m_window) return false;
	return m_window->hasFocus();
}
/////////////////////////////////



/////////////////////////////////
// IsPointerInsideWindow - returns true when the pointer is currently inside the window client area. This method checks if the window reference is valid and retrieves the current mouse position relative to the window, comparing it against the window size 
// to determine if the pointer is within the bounds of the window.
bool InputController::IsPointerInsideWindow() const {
	if (!m_window) return false;
	// getPosition(window) returns position relative to the window client area
	sf::Vector2i pos = sf::Mouse::getPosition(*m_window);
	sf::Vector2u size = m_window->getSize();
	return pos.x >= 0 && pos.y >= 0 && pos.x < static_cast<int>(size.x) && pos.y < static_cast<int>(size.y);
}
/////////////////////////////////



/////////////////////////////////
// IsMouseEnabled - returns true when mouse input should be processed (window focused or pointer over window). This method checks if the window reference is valid and accepts mouse input if either the window has focus or the pointer is currently inside 
// the window, allowing for more flexible mouse input handling.
bool InputController::IsMouseEnabled() const {
	if (!m_window) return false;
	// Accept mouse input if either window has focus OR the pointer is inside the window
	return m_window->hasFocus() || IsPointerInsideWindow();
}
/////////////////////////////////



/////////////////////////////////
// IsMouseButtonDown - returns true if the specified mouse button is down and mouse input is enabled. This method checks if mouse input is currently enabled and then uses SFML's isButtonPressed function to check the state of the specified mouse button.
bool InputController::IsMouseButtonDown(sf::Mouse::Button button) const {
	return IsMouseEnabled() && sf::Mouse::isButtonPressed(button);
}
/////////////////////////////////



/////////////////////////////////
// Update - updates the input controller by polling for SFML events and triggering the appropriate actions based on user input, such as key presses, mouse movements, and window events. 
// It takes the delta time since the last update as a parameter for potential use in input handling logic (e.g., for timing-based input actions).
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
/////////////////////////////////



/////////////////////////////////
// SetGameController - sets the current GameController reference for this input controller, allowing it to access the input action mappings defined in the GameController and trigger the appropriate actions based on user input events.
void InputController::SetGameController(GameController* controller) {
	m_CurrentController = controller;
}
/////////////////////////////////