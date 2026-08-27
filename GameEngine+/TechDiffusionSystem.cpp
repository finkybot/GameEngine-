/////////////////////////////////
// TechDiffusionSystem.cpp - Implementation of the TechDiffusionSystem class, which handles the diffusion of technologies between civilizations in the game. This system processes entities with CivilisationTechComponent and TechNodeComponent, 
// applying diffusion logic based on proximity, openness, literacy, and other factors. The implementation includes methods for updating the system, processing diffusion for individual civilizations, applying diffusion effects, calculating 
// proximity between civilizations, and finding specific tech nodes by ID.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "TechDiffusionSystem.h"
/////////////////////////////////



/////////////////////////////////
// Update - Overrides the base class Update method to implement the technology diffusion logic. It processes all entities with CivilisationTechComponent and TechNodeComponent, updating their known technologies based on interactions with other civilizations.
void TechDiffusionSystem::Update(float dt, EntityManager& entityManager) {
	// Get a reference to the list of all entities managed by the EntityManager
	auto& allEntities = entityManager.GetEntities();

	// Loop through all entities and process those with CivilisationTechComponent
	for (auto& ePtr : allEntities) {
		// Skip dead entities
		if (!ePtr->IsAlive())
			continue;

		// Get the CivilisationTechComponent from the entity
		Entity* civ = ePtr.get();
		
		// Skip entities that don't have a CivilisationTechComponent
		auto* civTech = civ->GetComponent<CCivilisationTech>();
		if (!civTech)
			continue;

		// Process technology diffusion for the civilization entity
		ProcessTechDiffusionForCivilisation(civ, civTech, entityManager, dt);
	}
}
/////////////////////////////////



/////////////////////////////////
// ProcessTechDiffusionForCivilisation - Processes the technology diffusion for a single civilization entity with a CCivilisationTech. It calculates the diffusion strength based on proximity and other factors, and applies diffusion effects to the civilization's known technologies.
void TechDiffusionSystem::ProcessTechDiffusionForCivilisation(Entity* civEntity, CCivilisationTech* civTechComp, EntityManager& entityManager, float dt) {
	// Get a reference to the list of all entities managed by the EntityManager
	auto& allEntities = entityManager.GetEntities();

	// Loop through all other entities to calculate diffusion effects
	for (auto& otherPtr : allEntities) {
		// Skip dead entities
		if (!otherPtr->IsAlive())
			continue;

		// Skip the current civilization entity to avoid self-diffusion
		Entity* other = otherPtr.get();
		if (other == civEntity)
			continue;

		// Get the CCivilisationTech from the other entity
		auto* otherTech = other->GetComponent<CCivilisationTech>();
		if (!otherTech)
			continue;

		// Calculate the proximity between the two civilizations
		float proximity = CalculateProximity(civEntity, other);
		if (proximity <= 0.0f)
			continue;

		// Get the openness of the current civilization
		float openness = civTechComp->openness;
		
		// Calculate the diffusion strength based on base rate, openness, and proximity
		float diffusionStrength = baseDiffusionRate * openness * proximity;

		// Apply the diffusion effects to the current civilization's known technologies based on the other civilization's known technologies
		ApplyDiffusion(civTechComp, otherTech, diffusionStrength, entityManager, dt);
	}
}
/////////////////////////////////



/////////////////////////////////
// ApplyDiffusion - Applies the diffusion effects from another civilization's known technologies to the current civilization's known technologies. It updates the passive progress towards unlocking new technologies based on the diffusion strength and time delta.
void TechDiffusionSystem::ApplyDiffusion(CCivilisationTech* civTech, CCivilisationTech* otherCivTech, float diffusionStrength, EntityManager& entityManager, float dt) {
	// Iterate through the known technologies of the other civilization
	for (const auto& [techId, knownLevel] : otherCivTech->knownTechs) {
		// Skip technologies that the current civilization already knows
		if (civTech->knownTechs.contains(techId))
			continue;

		// Find the TechNode for the technology ID
		CTechNode* techNode = FindTechNode(entityManager, techId);
		if (!techNode)
			continue;

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
	Vec2 posA =	civEntity->GetPosition();
	Vec2 posB = otherCivEntity->GetPosition();

	float dist = (posA - posB).Mag();

	if (dist > 2000.0f)
		return 0.0f;

	return 1.0f - (dist / 2000.0f);
}
/////////////////////////////////



/////////////////////////////////
// FindTechNode - Searches for a CTechNode in the EntityManager based on the provided techId. Returns a pointer to the CTechNode if found, or nullptr if not found.
CTechNode* TechDiffusionSystem::FindTechNode(EntityManager& entityManager, const std::string& techId) {
	auto& allEntities = entityManager.GetEntities();

	for (auto& ePtr : allEntities) {
		Entity* e = ePtr.get();
		CTechNode* node = e->GetComponent<CTechNode>();
		if (!node)
			continue;

		if (node->id == techId)
			return node;
	}

	return nullptr;
}
/////////////////////////////////