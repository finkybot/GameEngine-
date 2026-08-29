/////////////////////////////////
// KnowledgeParticleMovementSystem.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include "System.h"
#include "EntityManager.h"
#include "CKnowledgeParticle.h"
#include "CParticleInfluence.h"
#include "CCivilisationTech.h"
#include "CTransform.h"
#include "Vec2.h"
/////////////////////////////////



/////////////////////////////////
// KnowledgeParticleMovementSystem class - Manages the movement and influence of knowledge particles in the game world. It processes entities with CKnowledgeParticle, CTransform, and CParticleInfluence components, applying their influence on civilizations within their radius.
// 								|
//								|_______________________________________________________________________
class KnowledgeParticleMovementSystem : public System {
	/////////////////////////////////
	// Public interface for the KnowledgeParticleMovementSystem class
public:
	/////////////////////////////////
	// Update - Overrides the base class Update method to implement the knowledge particle movement logic. It processes all entities with CKnowledgeParticle, CTransform, and CParticleInfluence components, updating their positions and applying influence to civilizations within their radius.
	void Update(float dt, EntityManager& entityManager) override;
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the KnowledgeParticleMovementSystem class. These variables can be used to track internal state, configuration, or other relevant data needed for the system's operation.
private:
	/////////////////////////////////
	// ApplyInfluence - Applies the influence of a knowledge particle on civilizations within its influence radius. It calculates the influence strength based on distance and falloff, and updates the known technologies of affected civilizations accordingly.
	void ApplyInfluence(CKnowledgeParticle* kp, CTransform* particleTransform, CParticleInfluence* influence, EntityManager& entityManager, float dt);
	/////////////////////////////////



	/////////////////////////////////
	// Particle processing budget for the KnowledgeParticleMovementSystem class
	size_t m_particleBudget = 20;		// max particles processed per frame
	size_t m_lastParticleIndex = 0;		// rolling index for round‑robin
	/////////////////////////////////
};
/////////////////////////////////
