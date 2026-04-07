#pragma once
#include <SFML/Graphics/Color.hpp>
#include <string>
#include "Component.h"
#include "Vec2.h"

// Simple text component (pure data)
class CText : public Component
{
public:
    std::string text;                                               // text to display
    std::string fontKey = "default";                                // key used with FontManager
    unsigned int charSize = 18;                                     // pixel size
    sf::Color color = sf::Color::White;                             // font colour (color for dumb fucks) default to white
    Vec2 offset = { 0,0 };                                          // local offset relative to entity position (if any)
	enum class Align { Left, Center, Right } align = Align::Left;   // horizontal alignment of text relative to position (default to left)
	bool visible = true;                                            // visibility flag, default to true
	float zOrder = 0.0f;                                            // optional layering.... i'll probably need this at some point for rendering order control, but for now it can just default to 0.0f and we can implement layering logic in the render system later if needed.

    CText() = default;
    explicit CText(const std::string& t, const std::string& fk = "default", unsigned int sz = 18) : text(t), fontKey(fk), charSize(sz) {}
	explicit CText(const std::string& t, const sf::Color& col, const std::string& fk = "default", unsigned int sz = 18) : text(t), fontKey(fk), charSize(sz), color(col) {}
};