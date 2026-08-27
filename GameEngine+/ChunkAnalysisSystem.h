/////////////////////////////////
// ChunkAnalysisSystem.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include "System.h"
#include "EntityManager.h"
#include "CChunkKnowledge.h"
#include "CCivilisationTech.h"
#include "CTransform.h"
/////////////////////////////////



/////////////////////////////////
// ChunkAnalysisSystem class - Analyzes chunks of the game world and applies their knowledge effects to civilizations. It processes entities with CChunkKnowledge and CTransform components, updating the knowledge state of civilizations based on their proximity to chunks and the properties of those chunks.
// 								|
//								|_______________________________________________________________________
class ChunkAnalysisSystem : public System {
	/////////////////////////////////
	// Public interface for the ChunkAnalysisSystem class
public:
	/////////////////////////////////
	// Update - Overrides the base class Update method to implement the chunk analysis logic. It processes all entities with CChunkKnowledge and CTransform components, applying their knowledge effects to civilizations based on proximity and other factors.
	void Update(float dt, EntityManager& entityManager) override;
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the ChunkAnalysisSystem class. These variables can be used to track internal state, configuration, or other relevant data needed for the system's operation.
private:
	/////////////////////////////////
	// ProcessChunk - Processes a single chunk entity with CChunkKnowledge, applying its knowledge effects to civilizations within its influence radius. It calculates the influence strength based on proximity and other factors, and updates the known technologies of affected civilizations accordingly.
	void ProcessChunk(Entity* chunkEntity, CChunkKnowledge* chunkKnowledge, float dt);
	/////////////////////////////////



	/////////////////////////////////
	// ApplyChunkEffectsToCivs - Applies the knowledge effects of a chunk to civilizations within its influence radius. It calculates the influence strength based on proximity and other factors, and updates the known technologies of affected civilizations accordingly.
	void ApplyChunkEffectsToCivs(Entity* chunkEntity, CChunkKnowledge* chunkKnowledge, EntityManager& entityManager, float dt);
	/////////////////////////////////
};
/////////////////////////////////
