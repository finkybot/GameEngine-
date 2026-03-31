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
    void ApplyPosition(float x, float y) override;
public:
    CRectangle();
    CRectangle(float x, float y);

    Vec2 GetCentrePoint() const override;

    float GetHeight() const override { return m_rectangle.getSize().y; }
    float GetMidLength() const override { return m_midLength; }
    float GetRadius() const override { return m_rectangle.getSize().x; }
    float GetWidth() const override { return m_rectangle.getSize().x; }

    sf::Color GetColor() const { return m_rectangle.getFillColor(); }       // Get the current fill color of the rectangle shape as an SFML Color object
    sf::Shape& GetShape()  override { return m_rectangle; }					// Get a reference to the underlying SFML shape (used for drawing and collision detection)

    void SetColor(float r, float g, float b, int alpha);                    // Set the fill color of the circle shape using RGBA values (alpha is an integer in the range [0, 255])
    void SetRadius(float radius) override { m_rectangle.setSize(sf::Vector2f(radius, radius)); }

};



