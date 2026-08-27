/////////////////////////////////
// KnowledgeParticleSpawnersystem class definition
/////////////////////////////////



/////////////////////////////////
#pragma once
#include "System.h"
#include "EntityManager.h"
#include "CKnowledgeParticle.h"
#include "CParticleInfluence.h"
#include "CCivilisationTech.h"
#include "CChunkKnowledge.h"
#include "TechRegistry.h"
#include "CTransform.h"
#include "Vec2.h"

/////////////////////////////////
 
 

/////////////////////////////////
// KnowledgeParticleSpawnerSystem class - Manages the spawning of knowledge particles in the game world. It processes entities with CChunkKnowledge and CTransform components, creating knowledge particles based on the properties of chunks and their proximity to civilizations.
// 								|
//								|_______________________________________________________________________
class KnowledgeParticleSpawnerSystem : public System {
	/////////////////////////////////
	// Public interface for the KnowledgeParticleSpawnerSystem class
public:
	/////////////////////////////////
	// Update - Overrides the base class Update method to implement the knowledge particle spawning logic. It processes all entities with CChunkKnowledge and CTransform components, creating knowledge particles based on the properties of chunks and their proximity to civilizations.
    void Update(EntityManager& entityManager, float deltaTime);
	/////////////////////////////////


	/////////////////////////////////
	// Private member variables for the KnowledgeParticleSpawnerSystem class. These variables can be used to track internal state, configuration, or other relevant data needed for the system's operation.
private:
	/////////////////////////////////
	void SpawnFromTechUnlocks(Entity* civEntity, CCivilisationTech* civTech, EntityManager&	entityManager); // SpawnFromTechUnlocks - Spawns knowledge particles based on the technologies unlocked by a civilization entity.)
	void SpawnFromChunkKnowledge(Entity* chunkEntity, CChunkKnowledge* chunkKnowledge, EntityManager& entityManager); // SpawnFromChunkKnowledge - Spawns knowledge particles based on the properties of a chunk entity with CChunkKnowledge.)
	void SpawnParticle(EntityManager& entityManager, const std::string& techId, float value, const Vec2& position, float influenceRadius, float	influenceFalloff); // SpawnParticle - Creates a new knowledge particle entity with the specified properties and adds it to the EntityManager.)
	/////////////////////////////////

};
/////////////////////////////////