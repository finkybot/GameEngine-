/////////////////////////////////
// CCircle.cpp - implementation of the CCircle component, which represents a circular shape in the game engine. This component inherits from CShape and provides specific implementations for circle shapes using SFML's CircleShape class. The CCircle component includes methods 
// for getting shape properties such as height, mid-length, radius, and width, as well as methods for getting the center point and setting the color of the circle. The ApplyPosition method is implemented to update the position of the underlying SFML CircleShape based on the 
// provided x and y coordinates, but overall its a component in an ECS architecture, so it primarily manages the properties of the circle shape and provides methods for manipulating those properties as needed by the game logic and rendering systems.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "CCircle.h"
#include <SFML/Graphics/Color.hpp>
#include <string>
/////////////////////////////////



/////////////////////////////////
// CCircle implementation
CCircle::CCircle() {
	m_circle = sf::CircleShape(3.f);
	m_midLength = 4.f;
	m_circle.setFillColor(sf::Color(200, 120, 80, 220));
	m_circle.setOrigin(sf::Vector2f(3.f, 3.f));
}
/////////////////////////////////



/////////////////////////////////
// CCircle constructor with size parameter. This constructor initializes the circle shape with a specified radius, sets the mid-length property to the radius plus one for collision detection purposes, and applies default styling to ensure the circle is visible when rendered.
CCircle::CCircle(float size) {
	m_circle = sf::CircleShape(size);
	m_midLength = size + 1.f;
	// Ensure visible default styling: fill color and origin so transform position is the circle center
	m_circle.setFillColor(sf::Color(200, 120, 80, 220));
	m_circle.setOrigin(sf::Vector2f(size, size));
}
/////////////////////////////////



/////////////////////////////////
// GetCentrePoint - returns the center point of the circle shape as a Vec2 object. The center point is calculated as the position of the circle plus the radius in both x and y directions, since SFML circles are positioned at their top-left corner.
Vec2 CCircle::GetCentrePoint() const {
	return Vec2(m_circle.getPosition().x + m_circle.getRadius(), m_circle.getPosition().y + m_circle.getRadius());
}

void CCircle::SetColor(float r, float g, float b, int alpha) {
	m_circle.setFillColor(sf::Color(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), alpha));
}