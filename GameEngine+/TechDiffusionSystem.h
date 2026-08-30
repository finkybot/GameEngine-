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
#include <atomic>
#include <cstdint>
#include "WorldDiffusionConfig.h"
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
	TechDiffusionSystem(TechRegistry& registry, const WorldDiffusionConfig& cfg): techRegistry(registry), config(cfg) {}

	/////////////////////////////////



	/////////////////////////////////
	void ProcessTechDiffusionForCivilisation(Entity* civEntity, CCivilisationTech* civTechComp, EntityManager& entityManager, float dt);
	/////////////////////////////////



	/////////////////////////////////
	uint32_t ConsumeSehCatchCount() { return m_sehCatchCount.exchange(0, std::memory_order_acq_rel); }
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
	WorldDiffusionConfig config; // Configuration for the diffusion system, containing parameters that affect how technologies diffuse between civilizations
	/////////////////////////////////
	
	

	/////////////////////////////////
	void ApplyDiffusion(CCivilisationTech* civTech, CCivilisationTech* otherCivTech, float diffusionStrength, EntityManager& entityManager, float dt);
	float CalculateProximity(Entity* civEntity, Entity* otherCivEntity);
	std::atomic<uint32_t> m_sehCatchCount{ 0 };
	/////////////////////////////////
};
/////////////////////////////////