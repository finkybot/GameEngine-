/////////////////////////////////
// TechNodeComponent.hpp
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include "Component.h"
#include "Vec2.h"
#include <string>
#include <vector>
/////////////////////////////////



/////////////////////////////////
// CTechNode - Represents a technology node in a civilization tech tree, with properties for ID, category, prerequisites, difficulty, progress, and compatibility.
//								|
//								|_______________________________________________________________________
class CTechNode : public Component {
	/////////////////////////////////
	// Public member variables for the TechNode component
public:
	/////////////////////////////////
	std::string id;									// Unique identifier for the tech node
	std::string category;							// Category of the tech node (e.g., "military", "agriculture", "science", "cultural")
	std::vector<std::string> prerequisites;			// List of prerequisite tech node IDs that must be unlocked before this node can be unlocked
	float baseDifficulty = 1.0f;					// Base difficulty level for unlocking this tech node (higher values = more difficult)
	float currentProgress = 0.0f;					// Current progress towards unlocking this tech node (0.0 = not started, 1.0 = fully unlocked)
	float requiredKnowledge = 1.0f;					// Total knowledge required to unlock this tech node (e.g., 100.0 = fully unlocked)
	float mutationPotential = 0.0f;					// Chance for this tech node to mutate into a different tech node (0.0 = no mutation, 1.0 = guaranteed mutation)
	std::vector<std::string> compatibilityTags;		// List of tags indicating compatibility with other tech nodes or game mechanics (e.g., "compatible_with_advanced_military")
	/////////////////////////////////



	/////////////////////////////////
	// Public methods for the TechNode component
public:	
		CTechNode() = default;
	/////////////////////////////////


	/////////////////////////////////
	explicit CTechNode(const std::string& techId) :id(techId) {}
	/////////////////////////////////
};
/////////////////////////////////
