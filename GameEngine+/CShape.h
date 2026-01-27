#pragma once
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include "Vec2.h"

// Class declarations.
class CShape
{
protected:
    Vec2 m_position{};          // cached world position
    Vec2 m_velocity{};
    Vec2 m_velocityChanged{};   // optional alternate velocity
    sf::Text* m_text = nullptr;
    int  m_textMidPoint{};
    float m_midLength{};

    // Derived classes must apply the position to their SFML shape implementation.
    virtual void ApplyPosition(float x, float y) = 0;

public:
    CShape();
    virtual ~CShape();

    virtual void DrawShape(sf::RenderWindow& window) = 0;
    virtual void Includer(sf::RenderWindow& window) = 0;
    virtual void SetTextPosition(float fontOffset) = 0;

    // Centralized position and movement
    void SetPosition(float x, float y);
    virtual void MoveShape(float deltaTime = 1.0f / 60.0f);  // Default to 60 FPS for backward compatibility

    const Vec2& GetPosition() const noexcept { return m_position; }

    void DrawText(sf::RenderWindow& window);
    void SetInitialVelocity(float x, float y);
    void SetVelocity(float x, float y);
    Vec2 GetVelocity() const;
    void SetMidLength(float length);

    virtual float GetWidth() const = 0;
    virtual float GetHeight() const = 0;
    virtual Vec2 GetCentrePoint() const = 0;
    virtual float GetMidLength() const = 0;
    virtual float GetRadius() const = 0;
	virtual void SetRadius(float radius) = 0;
};

