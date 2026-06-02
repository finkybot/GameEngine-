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
// Load default cursor (Issue Solved)
	// Note: SFML 3's sf::Cursor cannot be default-constructed or copied reliably, so we use optional unique_ptrs to manage them. 
	// We load system cursors for Default, Pointer, and Crosshair modes. If loading fails, we log an error and reset the pointer to ensure we don't use an invalid cursor later.
	if (!m_defaultCursor) {
		m_defaultCursor = sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow);
		m_crosshairCursor = sf::Cursor::createFromSystem(sf::Cursor::Type::Cross);
		m_pointerCursor = sf::Cursor::createFromSystem(sf::Cursor::Type::Hand);
		
		if (m_defaultCursor && m_crosshairCursor && m_pointerCursor) {
			std::cout << "Successfully loaded cursors\n";
		} else {
			std::cerr << "Failed to load cursors\n";
			m_defaultCursor.reset();
			m_crosshairCursor.reset();
			m_pointerCursor.reset();
		}
	}

	// Load eyedropper texture for software cursor rendering in Crosshair mode
	//if (!m_eyeDropperTexture) {
	//	m_eyeDropperTexture = std::make_unique<sf::Texture>();
	//	if (m_eyeDropperTexture->loadFromFile("assets/cursors/eyedropper_small.png")) {
	//		// Create sprite with the loaded texture
	//		m_eyeDropperSprite.reset(new sf::Sprite(*m_eyeDropperTexture));
	//		m_eyeDropperSprite->setOrigin(sf::Vector2f(0.0f, 0.0f)); // Top-left origin
	//		m_cursorHotspot = sf::Vector2f(0.0f, 0.0f); // Hotspot at top-left
	//		std::cout << "Successfully loaded eyedropper cursor texture\n";
	//	} else {
	//		std::cerr << "Failed to load eyedropper cursor from assets/cursors/eyedropper_small.png\n";
	//		m_eyeDropperTexture.reset();
	//	}
	//}
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

	

	switch (m_mode) {
	case Mode::Default:
		//m_window->setMouseCursorVisible(true);
		m_window->setMouseCursor(*m_defaultCursor);
		break;
	case Mode::Pointer:
		//m_window->setMouseCursorVisible(true);
		m_window->setMouseCursor(*m_pointerCursor);
		break;
	case Mode::Crosshair:
		// Hide OS cursor for eyedropper mode
		//m_window->setMouseCursorVisible(false);
		m_window->setMouseCursor(*m_crosshairCursor);

		break;
	default:
		m_window->setMouseCursor(*m_defaultCursor);
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
