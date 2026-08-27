/////////////////////////////////
// CChunkKnowledge.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include "Component.h"
#include <unordered_map>
#include <string>
/////////////////////////////////



/////////////////////////////////
// struct CChunkKnowledge - Represents the knowledge contained within a specific chunk of the game world. It contains a mapping of technology IDs to their corresponding knowledge values, allowing for interaction with civilizations and their tech trees.
// 								|
//								|_______________________________________________________________________
struct CChunkKnowledge : public Component {
	/////////////////////////////////
	float knowledgeDensity = 1.0f; // Overall density of knowledge in this chunk (0.0 = no knowledge, 1.0 = maximum knowledge)
	float innovationPressure = 0.5f; // Innovation pressure within this chunk (0.0 = no pressure, 1.0 = maximum pressure)
	float diffusionMultiplier =	1.0f; // Multiplier affecting the diffusion of knowledge from this chunk to civilizations (0.0 = no diffusion, 1.0 = normal diffusion)
	std::unordered_map<std::string, float> techAffinity; // Mapping of technology IDs to their corresponding affinity values within this chunk (0.0 = no affinity, 1.0 = maximum affinity)
	float decayRate = 0.01f; // Rate at which knowledge in this chunk decays over time (0.0 = no decay, 1.0 = maximum decay)
	/////////////////////////////////
};
/////////////////////////////////