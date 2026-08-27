/////////////////////////////////
// CivilisationTechComponent.hpp
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include "Component.h"
#include <unordered_map>
#include <string>
/////////////////////////////////



/////////////////////////////////
// CCivilisationTech - 
//								|
//								|_______________________________________________________________________
class CCivilisationTech : public Component {
	/////////////////////////////////
	// Public member variables for the	CCivilisationTech component
public:
	/////////////////////////////////
	std::unordered_map<std::string, float> knownTechs;		// All technologies this civilisation is aware of, mapped by their unique tech node IDs. Each entry contains the current progress towards unlocking that technology (0.0 = not started, 1.0 = fully unlocked).
	std::unordered_map<std::string, float> activeResearch;	// Technologies currently being researched, mapped by their unique tech node IDs. Each entry contains the current progress towards unlocking that technology (0.0 = not started, 1.0 = fully unlocked).
	std::unordered_map<std::string, float> passiveProgress; // Technologies that are passively progressing due to diffusion or other factors, mapped by their unique tech node IDs. Each entry contains the current progress towards unlocking that technology (0.0 = not started, 1.0 = fully unlocke).
	
	float openness = 0.5f;	// Openness to new technologies (0.0 = closed-minded, 1.0 = highly open-minded)
	float literacy = 0.5f;	// Literacy level of the civilisation (0.0 = illiterate, 1.0 = fully literate)
	
	float innovationPressure = 0.5f;	// Innovation pressure of the civilisation (environmental and societal factors)
	float diffusionAffinity = 0.5f;		// Affinity for technology diffusion (0.0 = resistant to adopting new tech, 1.0 = highly receptive to new tech)
	float retentionStrength = 0.5f;		// Retention strength of the civilisation (0.0 = forgetful, 1.0 = strong memory and knowledge retention)
	/////////////////////////////////



	/////////////////////////////////
	// Public methods for the CCivilisationTech component
public:
	/////////////////////////////////
		CCivilisationTech() = default;
	/////////////////////////////////
};
/////////////////////////////////