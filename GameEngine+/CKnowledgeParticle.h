/////////////////////////////////
// CKnowledgeParticle.h - Header file for the CKnowledgeParticle component, which represents a knowledge particle in the game world. It includes necessary headers and defines the CKnowledgeParticle struct, inheriting from the Component class. The struct contains a 
// tech ID and a value representing the amount of knowledge carried by the particle.
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include "Component.h"
#include <string>
/////////////////////////////////



/////////////////////////////////
// struct CKnowledgeParticle - Represents a knowledge particle in the game world, carrying information about a specific technology. It contains the tech ID and the amount of knowledge it represents, allowing for interaction with civilizations and their tech trees.
// 								|
//								|_______________________________________________________________________
struct CKnowledgeParticle : public Component {
	/////////////////////////////////
	std::string techId; // The tech this particle represents
	float value = 1.0f; // Amount of knowledge carried
	/////////////////////////////////
};
/////////////////////////////////