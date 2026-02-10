#include "CShape.h"

CShape::CShape()
{
}

CShape::~CShape()
{
}

void CShape::SetInitialVelocity(float x, float y)
{
	m_velocity.x = x;
	m_velocity.y = y;
}
	
Vec2 CShape::GetVelocity() const
{
	return m_velocity;
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
