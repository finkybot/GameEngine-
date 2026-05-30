/////////////////////////////////
// CText.h
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the CText component. We include SFML graphics headers for color representation, as well as the base Component class for ECS architecture and a custom Vec2 class for 2D vector operations.
#pragma once
#include <SFML/Graphics/Color.hpp>
#include <string>
#include "Component.h"
#include "Vec2.h"
/////////////////////////////////



/////////////////////////////////
// CText component - represents a text element with properties for the text content, font, character size, color, alignment, visibility, and optional z-order for rendering control.
class CText : public Component {
	/////////////////////////////////
	// Public data members for CText.
public:
	/////////////////////////////////
	// Member variables
	std::string text;					// text to display
	std::string fontKey = "default";	// key used with FontManager
	unsigned int charSize = 18;			// pixel size
	sf::Color color = sf::Color::White; // font colour (color for dumb fucks) default to white
	Vec2 offset = {0, 0};				// local offset relative to entity position (if any)
	/////////////////////////////////



	/////////////////////////////////
	// Alignment options for text rendering. This determines how the text is aligned relative to the entity's position when rendered. The default alignment is Left, meaning the text will be rendered starting from the entity's position
	// and extending to the right. Center alignment will center the text on the entity's position, while Right alignment will render the text ending at the entity's position and extending to the left.
	enum class Align {
		Left,
		Center,
		Right
	} align = Align::Left; // horizontal alignment of text relative to position (default to left)
	/////////////////////////////////



	/////////////////////////////////
	// Visibility flag for the text. This determines whether the text should be rendered or not. If set to true, the text will be visible and rendered on the screen; if set to false, 
	// the text will be hidden and not rendered. The default value is true, meaning the text will be visible by default.
	bool visible = true;   // visibility flag, default to true
	float zOrder = 0.0f; // optional layering.... i'll probably need this at some point for rendering order control, but for now it can just default to 0.0f and we can implement layering logic in the render system later if needed.
	/////////////////////////////////



	/////////////////////////////////
	// Constructors for the CText component.
	/////////////////////////////////
	


	/////////////////////////////////
	// Default constructor - initializes the text component with default properties.
	CText() = default;
	/////////////////////////////////



	/////////////////////////////////
	// Constructor with parameters - initializes the text component with specified text, font key, character size, and color. The font key is used to look up the font in the FontManager, 
	// while the character size determines the pixel size of the rendered text. The color parameter allows for setting a custom color for the text, overriding the default white color.
	explicit CText(const std::string& t, const std::string& fk = "default", unsigned int sz = 18)
		: text(t), fontKey(fk), charSize(sz) {}
	/////////////////////////////////



	/////////////////////////////////
	// Constructor with color parameter - initializes the text component with specified text, color, font key, and character size. This constructor allows for setting a custom color for the text while also specifying the font and size.
	explicit CText(const std::string& t, const sf::Color& col, const std::string& fk = "default", unsigned int sz = 18)
		: text(t), fontKey(fk), charSize(sz), color(col) {}
	/////////////////////////////////
};
/////////////////////////////////