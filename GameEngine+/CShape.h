/////////////////////////////////
// CShape component - base class for shape components
/////////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the CShape component. We include SFML graphics headers for shapes and rendering, and a custom Vec2 class for 2D vector operations, as well as the base Component class for ECS architecture.
#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include "Vec2.h"
#include "Component.h"
/////////////////////////////////



/////////////////////////////////
// CShape component - base class for shape components
class CShape : public Component {
	/////////////////////////////////
	// Public data members for CShape.
public:
	/////////////////////////////////
	// Member variables for position, velocity, and mid-length. Position is now managed by the CTransform component for consistency across entities, so the m_position member variable is deprecated and should not be used directly in shape components.
	// Vec2 m_position{}; - Deprecated: Position is now managed by CTransform component for consistency across entities.
	// Vec2 m_velocity{}; - Deprecated: Velocity is now managed by CTransform component for consistency across entities.
	float m_midLength{}; // mid-length property for shape extent (e.g. half-width for rectangles, radius for circles)
	/////////////////////////////////



	/////////////////////////////////
	// Protected methods. (currently only ApplyPosition)
protected:
	/////////////////////////////////
	// ApplyPosition - pure virtual method to apply the position to the underlying SFML shape. This method must be implemented by derived shape classes to update the position of their specific SFML shape based on the provided x and y coordinates.
	virtual void ApplyPosition(float x, float y) = 0;
	/////////////////////////////////
	 
	 
	
	/////////////////////////////////
	// Public methods for shape manipulation and rendering.
public:
	/////////////////////////////////
	// Constructor and virtual destructor for the CShape component. The constructor initializes the shape with default properties, while the virtual destructor allows for proper cleanup of derived shape classes when deleted through a base class pointer.
	CShape();		  
	virtual ~CShape();
	/////////////////////////////////



	/////////////////////////////////
	// Pure virtual methods to be implemented by derived shape classes. These methods provide a common interface for getting shape properties such as height, 
	// mid-length, radius, and width, as well as accessing the underlying SFML shape and the center point of the shape for spatial partitioning and collision detection.
	virtual float GetHeight() const = 0;
	virtual float GetMidLength() const = 0; // Get the mid-length property (used for collision detection and quadtree inclusion)
	virtual float GetRadius() const = 0; // Get the radius (for circular shapes, returns 0 for non-circular shapes)
	virtual float GetWidth() const = 0;	 // Get the width of the bounding box
	/////////////////////////////////



	/////////////////////////////////
	// GetShape - pure virtual method to get a reference to the underlying SFML shape. This method must be implemented by derived shape classes to return a reference to their specific SFML shape, which is used for drawing and collision detection.
	virtual sf::Shape& GetShape() = 0; // Get a reference to the underlying SFML shape (implemented by derived classes, used for drawing and collision detection)
	virtual Vec2 GetCentrePoint() const = 0; // Get the center point of the shape (used for spatial partitioning and collision detection)
	virtual void SetRadius(float radius) = 0; // Set the radius (for circular shapes, does nothing for non-circular shapes)
	/////////////////////////////////



	/////////////////////////////////
	// SetMidLength - sets the mid-length property of the shape, which is used for collision detection and quadtree inclusion. This method allows derived shape classes to update the mid-length value based on their specific geometry (e.g., half-width for rectangles, radius for circles).
	void SetMidLength(float midLength) { m_midLength = midLength; }
	/////////////////////////////////
};
/////////////////////////////////
