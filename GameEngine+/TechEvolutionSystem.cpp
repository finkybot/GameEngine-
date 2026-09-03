/////////////////////////////////
// TechEvolutionSystem.cpp
// Implementation of the TechEvolutionSystem class, which manages
// the evolution and unlocking of technologies for civilizations.
// It uses passive progress, active research, prerequisites, and
// proximity-based bonuses via the "Civilisations" spatial layer.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "TechEvolutionSystem.h"
#include "CTransform.h"
#include "Entity.h"
#include <cmath>
/////////////////////////////////



/////////////////////////////////
// Update - legacy entry point (unused; evolution is driven by jobs)
void TechEvolutionSystem::Update(float dt, EntityManager& entityManager) {
	(void)dt;
	(void)entityManager;
}
/////////////////////////////////




/////////////////////////////////
// FindTechNode - helper to look up a tech node by ID
const CTechNode* TechEvolutionSystem::FindTechNode(const std::string& techId) {
	return techRegistry.GetTechNode(techId);
}
/////////////////////////////////



/////////////////////////////////
// PrerequisitesMet - checks if all prerequisites for a tech are known
bool TechEvolutionSystem::PrerequisitesMet(const CCivilisationTech& civTech, const CTechNode& techNode) {
	// Check if all prerequisites for the given tech node are met based on the known technologies of the civilization.
	for (const auto& prereqId : techNode.prerequisites) {
		// If a prerequisite is not known or has a progress less than 1.0, return false.
		auto it = civTech.knownTechs.find(prereqId);
		if (it == civTech.knownTechs.end() || it->second < 1.0f)
			return false;
	}

	// All prerequisites are met, return true.
	return true;
}
/////////////////////////////////



/////////////////////////////////
// CalculateResearchRate - base research rate modified by civ stats
float TechEvolutionSystem::CalculateResearchRate(const CCivilisationTech& civTech, const CTechNode& techNode, float baseRate) {
	// Start with the base research rate
	float rate = baseRate;

	// Literacy and openness can boost research
	rate *= civTech.literacy;
	rate *= civTech.openness;

    // Difficulty scaling (higher difficulty → slower research)
	if (techNode.baseDifficulty > 0.0f)
		rate *= (1.0f / techNode.baseDifficulty);

	// Clamp to avoid extreme values
	if (rate < 0.0f)
		rate = 0.0f;
	if (rate > 10.0f)
		rate = 10.0f;

	// Return the final calculated research rate
	return rate;
}
/////////////////////////////////



/////////////////////////////////
// ProcessCivilisationTech - main evolution step for a single civ
void TechEvolutionSystem::ProcessCivilisationTech(Entity* civ, CCivilisationTech* civTech, EntityManager& em, float dt) {
	// Ensure valid pointers to civilisation entity and technology component
	if (!civ || !civTech) return;

	// -----------------------------
	// 1. Base research rate
	// -----------------------------
	float baseRate = globalResearchRate;

	// -----------------------------
	// 2. Spatial proximity bonus
	//    via "Civilisations" layer
	// -----------------------------
	float proximityBoost = 1.0f;

	// Access the spatial layer registry to query nearby civilizations
	auto* registry = em.GetSpatialLayerRegistry();
	
	// If the registry exists, query the "Civilisations" layer for nearby civilizations
	if (registry) {
		// Get the "Civilisations" spatial layer for proximity queries
		auto& civLayer = registry->GetLayer("Civilisations");

		// Get the position of the current civilization entity
		auto* t = civ->GetComponent<CTransform>();
		const Vec2 civPos = t ? t->position : civ->GetCentrePoint();

		// Use diffusion config for proximity radius if available
		const float maxDist = m_diffusionConfig ? m_diffusionConfig->maxProximityDistance : 250.0f;


		// Query the spatial layer for nearby civilizations within the specified distance
		std::vector<Entity*> nearby;

		// Perform the query to find nearby civilizations, excluding the current civilization entity
		civLayer.Query(nearby, civPos, maxDist, civ);

		// Iterate through the nearby civilizations to calculate the proximity boost based on their known technologies
		for (Entity* other : nearby) {
			// Skip if the other civilization is null, not alive, or is the same as the current civilization
			if (!other || !other->IsAlive() || other == civ) continue;

			// Get the technology component of the other civilization
			auto* otherTech = other->GetComponent<CCivilisationTech>();

			// Skip if the other civilization does not have a technology component
			if (!otherTech)	continue;

			// Get the position of the other civilization entity
			auto* ot = other->GetComponent<CTransform>();
			const Vec2 otherPos = ot ? ot->position : other->GetCentrePoint();

			// Calculate the distance between the current civilization and the other civilization
			float dx = otherPos.x - civPos.x;
			float dy = otherPos.y - civPos.y;
			float dist = std::sqrt(dx * dx + dy * dy); // Euclidean distance

			// Skip if the distance exceeds the maximum proximity distance
			if (dist > maxDist)	continue;

			// Calculate a proximity factor based on distance, using a smoothstep function for gradual falloff
			float p = 1.0f - (dist / maxDist);
			p = p * p * (3.0f - 2.0f * p); // smoothstep

			// Civs with more knowledge accelerate research
			for (const auto& [techId, otherKnown] : otherTech->knownTechs) {
				// Check if the current civilization knows this technology
				float selfKnown = civTech->knownTechs[techId];

				// If the other civilization knows more about this technology than the current civilization, apply a proximity boost
				if (otherKnown > selfKnown) {
					proximityBoost += p * 0.25f; // tune multiplier
				}
			}
		}
	}

	// Clamp proximity boost
	if (proximityBoost < 0.5f)
		proximityBoost = 0.5f;
	if (proximityBoost > 3.0f)
		proximityBoost = 3.0f;

	// -----------------------------
	// 3. Apply research to active techs
	// -----------------------------
	for (auto it = civTech->activeResearch.begin(); it != civTech->activeResearch.end();) {
		const std::string techId = it->first;
		float& progress = it->second;

		// Find the corresponding tech node in the registry
		const CTechNode* node = FindTechNode(techId);

		// Skip if the tech node is not found
		if (!node) {
			++it;
			continue;
		}

		// Check if prerequisites are met for this tech node
		if (!PrerequisitesMet(*civTech, *node)) {
			++it;
			continue;
		}

		// Calculate the research rate for this tech node, factoring in base rate and proximity boost
		float rate = CalculateResearchRate(*civTech, *node, baseRate);
		rate *= proximityBoost;

		// Update the progress for this tech node based on the calculated rate and delta time
		progress += rate * dt;

		// Completion check
		if (progress >= node->requiredKnowledge) {
			progress = node->requiredKnowledge;
			civTech->knownTechs[techId] = 1.0f;
			if (civTech->unlockedTechs.insert(techId).second) {
				m_totalTechCompleted++;
			}
			it = civTech->activeResearch.erase(it);
			continue;
		}

		++it;
	}
}
/////////////////////////////////