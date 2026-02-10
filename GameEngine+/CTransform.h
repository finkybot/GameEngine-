#pragma once
#include "Vec2.h"
#include "Component.h"

/// <summary>
/// Transform component - stores position and velocity data
/// Pure data component with no logic
/// </summary>
class CTransform : public Component
{
public:
	/// <summary>Current world position</summary>
	Vec2 position = { 0, 0 };
	
	/// <summary>Velocity vector (units per second)</summary>
	Vec2 velocity = { 0, 0 };

	/// <summary>Default constructor</summary>
	CTransform() {}
	
	/// <summary>
	/// Constructor with initial position and velocity
	/// </summary>
	/// <param name="pos">Initial position</param>
	/// <param name="vel">Initial velocity</param>
	CTransform(const Vec2& pos, const Vec2& vel) : position(pos), velocity(vel) {};
};



