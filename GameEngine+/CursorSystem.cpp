/////////////////////////////////
// CursorSystem.cpp
/////////////////////////////////



/////////////////////////////////
// Includes and namespace aliases for the CursorSystem implementation. We include necessary headers for SFML graphics and the CursorSystem class definition.
#include "CursorSystem.h"
#include <SFML/Window.hpp>
#include <SFML/Graphics/Image.hpp>
#include <iostream>

#include <SFML/Window/Cursor.hpp>
#pragma message("Cursor constructor is: " SFML_VERSION_MAJOR "." SFML_VERSION_MINOR "." SFML_VERSION_PATCH)
/////////////////////////////////


/////////////////////////////////
// Initialize the cursor system, including loading cursor textures and setting up any necessary state. This method will be called during the game's initialization phase to prepare the cursor for use.
void CursorSystem::Initialize(sf::RenderWindow* window) {
	m_window = window;
	LoadCursors();
	SetMode(Mode::Default);
	Update(0.0f);
}
/////////////////////////////////



/////////////////////////////////
// Load cursor textures for different cursor states (e.g., default, pointer, crosshair). This method will handle loading the necessary textures from disk and storing them for use when rendering the cursor.
void CursorSystem::LoadCursors() {

	std::cout << "Loading cursors...\n";

	// Load eyedropper texture for software cursor rendering in Crosshair mode
	if (!m_eyeDropperTexture) {
		m_eyeDropperTexture = std::make_unique<sf::Texture>();
		if (m_eyeDropperTexture->loadFromFile("assets/cursors/eyedropper_small.png")) {
			// Create sprite with the loaded texture
			m_eyeDropperSprite.reset(new sf::Sprite(*m_eyeDropperTexture));
			m_eyeDropperSprite->setOrigin(sf::Vector2f(0.0f, 0.0f)); // Top-left origin
			m_cursorHotspot = sf::Vector2f(0.0f, 0.0f); // Hotspot at top-left
			std::cout << "Successfully loaded eyedropper cursor texture\n";
		} else {
			std::cerr << "Failed to load eyedropper cursor from assets/cursors/eyedropper_small.png\n";
			m_eyeDropperTexture.reset();
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// Set the current cursor mode (e.g., default, pointer, crosshair). This method will allow other parts of the game to change the cursor's appearance based on the context (e.g., hovering over an interactive object).
void CursorSystem::SetMode(Mode mode) {
	m_mode = mode;
	Update(0.0f); // Update cursor immediately to reflect mode change
}
/////////////////////////////////



/////////////////////////////////
// Update method for the CursorSystem class. This method will handle updating the cursor's position based on mouse input and any necessary state changes (e.g., changing cursor appearance based on context).
void CursorSystem::Update(float deltaTime) {
	if (!m_window)
		return; // Ensure we have a valid window pointer

	// For now, we'll use a simple approach: toggle mouse visibility based on mode.
	// A full cursor system with OS-level cursor changes requires static cursors or custom rendering.
	// Mode::Crosshair (eyedropper) will hide the OS cursor so we can render a custom one if needed.

	switch (m_mode) {
	case Mode::Default:
	case Mode::Pointer:
		m_window->setMouseCursorVisible(true);
		break;
	case Mode::Crosshair:
		// Hide OS cursor for eyedropper mode
		m_window->setMouseCursorVisible(false);

		break;
	default:
		m_window->setMouseCursorVisible(true);
		break;
	}

}
/////////////////////////////////



/////////////////////////////////
// Render method for the CursorSystem class. This method will handle drawing the cursor on top of the game scene, ensuring that it is rendered in the correct position and with the appropriate appearance. 
// In Crosshair mode, renders a custom eyedropper sprite at the mouse position.
void CursorSystem::Render() {
	// Only render custom cursor if we're in Crosshair mode and have a valid window and sprite
	if (!m_window || m_mode != Mode::Crosshair) return;
	if (!m_eyeDropperSprite) return;

	// Save the current view and switch to UI/screen coordinates
	sf::View prev = m_window->getView();
	m_window->setView(m_window->getDefaultView());

	// Get mouse position in screen pixels and update sprite position
	sf::Vector2i mousePixel = sf::Mouse::getPosition(*m_window);
	sf::Vector2f screenPos = sf::Vector2f(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

	// Adjust by hotspot so the sprite appears at the correct click point
	m_eyeDropperSprite->setPosition(screenPos - m_cursorHotspot);

	// Draw the eyedropper sprite
	m_window->draw(*m_eyeDropperSprite);

	// Restore the previous view
	m_window->setView(prev);
}
/////////////////////////////////
