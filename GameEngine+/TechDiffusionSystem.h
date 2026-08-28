/////////////////////////////////
// TechDiffusionSystem.hpp
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include "System.h"
#include "EntityManager.h"
#include "CCivilisationTech.h"
#include "CTechNode.h"
#include "TechRegistry.h"
#include "Vec2.h"
/////////////////////////////////



/////////////////////////////////
// TechDiffusionSystem class - Manages the diffusion of technologies between civilizations. It processes entities with CCivilisationTech and CTechNode, handling the spread of technologies based on defined rules and interactions between civilizations.
// 								|	
//								|_______________________________________________________________________
class TechDiffusionSystem : public System {
	/////////////////////////////////
	// Public interface for the TechDiffusionSystem class
public:
	/////////////////////////////////
	// Constructor for the TechDiffusionSystem class, taking a reference to a TechRegistry for accessing technology nodes and their properties.
	TechDiffusionSystem(TechRegistry& registry) : techRegistry(registry) {}
	/////////////////////////////////



	/////////////////////////////////
	void ProcessTechDiffusionForCivilisation(Entity* civEntity, CCivilisationTech* civTechComp, EntityManager& entityManager, float dt);
	/////////////////////////////////



	/////////////////////////////////
	float baseDiffusionRate = 0.005f; // Base diffusion rate modifier affecting all civilizations
	/////////////////////////////////



	/////////////////////////////////
	// Update - Overrides the base class Update method to implement the technology diffusion logic. It processes all entities with CivilisationTechComponent and TechNodeComponent, updating their known technologies based on interactions with other civilizations.
	void Update(float dt, EntityManager& entityManager) override;
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the TechDiffusionSystem class. These variables can be used to track internal state, configuration, or other relevant data needed for the system's operation.
private:
	/////////////////////////////////
	// Reference to the TechRegistry for accessing technology nodes and their properties
	TechRegistry& techRegistry; // Reference to the TechRegistry for accessing technology nodes and their properties
	/////////////////////////////////
	
	

	/////////////////////////////////
	void ApplyDiffusion(CCivilisationTech* civTech, CCivilisationTech* otherCivTech, float diffusionStrength, EntityManager& entityManager, float dt);
	float CalculateProximity(Entity* civEntity, Entity* otherCivEntity);
	/////////////////////////////////
};
/////////////////////////////////