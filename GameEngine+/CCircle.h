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
    CCircle();
    CCircle(float size, std::string& text, float textSize, sf::Font& font, Vec3 color);

    void DrawShape(sf::RenderWindow& window) override;
    void Includer(sf::RenderWindow& window) override;
    void SetTextPosition(float fontOffset) override;
    void MoveShape(float deltaTime = 1.0f / 60.0f) override; // custom to use velocityChanged

    void SetRadius(float radius) override { m_circle.setRadius(radius); }
    void SetColor(float r, float g, float b, int alpha);
    sf::Color GetColor() const { return m_circle.getFillColor(); }

    float GetMidLength() const override { return m_midLength; }
    float GetWidth() const override { return m_circle.getRadius() * 2.f; }
    float GetHeight() const override { return m_circle.getRadius() * 2.f; }
    Vec2 GetCentrePoint() const override;
    float GetRadius() const override { return m_circle.getRadius(); }
};

