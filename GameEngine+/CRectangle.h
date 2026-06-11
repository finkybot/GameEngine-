/////////////////////////////////
// CRectangle.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "CShape.h"
#include "Vec2.h"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>
#include <string>
/////////////////////////////////



/////////////////////////////////
// CRectangle component -	| Represents a rectangle shape in the game, with properties for size, color, and position. This class inherits from CShape and 
//							| implements the necessary methods to manipulate and render a rectangle shape using SFML.
//							|___________________________________________________________________________________
class CRectangle : public CShape {
	/////////////////////////////////
	// Private member variable for the underlying SFML RectangleShape.
private:
	sf::RectangleShape m_rectangle;
	/////////////////////////////////



	/////////////////////////////////
	// Protected method to apply position to the underlying SFML shape.
protected:
	void ApplyPosition(float x, float y) override { m_rectangle.setPosition({x, y}); }
	/////////////////////////////////



	/////////////////////////////////
	// Public methods for manipulating the rectangle shape, including constructors, size and color setters, 
	// and property getters.
public:
	/////////////////////////////////
	// The default constructor initializes the rectangle with a default size.
	CRectangle() {
		m_rectangle = sf::RectangleShape(sf::Vector2f(10.f, 10.f));
		m_midLength = 10.f / 2.f + 1.f;
	}
	/////////////////////////////////



	/////////////////////////////////
	// Parameterized constructor  - Initializes the rectangle with specified width and height, and calculates the  mid-length 
	// property based on the larger dimension for use in collision detection and quadtree inclusion.
	CRectangle(float x, float y) {
		m_rectangle = sf::RectangleShape(sf::Vector2f(x, y));
		m_midLength = ((x >= y) ? x : y) + 1.f;
	}
	/////////////////////////////////



	/////////////////////////////////
	// SetSize - Set explicit size (width, height) for the rectangle.
	void SetSize(float width, float height) { m_rectangle.setSize(sf::Vector2f(width, height)); }
	/////////////////////////////////



	/////////////////////////////////
	// GetCentrePoint -	Calculates and returns the center point of the rectangle shape for use in spatial partitioning 
	// and collision detection. This method overrides the pure virtual method from the CShape base class.
	Vec2 GetCentrePoint() const override {
		sf::Vector2f pos = m_rectangle.getPosition();
		sf::Vector2f size = m_rectangle.getSize();
		return Vec2(pos.x + size.x / 2.f, pos.y + size.y / 2.f);
	}
	/////////////////////////////////



	/////////////////////////////////
	// GetHeight -	GetMidLength, GetRadius, and GetWidth - these methods return the height, mid-length, radius 
	// (which is the width for a rectangle), and width of the rectangle shape, respectively. These 
	// properties are used for rendering, collision detection, and spatial partitioning.
	float GetHeight() const override { return m_rectangle.getSize().y; }
	float GetMidLength() const override { return m_midLength; }
	float GetRadius() const override { return m_rectangle.getSize().x; }
	float GetWidth() const override { return m_rectangle.getSize().x; }
	/////////////////////////////////



	/////////////////////////////////
	// GetColor - Return the current fill color of the rectangle shape
	// as an SFML Color object. This method allows for querying the 
	// current color of the shape
	sf::Color GetColor() const { return m_rectangle.getFillColor();	} 
	/////////////////////////////////
	


	/////////////////////////////////
	// GetShape - Returns a reference to the underlying SFML RectangleShape.
	sf::Shape& GetShape() override {
		return m_rectangle;
	}
	/////////////////////////////////



	/////////////////////////////////
	// SetColor - Sets the fill color of the rectangle shape using RGBA values. The red, green, and blue parameters are expected to be in the range [0, 255]
	void SetColor(float r, float g, float b, int alpha) { m_rectangle.setFillColor(sf::Color(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), alpha)); }
	/////////////////////////////////



	/////////////////////////////////
	// SetRadius - Sets the radius of the rectangle shape, which is equivalent to setting the width 
	// for a rectangle. This method overrides the pure virtual method from the CShape base class.
	void SetRadius(float radius) override { m_rectangle.setSize(sf::Vector2f(radius, radius)); }
	/////////////////////////////////
};
/////////////////////////////////


