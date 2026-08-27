/////////////////////////////////
// TechRegistry.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include "CTechNode.h"
/////////////////////////////////



/////////////////////////////////
// TechRegistry class 
class TechRegistry {
	/////////////////////////////////
	// Public member variables for the TechRegistry class
public:
	/////////////////////////////////
	// RegisterTechNode - Registers a new technology node in the registry. It takes a CTechNode object as input and adds it to the internal mapping of tech nodes, allowing for easy retrieval and management of technology nodes within the game.
	void RegisterTechNode(const CTechNode& techNode);
	/////////////////////////////////



	/////////////////////////////////
	// Retrieve a tech node by ID (returns nullptr if not found)
	const CTechNode* GetTechNode(const std::string& techId) const;
	/////////////////////////////////



	/////////////////////////////////
	// PrerequisitesMet - Checks if the prerequisites for a given technology node are met based on the known technologies. It takes a string representing the tech node ID and a mapping of known technologies with their levels, and returns true if all prerequisites are met, false otherwise.
	bool PrerequisitesMet(const std::string& techId, const std::unordered_map<std::string, float>& knownTechs);
	/////////////////////////////////


	
	/////////////////////////////////
	// Get all tech nodes IDs - Returns a vector of strings containing the IDs of all registered technology nodes in the registry. This allows for easy iteration and management of all available technology nodes within the game.
	std::vector<std::string> GetAllTechNodeIDs() const;
	/////////////////////////////////



	/////////////////////////////////
	// LoadDefaults - Loads default tech nodes into the registry. This function initializes the registry with a predefined set of technology nodes, allowing for a consistent starting point for technology progression in the game.
	void LoadDefaults(); // Load default tech nodes into the registry
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the TechRegistry class
private:
	/////////////////////////////////
	// Mapping of tech node IDs to their corresponding CTechNode objects, allowing for efficient retrieval and management of technology nodes within the registry.
	std::unordered_map<std::string, CTechNode> techNodes;
	/////////////////////////////////
};
/////////////////////////////////
