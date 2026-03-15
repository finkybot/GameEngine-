// ***** CShape.h - CShape class definition *****
#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include "Vec2.h"
#include "Component.h"

// / CShape component - base class for shape components
class CShape : public Component
{
	//  ***** Public data members (ECS pure data component) *****
public:
    Vec2 m_position{}; // cached world position
	Vec2 m_velocity{}; // velocity for movement (used by PhysicsSystem)
	float m_midLength{}; // mid-length property for shape extent (e.g. half-width for rectangles, radius for circles)

	// ***** Protected methods for derived classes to implement shape-specific logic *****
protected:
	virtual void ApplyPosition(float x, float y) = 0; // Apply position to the underlying SFML shape (implemented by derived classes)

	// ***** Public methods *****
public:
	CShape(); // Constructor - initializes position, velocity, and mid-length to default values
	virtual ~CShape(); // Virtual destructor for proper cleanup of derived classes

	virtual void DrawShape(sf::RenderWindow& window) = 0; // Draw the shape to the SFML render window (implemented by derived classes)
	virtual void Includer(sf::RenderWindow& window) = 0; // Include the shape in the quadtree for spatial partitioning (implemented by derived classes)
	virtual void SetTextPosition(float fontOffset) = 0; // Set the position of the text component relative to the shape (implemented by derived classes)
	
    void SetPosition(float x, float y); // Set the world position (updates m_position and applies to shape)
	const Vec2& GetPosition() const noexcept { return m_position; } // Get the current world position
	void SetInitialVelocity(float x, float y); // Set the initial velocity (used by PhysicsSystem for movement)
	Vec2 GetVelocity() const; // Get the current velocity
	void SetMidLength(float length); // Set the mid-length property (used for collision detection and quadtree inclusion)
    
    virtual float GetWidth() const = 0; // Get the width of the bounding box
    virtual float GetHeight() const = 0; // Get the height of the bounding box
	virtual Vec2 GetCentrePoint() const = 0; // Get the center point of the shape (used for spatial partitioning and collision detection)
    virtual float GetMidLength() const = 0; // Get the mid-length property (used for collision detection and quadtree inclusion)
	virtual float GetRadius() const = 0; // Get the radius (for circular shapes, returns 0 for non-circular shapes)
	virtual void SetRadius(float radius) = 0; // Set the radius (for circular shapes, does nothing for non-circular shapes)
};
