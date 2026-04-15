#include "CRectangle.h"
#include <SFML/Graphics/Color.hpp>
#include <string>

CRectangle::CRectangle() {
	m_rectangle = sf::RectangleShape(sf::Vector2f(10.f, 10.f));
	m_midLength = 10.f / 2.f + 1.f;
}

CRectangle::CRectangle(float x, float y) {
	m_rectangle = sf::RectangleShape(sf::Vector2f(x, y));
	m_midLength = ((x >= y) ? x : y) + 1.f;
}

void CRectangle::SetSize(float width, float height) {
	m_rectangle.setSize(sf::Vector2f(width, height));
}

// Includer removed; boundary handling moved to PhysicsSystem

void CRectangle::SetColor(float r, float g, float b, int alpha) {
    m_rectangle.setFillColor(sf::Color(static_cast<int>(r), static_cast<int>(g), static_cast<int>(b), alpha));
}

void CRectangle::ApplyPosition(float x, float y) {
	m_rectangle.setPosition(sf::Vector2f(x, y));
}

Vec2 CRectangle::GetCentrePoint() const {
	return Vec2(m_rectangle.getPosition().x + m_rectangle.getSize().x * 0.5f,
				m_rectangle.getPosition().y + m_rectangle.getSize().y * 0.5f);
}
