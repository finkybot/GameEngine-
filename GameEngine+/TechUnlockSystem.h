/////////////////////////////////
// TechUnlockSystem.h - Header file for the TechUnlockSystem class, which manages the unlocking of technologies for civilizations in the game. It interacts with the TechRegistry to retrieve tech nodes and applies their effects to civilizations based on their progress and research status.
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
// TechUnlockSystem class - Manages the unlocking of technologies for civilizations in the game. It processes entities with CCivilisationTech and CTechNode, applying the effects of unlocked technologies to civilizations based on their progress and research status.
// 								|
//								|_______________________________________________________________________
class TechUnlockSystem : public System {
	/////////////////////////////////
	// Public interface for the TechUnlockSystem class
public:
	/////////////////////////////////
	// Constructor for the TechUnlockSystem class, taking a reference to a TechRegistry for accessing technology nodes and their properties.
	TechUnlockSystem(TechRegistry& registry) : techRegistry(registry) {}
	/////////////////////////////////



	/////////////////////////////////
	// Update - Overrides the base class Update method to implement the technology unlocking logic. It processes all entities with CCivilisationTech, checking their research progress and applying the effects of unlocked technologies as appropriate.
	void Update(float dt, EntityManager& entityManager) override;
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the TechUnlockSystem class. These variables can be used to track internal state,
private:
	/////////////////////////////////
	// Reference to the TechRegistry for accessing technology nodes and their properties
	TechRegistry& techRegistry;
	/////////////////////////////////



	/////////////////////////////////
	// Processes a civilization entity, checking its research progress and applying the effects of unlocked technologies as appropriate
	void ProcessCivilisation(Entity* civ, CCivilisationTech* civTech, EntityManager& entityManager);
	/////////////////////////////////
	 
	 
	
	/////////////////////////////////
	// Applies the effects of a technology node to a civilization entity
	void ApplyTechEffects(Entity* civ, CCivilisationTech* civTech, const CTechNode& techNode);
	/////////////////////////////////
	


	/////////////////////////////////
	// Spawns knowledge particles for a civilization entity when a technology is unlocked
	void SpawnKnowledgeParticles(Entity* civ, const CTechNode& techNode, EntityManager& entityManager);
	/////////////////////////////////
};
/////////////////////////////////
