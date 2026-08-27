/////////////////////////////////
// TechUnlockSystem.cpp - Implementation of the TechUnlockSystem class, which handles the unlocking of technologies for civilizations and the application of their effects.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "TechUnlockSystem.h"
#include "CTransform.h"
#include "CKnowledgeParticle.h"
#include "CParticleInfluence.h"
#include <cmath>
/////////////////////////////////



/////////////////////////////////
// Update - Overrides the base class Update method to implement the technology unlocking logic. It processes all entities with CCivilisationTech, checking their research progress and applying the effects of unlocked technologies as appropriate.
void TechUnlockSystem::Update(float dt, EntityManager& entityManager)
{
    auto& entities = entityManager.GetEntities();

    for (auto& ePtr : entities)
    {
        Entity* civ = ePtr.get();
        if (!civ->IsAlive()) continue;

        auto* civTech = civ->GetComponent<CCivilisationTech>();
        if (!civTech) continue;

        ProcessCivilisation(civ, civTech, entityManager);
    }
}
/////////////////////////////////



/////////////////////////////////
// ProcessCivilisation - Processes a civilization entity, checking its research progress and applying the effects of unlocked technologies as appropriate. It iterates through the known technologies of the civilization, checking their progress and applying effects for those that are fully unlocked.
void TechUnlockSystem::ProcessCivilisation(Entity* civ, CCivilisationTech* civTech, EntityManager& entityManager)
{
    for (auto& [techId, progress] : civTech->knownTechs)
    {
        // Only trigger unlock effects once
        if (progress < 1.0f)
            continue;

        // Already processed?
        if (civTech->unlockedTechs.count(techId))
            continue;

        // Get the tech node from the registry
        const CTechNode* node = techRegistry.GetTechNode(techId);
        
        // If the tech node is not found, skip to the next technology
        if (!node) continue;

        // Mark as unlocked
        civTech->unlockedTechs.insert(techId);

        // Apply bonuses
        ApplyTechEffects(civ, civTech, *node);

        // Spread knowledge
        SpawnKnowledgeParticles(civ, *node, entityManager);
    }
}
/////////////////////////////////



/////////////////////////////////
// ApplyTechEffects - Applies the effects of a technology node to a civilization entity. This function can be customized to apply different bonuses based on the category of the technology node, such as agriculture, culture, or military.
void TechUnlockSystem::ApplyTechEffects(Entity* civ, CCivilisationTech* civTech, const CTechNode& techNode)
{
	// Category-based effects (data-driven later)
	if (techNode.category == "agriculture") {
		// Increase food production and population growth for agriculture technologies
		civTech->foodProduction += 0.15f;
		civTech->populationGrowth += 0.05f;
	} else if (techNode.category == "military") {
		// Increase military strength for military technologies
		civTech->militaryStrength += 0.20f;
	} else if (techNode.category == "culture") {
		// Increase cultural development and literacy for culture technologies
		civTech->culturalDevelopment += 0.10f;
		civTech->literacy += 0.05f;
	} else if (techNode.category == "engineering") {
		// Increase industrial output for engineering technologies
		civTech->industrialOutput += 0.15f;
	} else if (techNode.category == "economics") {
		// Increase economic efficiency and trade capacity for economics technologies
		civTech->economicEfficiency += 0.10f;
		civTech->tradeCapacity += 0.05f;
	}

	// Optional: tech-specific effects (data-driven later)
	// Example:
	// if (techNode.id == "agriculture.irrigation")
	//     civTech->foodProduction += 0.25f;
}
/////////////////////////////////



/////////////////////////////////
// SpawnKnowledgeParticles - Spawns knowledge particles for a civilization entity when a technology is unlocked. It creates a few knowledge particle entities around the civilization's position, each with a specified influence radius and falloff, to represent the spread of knowledge from the unlocked technology.
void TechUnlockSystem::SpawnKnowledgeParticles(Entity* civ, const CTechNode& techNode, EntityManager& entityManager)
{
    auto* transform = civ->GetComponent<CTransform>();
    if (!transform) return;

    // Spawn a few particles around the civ
    for (int i = 0; i < 3; i++)
    {
        Entity* p = entityManager.AddEntity(EntityType::KnowledgeParticle);

        auto* kp = p->AddComponent<CKnowledgeParticle>();
        kp->techId = techNode.id;
        kp->value = 0.2f;

        auto* influence = p->AddComponent<CParticleInfluence>();
        influence->influenceRadius = 150.0f;
        influence->influenceFalloff = 1.0f;

        auto* t = p->AddComponent<CTransform>();
        t->position = transform->position;
        t->velocity = Vec2::RandomDirection() * 40.0f;
    }
}
/////////////////////////////////
