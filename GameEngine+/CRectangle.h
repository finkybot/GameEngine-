#pragma once

#include "CShape.h"
#include "Vec2.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include <string>

class CRectangle : public CShape
{
    sf::RectangleShape m_rectangle;
protected:
    void ApplyPosition(float x, float y) override { m_rectangle.setPosition(sf::Vector2f(x, y)); }
public:
    CRectangle();
    CRectangle(float x, float y);
    CRectangle(float x, float y, std::string& text, float textSize, sf::Font& font, Vec3 color);

    void DrawShape(sf::RenderWindow& window) override;
    void Includer(sf::RenderWindow& window) override;
    void SetTextPosition(float fontOffset) override;
    void MoveShape() override; // custom to use current velocity
    void SetColor(float r, float g, float b);
    void SetRadius(float radius) override { m_rectangle.setSize(sf::Vector2f(radius,radius)); }

    Vec2 GetCentrePoint() const override { return { 0,0 }; }
    float GetWidth() const override { return m_rectangle.getSize().x; }
    float GetHeight() const override { return m_rectangle.getSize().y; }
    float GetMidLength() const override { return m_midLength; }
    float GetRadius() const override { return m_rectangle.getSize().x; }
};



