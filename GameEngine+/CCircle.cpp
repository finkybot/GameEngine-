#include "CCircle.h"
#include <SFML/Graphics/Color.hpp>
#include <string>

CCircle::CCircle()
{
    m_circle = sf::CircleShape(3.f);
    m_midLength = 4.f;
    m_circle.setFillColor(sf::Color(255, 255, 255, 255));
}

CCircle::CCircle(float size)
{
    m_circle = sf::CircleShape(size);
    m_midLength = size + 1.f;
}

void CCircle::DrawShape(sf::RenderWindow& window)
{
    window.draw(m_circle);
}

void CCircle::Includer(sf::RenderWindow& window)
{
    // Check if circle has moved off the left edge of screen and despawn it
    if (m_circle.getPosition().x + (m_circle.getRadius() * 2.f) < 0)
    {
        // Entity will be marked for destruction - handled by owner entity
        return;
    }

    /* LEGACY: Boundary bouncing code (kept for future use)
    if (m_circle.getPosition().x < 0 || m_circle.getPosition().x + (m_circle.getRadius() * 2.f) > window.getSize().x)
    {
        m_velocity.x = -m_velocity.x;
    }

    if (m_circle.getPosition().y < 0 || m_circle.getPosition().y + (m_circle.getRadius() * 2.f) > window.getSize().y)
    {
        m_velocity.y = -m_velocity.y;
    }
    */
}

void CCircle::SetTextPosition(float fontOffset)
{
}

void CCircle::SetColor(float r, float g, float b, int alpha)
{
    m_circle.setFillColor(sf::Color(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), alpha));
}

Vec2 CCircle::GetCentrePoint() const
{
    return Vec2(m_circle.getPosition().x + m_circle.getRadius(),
                m_circle.getPosition().y + m_circle.getRadius());
}
