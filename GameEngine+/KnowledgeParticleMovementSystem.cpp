/////////////////////////////////
// KnowledgeParticleMovementSystem.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "KnowledgeParticleMovementSystem.h"
/////////////////////////////////



/////////////////////////////////
// Update - Overrides the base class Update method to implement the knowledge particle movement logic. It processes all entities with CKnowledgeParticle, CTransform, and CParticleInfluence components, updating their positions and applying influence to civilizations within their radius.
void KnowledgeParticleMovementSystem::Update(float dt, EntityManager& entityManager) {
	// Get a reference to the list of all entities managed by the EntityManager
	auto& entities = entityManager.GetEntities();

	// Loop through all entities and process those with CKnowledgeParticle, CTransform, and CParticleInfluence
	for (auto& ePtr : entities) {
		// Get the raw pointer to the entity from the unique_ptr
		Entity* particle = ePtr.get();

		// Skip dead entities
		if (!particle->IsAlive()) continue;

		// Get the CKnowledgeParticle, CTransform, and CParticleInfluence components from the entity
		auto* kp = particle->GetComponent<CKnowledgeParticle>();
		auto* transform = particle->GetComponent<CTransform>();
		auto* influence = particle->GetComponent<CParticleInfluence>();

		// If any of the required components are missing, skip to the next entity
		if (!kp || !transform || !influence) continue;

		// Update the position of the knowledge particle based on its velocity and the time delta
		transform->position += transform->velocity * dt;

		// Optional: simple damping
		transform->velocity *= 0.99f;

		// Apply the influence of the knowledge particle on civilizations within its influence radius
		ApplyInfluence(kp, transform, influence, entityManager, dt);

		//	Decrease the value of the knowledge particle over time, and destroy it if its value reaches zero
		kp->value -= dt * 0.05f;
		if (kp->value <= 0.0f) particle->Destroy();
	}
}
/////////////////////////////////



/////////////////////////////////
// ApplyInfluence - Applies the influence of a knowledge particle on civilizations within its influence radius. It calculates the influence strength based on distance and falloff, and updates the known technologies of affected civilizations accordingly.
void KnowledgeParticleMovementSystem::ApplyInfluence(CKnowledgeParticle* kp, CTransform* particleTransform, CParticleInfluence* influence, EntityManager& entityManager, float dt) {
	// Get a reference to the list of all entities managed by the EntityManager
	auto& entities = entityManager.GetEntities();

	// Loop through all entities and process those with CCivilisationTech
	for (auto& ePtr : entities) {
		// Get the raw pointer to the entity from the unique_ptr
		Entity* civ = ePtr.get();

		// Skip dead entities
		if (!civ->IsAlive()) continue;

		// Get the CCivilisationTech and CTransform components from the civilization entity
		auto* civTech = civ->GetComponent<CCivilisationTech>();
		auto* civTransform = civ->GetComponent<CTransform>();

		// If either component is missing, skip to the next entity
		if (!civTech || !civTransform) continue;

		// Calculate the distance between the knowledge particle and the civilization
		float dist = (particleTransform->position - civTransform->position).Mag();

		// If the distance is greater than the influence radius, skip to the next entity
		if (dist > influence->influenceRadius) continue;

		// Calculate the falloff based on distance and influence radius, and apply the influence to the civilization's known technologies
		float falloff = 1.0f - (dist / influence->influenceRadius);
		float amount = kp->value * falloff * dt;

		// Passive progress boost
		civTech->passiveProgress[kp->techId] += amount;

		// Innovation pressure boost (tiny)
		civTech->innovationPressure += amount * 0.001f;
	}
}
/////////////////////////////////
