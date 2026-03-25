// ***** CExplosion.h - header file for the CExplosion component, which represents an explosion shape with properties and methods for drawing, movement, and collision handling *****
#pragma once
#include "CShape.h"
#include <SFML/Graphics/Shape.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>
#include "Vec2.h"

// / CExplosion component - represents an explosion shape with properties and methods for drawing, movement, and collision handling
class CExplosion : public CShape
{
	// ***** Public member variables for the explosion shape *****
public:
	sf::CircleShape m_circle; // SFML CircleShape object representing the visual circle shape

	//  ***** Protected member variables for shape-specific properties *****
protected:
	void ApplyPosition(float x, float y) override { m_circle.setPosition(sf::Vector2f(x, y)); }

	//  ***** Public methods for shape manipulation and rendering *****
public:
	CExplosion();															// Default constructor - initializes the circle with default properties
	CExplosion(float size);													// Constructor with size parameter - initializes the circle with a specified radius

	float GetHeight() const override { return m_circle.getRadius() * 2.f; } // Get the height (diameter) of the circle, which is twice the radius
	float GetMidLength() const override { return m_midLength; }             // Get the mid-length property, which for a circle is equivalent to the radius
	float GetRadius() const override { return m_circle.getRadius(); }		// Get the radius of the circle shape
	float GetWidth() const override { return m_circle.getRadius() * 2.f; }  // Get the width (diameter) of the circle, which is twice the radius

	sf::Color GetColor() const { return m_circle.getFillColor(); }          // Get the current fill color of the circle shape as an SFML Color object
	sf::Shape& GetShape()  override { return m_circle; }					// Get a reference to the underlying SFML shape (used for drawing and collision detection)
	Vec2 GetCentrePoint() const override;									// Get the center point of the circle shape, which is the position plus the radius in both x and y directions

	void SetColor(float r, float g, float b, int alpha);                    // Set the fill color of the explosion shape using RGBA values (alpha is an integer in the range [0, 255])
	void SetRadius(float radius) override { m_circle.setRadius(radius); }   // Set the radius of the explosion shape
};