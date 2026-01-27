#pragma once
#include "Vec2.h"

class CTransform
{
public:
	Vec2 position = { 0,0 };
	Vec2 velocity = { 0,0 };

	CTransform() {}
	CTransform(const Vec2& pos, const Vec2& vel) : position(pos), velocity(vel) {};
};

