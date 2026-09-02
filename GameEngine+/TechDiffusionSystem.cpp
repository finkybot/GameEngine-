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
// Update - Overrides the base class Update method 
void TechDiffusionSystem::Update(float dt, EntityManager& entityManager) {
	(void)dt; // Unused parameter, but kept for consistency with the base class interface
	(void)entityManager; // Unused parameter, but kept for consistency with the base class interface
}

/////////////////////////////////



/////////////////////////////////
// ProcessCivToCivDiffusion - Processes technology diffusion between civilizations based on proximity and other factors. This method queries nearby civilizations and applies diffusion effects based on their openness, literacy, and other attributes.
void TechDiffusionSystem::ProcessCivToCivDiffusion(Entity* civEntity, CCivilisationTech* civTech, EntityManager& em, float dt) {
	// Guard against null pointers for the civilization entity and its technology component. If either is null, we cannot proceed with diffusion processing.
	if (!civEntity || !civTech) return;

	// Get the CTransform component from the civilization entity to determine its position in the game world
	auto* civTransform = civEntity->GetComponent<CTransform>();

	// Determine the position of the civilization entity. If the CTransform component is not available, we fall back to using the entity's center point.
	const Vec2 civPos = civTransform ? civTransform->position : civEntity->GetCentrePoint();

	// Get the maximum proximity distance from the configuration, which defines how far we will search for nearby civilizations to apply diffusion effects.
	const float maxDist = config.maxProximityDistance;

	// Get the spatial layer registry from the entity manager to access the "Civilisations" layer for querying nearby civilizations.
	auto* registry = em.GetSpatialLayerRegistry();

	// Guard against null pointer for the spatial layer registry. If it is not available, we cannot perform spatial queries for nearby civilizations.
	if (!registry) return;

	// Get the "Civilisations" spatial layer from the registry, which contains all civilization entities for proximity queries.
	auto& civLayer = registry->GetLayer("Civilisations");

	// Query the spatial layer for nearby civilization entities within the maximum proximity distance, excluding the current civilization entity itself. The results are stored in the nearby vector.
	std::vector<Entity*> nearby;
	civLayer.Query(nearby, civPos, maxDist, civEntity);

	// Loop through the nearby civilization entities and apply diffusion effects based on their proximity and attributes.
	for (Entity* other : nearby) {
		// Guard against null pointer for the other civilization entity, check if it is alive, and ensure it is not the same as the current civilization entity. If any of these conditions are true, we skip processing for this entity.
		if (!other || !other->IsAlive() || other == civEntity) continue;

		// Get the CCivilisationTech component from the other civilization entity to access its technology attributes for diffusion processing.
		auto* otherTech = other->GetComponent<CCivilisationTech>();

		// Guard against null pointer for the other civilization's technology component. If it is not available, we cannot apply diffusion effects from this entity.
		if (!otherTech) continue;

		//Get the CTransform component from the other civilization entity to determine its position in the game world
		auto* otherTransform = other->GetComponent<CTransform>();
		const Vec2 otherPos = otherTransform ? otherTransform->position : other->GetCentrePoint();

		// Calculate the distance between the current civilization and the other civilization. If the distance exceeds the maximum proximity distance, we skip processing for this entity.
		float dx = otherPos.x - civPos.x;
		float dy = otherPos.y - civPos.y;
		float dist = std::sqrt(dx * dx + dy * dy);

		// Guard against the case where the distance is greater than the maximum proximity distance. If so, we skip processing for this entity.
		if (dist > maxDist) continue;

		// Calculate the proximity factor based on the distance and maximum proximity distance. This factor will be used to scale the diffusion strength based on how close the other civilization is.
		float proximity = 1.0f - (dist / maxDist);
		proximity = proximity * proximity * (3.0f - 2.0f * proximity); // smoothstep

		// Guard against the case where the proximity factor is less than or equal to zero. If so, we skip processing for this entity.
		if (proximity <= 0.0f) continue;

		// Calculate the diffusion strength based on the base diffusion rate from the configuration, the proximity factor, and the openness and diffusion affinity of the other civilization's technology component. 
		// This value will determine how much influence the other civilization has on the current civilization's technology progress.
		float diffusionStrength = config.baseDiffusionRate * proximity * civTech->openness * civTech->diffusionAffinity;

		// Guard against the case where the diffusion strength is less than or equal to zero. If so, we skip processing for this entity.
		ApplyDiffusion(civTech, otherTech, diffusionStrength, em, dt);
	}
}
/////////////////////////////////



