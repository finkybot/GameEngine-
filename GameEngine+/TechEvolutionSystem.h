/////////////////////////////////
// TechEvolutionSystem.hpp
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include "System.h"
#include "EntityManager.h"
#include "CCivilisationTech.h"
#include "CTechNode.h"
#include "TechRegistry.h"
/////////////////////////////////



/////////////////////////////////
// TechEvolutionSystem class - Manages the evolution of technologies in a civilization. It processes entities with CCivilisationTech and CTechNode, handling the progression and unlocking of technologies based on defined rules and dependencies.
//								|
//								|_______________________________________________________________________
class TechEvolutionSystem : public System {
	/////////////////////////////////
	// Public interface for the TechEvolutionSystem class
public:
	/////////////////////////////////
	TechEvolutionSystem(TechRegistry& techRegistry) : techRegistry(techRegistry) {}
	/////////////////////////////////
	 
	
	/////////////////////////////////
	void ProcessCivilisationTech(Entity* entity, CCivilisationTech* civTechComp, EntityManager& entityManager, float dt);
	/////////////////////////////////


	
	/////////////////////////////////
	float globalResearchRate = 0.15f; // Global research rate modifier affecting all civilizations
	/////////////////////////////////



	/////////////////////////////////
	// Update - Overrides the base class Update method to implement the technology evolution logic. It processes all entities with CCivilisationTech and CTechNode, updating their research progress and unlocking technologies as appropriate.
	void Update(float dt, EntityManager& entityManager) override;
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the TechEvolutionSystem class. These variables can be used to track internal state, configuration, or other relevant data needed for the system's operation.
private:
	TechRegistry& techRegistry; // Reference to the TechRegistry for accessing technology nodes and their properties	

	CTechNode* FindTechNode(const std::string& techId);
	bool PrerequisitesMet(const CCivilisationTech& civTech, const CTechNode& techNode);
	float CalculateResearchRate(const CCivilisationTech& civTech, const CTechNode& techNode, float baseRate);
	/////////////////////////////////
};
/////////////////////////////////