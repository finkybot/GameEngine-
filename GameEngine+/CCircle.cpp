#include "CCircle.h"
#include <SFML/Graphics/Color.hpp>
#include <string>

CCircle::CCircle()
{
    m_circle = sf::CircleShape(3.f);
    m_midLength = 4.f;
    m_circle.setFillColor(sf::Color(255, 255, 255, 255));
}

CCircle::CCircle(float size, std::string& text, float textSize, sf::Font& font, Vec3 color)
{
    m_circle = sf::CircleShape(size);

    size_t i = text.length() / 2;
    m_text = new sf::Text(font, text, textSize);
    m_textMidPoint = m_text->findCharacterPos(i).x;
    m_midLength = size + 1.f;
    m_text->setFillColor(sf::Color(color.x, color.y, color.z, 255));
}

void CCircle::DrawShape(sf::RenderWindow& window)
{
    window.draw(m_circle);
    if (m_text) window.draw(*m_text);
}

void CCircle::Includer(sf::RenderWindow& window)
{
    if (m_circle.getPosition().x < 0 || m_circle.getPosition().x + (m_circle.getRadius() * 2.f) > window.getSize().x)
    {
        m_velocityChanged.x = -m_velocityChanged.x;
    }

    if (m_circle.getPosition().y < 0 || m_circle.getPosition().y + (m_circle.getRadius() * 2.f) > window.getSize().y)
    {
        m_velocityChanged.y = -m_velocityChanged.y;
    }
}

void CCircle::SetTextPosition(float fontOffset)
{
    if (!m_text) return;
    m_text->setPosition({ m_circle.getPosition().x + m_circle.getRadius() - m_textMidPoint,
                          m_circle.getPosition().y + (m_circle.getRadius() - fontOffset) });
}

void CCircle::MoveShape(float deltaTime)
{
    m_velocity = m_velocityChanged; // adopt changed velocity
    ApplyPosition(m_circle.getPosition().x + m_velocity.x * deltaTime,
                  m_circle.getPosition().y + m_velocity.y * deltaTime);
    m_position.x = m_circle.getPosition().x;
    m_position.y = m_circle.getPosition().y;
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
