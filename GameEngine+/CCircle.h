// ***** CCircle.h - Header file for CCircle class, Component for circle shapes *****
#pragma once
#include "CShape.h"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>
#include "Vec2.h"

// / CCircle component - represents a circle shape with properties and methods for drawing, movement, and collision handling
class CCircle : public CShape
{
	//  ***** Protected member variables for shape-specific properties *****
protected:
    sf::CircleShape m_circle;
    void ApplyPosition(float x, float y) override { m_circle.setPosition(sf::Vector2f(x, y)); }

	//  ***** Public methods for shape manipulation and rendering *****
public:
	CCircle();              // Default constructor - initializes the circle with default properties
	CCircle(float size);    // Constructor with size parameter - initializes the circle with a specified radius

	void DrawShape(sf::RenderWindow& window) override;                      // Draw the circle shape to the SFML render window
	void Includer(sf::RenderWindow& window) override;                       // Include the circle shape in the quadtree for spatial partitioning (implementation depends on how the quadtree is structured, likely involves adding the circle's bounding box to the quadtree)
	void SetTextPosition(float fontOffset) override;                        // Set the position of the text component relative to the circle shape (implementation depends on how the text is structured, likely involves setting the text position based on the circle's position and radius)
	void SetRadius(float radius) override { m_circle.setRadius(radius); }   // Set the radius of the circle shape
	void SetColor(float r, float g, float b, int alpha);                    // Set the fill color of the circle shape using RGBA values (alpha is an integer in the range [0, 255])
	sf::Color GetColor() const { return m_circle.getFillColor(); }          // Get the current fill color of the circle shape as an SFML Color object
	float GetMidLength() const override { return m_midLength; }             // Get the mid-length property, which for a circle is equivalent to the radius
	float GetWidth() const override { return m_circle.getRadius() * 2.f; }  // Get the width (diameter) of the circle, which is twice the radius
	float GetHeight() const override { return m_circle.getRadius() * 2.f; } // Get the height (diameter) of the circle, which is twice the radius
	Vec2 GetCentrePoint() const override;									// Get the center point of the circle shape, which is the position plus the radius in both x and y directions
	float GetRadius() const override { return m_circle.getRadius(); }		// Get the radius of the circle shape
};


