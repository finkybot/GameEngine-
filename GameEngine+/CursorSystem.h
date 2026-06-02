/////////////////////////////////
// CursorSystem.h: minimal cursor system implementation for the game engine. This class will manage the cursor's appearance and behavior, allowing for different cursor states (e.g., default, pointer, crosshair) based on the context of the game. 
// The system will handle loading cursor textures, updating the cursor position based on mouse input, and rendering the cursor on top of the game scene.
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include<SFML/Graphics.hpp>
#include <optional>
/////////////////////////////////




/////////////////////////////////
// CursorSystem class - manages the cursor's appearance and behavior in the game engine.
class CursorSystem {
	/////////////////////////////////
public:
	/////////////////////////////////
	// Enumeration for different cursor modes. This enum defines the various states the cursor can be in, such as default, pointer, and crosshair. The cursor's appearance will change based on the current mode.
	enum class Mode {
		Default,  // Default cursor mode
		Pointer,  // Pointer cursor mode (e.g., when hovering over interactive objects)
		Crosshair // Crosshair cursor mode (e.g., for aiming)
	};
	/////////////////////////////////



	/////////////////////////////////
	// Constructor and destructor for the CursorSystem class. The constructor initializes the cursor system with a reference to the render window, while the destructor can be used for any necessary cleanup (e.g., releasing resources).
	CursorSystem() = default;
	/////////////////////////////////



	/////////////////////////////////
	// Initialize the cursor system, including loading cursor textures and setting up any necessary state. This method will be called during the game's initialization phase to prepare the cursor for use.
	void Initialize(sf::RenderWindow* window);
	/////////////////////////////////



	/////////////////////////////////
	// Load cursor textures for different cursor states (e.g., default, pointer, crosshair). This method will handle loading the necessary textures from disk and storing them for use when rendering the cursor.
	void LoadCursors();
	/////////////////////////////////



	/////////////////////////////////
	// Set the current cursor mode (e.g., default, pointer, crosshair). This method will allow other parts of the game to change the cursor's appearance based on the context (e.g., hovering over an interactive object).
	Mode GetMode() const { return m_mode; }
	/////////////////////////////////



	/////////////////////////////////
	// Set the current cursor mode (e.g., default, pointer, crosshair). This method will allow other parts of the game to change the cursor's appearance based on the context (e.g., hovering over an interactive object).
	void SetMode(Mode mode);
	/////////////////////////////////



	/////////////////////////////////
	// Update method for the CursorSystem class. This method will handle updating the cursor's position based on mouse input and any necessary state changes (e.g., changing cursor appearance based on context).
	void Update(float deltaTime);
	/////////////////////////////////



	/////////////////////////////////
	// Render method for the CursorSystem class. This method will handle drawing the cursor on top of the game scene, ensuring that it is rendered in the correct position and with the appropriate appearance.
	void Render();
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the CursorSystem class.
private:
	/////////////////////////////////
	// Pointer to the render window for rendering the cursor. This allows the cursor system to draw the cursor on top of the game scene using the SFML rendering context.
	sf::RenderWindow* m_window = nullptr; // Pointer to the render window for rendering the cursor
	Mode m_mode = Mode::Default;		  // Current cursor mode (e.g., default, pointer, crosshair)

	// For custom eyedropper cursor rendering (software-drawn cursor)		
	std::unique_ptr<sf::Texture> m_eyeDropperTexture;
	std::unique_ptr<sf::Sprite> m_eyeDropperSprite;
	sf::Vector2f m_cursorHotspot = sf::Vector2f(0.0f, 0.0f); // Offset from mouse position
	/////////////////////////////////

	// Note: sf::Cursor in SFML 3 cannot be default-constructed/copied reliably here.
	// We manage cursor appearance via software rendering and visibility toggles.



	/////////////////////////////////
	std::optional<sf::Cursor> m_defaultCursor;
	std::optional<sf::Cursor> m_crosshairCursor;
	std::optional<sf::Cursor> m_pointerCursor;
	/////////////////////////////////



};
/////////////////////////////////
