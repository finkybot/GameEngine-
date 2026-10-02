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
#include "../SoundSystem.h"
#include "../CStatic.h"
#include <algorithm>
#include <cmath>
#include <iostream>
/////////////////////////////////



/////////////////////////////////
// DetectAndResolve - detects and resolves collisions between entities. It iterates through the list of entities, queries the spatial hash for nearby entities, checks for actual collisions, and applies the appropriate collision response based on entity types.
void CollisionSystem::DetectAndResolve(const std::vector<std::unique_ptr<Entity>>& entities, float deltaTime) {
	static std::vector<Entity*> nearbyEntities;
	static size_t lastEntityCount = 0;

	if (entities.size() != lastEntityCount && !entities.empty()) {
		nearbyEntities.reserve(std::max(size_t(16), entities.size() / 100));
		lastEntityCount = entities.size();
	}

	std::unordered_set<uint64_t> visitedPairs;
	visitedPairs.reserve(entities.size() * 2);

	for (auto& up : entities) {
		Entity* currentEntity = up.get();
		if (!currentEntity->IsAlive())
			continue;

		if (currentEntity->GetType() == EntityType::Explosion)
			continue;

		if (currentEntity->HasComponent<CStatic>())
			continue;

		if (!m_spatialIndex)
			continue;

		const Vec2& position = currentEntity->GetPosition();
		float radius = currentEntity->GetRadius();

		nearbyEntities.clear();
		m_spatialIndex->QueryEntities(nearbyEntities, position, radius * 1.75f, currentEntity);

		for (Entity* entityPtr : nearbyEntities) {
			if (!entityPtr || !entityPtr->IsAlive())
				continue;

			if (currentEntity->GetId() >= entityPtr->GetId())
				continue;

			uint64_t key = (uint64_t(currentEntity->GetId()) << 32) | uint64_t(entityPtr->GetId());

			if (visitedPairs.count(key))
				continue;
			visitedPairs.insert(key);

			if (!IsColliding(currentEntity, entityPtr))
				continue;

			if (entityPtr->GetType() == EntityType::Explosion)
				continue;

			ResolveCollision(currentEntity, entityPtr);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// DetectAndResolveSpatial - detects and resolves collisions between entities using a spatial index for efficient broad-phase collision detection. It queries the spatial index for nearby entities, checks for actual collisions, and applies the appropriate collision response based on entity types.
void CollisionSystem::DetectAndResolveSpatial(const std::vector<std::unique_ptr<Entity>>& entities, ISpatialIndex* spatialIndex, float deltaTime) {
	// Phase‑4: spatial-aware collision entry point
	m_spatialIndex = spatialIndex;

	if (!m_spatialIndex)
		return;

	// Reuse existing spatial collision loop
	DetectAndResolve(entities, deltaTime);
}
/////////////////////////////////



/////////////////////////////////
// IsColliding - checks if two entities are colliding based on their positions and radii. It calculates the distance between the centers of the two entities and compares it to the sum of their radii to determine if a collision is occurring.
bool CollisionSystem::IsColliding(const Entity* entity1, const Entity* entity2) const {
	if (!m_entityManager)
		return false;

	auto& T = m_entityManager->GetTransformSoA();

	size_t i1 = entity1->transformIndex;
	size_t i2 = entity2->transformIndex;
	if (i1 == SIZE_MAX || i2 == SIZE_MAX)
		return false;

	float x1 = T.posX[i1];
	float y1 = T.posY[i1];
	float x2 = T.posX[i2];
	float y2 = T.posY[i2];

	float dx = x2 - x1;
	float dy = y2 - y1;
	float distSq = dx * dx + dy * dy;

	float r1 = entity1->GetRadius();
	float r2 = entity2->GetRadius();
	float radiusSum = r1 + r2;
	float radiusSumSq = radiusSum * radiusSum;

	return distSq <= radiusSumSq;
}
/////////////////////////////////



/////////////////////////////////
// ResolveCollision - resolves a collision between two entities based on their types (enemies vs allies). If the entities are enemies (different tags), it spawns an explosion at the collision point, blends their colors for the explosion effect, and destroys both entities.
int CollisionSystem::ResolveCollision(Entity* entity1, Entity* entity2) const {
	if (!m_entityManager)
		return 0;

	auto& T = m_entityManager->GetTransformSoA();

	size_t i1 = entity1->transformIndex;
	size_t i2 = entity2->transformIndex;
	if (i1 == SIZE_MAX || i2 == SIZE_MAX)
		return 0;

	// Enemies → explosion, allies → bounce
	if (AreEnemies(entity1, entity2)) {
		auto shape1 = entity1->GetComponent<CShape>();
		auto shape2 = entity2->GetComponent<CShape>();
		if (!shape1 || !shape2)
			return 0;

		// Explosion velocity from SoA
		Vec2 currentVel(T.velX[i1], T.velY[i1]);
		Vec2 otherVel(T.velX[i2], T.velY[i2]);

		float currentSpeed = std::sqrt(currentVel.x * currentVel.x + currentVel.y * currentVel.y);
		float otherSpeed = std::sqrt(otherVel.x * otherVel.x + otherVel.y * otherVel.y);

		Vec2 explosionVelocity = (currentSpeed >= otherSpeed) ? currentVel : otherVel;
		Vec2 explosionVelocity2 = (currentSpeed < otherSpeed) ? currentVel : otherVel;

		explosionVelocity *= 0.5f;
		explosionVelocity2 *= 0.5f;

		// Blinding yellow blend
		sf::Color col1 = shape1->GetColor();
		sf::Color col2 = shape2->GetColor();

		// Convert to floats
		float r1 = col1.r / 255.0f;
		float g1 = col1.g / 255.0f;
		float b1 = col1.b / 255.0f;

		float r2 = col2.r / 255.0f;
		float g2 = col2.g / 255.0f;
		float b2 = col2.b / 255.0f;

		// Compute brightness of each color
		float brightness1 = (r1 + g1 + b1) / 3.0f;
		float brightness2 = (r2 + g2 + b2) / 3.0f;

		// Pick the brighter one
		float brightness = std::max(brightness1, brightness2);

		// Push toward yellow (R+G high, B low)
		float yellowR = 1.0f;
		float yellowG = 1.0f;
		float yellowB = 0.0f;

		// Mix original brightness with yellow
		float mix = 0.85f; // 0 = original color, 1 = pure yellow
		float finalR = (1.0f - mix) * brightness + mix * yellowR;
		float finalG = (1.0f - mix) * brightness + mix * yellowG;
		float finalB = (1.0f - mix) * brightness + mix * yellowB;

		// Convert back to 0–255
		Vec3 blendedColor(finalR * 255.0f, finalG * 255.0f, finalB * 255.0f);

		// Collision point
		float x1 = T.posX[i1];
		float y1 = T.posY[i1];
		float x2 = T.posX[i2];
		float y2 = T.posY[i2];

		Vec2 p1(x1, y1);
		Vec2 p2(x2, y2);

		Vec2 distanceVec = p2 - p1;
		float distance = p1.Distance(p2);

		Vec2 collisionPoint;
		if (distance > 0.0f) {
			Vec2 direction = distanceVec / distance;
			collisionPoint = p1 + direction * entity1->GetRadius();
		} else {
			collisionPoint = (p1 + p2) * 0.5f;
		}

		const float explosionRadius = 5.0f;
		Vec2 explosionPosition = collisionPoint - Vec2(explosionRadius, explosionRadius);

		//bool canPlaySound = m_soundSystem ? m_soundSystem->CanPlayNewSound(*m_entityManager) : true;

		// Spawn explosion 1 using AddEntity's correct setup
		Entity* en = m_entityManager->AddEntity(EntityType::Explosion);
		Entity* en2 = m_entityManager->AddEntity(EntityType::Explosion);

		auto* t = en->GetComponent<CTransform>();
		t->position = explosionPosition;
		t->velocity = explosionVelocity;


		auto* t2 = en2->GetComponent<CTransform>();
		t2->position = explosionPosition;
		t2->velocity = explosionVelocity2;


		auto* s1 = en->GetComponent<CShape>();
		if (s1) {
			s1->SetColor(blendedColor.x, blendedColor.y, blendedColor.z, 200);
		}
		
		auto* s2 = en2->GetComponent<CShape>();
		if (s2) {
			s2->SetColor(blendedColor.x, blendedColor.y, blendedColor.z, 200);
		}


		////std::cout << "Explosion creation time: " << en->m_creationTime.time_since_epoch().count() << std::endl;


		//if (canPlaySound) {
		//	auto soundEffect = en->AddComponent<CSoundEffect>();
		//	soundEffect->m_Path = "assets/sounds/medium-explosion.ogg";
		//	soundEffect->m_volume = 75.0f;
		//	soundEffect->m_loop = false;
		//	soundEffect->m_priority = SoundPriority::Critical;
		//	soundEffect->m_is3D = true;
		//	soundEffect->m_3DMinDistance = 200.0f;
		//	soundEffect->m_3DMaxDistance = 2000.0f;
		//	soundEffect->m_shouldPlay = true;
		//}

		m_entityManager->KillEntity(entity1);
		m_entityManager->KillEntity(entity2);
		return 2;
	}

	// Allies → bounce
	BounceEntities(entity1, entity2);
	return 0;
}
/////////////////////////////////



/////////////////////////////////
// BounceEntities - applies an elastic collision response to bounce allied entities apart. It calculates the collision normal, relative velocity, and applies an impulse to update the velocities of both entities based
void CollisionSystem::BounceEntities(Entity* entity1, Entity* entity2) const {
	if (!m_entityManager)
		return;

	auto& T = m_entityManager->GetTransformSoA();

	size_t i1 = entity1->transformIndex;
	size_t i2 = entity2->transformIndex;
	if (i1 == SIZE_MAX || i2 == SIZE_MAX)
		return;

	float x1 = T.posX[i1];
	float y1 = T.posY[i1];
	float x2 = T.posX[i2];
	float y2 = T.posY[i2];

	float dx = x2 - x1;
	float dy = y2 - y1;
	float dist = std::sqrt(dx * dx + dy * dy);
	if (dist <= 0.0001f)
		return;

	Vec2 unitNorm(dx / dist, dy / dist);

	Vec2 v1(T.velX[i1], T.velY[i1]);
	Vec2 v2(T.velX[i2], T.velY[i2]);
	Vec2 relVel = v1 - v2;

	float velAlongNormal = relVel.x * unitNorm.x + relVel.y * unitNorm.y;

	// Only bounce if they are actually moving toward each other
	if (velAlongNormal > 0.0f)
		return;

	// MUCH LOWER restitution — stable
	float restitution = 0.05f;

	float impulse = -(1.0f + restitution) * velAlongNormal * 0.5f;

	// Apply impulse
	T.velX[i1] = v1.x - impulse * unitNorm.x;
	T.velY[i1] = v1.y - impulse * unitNorm.y;

	T.velX[i2] = v2.x + impulse * unitNorm.x;
	T.velY[i2] = v2.y + impulse * unitNorm.y;

	// --- FIXED: FULL SEPARATION (no continuous bounce) ---
	float overlap = (entity1->GetRadius() + entity2->GetRadius()) - dist;
	if (overlap > 0.0f) {

		// FULL correction — stops repeated bounce
		const float percent = 1.0f; // instead of 0.2
		const float slop = 0.01f;

		float correction = std::max(overlap - slop, 0.0f) * 0.5f * percent;

		T.posX[i1] = x1 - correction * unitNorm.x;
		T.posY[i1] = y1 - correction * unitNorm.y;

		T.posX[i2] = x2 + correction * unitNorm.x;
		T.posY[i2] = y2 + correction * unitNorm.y;
	}
}
/////////////////////////////////



/////////////////////////////////
// AreEnemies - checks if two entities are enemies based on their types/tags. In this implementation, entities are considered enemies if they belong to different teams/types, which is determined by comparing their EntityType values.
bool CollisionSystem::AreEnemies(const Entity* entity1, const Entity* entity2) const {
	return entity1->GetType() != entity2->GetType(); // Entities are enemies if they belong to different teams/types
}
/////////////////////////////////