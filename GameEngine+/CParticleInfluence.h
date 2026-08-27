/////////////////////////////////
// CParticleInfluence.h

/////////////////////////////////
// Includes and forward declarations
#pragma once
#include "Component.h"
/////////////////////////////////



/////////////////////////////////
// struct CParticleInfluence - Represents the influence of a knowledge particle on civilizations within a certain radius. It contains parameters for the influence radius and falloff, allowing for interaction with civilizations and their tech trees.
// 								|
//								|_______________________________________________________________________
struct CParticleInfluence : public Component {
	/////////////////////////////////
	float influenceRadius = 100.0f; // The radius within which this particle can influence civilizations
	float influenceFalloff = 1.0f; // The strength of the influence this particle has on civilizations within its radius
	/////////////////////////////////
};
/////////////////////////////////
