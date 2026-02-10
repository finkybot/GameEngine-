#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include "Vec2.h"
#include "Component.h"

/// <summary>
/// Base shape component - provides position, velocity, and rendering interface
/// Derived classes (CCircle, CRectangle) implement specific shape logic
/// </summary>
class CShape : public Component
{
public:
    // Public data members (ECS pure data component)
    Vec2 m_position{};          // cached world position
    Vec2 m_velocity{};
    float m_midLength{};

protected:
    /// <summary>
    /// Apply position to the derived shape implementation (called by SetPosition)
    /// </summary>
    virtual void ApplyPosition(float x, float y) = 0;

public:
    /// <summary>
    /// Constructor
    /// </summary>
    CShape();
    
    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~CShape();

    /// <summary>
    /// Draw the shape to the render window (pure virtual - implemented by derived classes)
    /// </summary>
    virtual void DrawShape(sf::RenderWindow& window) = 0;
    
    /// <summary>
    /// Handle per-frame inclusion logic (boundary checks, etc.)
    /// </summary>
    virtual void Includer(sf::RenderWindow& window) = 0;
    
    /// <summary>
    /// Update text position based on shape geometry
    /// </summary>
    virtual void SetTextPosition(float fontOffset) = 0;

    /// <summary>
    /// Set the position and apply it to the underlying SFML shape
    /// </summary>
    void SetPosition(float x, float y);

    /// <summary>
    /// Get the current world position
    /// </summary>
    const Vec2& GetPosition() const noexcept { return m_position; }

    /// <summary>
    /// Set the initial velocity (directly updates m_velocity)
    /// </summary>
    void SetInitialVelocity(float x, float y);
    
    /// <summary>
    /// Get the current velocity
    /// </summary>
    Vec2 GetVelocity() const;
    
    /// <summary>
    /// Set the mid-length property (extent of the shape)
    /// </summary>
    void SetMidLength(float length);

    // Pure virtual geometry queries - implemented by derived classes
    /// <summary>Get the width of the bounding box</summary>
    virtual float GetWidth() const = 0;
    
    /// <summary>Get the height of the bounding box</summary>
    virtual float GetHeight() const = 0;
    
    /// <summary>Get the center point of the shape</summary>
    virtual Vec2 GetCentrePoint() const = 0;
    
    /// <summary>Get the mid-length property</summary>
    virtual float GetMidLength() const = 0;
    
    /// <summary>Get the radius (for circular shapes)</summary>
    virtual float GetRadius() const = 0;
    
    /// <summary>Set the radius (for circular shapes)</summary>
    virtual void SetRadius(float radius) = 0;
};
