// CExplosion.cpp - Implementation of the CExplosion class, which represents an explosion shape in the game engine. It inherits from CShape and uses an SFML CircleShape to represent the visual aspect of the explosion. The class provides methods for setting the position, color, and radius of the explosion, as well as retrieving its properties for rendering and collision detection.
#include "CExplosion.h"
#include <SFML/Graphics/Color.hpp>
#include <string>


CExplosion::CExplosion()
{
    m_circle = sf::CircleShape(3.f);
    m_midLength = 4.f;
    m_circle.setFillColor(sf::Color(220, 80, 40, 220));
    m_circle.setOrigin(sf::Vector2f(3.f, 3.f));
}

CExplosion::CExplosion(float size)
{
    m_circle = sf::CircleShape(size);
    m_midLength = size + 1.f;
    m_circle.setFillColor(sf::Color(220, 120, 40, 220));
    m_circle.setOrigin(sf::Vector2f(size, size));
}

Vec2 CExplosion::GetCentrePoint() const
{
    return Vec2(m_circle.getPosition().x + m_circle.getRadius(),
        m_circle.getPosition().y + m_circle.getRadius());
}

void CExplosion::SetColor(float r, float g, float b, int alpha)
{
    m_circle.setFillColor(sf::Color(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), alpha));
}