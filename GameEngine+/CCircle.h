/////////////////////////////////
// CCircle.h - header file for the CCircle component in the GameEngine+ project
/////////////////////////////////



/////////////////////////////////
// Include guards and necessary headers for the CCircle component. We include the base CShape component, SFML graphics headers for shapes and rendering, and a custom Vec2 class for 2D vector operations.
#pragma once
#include "CShape.h"
#include <SFML/Graphics/Shape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>
#include "Vec2.h"
/////////////////////////////////
 


/////////////////////////////////
// CCircle component - represents a circle shape with properties and methods for drawing, movement, and collision handling
class CCircle : public CShape {
	/////////////////////////////////
	// Public data members for CCircle
public:
	sf::CircleShape m_circle; // SFML CircleShape object representing the visual circle shape
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// Protected method to apply position to the underlying SFML shape.
protected:
	void ApplyPosition(float x, float y) override { m_circle.setPosition(sf::Vector2f(x, y)); }
	/////////////////////////////////



	/////////////////////////////////
	// Public methods for shape manipulation and rendering
public:
	/////////////////////////////////
	// Constructors for the CCircle component. The default constructor initializes the circle with default properties
	CCircle() {
		m_circle = sf::CircleShape(3.f);
		m_midLength = 4.f;
		m_circle.setFillColor(sf::Color(200, 120, 80, 220));
		m_circle.setOrigin(sf::Vector2f(3.f, 3.f));
	}
	/////////////////////////////////
	


	/////////////////////////////////
	// Constructor with size parameter - initializes the circle with a specified radius
	CCircle(float size)
	{
		m_circle = sf::CircleShape(size);
		m_midLength = size + 1.f;
		// Ensure visible default styling: fill color and origin so transform position is the circle center
		m_circle.setFillColor(sf::Color(200, 120, 80, 220));
		m_circle.setOrigin(sf::Vector2f(size, size));
	}
	/////////////////////////////////



	/////////////////////////////////
	// GetHeight - returns the height of the circle shape, which is equivalent to the diameter (twice the radius).
	float GetHeight() const override {	return m_circle.getRadius() * 2.f; }
	/////////////////////////////////



	/////////////////////////////////
	// GetMidLength - returns the mid-length property of the circle shape, which is equivalent to the radius.
	// where the mid-length is defined as the distance from the center to the edge of the shape.
	float GetMidLength() const override { return m_midLength; }
	/////////////////////////////////



	/////////////////////////////////
	// GetRadius - returns the radius of the circle shape.
	float GetRadius() const override { return m_circle.getRadius(); }
	/////////////////////////////////



	/////////////////////////////////
	// GetWidth - returns the width of the circle shape, which is equivalent to the diameter (twice the radius). 
	float GetWidth() const override { return m_circle.getRadius() * 2.f;}
	/////////////////////////////////



	/////////////////////////////////
	// GetColor - returns the current fill color of the circle shape as an SFML Color object.
	sf::Color GetColor() const { return m_circle.getFillColor(); }
	sf::Shape& GetShape() override {
		return m_circle;
	}
	/////////////////////////////////



	/////////////////////////////////
	// GetCentrePoint - returns the center point of the circle shape as a Vec2 object. The center point is calculated as the position of the circle plus the radius in both x and y directions, since SFML circles are positioned at their top-left corner.
	Vec2 GetCentrePoint() const	override { return Vec2(m_circle.getPosition().x + m_circle.getRadius(), m_circle.getPosition().y + m_circle.getRadius()); }
	/////////////////////////////////



	/////////////////////////////////
	// SetColor - sets the fill color of the circle shape using RGBA values. The r, g, and b parameters are floats representing the red, green, and blue components of the color (in the range [0.0f, 255.0f]), while the alpha parameter is an integer representing the opacity (in the range [0, 255]).
	void SetColor(float r, float g, float b, int alpha) { m_circle.setFillColor(sf::Color(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), alpha)); }
	/////////////////////////////////



	/////////////////////////////////
	// SetRadius - sets the radius of the circle shape. This method updates the radius of the underlying SFML CircleShape object, which in turn affects the size and position of the circle when drawn.
	void SetRadius(float radius) override { m_circle.setRadius(radius); }
	/////////////////////////////////
};
/////////////////////////////////
