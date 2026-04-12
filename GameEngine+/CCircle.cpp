// CCircle.cpp - Implementation of the CCircle class, which represents a circular shape in the game engine. This class inherits from CShape and provides specific functionality for circles, including drawing, spatial partitioning, and color manipulation.
#include "CCircle.h"
#include <SFML/Graphics/Color.hpp>
#include <string>

// Notes -- I'm working on removing the DrawShape method from CShape and instead relying on the RenderSystem to draw shapes directly using the GetShape() method. 
// This allows for more efficient rendering by batching draw calls and avoids the overhead of virtual function calls for drawing each shape. 
// The Includer method will be used to add the circle's bounding box to the quadtree for spatial partitioning, which is important for optimizing collision detection and other spatial queries.

CCircle::CCircle()
{
    m_circle = sf::CircleShape(3.f);
    m_midLength = 4.f;
    m_circle.setFillColor(sf::Color(200, 120, 80, 220));
    m_circle.setOrigin(sf::Vector2f(3.f, 3.f));
}

CCircle::CCircle(float size)
{
    m_circle = sf::CircleShape(size);
    m_midLength = size + 1.f;
    // Ensure visible default styling: fill color and origin so transform position is the circle center
    m_circle.setFillColor(sf::Color(200, 120, 80, 220));
    m_circle.setOrigin(sf::Vector2f(size, size));
}

Vec2 CCircle::GetCentrePoint() const
{
    return Vec2(m_circle.getPosition().x + m_circle.getRadius(),
                m_circle.getPosition().y + m_circle.getRadius());
}

void CCircle::SetColor(float r, float g, float b, int alpha)
{
    m_circle.setFillColor(sf::Color(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), alpha));
}