/////////////////////////////////
// ProcessParticleDiffusionForCivilisation - Processes technology diffusion from knowledge particles to a civilization based on proximity and other factors. This method queries nearby knowledge particles and applies diffusion effects to the civilization's passive progress towards unlocking new technologies.
void TechDiffusionSystem::ProcessParticleDiffusionForCivilisation(Entity* civEntity, CCivilisationTech* civTech, EntityManager& entityManager, float dt) {
	// Guard against null pointers for the civilization entity and its technology component. If either is null, we cannot proceed with diffusion processing.
	(void)entityManager;

	// Get the CTransform component from the civilization entity to determine its position in the game world
	auto* civTransform = civEntity->GetComponent<CTransform>();

	// Guard against null pointer for the CTransform component. If it is not available, we cannot perform spatial queries for nearby knowledge particles.
	if (!civTransform) return;

	// Guard against null pointer for the BVH system. If it is not available, we cannot perform spatial queries for nearby knowledge particles.
	if (!m_bvhSystem) return;

	// Query the BVH system for nearby knowledge particles within the particle influence radius defined in the configuration. The results are stored in the nearby vector.
	std::vector<ParticleData*> nearby;
	m_bvhSystem->QuerySphere(civTransform->position, config.particleInfluenceRadius, nearby);

	// Loop through the nearby knowledge particles and apply diffusion effects based on their proximity and influence on the civilization's passive progress towards unlocking new technologies.
	for (auto* pdata : nearby) {
		// Guard against null pointer for the ParticleData. If it is not available, we skip processing for this particle.
		if (!pdata)	continue;

		// Guard against null pointer for the entity associated with the ParticleData. If it is not available or if the entity is not alive, we skip processing for this particle.
		Entity* pEntity = pdata->entity;

		// Guard against null pointer for the entity associated with the ParticleData. If it is not available or if the entity is not alive, we skip processing for this particle.
		if (!pEntity || !pEntity->IsAlive()) continue;

		// Get the CKnowledgeParticle component from the entity to access its technology influence and value. If it is not available, we skip processing for this particle.
		auto* kp = pEntity->GetComponent<CKnowledgeParticle>();
		auto* pTransform = pEntity->GetComponent<CTransform>();

		// Guard against null pointers for the CKnowledgeParticle and CTransform components. If either is not available, we skip processing for this particle.
		if (!kp || !pTransform)	continue;

		// Calculate the distance between the civilization and the knowledge particle. If the distance exceeds the particle influence radius, we skip processing for this particle.
		float dx = pTransform->position.x - civTransform->position.x;
		float dy = pTransform->position.y - civTransform->position.y;
		float dist = std::sqrt(dx * dx + dy * dy);
		if (dist >= config.particleInfluenceRadius) continue;

		// Calculate the falloff factor based on the distance and particle influence radius. This factor will be used to scale the influence of the knowledge particle on the civilization's passive progress.
		float falloff = 1.0f - (dist / config.particleInfluenceRadius);
		
		// Calculate the boost value based on the knowledge particle's value and the falloff factor. This value represents the effective influence of the knowledge particle on the civilization's passive progress towards unlocking new technologies.
		float boost = kp->value * falloff;

		// Check if the civilization already knows the technology represented by the knowledge particle. If it does, we skip processing for this particle to avoid redundant progress updates.
		auto knownIt = civTech->knownTechs.find(kp->techId);

		// Guard against the case where the civilization already knows the technology fully (progress >= 1.0f). If so, we skip processing for this particle to avoid redundant progress updates.
		if (knownIt != civTech->knownTechs.end() && knownIt->second >= 1.0f) continue;

		// Get the tech node from the registry to access its properties, such as required knowledge for unlocking. If the tech node is not found, we skip processing for this particle.
		float& progress = civTech->passiveProgress[kp->techId];
		float required = kp->value;

		// Normalized passive progress [0..1]
		float norm = progress / required;

		// Damping curve: fast early, slow late, never zero
		float factor = 1.0f - norm;

		// Ensure that the factor does not fall below a minimum threshold to prevent the passive progress from freezing completely. This allows for continued progress even as it approaches the required knowledge.
		factor = std::max(0.02f, factor);

		// Passive gain rate (tune this for ~20 min to reach 100%)
		const float passiveScale = 0.02f;
		progress += boost * dt * passiveScale * factor;
	}
}
/////////////////////////////////



