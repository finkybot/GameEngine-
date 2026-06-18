/////////////////////////////////
// CollisionSystem.cpp - implementation of the CollisionSystem class, which is responsible for detecting and resolving collisions between entities in the game. This includes checking for collisions based on entity positions and radii, determining if entities are enemies or allies, 
// and applying appropriate collision responses such as spawning explosions for enemy collisions or bouncing allied entities apart.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "CollisionSystem.h"
#include "SpawnSystem.h"
#include "../Entity.h"
#include "../CShape.h"
#include "../CCircle.h"
#include "../CExplosion.h"
#include "../CSoundEffect.h"
#include "../EntityType.h"
#include "../EntityManager.h"
#include "../CStatic.h"
#include <algorithm>
#include <cmath>
#include <iostream>
/////////////////////////////////



/////////////////////////////////
// DetectAndResolve - detects and resolves collisions between entities. It iterates through the list of entities, queries the spatial hash for nearby entities, checks for actual collisions, and applies the appropriate collision response based on entity types.
void CollisionSystem::DetectAndResolve(const std::vector<std::unique_ptr<Entity>>& entities,
									   SpatialHashGrid<Entity>& spatialHash, float deltaTime) {
	static std::vector<Entity*> nearbyEntities;
	static size_t lastEntityCount = 0;

	if (entities.size() != lastEntityCount && !entities.empty()) {
		nearbyEntities.reserve(std::max(size_t(16), entities.size() / 100));
		lastEntityCount = entities.size();
	}

	int deathCount = 0;

	for (auto iterator = entities.begin(); iterator != entities.end(); ++iterator) {
		Entity* currentEntity = iterator->get();

		if (!currentEntity->IsAlive())
			continue;

		// Skip collision detection for explosions - they're visual effects only
		if (currentEntity->GetType() == EntityType::Explosion)
			continue;

		// Skip static tiles as they are handled separately and do not move
		if (currentEntity->HasComponent<CStatic>())
			continue;

		const Vec2& position = currentEntity->GetPosition();
		float radius = currentEntity->GetRadius();

		// Query nearby entities using spatial hash
		nearbyEntities.clear();
		spatialHash.Query(nearbyEntities, position, radius * 3.0f, currentEntity);

		for (Entity* entityPtr : nearbyEntities) {
			// Validate pointer is still alive (safety check)
			if (!entityPtr->IsAlive())
				continue;

			if (!IsColliding(currentEntity, entityPtr))
				continue;

			// Skip if other entity is an explosion
			if (entityPtr->GetType() == EntityType::Explosion)
				continue;

			deathCount += ResolveCollision(currentEntity, entityPtr);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// IsColliding - checks if two entities are colliding based on their positions and radii. It calculates the distance between the centers of the two entities and compares it to the sum of their radii to determine if a collision is occurring.
bool CollisionSystem::IsColliding(const Entity* entity1, const Entity* entity2) const {
	// Calculate the distance between the two circles' centres
	Vec2 distanceVec = entity2->GetCentrePoint() - entity1->GetCentrePoint();

	// Calculate the square of the distance of the two centres
	float distanceSquared = distanceVec.Mag2();

	// Calculate the sum of the two circles' radii
	float radiusSum = entity1->GetRadius() + entity2->GetRadius();
	float radiusSumSquared = radiusSum * radiusSum;

	// If the distance squared is less than or equal to the sum of the radii squared, a collision has occurred
	return distanceSquared <= radiusSumSquared;
}
/////////////////////////////////



/////////////////////////////////
// ResolveCollision - resolves a collision between two entities based on their types (enemies vs allies). If the entities are enemies (different tags), it spawns an explosion at the collision point, blends their colors for the explosion effect, and destroys both entities.
int CollisionSystem::ResolveCollision(Entity* entity1, Entity* entity2) const {
	// Check if entities are enemies (different tags) or allies (same tag)
	if (AreEnemies(entity1, entity2)) {
		auto shape1 = entity1->GetComponent<CShape>();
		auto shape2 = entity2->GetComponent<CShape>();
		if (!shape1 || !shape2)
			return 0;

		// Spawn explosion at collision point
		// Prefer velocities from CTransform when available
		Vec2 currentVel = Vec2::Zero;
		Vec2 otherVel = Vec2::Zero;
		if (auto t1 = entity1->GetComponent<CTransform>())
			currentVel = t1->m_velocity;
		//else currentVel = shape1->m_velocity;
		if (auto t2 = entity2->GetComponent<CTransform>())
			otherVel = t2->m_velocity;
		//else otherVel = shape2->m_velocity;

		float currentSpeed = std::sqrt(currentVel.x * currentVel.x + currentVel.y * currentVel.y);
		float otherSpeed = std::sqrt(otherVel.x * otherVel.x + otherVel.y * otherVel.y);

		Vec2 explosionVelocity = (currentSpeed >= otherSpeed) ? currentVel : otherVel;

		// Blend the colors of the two colliding entities
		Vec3 blendedColor(255, 255, 255); // default to white
		CCircle* currentCircle = dynamic_cast<CCircle*>(shape1);
		CCircle* otherCircle = dynamic_cast<CCircle*>(shape2);

		// Only blend colors if both shapes are circles (have color)
		if (currentCircle && otherCircle) {
			sf::Color currentColor = currentCircle->GetColor();
			sf::Color otherColor = otherCircle->GetColor();

			// Average the colors for a blend effect
			blendedColor.x = (currentColor.r + otherColor.r) / 2.0f;
			blendedColor.y = (currentColor.g + otherColor.g) / 2.0f;
			blendedColor.z = (currentColor.b + otherColor.b) / 2.0f;
		}

		// Calculate collision point as the point on the edge of entity1 in the direction of entity2
		Vec2 distanceVec = entity2->GetCentrePoint() - entity1->GetCentrePoint();
		float distance = entity1->GetCentrePoint().Distance(entity2->GetCentrePoint());

		// If distance is zero (perfect overlap), default to the midpoint between the two entities
		Vec2 collisionPoint;
		if (distance > 0.0f) {
			Vec2 direction = distanceVec / distance;
			collisionPoint = entity1->GetCentrePoint() + direction * entity1->GetRadius();
		} else {
			collisionPoint = (entity1->GetPosition() + entity2->GetPosition()) * 0.5f;
		}

		// Adjust collision point to account for SFML's top-left positioning
		// The explosion radius is 5.0f, so subtract it to get the correct top-left position
		const float explosionRadius = 5.0f;
		Vec2 explosionPosition = collisionPoint - Vec2(explosionRadius, explosionRadius);

		// Use SpawnSystem if available, otherwise fall back to direct creation
		if (m_spawnSystem) {
			m_spawnSystem->SpawnExplosion(
				explosionPosition.x, explosionPosition.y,
				explosionVelocity.x, explosionVelocity.y,
				static_cast<unsigned char>(blendedColor.x),
				static_cast<unsigned char>(blendedColor.y),
				static_cast<unsigned char>(blendedColor.z),
				200,  // alpha
				Spawn::ComponentFlags::Default
			);
		} else {
			// Fallback: create explosion directly
			Entity* en = m_entityManager->AddEntity(EntityType::Explosion);
			en->AddComponent<CTransform>(explosionPosition, explosionVelocity);

			auto explosion = std::make_unique<CExplosion>();
			explosion->SetRadius(explosionRadius);
			explosion->SetColor(blendedColor.x, blendedColor.y, blendedColor.z, 200);
			en->AddComponentPtr<CShape>(std::move(explosion));

			auto soundEffect = en->AddComponent<CSoundEffect>();
			soundEffect->m_Path = "assets/sounds/medium-explosion.ogg";
			soundEffect->m_volume = 75.0f;
			soundEffect->m_loop = false;
			soundEffect->m_priority = SoundPriority::SFX;
			soundEffect->m_is3D = true;
			soundEffect->m_3DMinDistance = 200.0f;
			soundEffect->m_3DMaxDistance = 2000.0f;
			soundEffect->m_shouldPlay = true;
		}

		m_entityManager->KillEntity(entity1);
		m_entityManager->KillEntity(entity2);

		return 2; // Two entities destroyed
	} else		  // Allies - bounce them apart
	{
		BounceEntities(entity1, entity2);
		return 0; // No entities destroyed
	}
}
/////////////////////////////////



/////////////////////////////////
// BounceEntities - applies an elastic collision response to bounce allied entities apart. It calculates the collision normal, relative velocity, and applies an impulse to update the velocities of both entities based
void CollisionSystem::BounceEntities(Entity* entity1, Entity* entity2) const {
	// Get the shape components to access velocity and position
	auto shape1 = entity1->GetComponent<CShape>();
	auto shape2 = entity2->GetComponent<CShape>();

	// Guard clause to ensure both entities have shape components
	if (!shape1 || !shape2)
		return;

	// Distance between the two entity centres
	Vec2 distanceVec = entity2->GetCentrePoint() - entity1->GetCentrePoint();
	float scalerDist = entity1->GetCentrePoint().Distance(entity2->GetCentrePoint());

	// Guard clause
	if (scalerDist == 0.0f)
		return; // Prevent division by zero

	// Collision normal, Scaler division to get unit normal vector
	Vec2 unitNorm = distanceVec / scalerDist;

	// Relative velocity
	Vec2 relVel = entity1->GetComponent<CTransform>()->m_velocity - entity2->GetComponent<CTransform>()->m_velocity;

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
	Vec2 vel1 = entity1->GetComponent<CTransform>()->m_velocity;
	Vec2 vel2 = entity2->GetComponent<CTransform>()->m_velocity;

	// Update velocities based on impulse and collision normal
	entity1->GetComponent<CTransform>()->m_velocity =
		Vec2(vel1.x - impulse * unitNorm.x, vel1.y - impulse * unitNorm.y);
	entity2->GetComponent<CTransform>()->m_velocity =
		Vec2(vel2.x + impulse * unitNorm.x, vel2.y + impulse * unitNorm.y);

	// Positional correction to avoid sinking
	float overlap = (entity1->GetRadius() + entity2->GetRadius()) - scalerDist;
	if (overlap > 0) {
		const float percent = 0.2f; // usually 20% to 80%
		const float slop = 0.01f;	// usually 0.01 to 0.1
		float correction = std::max(overlap - slop, 0.0f) / 2 * percent;

		Vec2 pos1 = entity1->GetComponent<CTransform>()->m_position;
		Vec2 pos2 = entity2->GetComponent<CTransform>()->m_position;

		entity1->GetComponent<CTransform>()->m_position =
			Vec2(pos1.x - correction * unitNorm.x, pos1.y - correction * unitNorm.y);
		entity2->GetComponent<CTransform>()->m_position =
			Vec2(pos2.x + correction * unitNorm.x, pos2.y + correction * unitNorm.y);
	}
}
/////////////////////////////////



/////////////////////////////////
// AreEnemies - checks if two entities are enemies based on their types/tags. In this implementation, entities are considered enemies if they belong to different teams/types, which is determined by comparing their EntityType values.
bool CollisionSystem::AreEnemies(const Entity* entity1, const Entity* entity2) const {
	return entity1->GetType() != entity2->GetType(); // Entities are enemies if they belong to different teams/types
}
/////////////////////////////////