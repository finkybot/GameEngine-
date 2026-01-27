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

CRectangle::CRectangle(float x, float y, std::string& text, float textSize, sf::Font& font, Vec3 color)
{
	m_rectangle = sf::RectangleShape(sf::Vector2f(x, y));

	size_t i = text.length() / 2;
	m_text = new sf::Text(font, text, textSize);
	m_textMidPoint = m_text->findCharacterPos(i).x;
	m_midLength = ((x >= y) ? x : y) + 1.f;
	m_text->setFillColor(sf::Color(color.x, color.y, color.z, 255));
}

void CRectangle::DrawShape(sf::RenderWindow& window)
{
	window.draw(m_rectangle);
	if (m_text) window.draw(*m_text);
}

void CRectangle::Includer(sf::RenderWindow& window)
{
	if (m_rectangle.getPosition().x < 0 || m_rectangle.getPosition().x + m_rectangle.getSize().x > window.getSize().x)
	{
		m_velocity.x = -m_velocity.x;
	}

	if (m_rectangle.getPosition().y < 0 || m_rectangle.getPosition().y + m_rectangle.getSize().y > window.getSize().y)
	{
		m_velocity.y = -m_velocity.y;
	}
}

void CRectangle::SetTextPosition(float fontOffset)
{
	if (!m_text) return;
	m_text->setPosition({ m_rectangle.getPosition().x + (m_rectangle.getSize().x * 0.5f) - m_textMidPoint,
						  m_rectangle.getPosition().y + (m_rectangle.getSize().y * 0.5f) - fontOffset });
}

void CRectangle::MoveShape()
{
	ApplyPosition(m_rectangle.getPosition().x + m_velocity.x,
				  m_rectangle.getPosition().y + m_velocity.y);
	m_position.x = m_rectangle.getPosition().x;
	m_position.y = m_rectangle.getPosition().y;
}

void CRectangle::SetColor(float r, float g, float b)
{
	m_rectangle.setFillColor(sf::Color(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b)));
}