/////////////////////////////////
// ApplyDiffusion - Applies the diffusion effects from another civilization's known technologies to the current civilization's known technologies. It updates the passive progress towards unlocking new technologies based on the diffusion strength and time delta.
void TechDiffusionSystem::ApplyDiffusion(CCivilisationTech* civTech, CCivilisationTech* otherCivTech,
										 float diffusionStrength, EntityManager& entityManager, float dt) {
	// Guard against null pointers for the civilization technology components. If either is null, we cannot proceed with applying diffusion effects.
	(void)entityManager;

	// Guard against null pointers for the civilization technology components. If either is null, we cannot proceed with applying diffusion effects.
	if (!civTech || !otherCivTech)
		return;

	// Guard against the case where the other civilization has no known technologies. If it does not, we cannot apply any diffusion effects.
	__try {
		// Guard against the case where the other civilization has an unreasonably large number of known technologies. If it does, we skip processing to avoid potential performance issues or unrealistic diffusion effects.
		if (otherCivTech->knownTechs.empty())
			return;

		// Guard against the case where the other civilization has an unreasonably large number of known technologies. If it does, we skip processing to avoid potential performance issues or unrealistic diffusion effects.
		constexpr size_t kMaxReasonableKnownTechs = 10000;
		if (otherCivTech->knownTechs.size() > kMaxReasonableKnownTechs)
			return;

		// Loop through the other civilization's known technologies and apply diffusion effects to the current civilization's passive progress towards unlocking new technologies.
		for (const auto& [techId, knownLevel] : otherCivTech->knownTechs) {
			// Guard against the case where the known level of the technology is less than or equal to zero. If it is, we skip processing for this technology as it does not contribute to diffusion.
			auto it = civTech->knownTechs.find(techId);
			if (it != civTech->knownTechs.end() && it->second >= 1.0f)
				continue;

			// Guard against the case where the known level of the technology is less than or equal to zero. If it is, we skip processing for this technology as it does not contribute to diffusion.
			const CTechNode* techNode = techRegistry.GetTechNode(techId);
			if (!techNode)
				continue;

			// Guard against the case where the required knowledge for the technology is less than or equal to zero. If it is, we skip processing for this technology as it does not contribute to diffusion.
			float& progress = civTech->passiveProgress[techId];
			float required = techNode->requiredKnowledge;

			// Guard against the case where the required knowledge for the technology is less than or equal to zero. If it is, we skip processing for this technology as it does not contribute to diffusion.
			float norm = progress / required;
			float factor = 1.0f - norm;
			factor = std::max(0.05f, factor);

			// Passive gain rate (tune this for ~20 min to reach 100%)
			const float passiveScale = 0.12f;
			progress += diffusionStrength * dt * passiveScale * factor;

			// If the current civilization does not have this technology in its active research and the progress exceeds 5% of the required knowledge, we add it to the active research list to indicate that it is being actively researched.
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
	// Guard against null pointers for the civilization entities. If either is null, we cannot calculate proximity and return 0.0f.
	Vec2 posA = civEntity->GetPosition();
	Vec2 posB = otherCivEntity->GetPosition();

	// Calculate the distance between the two civilization entities using their positions. If the distance exceeds the maximum proximity distance defined in the configuration, we return 0.0f to indicate no proximity.
	float dist = (posA - posB).Mag();

	// Guard against the case where the distance is greater than the maximum proximity distance. If so, we return 0.0f to indicate no proximity.
	if (dist > config.maxProximityDistance)	return 0.0f;

	// Calculate the proximity factor based on the distance and maximum proximity distance. This factor will be used to scale the diffusion strength based on how close the other civilization is.
	float x = dist / config.maxProximityDistance;

	// Use a smoothstep function to calculate the proximity value, which provides a smooth transition from 1.0 (close) to 0.0 (distant) based on the normalized distance.
	return 1.0f - (x * x * (3.0f - 2.0f * x));
}
/////////////////////////////////