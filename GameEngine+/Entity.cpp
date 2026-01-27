#include "Entity.h"
#include <algorithm>

// Constructor
Entity::Entity(const std::string& tag, size_t id): m_tag(tag), m_id(id)
{
}

const std::string& Entity::GetTag()
{
	return m_tag;
}

bool Entity::IsAlive() const
{
	return m_alive;
}

void Entity::Destroy()
{
	m_alive = false;
}


/// <summary>
/// Entity Circle-Circle collision response; in this case we will use simple elastic collision response between two circles.
/// </summary>
/// <param name="pItr">Pointer to other Entity to bounce with</param>
void Entity::Bounce(const Entity* pItr) const
{
	// Distance between the two entity centres, I'm using Circles of various sizes at the moment;
	Vec2 distanceVec = pItr->GetCentrePoint() - GetCentrePoint();
	float scalerDist = GetCentrePoint().Distance(pItr->GetCentrePoint());

	// Guard clause
	if (scalerDist == 0.0f) return; // Prevent division by zero
	
	// We are safe to continue
	// Collision normal, Scaler devision to get unit normal vector
	Vec2 unitNorm = distanceVec / scalerDist;

	// Relative velocity
	Vec2 relVel = pItr->cShape->GetVelocity() - cShape->GetVelocity();

	// Velocity along the normal
	float velAlongNormal = relVel.x * unitNorm.x + relVel.y * unitNorm.y;

	// Do not resolve if velocities are separating
	if (velAlongNormal > 0)
		return;

	// Coefficient of restitution (elasticity)
	float restitution = 0.9f; // 1.0 for perfectly elastic collision

	// Impulse scalar
	float impulse = -(1 + restitution) * velAlongNormal;

	impulse /= 2; // Assuming equal mass for both circles
		
	// Apply impulse to the circles' velocities
	cShape->SetVelocity(cShape->GetVelocity().GetX() - impulse * unitNorm.x, cShape->GetVelocity().GetY() - impulse * unitNorm.y);
	pItr->cShape->SetVelocity(pItr->cShape->GetVelocity().GetX() + impulse * unitNorm.x, pItr->cShape->GetVelocity().GetY() + impulse * unitNorm.y);

	// Positional correction to avoid sinking
	float overlap = (GetRadius() + pItr->GetRadius()) - scalerDist;
	if (overlap > 0)
	{
		const float percent = 0.2f; // usually 20% to 80%
		const float slop = 0.01f; // usually 0.01 to 0.1
		float correction = std::max(overlap - slop, 0.0f) / 2 * percent; // Divide by 2 to distribute correction equally
		cShape->SetPosition(cShape->GetPosition().GetX() - correction * unitNorm.x, cShape->GetPosition().GetY() - correction * unitNorm.y);
		pItr->cShape->SetPosition(pItr->cShape->GetPosition().GetX() + correction * unitNorm.x, pItr->cShape->GetPosition().GetY() + correction * unitNorm.y);
	}
}

/// <summary>
/// Entity Circle-Circle collision detection.
/// </summary>
/// <param name="pItr">Pointer to other Entity to test intersection with</param>
/// <returns>True if circles intersect, false otherwise</returns>
bool Entity::Intersects(const Entity* pItr) const
{
	// Calculate the distance between the two circles' centres
	Vec2 distanceVec = pItr->GetCentrePoint() - GetCentrePoint();

	// Calculate the square of the distance of the two centres
	float distancesquared = distanceVec.Mag2();

	// Calculate the sum of the two circles' radii
	float radiusSum = cShape->GetRadius() + pItr->cShape->GetRadius();
	float radiusSumSquared = radiusSum * radiusSum;

	// If the distance squared is less than or equal to the sum of the radii squared, a collision has occurred
	return distancesquared <= radiusSumSquared;
}
