#include "CShape.h"

CShape::CShape()
{
    // No default text constructed (font handling left to derived/user code)
    m_text = nullptr;
}

CShape::~CShape()
{
    if (m_text)
    {
        delete m_text;
        m_text = nullptr;
    }
}

// Method definitions.
void CShape::DrawText(sf::RenderWindow& window)
{
    if (m_text) window.draw(*m_text);
}

void CShape::SetInitialVelocity(float x, float y)
{
	m_velocity.x = x;
	m_velocity.y = y;
}

void CShape::SetVelocity(float x, float y)
{
	//m_velocity.x = x;
	//m_velocity.y = y;
	m_velocityChanged.x = x;
	m_velocityChanged.y = y;
}
	
Vec2 CShape::GetVelocity() const
{
	return { m_velocity.x, m_velocity.y };
}

void CShape::SetMidLength(float length)
{
	m_midLength = length;
}

void CShape::SetPosition(float x, float y)
{
	ApplyPosition(x, y);
	m_position.x = x;
	m_position.y = y;
}

void CShape::MoveShape()
{
	ApplyPosition(m_position.x + m_velocity.x, m_position.y + m_velocity.y);
	m_position.x += m_velocity.x;
	m_position.y += m_velocity.y;
}

