#include "CExplosion.h"
#include <SFML/Graphics/Color.hpp>
#include <string>


CExplosion::CExplosion()
{
    m_circle = sf::CircleShape(3.f);
    m_midLength = 4.f;
    m_circle.setFillColor(sf::Color(255, 255, 255, 255));
}

CExplosion::CExplosion(float size)
{
    m_circle = sf::CircleShape(size);
    m_midLength = size + 1.f;
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