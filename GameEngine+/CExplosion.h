/////////////////////////////////
// CExplosion.h - Header file for the CExplosion component, which represents an explosion shape in the game engine
/////////////////////////////////



/////////////////////////////////
// Include guards and necessary headers for the CExplosion component. We include the base CShape component, SFML graphics headers for shapes and rendering, and a custom Vec2 class for 2D vector operations.
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
// CExplosion component - represents an explosion shape with properties and methods for drawing, movement, and collision handling
class CExplosion : public CShape {
	/////////////////////////////////
	// Public data members for CExplosion
public:
	sf::CircleShape m_circle; // SFML CircleShape object representing the visual circle shape
	/////////////////////////////////



	/////////////////////////////////
	// Protected member variables for shape-specific properties
protected:
	void ApplyPosition(float x, float y) override { m_circle.setPosition(sf::Vector2f(x, y)); }
	/////////////////////////////////



	/////////////////////////////////
	// Public methods for shape manipulation and rendering
public:
	/////////////////////////////////
	// Constructors for the CExplosion component. The default constructor initializes the circle with default properties, while the constructor with a size parameter initializes the circle with a specified radius.
	CExplosion();			// Default constructor - initializes the circle with default properties
	CExplosion(float size); // Constructor with size parameter - initializes the circle with a specified radius
	/////////////////////////////////



	/////////////////////////////////
	// GetHeight - returns the height of the circle shape, which is equivalent to the diameter (twice the radius).
	float GetHeight() const override { return m_circle.getRadius() * 2.f; }
	/////////////////////////////////



	/////////////////////////////////
	// GetMidLength - returns the mid-length property of the circle shape, which is equivalent to the radius.
	float GetMidLength() const override { return m_midLength; }



	/////////////////////////////////
	// GetRadius - returns the radius of the circle shape, which is used for circular collision detection and quadtree inclusion.
	float GetRadius() const override { return m_circle.getRadius(); }
	/////////////////////////////////



	/////////////////////////////////
	// GetWidth - returns the width of the circle shape, which is equivalent to the diameter (twice the radius).
	float GetWidth() const override { return m_circle.getRadius() * 2.f; }
	/////////////////////////////////



	/////////////////////////////////
	// GetColor - returns the current fill color of the circle shape as an SFML Color object.
	sf::Color GetColor() const { return m_circle.getFillColor(); }
	/////////////////////////////////



	/////////////////////////////////
	// GetShape - returns a reference to the underlying SFML shape object, which is used for drawing and collision detection.
	sf::Shape& GetShape() override { return m_circle; } 	
	/////////////////////////////////
	
	
	
	/////////////////////////////////
	// GetCentrePoint - returns the center point of the circle shape, which is the position plus the radius in both x and y directions. This is used for collision detection and spatial hashing.
	Vec2 GetCentrePoint() const	override;
	/////////////////////////////////



	/////////////////////////////////
	// SetColor - sets the fill color of the explosion shape using RGBA values, where alpha is an integer in the range [0, 255]. This method updates the fill color of the underlying SFML CircleShape object.
	void SetColor(float r, float g, float b, int alpha);
	/////////////////////////////////



	/////////////////////////////////
	//	SetRadius - sets the radius of the explosion shape. This method updates the radius of the underlying SFML CircleShape object and also updates the origin to keep the shape centered as it expands.
	void SetRadius(float radius) override {
		m_circle.setRadius(radius);
		m_circle.setOrigin(sf::Vector2f(radius, radius));
	}
	/////////////////////////////////
};
/////////////////////////////////
