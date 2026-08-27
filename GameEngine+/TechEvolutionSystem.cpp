/////////////////////////////////
// TechEvolutionSystem.cpp - Implementation of the TechEvolutionSystem class, responsible for managing technology evolution in the game. It updates research progress and unlocks technologies for civilizations based on their research rates and other factors.
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "TechEvolutionSystem.h"
/////////////////////////////////



/////////////////////////////////
// Update - Overrides the base class Update method to implement the technology evolution logic. It processes all entities with CCivilisationTech and CTechNode, updating their research progress and unlocking technologies as appropriate.
void TechEvolutionSystem::Update(float dt, EntityManager& entityManager) {

	// Get a reference to the list of entities managed by the EntityManager
	auto& entities = entityManager.GetEntities();

	// loop through all entities and process those with CCivilisationTech
	for (auto& up : entities) {
		Entity* entity = up.get();
		// Skip dead entities
		if (!entity->IsAlive())
			continue;
		// Get the CCivilisationTech from the entity
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
	// We must iterate using an iterator, not a range-for loop
	for (auto it = civTechComp->activeResearch.begin(); it != civTechComp->activeResearch.end();) {
		const std::string& techId = it->first;
		float& progress = it->second;

		CTechNode* techNode = FindTechNode(entityManager, techId);
		if (!techNode) {
			++it;
			continue;
		}

		if (PrerequisitesMet(*civTechComp, *techNode)) {
			float researchRate = CalculateResearchRate(*civTechComp, *techNode, globalResearchRate);
			progress += researchRate * dt;

			if (progress >= techNode->requiredKnowledge) {
				// Unlock tech
				civTechComp->knownTechs[techId] = 1.0f;

				// Erase safely using iterator return value
				it = civTechComp->activeResearch.erase(it);
				continue; // Skip increment because erase already advanced the iterator
			}
		}

		++it; // Normal increment
	}
}
/////////////////////////////////



/////////////////////////////////
// FindTechNode - Searches for a CTechNode in the EntityManager based on the provided techId. Returns a pointer to the CTechNode if found, or nullptr if not found.
CTechNode* TechEvolutionSystem::FindTechNode(EntityManager& entityManager, const std::string& techId) {
	// Get a reference to the list of all entities managed by the EntityManager
	auto& allEntities = entityManager.GetEntities();

	// Iterate through all entities to find the one with the matching CTechNode
	for (auto& ePtr : allEntities) {
		Entity* e = ePtr.get();

		// Skip dead entities
		if (!e->IsAlive())
			continue;

		// Get the CTechNode from the entity
		CTechNode* node = e->GetComponent<CTechNode>();
		if (!node)
			continue;

		// Check if the CTechNode's ID matches the requested techId
		if (node->id == techId)
			return node; // Return the found CTechNode
	}

	// If no matching CTechNode is found, return nullptr
	return nullptr;
}
/////////////////////////////////



/////////////////////////////////
// PrerequisitesMet - Checks if the prerequisites for a given CTechNode are met based on the known technologies in the civTech. Returns true if all prerequisites are met, false otherwise.
bool TechEvolutionSystem::PrerequisitesMet(const CCivilisationTech& civTech, const CTechNode& techNode) {
	// Iterate through all prerequisites of the CTechNode
	for (const auto& prereq : techNode.prerequisites) {
		
		// Check if the prerequisite is not known in the civTech
		if (civTech.knownTechs.find(prereq) == civTech.knownTechs.end()) {
			return false; // A prerequisite is not met
		}
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

	// Tech difficulty scaling
	float difficultyFactor = 1.0f / techNode.baseDifficulty; // higher difficulty → slower research

	// Combine factors
	float rate = baseRate * globalResearchRate;
	rate *= literacyFactor;
	rate *= innovationFactor;
	rate *= difficultyFactor;

	return rate;
}
/////////////////////////////////