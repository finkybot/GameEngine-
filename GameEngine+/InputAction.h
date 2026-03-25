// ***** InputAction.h - Abstraction layer for SFML inputs. *****
#pragma once

// Includes.
#include <functional>
#include <stdint.h>
#include <SFML/Window/Event.hpp>

// Using defines.
using InputKey = sf::Keyboard::Key;	                                // Defined InputKey as SFML Keyboard Key, which represents the key code of a keyboard key and can be used to identify which key was pressed or released in user input events.
using InputState = uint8_t;	                                        // Defined InputState as an unsigned 8-bit integer, which can be used to represent the state of an input (e.g., pressed or released) in a compact form for use in input action callbacks and event handling.
using MouseButton = sf::Mouse::Button;	                            // Defined MouseButton as SFML Mouse Button, which represents the button code of a mouse button and can be used to identify which mouse button was pressed or released in user input events.

using InputAction = std::function<void(uint32_t, InputState)>;      // Defined InputAction as a std::function that takes a uint32_t representing delta time and an InputState representing the state of the input (e.g., pressed or released), and returns void. This type can be used to define callback functions for handling specific input actions in response to user input events, allowing for flexible and customizable input handling logic in the game engine.

// Struct to represent a button action mapping, associating an InputKey with its corresponding InputAction callback function. This struct can be used to define the mapping of keyboard keys to their respective actions in the GameController, allowing for easy retrieval and execution of actions based on user input events.
struct ButtonAction
{
    InputKey key;
    InputAction action;
};

// Helper functions to check state
namespace InputHelper // Namespace to encapsulate helper functions for checking input states, providing a clear and organized way to determine whether an input is currently pressed or released based on its InputState value. These functions can be used throughout the game engine to simplify input state checks and improve code readability when handling user input events.
{
	inline InputState PressedState() { return 1; }                      // Returns the InputState value representing a pressed state, which can be used in input action callbacks and event handling to indicate that an input is currently active (e.g., a key is being held down or a mouse button is pressed).
	inline InputState ReleasedState() { return 0; }                     // Returns the InputState value representing a released state, which can be used in input action callbacks and event handling to indicate that an input is currently inactive (e.g., a key is not being held down or a mouse button is released).
	inline bool IsPressed(InputState state) { return state != 0; }      // Helper function to check if the given InputState represents a pressed state (non-zero value), returning true if the input is currently active and false otherwise.
}

// Struct to represent the position of the mouse cursor, containing x and y coordinates. This struct can be used in mouse input action callbacks to provide the current position of the mouse cursor when handling mouse movement or button events, allowing for responsive and interactive input handling based on the user's mouse position.
struct MousePosition
{
	int32_t xPos, yPos; // x and y coordinates of the mouse cursor position, represented as 32-bit signed integers to accommodate a wide range of screen resolutions and coordinate values.
};

using MouseMovedAction = std::function<void(const MousePosition& mousePosition)>;				// Defined MouseMovedAction as a std::function that takes a const reference to a MousePosition struct representing the current position of the mouse cursor, and returns void. This type can be used to define callback functions for handling mouse movement events in response to user input, allowing for responsive and interactive input handling based on the user's mouse position.
using MouseInputAction = std::function<void(InputState& state, const MousePosition& position)>;	// Defined MouseInputAction as a std::function that takes a reference to an InputState representing the state of the input (e.g., pressed or released) and a const reference to a MousePosition struct representing the current position of the mouse cursor, and returns void. This type can be used to define callback functions for handling mouse button events in response to user input, allowing for responsive and interactive input handling based on the user's mouse position.

// Struct to represent a mouse button action mapping, associating a MouseButton with its corresponding MouseInputAction callback function. This struct can be used to define the mapping of mouse buttons to their respective actions in the GameController, allowing for easy retrieval and execution of actions based on user input events involving mouse buttons.
struct MouseButtonAction
{
    MouseButton mouseButton;
    MouseInputAction mouseInputAction;
};