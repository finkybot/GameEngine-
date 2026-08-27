/////////////////////////////////
// KnowledgeParticleSpawnerSystem.cpp - Implementation of the KnowledgeParticleSpawnerSystem class, which is responsible for spawning knowledge particles based on the properties of civilizations and chunks in the game world. The system processes entities with CChunkKnowledge and CTransform components, 
// creating knowledge particles that represent the diffusion of knowledge and technology within the game environment.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "KnowledgeParticleSpawnerSystem.h"
#include "EntityManager.h"
#include "CChunkKnowledge.h"
#include "CCivilisationTech.h"
#include "CTransform.h"
#include "CKnowledgeParticle.h"
#include "CParticleInfluence.h"
#include "Vec2.h"
#include <algorithm>
/////////////////////////////////



/////////////////////////////////
// Update - Overrides the base class Update method to implement the knowledge particle spawning logic. It processes all entities with CChunkKnowledge and CTransform components, creating knowledge particles based on the properties of chunks and their proximity to civilizations.
void KnowledgeParticleSpawnerSystem::Update(EntityManager& entityManager, float deltaTime) {
	// Get a reference to the list of all entities managed by the EntityManager
	auto& entities = entityManager.GetEntities();

	// Loop through all entities and process those with CChunkKnowledge and CCivilisationTech
	for (auto& upEntity : entities) {
		// Get the raw pointer to the entity from the unique_ptr
		Entity* entity = upEntity.get();
		
		// Skip dead entities
		if (!entity->IsAlive())	continue;

		// Get the CChunkKnowledge component from the entity
		auto chunkKnowledgeComp = entity->GetComponent<CChunkKnowledge>();
		
		// If the entity has a CChunkKnowledge, process its knowledge particle spawning
		if (chunkKnowledgeComp) {
			SpawnFromChunkKnowledge(entity, chunkKnowledgeComp, entityManager);
		}

		// Get the CCivilisationTech component from the entity
		auto civTechComp = entity->GetComponent<CCivilisationTech>();
		
		// If the entity has a CCivilisationTech, process its knowledge particle spawning
		if (civTechComp) {
			SpawnFromTechUnlocks(entity, civTechComp, entityManager);
		}
	}
}



/////////////////////////////////
// SpawnFromTechUnlocks - Spawns knowledge particles based on the technologies unlocked by a civilization entity. It retrieves the known technologies from the CCivilisationTech component and creates knowledge particles for each technology, applying influence to nearby civilizations.
void KnowledgeParticleSpawnerSystem::SpawnFromTechUnlocks(Entity* civEntity, CCivilisationTech* civTech, EntityManager& entityManager) {
	// Get the CTransform component from the civilization entity to determine its position in the game world
	auto* transform = civEntity->GetComponent<CTransform>();

	// If the civilization entity does not have a CTransform component, we cannot determine its position, so we return early
	if (!transform) return;

	for (auto& [techId, progress] : civTech->knownTechs) {
		// Only spawn particles for technologies that are fully unlocked (progress >= 1.0)
		if (progress >= 1.0f) {
			// Define the influence radius and falloff for the knowledge particle
			float influenceRadius = 150.0f; // Example value, can be adjusted based on game design
			float influenceFalloff = 1.0f;	// Example value, can be adjusted based on game design
			
			// Define the number of particles to spawn for each unlocked technology
			for (int i = 0; i < 3; i++) {
				// Spawn a knowledge particle for the unlocked technology at the civilization's position
				SpawnParticle(entityManager, techId, progress, transform->position, influenceRadius, influenceFalloff);
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// SpawnFromChunkKnowledge - Spawns knowledge particles based on the properties of a chunk entity with CChunkKnowledge. It retrieves the knowledge density and tech affinity from the CChunkKnowledge component and creates knowledge particles for each technology, applying influence to nearby civilizations.
void KnowledgeParticleSpawnerSystem::SpawnFromChunkKnowledge(Entity* chunkEntity, CChunkKnowledge* chunkKnowledge, EntityManager& entityManager) {
	// Get the CTransform component from the chunk entity to determine its position in the game world
	auto* transform = chunkEntity->GetComponent<CTransform>();
	if (!transform)
		return;

	// Only spawn if density is high enough
	if (chunkKnowledge->knowledgeDensity < 5.0f)
		return;

	// Spawn rate scaled but capped
	int numParticles = std::min(10, static_cast<int>(chunkKnowledge->knowledgeDensity * 0.5f));

	// Spawn particles for each technology in the chunk's tech affinity
	for (int i = 0; i < numParticles; i++) {
		SpawnParticle(entityManager, "GeneralKnowledge", chunkKnowledge->knowledgeDensity * 0.05f, transform->position, 100.0f, 1.0f);
	}
}
/////////////////////////////////



/////////////////////////////////
// SpawnParticle - Creates a new knowledge particle entity with the specified properties and adds it to the EntityManager. It sets up the CKnowledgeParticle, CParticleInfluence, and CTransform components for the new entity, allowing it to interact with civilizations and the game world.
void KnowledgeParticleSpawnerSystem::SpawnParticle(EntityManager& entityManager, const std::string& techId, float value, const Vec2& position, float influenceRadius, float influenceFalloff) {
	// Create a new entity of type KnowledgeParticle
	Entity* p = entityManager.AddEntity(EntityType::KnowledgeParticle);

	   // Knowledge data
	   auto* kp = p->AddComponent<CKnowledgeParticle>();
	   kp->techId = techId;
	   kp->value = value;

	   // Influence data
	   auto* influence = p->AddComponent<CParticleInfluence>();
	   influence->influenceRadius = influenceRadius;
	   influence->influenceFalloff = influenceFalloff;

	   // Transform + movement (PhysicsSystem will move it)
	   auto* transform = p->AddComponent<CTransform>();
	   transform->position = position;
	   transform->velocity = Vec2::RandomDirection() * 40.0f;
   }
/////////////////////////////////