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
	void ProcessTechDiffusionForCivilisation(Entity* civEntity, CCivilisationTech* civTechComp, EntityManager& entityManager, float dt);
	void ApplyDiffusion(CCivilisationTech* civTech, CCivilisationTech* otherCivTech, float diffusionStrength, EntityManager& entityManager, float dt);
	float CalculateProximity(Entity* civEntity, Entity* otherCivEntity);
	CTechNode* FindTechNode(EntityManager& entityManager, const std::string& techId);
	/////////////////////////////////
};
/////////////////////////////////