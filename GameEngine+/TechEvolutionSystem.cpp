/////////////////////////////////
// TechEvolutionSystem.cpp - Implementation of the TechEvolutionSystem class, responsible for managing technology evolution in the game. It updates research progress and unlocks technologies for civilizations based on their research rates and other factors.
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "TechEvolutionSystem.h"
#include "CTransform.h"
#include "CKnowledgeParticle.h"
#include "CParticleInfluence.h"
#include "CChunkKnowledge.h"
#include <cmath>
/////////////////////////////////



/////////////////////////////////
// Update - Overrides the base class Update method to implement the technology evolution logic. It processes all entities with CCivilisationTech and CTechNode, updating their research progress and unlocking technologies as appropriate.
void TechEvolutionSystem::Update(float dt, EntityManager& entityManager) {
	// Get a reference to the list of all entities managed by the EntityManager
	auto& entities = entityManager.GetEntities();

	// Loop through all entities and process those with CCivilisationTech
	for (auto& up : entities) {
		Entity* entity = up.get();

		// Skip dead entities
		if (!entity->IsAlive())
			continue;

		// Get the CCivilisationTech component from the entity
		auto civTechComp = entity->GetComponent<CCivilisationTech>();
		
		// If the entity has a CCivilisationTech, process its technology evolution
		if (civTechComp) {
			ProcessCivilisationTech(entity, civTechComp, entityManager, dt);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// ProcessCivilisationTech - Processes the technology evolution for a single entity with a CCivilisationTech. It updates research progress, checks prerequisites, and unlocks technologies as appropriate.
void TechEvolutionSystem::ProcessCivilisationTech(Entity* entity, CCivilisationTech* civTechComp, EntityManager& entityManager, float dt) {
	// Get the CTransform component from the entity to determine its position in the game world
	auto* transform = entity->GetComponent<CTransform>();

	// If the entity does not have a CTransform component, return early as we cannot calculate proximity or apply knowledge particle effects
	if (!transform)	return;

	// 1) Promote passive → active research (with full skip rules)
	for (auto& [techId, passive] : civTechComp->passiveProgress) {
		// Skip if already fully known
		auto knownIt = civTechComp->knownTechs.find(techId);
		
		// If the technology is already known and fully researched, skip to the next technology
		if (knownIt != civTechComp->knownTechs.end() && knownIt->second >= 1.0f) continue;

		// Skip if already actively researching
		if (civTechComp->activeResearch.contains(techId)) continue;

		// Skip if prerequisites are not met
		CTechNode* node = FindTechNode(techId);
		if (!node || !PrerequisitesMet(*civTechComp, *node)) continue;

		// Promote passive → active once threshold reached (5%)
		if (passive >= node->requiredKnowledge * 0.05f)
			civTechComp->activeResearch[techId] = passive; // seed progress
	}

	// 2) Update active research progress
	for (auto it = civTechComp->activeResearch.begin(); it != civTechComp->activeResearch.end();) {
		const std::string& techId = it->first;
		float& progress = it->second;

		// Lookup tech definition
		CTechNode* techNode = FindTechNode(techId);
		if (!techNode) {
			++it;
			continue;
		}

		// Prerequisite check
		if (!PrerequisitesMet(*civTechComp, *techNode)) {
			++it;
			continue;
		}

		// Calculate research rate
		float researchRate = CalculateResearchRate(*civTechComp, *techNode, globalResearchRate);

		// Knowledge particle boost
		float knowledgeBoost = 0.0f;
		auto& knowledgeParticles = entityManager.GetEntities(EntityType::KnowledgeParticle);

		for (Entity* e : knowledgeParticles) {
			// Skip dead entities
			if (!e || !e->IsAlive()) continue;

			// Get the CKnowledgeParticle, CParticleInfluence, and CTransform components from the entity
			auto* kp = e->GetComponent<CKnowledgeParticle>();
			auto* influence = e->GetComponent<CParticleInfluence>();
			auto* pTransform = e->GetComponent<CTransform>();

			// If any of the required components are missing, skip to the next entity
			if (!kp || !influence || !pTransform) continue;

			// Skip if the knowledge particle does not match the current tech being researched
			float dist = transform->position.Distance(pTransform->position);

			// If the distance is within the influence radius, apply a knowledge boost based on the knowledge particle's value and the distance falloff
			if (dist < influence->influenceRadius) {
				float falloff = 1.0f - (dist / influence->influenceRadius);
				knowledgeBoost = std::min(knowledgeBoost + kp->value * falloff, 2.0f);
			}
		}

		// Apply knowledge boost
		researchRate += knowledgeBoost;

		// -------------------------------
		// PASSIVE BONUS (max +5%)
		// -------------------------------
		float passive = civTechComp->passiveProgress[techId];
		float passiveFraction = passive / techNode->requiredKnowledge;

		float passiveBonus = 0.0f;
		if (passiveFraction > 0.10f) {
			float scaled = (passiveFraction - 0.10f) / 0.90f; // maps 10%→100% to 0→1
			passiveBonus = scaled * 0.05f;					  // max +5%
		}

		researchRate *= (1.0f + passiveBonus);
		// -------------------------------

		// Debug values
		civTechComp->debugResearchRate[techId] = researchRate;
		civTechComp->debugKnowledgeBoost[techId] = knowledgeBoost;
		civTechComp->debugDifficultyFactor[techId] = (1.0f / techNode->baseDifficulty);

		// Apply research (real-time stable)
		progress += researchRate * dt;

		// Unlock tech
		if (progress >= techNode->requiredKnowledge) {
			civTechComp->knownTechs[techId] = 1.0f;
			it = civTechComp->activeResearch.erase(it);
			m_totalTechCompleted++;
			continue;
		}

		++it;
	}
}

/////////////////////////////////



/////////////////////////////////
// FindTechNode - Retrieves a pointer to a CTechNode from the TechRegistry based on the provided techId. Returns nullptr if the techId is not found in the registry.
CTechNode* TechEvolutionSystem::FindTechNode(const std::string& techId) {
	return const_cast<CTechNode*>(techRegistry.GetTechNode(techId));
}
/////////////////////////////////



/////////////////////////////////
// PrerequisitesMet - Checks if the prerequisites for a given CTechNode are met based on the known technologies in the civTech. Returns true if all prerequisites are met, false otherwise.
bool TechEvolutionSystem::PrerequisitesMet(const CCivilisationTech& civTech, const CTechNode& techNode) {
	// Iterate through all prerequisites of the CTechNode
	for (const auto& prereq : techNode.prerequisites) {
		
		// Check if the prerequisite is not known in the civTech
		auto it = civTech.knownTechs.find(prereq);
		if (it == civTech.knownTechs.end() || it->second < 1.0f) return false;
	}
	
	return true; // All prerequisites are met
}
/////////////////////////////////



/////////////////////////////////
// CalculateResearchRate - Calculates the research rate for a given CTechNode based on the CCivilisationTech and a baseRate. Returns the calculated research rate as a float.
float TechEvolutionSystem::CalculateResearchRate(const CCivilisationTech& civTech, const CTechNode& techNode, float baseRate) {
	// Civilisation traits
	float literacyFactor = 0.5f + civTech.literacy;				// literacy ∈ [0,1]
	float innovationFactor = 0.5f + civTech.innovationPressure; // innovation ∈ [0,1]
	float categoryBiasFactor = 1.0f;							// Default factor for category bias
	
	// Check if the techNode's category has a bias in civTech
	auto it = civTech.categoryBias.find(techNode.category);
	
	// If a bias is found, adjust the categoryBiasFactor accordingly
	if (it != civTech.categoryBias.end()) {
		categoryBiasFactor = 0.5f + it->second; // category bias ∈ [0,1]
	}

	// Tech difficulty scaling
	float difficultyFactor = 1.0f / techNode.baseDifficulty; // higher difficulty → slower research

	// Combine factors
	float rate = baseRate * globalResearchRate;
	rate *= literacyFactor;
	rate *= innovationFactor;
	rate *= categoryBiasFactor;
	rate *= difficultyFactor;

	return rate;
}
/////////////////////////////////