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
	// Technology progress tracking
	std::unordered_map<std::string, float> knownTechs;		// All technologies this civilisation is aware of, mapped by their unique tech node IDs. Each entry contains the current progress towards unlocking that technology (0.0 = not started, 1.0 = fully unlocked).
	std::unordered_map<std::string, float> activeResearch;	// Technologies currently being researched, mapped by their unique tech node IDs. Each entry contains the current progress towards unlocking that technology (0.0 = not started, 1.0 = fully unlocked).
	std::unordered_map<std::string, float> passiveProgress; // Technologies that are passively progressing due to diffusion or other factors, mapped by their unique tech node IDs. Each entry contains the current progress towards unlocking that technology (0.0 = not started, 1.0 = fully unlock)
	/////////////////////////////////
	
	
	/////////////////////////////////
	std::unordered_set<std::string>	unlockedTechs;			// Set of technologies that have been fully unlocked by the civilisation, represented by their unique tech node IDs. This set allows for quick checks to determine if a technology has been unlocked.
	/////////////////////////////////



	/////////////////////////////////
	// Technology bias and preferences
	std::unordered_map<std::string, float> categoryBias;	// Bias towards certain categories of technology, mapped by category names. Each entry contains a value representing the civilisation's preference for that category (0.0 = no interest, 1.0 = strong interest).
	/////////////////////////////////



	/////////////////////////////////
	// Civilization attributes
	float openness = 0.5f;	// Openness to new technologies (0.0 = closed-minded, 1.0 = highly open-minded)
	float literacy = 0.5f;	// Literacy level of the civilisation (0.0 = illiterate, 1.0 = fully literate)
	/////////////////////////////////



	/////////////////////////////////
	// Civilization performance metrics
	float foodProduction		=	1.0f;	// agriculture techs
	float militaryStrength		=	1.0f;	// military techs
	float culturalDevelopment	=	1.0f;	// culture techs
	float industrialOutput		=	1.0f;	// engineering / industry techs
	float economicEfficiency	=	1.0f;	// economics / trade techs
	float populationGrowth		=	1.0f;	// medicine / agriculture / culture techs
	/////////////////////////////////



	/////////////////////////////////
	// Civilization trade and knowledge retention metrics
	float tradeCapacity			=	1.0f;	// Trade capacity of the civilisation (0.0 = no trade, 1.0 = maximum trade capacity)
	float knowledgeRetention	=	1.0f;	// Knowledge retention of the civilisation (0.0 = forgetful, 1.0 = strong memory and knowledge retention)
	float diplomacyRating		=	1.0f; // Diplomacy rating of the civilisation (0.0 = hostile, 1.0 = highly diplomatic)
	/////////////////////////////////



	/////////////////////////////////
	// Innovation and diffusion attributes
	float innovationPressure	=	0.5f;	// Innovation pressure of the civilisation (environmental and societal factors)
	float diffusionAffinity		=	0.5f;	// Affinity for technology diffusion (0.0 = resistant to adopting new tech, 1.0 = highly receptive to new tech)
	float retentionStrength		=	0.5f;	// Retention strength of the civilisation (0.0 = forgetful, 1.0 = strong memory and knowledge retention)
	/////////////////////////////////



	/////////////////////////////////
	// Public methods for the CCivilisationTech component
public:
	/////////////////////////////////
		CCivilisationTech() = default;
	/////////////////////////////////
};
/////////////////////////////////