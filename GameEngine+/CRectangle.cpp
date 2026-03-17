#include "CRectangle.h"
#include <SFML/Graphics/Color.hpp>
#include <string>

CRectangle::CRectangle()
{
	m_rectangle = sf::RectangleShape(sf::Vector2f(10.f, 10.f));
	m_midLength = 10.f / 2.f + 1.f;
}

CRectangle::CRectangle(float x, float y)
{
	m_rectangle = sf::RectangleShape(sf::Vector2f(x, y));
	m_midLength = ((x >= y) ? x : y) + 1.f;
}

void CRectangle::DrawShape(sf::RenderWindow& window)
{
	window.draw(m_rectangle);
}

// Includer removed; boundary handling moved to PhysicsSystem

void CRectangle::SetTextPosition(float fontOffset)
{
}

void CRectangle::MoveShape(float deltaTime)
{
	ApplyPosition(m_rectangle.getPosition().x + m_velocity.x * deltaTime,
				  m_rectangle.getPosition().y + m_velocity.y * deltaTime);
	m_position.x = m_rectangle.getPosition().x;
	m_position.y = m_rectangle.getPosition().y;
}

void CRectangle::SetColor(float r, float g, float b)
{
	m_rectangle.setFillColor(sf::Color(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)));
}
