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
	auto& allEntities = entityManager.GetEntities();

	// Loop through all entities and process those with CKnowledgeParticle, CTransform, and CParticleInfluence components
	for (auto& ePtr : allEntities) {
		Entity* particle = ePtr.get();

		// Skip dead entities
		if (!particle->IsAlive())
			continue;

		// Get the CKnowledgeParticle component for techId and value
		auto* kp = particle->GetComponent<CKnowledgeParticle>();
		if (!kp)
			continue;

		// Get the CTransform component for position and velocity
		auto* transform = particle->GetComponent<CTransform>();
		if (!transform)
			continue;

		// Get the CParticleInfluence component for influence radius and falloff
		auto* influence = particle->GetComponent<CParticleInfluence>();
		if (!influence)
			continue;

		// Movement is handled by PhysicsSystem via transform->velocity
		// We only apply influence here.
		ApplyInfluence(kp, transform, influence, entityManager, dt);
	}
}
/////////////////////////////////



/////////////////////////////////
// ApplyInfluence - Applies the influence of a knowledge particle on civilizations within its influence radius. It calculates the influence strength based on distance and falloff, and updates the known technologies of affected civilizations accordingly.
void KnowledgeParticleMovementSystem::ApplyInfluence(CKnowledgeParticle* kp, CTransform* particleTransform,
													 CParticleInfluence* influence, EntityManager& entityManager,
													 float dt) {
	// Get a reference to the list of all entities managed by the EntityManager
	auto& allEntities = entityManager.GetEntities();

	// Loop through all entities to find civilizations within the influence radius
	for (auto& ePtr : allEntities) {
		Entity* civ = ePtr.get();

		// Skip dead entities
		if (!civ->IsAlive())
			continue;

		// Get the CCivilisationTech component for technology progress
		auto* civTech = civ->GetComponent<CCivilisationTech>();
		if (!civTech)
			continue;

		// Get the CTransform component for the civilization's position
		auto* civTransform = civ->GetComponent<CTransform>();
		if (!civTransform)
			continue;

		// Calculate the distance between the knowledge particle and the civilization
		float dist = (particleTransform->position - civTransform->position).Mag();
		if (dist > influence->influenceRadius)
			continue;

		// Calculate the influence strength based on distance and falloff
		float falloff = 1.0f - (dist / influence->influenceRadius);
		float amount = kp->value * falloff * dt;

		// Update the civilization's passive progress towards the technology represented by the knowledge particle
		civTech->passiveProgress[kp->techId] += amount;
		civTech->innovationPressure += amount * 0.001f;
	}
}
/////////////////////////////////
