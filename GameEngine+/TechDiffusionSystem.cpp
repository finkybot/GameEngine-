/////////////////////////////////
// TechDiffusionSystem.cpp - Implementation of the TechDiffusionSystem class, which handles the diffusion of technologies between civilizations in the game. This system processes entities with CivilisationTechComponent and TechNodeComponent, 
// applying diffusion logic based on proximity, openness, literacy, and other factors. The implementation includes methods for updating the system, processing diffusion for individual civilizations, applying diffusion effects, calculating 
// proximity between civilizations, and finding specific tech nodes by ID.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "TechDiffusionSystem.h"
#include "CTransform.h"
#include "CKnowledgeParticle.h"
#include "CParticleInfluence.h"
#include <cmath>
/////////////////////////////////



/////////////////////////////////
// Update - Overrides the base class Update method to implement the technology diffusion logic. It processes all entities with CivilisationTechComponent and TechNodeComponent, updating their known technologies based on interactions with other civilizations.
void TechDiffusionSystem::Update(float dt, EntityManager& entityManager) {
	// Get a reference to the list of all entities managed by the EntityManager
	auto& allEntities = entityManager.GetEntities();

	// Loop through all entities and process those with CCivilisationTech
	for (auto& ePtr : allEntities) {
		// Get the raw pointer to the entity from the unique_ptr
		Entity* civ = ePtr.get();

		//	Skip dead entities
		if (!civ || !civ->IsAlive()) continue;

		// Get the CCivilisationTech component from the civilization entity
		auto* civTech = civ->GetComponent<CCivilisationTech>();
		
		//	If the civilization entity does not have a CCivilisationTech component, skip to the next entity
		if (!civTech) continue;

		// Process the technology diffusion for the civilization entity
		ProcessTechDiffusionForCivilisation(civ, civTech, entityManager, dt);
	}
}

/////////////////////////////////



/////////////////////////////////
// ProcessTechDiffusionForCivilisation - Processes the technology diffusion for a single civilization entity with a CCivilisationTech. It calculates the diffusion strength based on proximity and other factors, and applies diffusion effects to the civilization's known technologies.
void TechDiffusionSystem::ProcessTechDiffusionForCivilisation(Entity* civEntity, CCivilisationTech* civTech, EntityManager& entityManager, float dt) {
	// Get the CTransform component from the civilization entity to determine its position in the game world
	auto* civTransform = civEntity->GetComponent<CTransform>();
	
	// If the civilization entity does not have a CTransform component, return early as we cannot calculate proximity
	if (!civTransform) return;

	// Get a reference to the list of all entities managed by the EntityManager
	auto& entities = entityManager.GetEntities();

	// Loop through all entities to find other civilizations and apply diffusion effects
	for (auto& otherPtr : entities) {
		// Skip dead entities
		Entity* other = otherPtr.get();
		if (!other || other == civEntity || !other->IsAlive()) continue;

		// Get the CCivilisationTech component from the other entity
		auto* otherTech = other->GetComponent<CCivilisationTech>();
		
		// If the other entity does not have a CCivilisationTech component, skip to the next entity
		if (!otherTech)	continue;

		// Calculate the proximity between the two civilizations
		float proximity = CalculateProximity(civEntity, other);
		
		// If the proximity is zero or negative, skip to the next entity as there is no diffusion effect
		if (proximity <= 0.0f) continue;

		// Calculate the diffusion strength based on base rate, proximity, openness, and diffusion affinity
		float openness = civTech->openness;
		float affinity = civTech->diffusionAffinity;
		float diffusionStrength = baseDiffusionRate * proximity * openness * affinity;

		// Apply the diffusion effects from the other civilization's known technologies to the current civilization's known technologies
		ApplyDiffusion(civTech, otherTech, diffusionStrength, entityManager, dt);
	}

	// Knowledge particle influence
	for (auto& ePtr : entities) {
		// Skip dead entities
		Entity* e = ePtr.get();
		if (!e || !e->IsAlive()) continue;

		// Get the CKnowledgeParticle, CParticleInfluence, and CTransform components from the entity
		auto* kp = e->GetComponent<CKnowledgeParticle>();
		auto* influence = e->GetComponent<CParticleInfluence>();
		auto* pTransform = e->GetComponent<CTransform>();

		// If any of the required components are missing, skip to the next entity
		if (!kp || !influence || !pTransform) continue;

		// Calculate the distance between the civilization and the knowledge particle
		float dist = civTransform->position.Distance(pTransform->position);
		
		// If the distance is within the influence radius, apply a passive progress boost to the civilization's known technologies based on the knowledge particle's value and the distance falloff
		if (dist < influence->influenceRadius) {
			float falloff = 1.0f - (dist / influence->influenceRadius);
			float boost = kp->value * falloff;

			// Skip if this civ already fully knows the tech
			auto knownIt = civTech->knownTechs.find(kp->techId);
			if (knownIt != civTech->knownTechs.end() && knownIt->second >= 1.0f) continue;

			// Passive progress boost for the tech carried by the particle
			civTech->passiveProgress[kp->techId] += boost * dt;
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// ApplyDiffusion - Applies the diffusion effects from another civilization's known technologies to the current civilization's known technologies. It updates the passive progress towards unlocking new technologies based on the diffusion strength and time delta.
void TechDiffusionSystem::ApplyDiffusion(CCivilisationTech* civTech, CCivilisationTech* otherCivTech, float diffusionStrength, EntityManager& entityManager, float dt) {
	// Guard against null pointers for the civilization technology components
	if (!civTech || !otherCivTech) return;
	if (otherCivTech->knownTechs.empty()) return;

	// Iterate through the known technologies of the other civilization
	for (const auto& [techId, knownLevel] : otherCivTech->knownTechs) {
		
		// Skip technologies that the current civilization already knows
		if (civTech->knownTechs.contains(techId))
			continue;

		// Find the TechNode for the technology ID
		const CTechNode* techNode = techRegistry.GetTechNode(techId);

		// If the TechNode is not found, skip to the next technology
		if (!techNode) continue;

		// Update the passive progress towards unlocking the technology
		float& progress = civTech->passiveProgress[techId];
		progress += diffusionStrength * dt;

		// If the passive progress exceeds 25% of the required knowledge, add it to active research
		if (!civTech->activeResearch.contains(techId) && progress > techNode->requiredKnowledge * 0.25f) {
			civTech->activeResearch[techId] = 0.0f; // 
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// CalculateProximity - Calculates the proximity between two civilization entities based on their positions. It returns a value between 0.0 and 1.0, where 1.0 indicates close proximity and 0.0 indicates distant or no proximity.
float TechDiffusionSystem::CalculateProximity(Entity* civEntity, Entity* otherCivEntity) {
	// Get the positions of the two civilization entities
	Vec2 posA =	civEntity->GetPosition();
	Vec2 posB = otherCivEntity->GetPosition();

	// Calculate the distance between the two positions
	float dist = (posA - posB).Mag();

	// If the distance is greater than 2000 units, return 0.0f to indicate no proximity
	if (dist > 2000.0f) return 0.0f;

	// Calculate a smooth falloff for proximity based on distance, using a cubic interpolation for a more natural transition
	float x = dist / 2000.0f;
	return 1.0f - (x * x * (3 - 2 * x));
}
/////////////////////////////////