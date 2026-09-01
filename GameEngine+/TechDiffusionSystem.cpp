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
#include <Windows.h>
#include <cmath>
/////////////////////////////////



/////////////////////////////////
// Update - Overrides the base class Update method to implement the  diffusion logic. It processes all entities with CivilisationTechComponent and TechNodeComponent, updating their known technologies based on interactions with other civilizations. (lagacy, not used) 
// Note: dont feed the functions, its called by entity manager but I am running the diffusion system in a job system, so this is not used.. (I'll leave the code here for now, but it is not used, I mean its not to be fucking used)
void TechDiffusionSystem::Update(float dt, EntityManager& entityManager) {
	//// Get a reference to the list of all entities managed by the EntityManager
	//auto& allEntities = entityManager.GetEntities();

	//// Loop through all entities and process those with CCivilisationTech
	//for (auto& ePtr : allEntities) {
	//	// Get the raw pointer to the entity from the unique_ptr
	//	Entity* civ = ePtr.get();

	//	//	Skip dead entities
	//	if (!civ || !civ->IsAlive()) continue;

	//	// Get the CCivilisationTech component from the civilization entity
	//	auto* civTech = civ->GetComponent<CCivilisationTech>();
	//	
	//	//	If the civilization entity does not have a CCivilisationTech component, skip to the next entity
	//	if (!civTech) continue;

	//	// Process the technology diffusion for the civilization entity
	//	ProcessCivToCivDiffusion(civ, civTech, entityManager, dt);
	//	ProcessParticleDiffusionForCivilisation(civ, civTech, entityManager, dt);
	//}
}

/////////////////////////////////



/////////////////////////////////
// ProcessCivToCivDiffusion - Processes the technology diffusion between two civilizations based on their proximity and other factors. It calculates the diffusion strength and applies diffusion effects from one civilization's known technologies to another's.
void TechDiffusionSystem::ProcessCivToCivDiffusion(Entity* civEntity, CCivilisationTech* civTech, EntityManager& em, float dt) {
	// Get a reference to the spatial hash grid from the EntityManager for efficient proximity queries
	//auto& grid = em.GetSpatialHash();
	auto& grid = em.GetSpatialLayerRegistry()->GetLayer("Civilisations");
	// Get the position of the civilization entity in the game world
	auto* civTransform = civEntity->GetComponent<CTransform>();
	const Vec2 civPos = civTransform ? civTransform->position : civEntity->GetCentrePoint();

	// Get the maximum proximity distance from the configuration for determining which civilizations are close enough to influence each other
	const float maxDist = config.maxProximityDistance;

	// Query the spatial hash grid for nearby entities within the maximum proximity distance, excluding the civilization entity itself
	std::vector<Entity*> nearby;
	grid.Query(nearby, civPos, maxDist, civEntity);

	    std::cout << "[Diffusion] Civ at (" << civPos.x << ", " << civPos.y << ") "
			  << "found " << nearby.size() << " neighbours\n";

	// Loop through the nearby entities and apply diffusion effects based on proximity and other factors
	for (Entity* other : nearby) {
		// Get the CCivilisationTech component from the other entity
		auto* otherTech = other->GetComponent<CCivilisationTech>();

		// If the other entity does not have a CCivilisationTech component, skip to the next entity
		if (!otherTech)	continue;

		// Calculate the distance between the two civilizations
		auto* otherTransform = other->GetComponent<CTransform>();
		const Vec2 otherPos = otherTransform ? otherTransform->position : other->GetCentrePoint();
		float dx = otherPos.x - civPos.x;
		float dy = otherPos.y - civPos.y;
		float dist = std::sqrt(dx * dx + dy * dy); // Euclidean distance, Euclid was a wierd guy, but he was right about this one

		// If the distance is greater than the maximum proximity distance, skip to the next entity
		if (dist > maxDist)	continue;
		
		// Calculate the proximity between the two civilizations
		float proximity = 1.0f - (dist / maxDist);
		proximity = proximity * proximity * (3 - 2 * proximity); // smoothstep

		// If the proximity is zero or negative, skip to the next entity as there is no diffusion effect
		if (proximity <= 0.0f)	continue;

		// Calculate the diffusion strength based on base rate, proximity, openness, and diffusion affinity
		float diffusionStrength = config.baseDiffusionRate * proximity * civTech->openness * civTech->diffusionAffinity;

		// Apply the diffusion effects from the other civilization's known technologies to the current civilization's known technologies
		ApplyDiffusion(civTech, otherTech, diffusionStrength, em, dt);
	}
}
/////////////////////////////////



