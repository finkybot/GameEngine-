/////////////////////////////////
// CRectangle.cpp
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the CRectangle implementation. This includes necessary headers for the game engine, SFML graphics, and standard library components.
#include "CRectangle.h"
#include <SFML/Graphics/Color.hpp>
#include <string>
/////////////////////////////////



/////////////////////////////////
// CRectangle implementation. This class represents a rectangle shape component in the game engine, providing functionality for setting size, color, position, and retrieving properties such as width, 
// height, and center point. It inherits from the CShape base class and uses SFML's RectangleShape for rendering and collision detection.
CRectangle::CRectangle() {
	m_rectangle = sf::RectangleShape(sf::Vector2f(10.f, 10.f));
	m_midLength = 10.f / 2.f + 1.f;
}
/////////////////////////////////



/////////////////////////////////
// Parameterized constructor - initializes the rectangle shape with specified width and height, and calculates the mid-length property based on the larger dimension for use in collision detection and quadtree inclusion.
CRectangle::CRectangle(float x, float y) {
	m_rectangle = sf::RectangleShape(sf::Vector2f(x, y));
	m_midLength = ((x >= y) ? x : y) + 1.f;
}
/////////////////////////////////



/////////////////////////////////
// SetSize - sets the size of the rectangle shape using the provided width and height parameters. This method updates the underlying SFML RectangleShape's size, which affects rendering and collision detection.
void CRectangle::SetSize(float width, float height) {
	m_rectangle.setSize(sf::Vector2f(width, height));
}
/////////////////////////////////



/////////////////////////////////
// SetColor - sets the fill color of the rectangle shape using RGBA values. The red, green, and blue parameters are expected to be in the range [0, 255], and the alpha parameter controls the opacity of the shape.
void CRectangle::SetColor(float r, float g, float b, int alpha) {
    m_rectangle.setFillColor(sf::Color(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), alpha));
}
/////////////////////////////////



/////////////////////////////////
// ApplyPosition - applies the specified x and y coordinates to the position of the underlying SFML RectangleShape. This method is called by the CShape base class to update the shape's position in the game world.
void CRectangle::ApplyPosition(float x, float y) {
	m_rectangle.setPosition(sf::Vector2f(x, y));
}
/////////////////////////////////



/////////////////////////////////
// GetCentrePoint - calculates and returns the center point of the rectangle shape. This method is useful for positioning, collision detection, and other spatial calculations within the game world.
Vec2 CRectangle::GetCentrePoint() const {
	return Vec2(m_rectangle.getPosition().x + m_rectangle.getSize().x * 0.5f,
				m_rectangle.getPosition().y + m_rectangle.getSize().y * 0.5f);
}
/////////////////////////////////