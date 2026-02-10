#pragma once
#include "CShape.h"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>
#include "Vec2.h"

class CCircle : public CShape
{
protected:
    sf::CircleShape m_circle;
    void ApplyPosition(float x, float y) override { m_circle.setPosition(sf::Vector2f(x, y)); }
public:
    /// <summary>
    /// Default constructor - creates a small circle with default properties
    /// </summary>
    CCircle();
    
    /// <summary>
    /// Constructor with parameters
    /// </summary>
    /// <param name="size">Radius of the circle</param>
    CCircle(float size);

    /// <summary>
    /// Draw the circle and optional text to the render window
    /// </summary>
    void DrawShape(sf::RenderWindow& window) override;
    
    /// <summary>
    /// Handle boundary collision logic
    /// </summary>
    void Includer(sf::RenderWindow& window) override;
    
    /// <summary>
    /// Update text position relative to circle center
    /// </summary>
    void SetTextPosition(float fontOffset) override;

    /// <summary>
    /// Set the radius of the circle
    /// </summary>
    void SetRadius(float radius) override { m_circle.setRadius(radius); }
    
    /// <summary>
    /// Set the fill color of the circle
    /// </summary>
    /// <param name="r">Red component (0-255)</param>
    /// <param name="g">Green component (0-255)</param>
    /// <param name="b">Blue component (0-255)</param>
    /// <param name="alpha">Alpha/transparency (0-255)</param>
    void SetColor(float r, float g, float b, int alpha);
    
    /// <summary>
    /// Get the current fill color of the circle
    /// </summary>
    sf::Color GetColor() const { return m_circle.getFillColor(); }

    /// <summary>
    /// Get the mid-length (radius + 1)
    /// </summary>
    float GetMidLength() const override { return m_midLength; }
    
    /// <summary>
    /// Get the width (diameter) of the circle
    /// </summary>
    float GetWidth() const override { return m_circle.getRadius() * 2.f; }
    
    /// <summary>
    /// Get the height (diameter) of the circle
    /// </summary>
    float GetHeight() const override { return m_circle.getRadius() * 2.f; }
    
    /// <summary>
    /// Get the center point of the circle
    /// </summary>
    Vec2 GetCentrePoint() const override;
    
    /// <summary>
    /// Get the radius of the circle
    /// </summary>
    float GetRadius() const override { return m_circle.getRadius(); }
};