/////////////////////////////////
// ProcessParticleDiffusionForCivilisation - 
void TechDiffusionSystem::ProcessParticleDiffusionForCivilisation(Entity* civEntity, CCivilisationTech* civTech, EntityManager& entityManager, float dt) {
	// Get the CTransform component from the civilization entity to determine its position in the game world
	auto* civTransform = civEntity->GetComponent<CTransform>();
	
	// Guard against null pointer for the CTransform component, if it is not set, we skip the particle diffusion processing; Bitches! We be guarding!
	if (!civTransform) return;

// --- PARTICLE INFLUENCE (BVH version) ---
	// Guard against null pointer for the BVH system, if it is not set, we skip the particle influence processing
	if (!m_bvhSystem) return;

	// Query the BVH system for nearby knowledge particles within the influence radius of the civilization's position
	std::vector<ParticleData*> nearby;
	m_bvhSystem->QuerySphere(civTransform->position, config.particleInfluenceRadius, nearby);

	// Loop through the nearby knowledge particles and apply their influence on the civilization's known technologies
	for (auto* pdata : nearby) {
		// Guard against null pointer for the ParticleData
		if (!pdata)	continue; 

		// Get the entity associated with the knowledge particle
		Entity* pEntity = pdata->entity;

		// Guard against null pointer for the entity and check if it is alive, if not, we skip the influence processing for this particle
		if (!pEntity || !pEntity->IsAlive())
			continue;

		// Skip dead entities
		auto* kp = pEntity->GetComponent<CKnowledgeParticle>();
		
		// Guess what? Guard against null pointer for the CKnowledgeParticle component, if it is not set, we skip the influence processing for this particle
		if (!kp) continue;

		auto* pTransform = pEntity->GetComponent<CTransform>();
		// Annnnnnd guard against null pointer for the CTransform component of the knowledge particle, if it is not set, we skip the influence processing for this particle
		if (!pTransform) continue;

		// Exact distance check
		float dx = pTransform->position.x - civTransform->position.x;
		float dy = pTransform->position.y - civTransform->position.y;
		float dist = std::sqrt(dx * dx + dy * dy);

		// Annnnnnnd guard against the case where the distance is greater than or equal to the particle's influence radius, if so, we skip the influence processing for this particle. 
		// Not a crash issue but a logic issue, so we continue to the next particle
		if (dist >= config.particleInfluenceRadius)	continue;

		// Calculate the falloff based on distance and the particle's influence radius, and compute the boost to passive progress
		float falloff = 1.0f - (dist / config.particleInfluenceRadius);
		float boost = kp->value * falloff;

		// Skip if civ already knows the tech fully
		auto knownIt = civTech->knownTechs.find(kp->techId);

		// Use a read-only check to see if the civilization already knows the technology fully (progress >= 1.0f). 
		// If so, skip applying the influence from this knowledge particle. (this should be thread-safe since we are only reading the map, not modifying it)
		if (knownIt != civTech->knownTechs.end() && knownIt->second >= 1.0f) continue;

		float& progress = civTech->passiveProgress[kp->techId];

		 // Assuming the required knowledge is equal to the particle's value for simplicity. Adjust as needed based on game design.
		float required = kp->value;

		// Normalized passive progress [0..1]
		float norm = progress / required;
		float factor = 1.0f - norm;
		factor = std::max(0.02f, factor);

		// Passive gain rate (tune this for ~20 min to reach 100%)
		const float passiveScale = 0.02f;
		progress += boost * dt * passiveScale * factor;
	}
}
/////////////////////////////////



/////////////////////////////////
// ApplyDiffusion - Applies the diffusion effects from another civilization's known technologies to the current civilization's known technologies. It updates the passive progress towards unlocking new technologies based on the diffusion strength and time delta.
void TechDiffusionSystem::ApplyDiffusion(CCivilisationTech* civTech, CCivilisationTech* otherCivTech, float diffusionStrength, EntityManager& entityManager, float dt) {
	// Guard against null pointers for the civilization technology components
	if (!civTech || !otherCivTech) return;

	// Guard against empty known technologies in the other civilization, as there is nothing to diffuse
	__try {
		if (otherCivTech->knownTechs.empty()) return;

		// Guard against excessive known technologies in the other civilization, as this may indicate a bug or exploit. We set a reasonable limit to prevent performance issues or unintended behavior. 
		constexpr size_t kMaxReasonableKnownTechs =	10000; // sounds err....reasonable? (tune this to taste, but this is a sanity check)
		
		// If the other civilization has more known technologies than the maximum reasonable limit, we skip applying diffusion to prevent potential performance issues or unintended behavior.
		if (otherCivTech->knownTechs.size() > kMaxReasonableKnownTechs)	return;

		// Loop through the known technologies of the other civilization and apply diffusion effects to the current civilization's known technologies
		for (const auto& [techId, knownLevel] : otherCivTech->knownTechs) {
			// Skip if civ already fully knows the tech
			auto it = civTech->knownTechs.find(techId);
			
			// Use a read-only check to see if the civilization already knows the technology fully (progress >= 1.0f).
			if (it != civTech->knownTechs.end() && it->second >= 1.0f) continue;

			// Get the tech node from the registry to access its properties, such as required knowledge for unlocking
			const CTechNode* techNode = techRegistry.GetTechNode(techId);
			
			// If the tech node is not found in the registry, skip to the next technology as we cannot apply diffusion effects without its properties
			if (!techNode) continue;

			// Update passive progress towards unlocking the technology based on diffusion strength, time delta, and a damping curve to ensure smooth progression. 
			// The damping curve ensures that the passive progress slows down as it approaches the required knowledge, preventing it from freezing completely.
			float& progress = civTech->passiveProgress[techId];
			float required = techNode->requiredKnowledge;

			// Normalized passive progress [0..1]
			float norm = progress / required;

			// Damping curve: fast early, slow late, never zero
			float factor = 1.0f - norm;
			factor = std::max(0.05f, factor); // <-- ensures passive never freezes

			// Passive gain rate (tune this for ~20 min to reach 100%)
			const float passiveScale = 0.12f; // <-- adjust to taste

			// Update the passive progress based on diffusion strength, time delta, passive scale, and the damping factor
			progress += diffusionStrength * dt * passiveScale * factor;

			// Trigger active research once past 5%
			if (!civTech->activeResearch.contains(techId) && progress > required * 0.05f) {
				civTech->activeResearch[techId] = progress;
			}
		}
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		m_sehCatchCount.fetch_add(1, std::memory_order_relaxed);
		return;
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

	// If the distance is greater than the maximum proximity distance, return 0.0f to indicate no proximity
	if (dist > config.maxProximityDistance) return 0.0f;

	// Calculate a smooth falloff for proximity based on distance, using a cubic interpolation for a more natural transition
	float x = dist / config.maxProximityDistance;
	return 1.0f - (x * x * (3 - 2 * x));
}
/////////////////////////////////