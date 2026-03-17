#include "CShape.h"

CShape::CShape(){}
CShape::~CShape(){}

void CShape::SetPosition(float x, float y)
{
	ApplyPosition(x, y);
	m_position.x = x;
	m_position.y = y;
}

void CShape::SetVelocity(float x, float y)
{
	m_velocity.x = x;
	m_velocity.y = y;
}

