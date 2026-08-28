/////////////////////////////////
// TechRegistry.cpp - Implementation of the TechRegistry class
/////////////////////////////////


/////////////////////////////////
// Includes
#include "TechRegistry.h"
/////////////////////////////////



/////////////////////////////////
// RegisterTechNode - Registers a new technology node in the registry. It takes a CTechNode object as input and adds it to the internal mapping of tech nodes, allowing for easy retrieval and management of technology nodes within the game.
void TechRegistry::RegisterTechNode(const CTechNode& techNode) {
	techNodes[techNode.id] = techNode;
}
/////////////////////////////////



/////////////////////////////////
// GetTech - Retrieves a technology node from the registry based on its unique ID. It takes a string representing the tech node ID as input and returns a pointer to the corresponding CTechNode object if found, or nullptr if not found.
const CTechNode* TechRegistry::GetTechNode(const std::string& techId) const {
	auto it = techNodes.find(techId);
	if (it != techNodes.end())
		return &it->second;

	return nullptr;
}
/////////////////////////////////



/////////////////////////////////
// PrerequisitesMet - Checks if the prerequisites for a given technology node are met based on the known technologies. It takes a string representing the tech node ID and a mapping of known technologies with their levels, and returns true if all prerequisites are met, false otherwise.
bool TechRegistry::PrerequisitesMet(const std::string& techId, const std::unordered_map<std::string, float>& knownTechs) {
	// Retrieve the technology node from the registry using its ID
	const CTechNode* techNode = GetTechNode(techId);

	// If the technology node is not found, return false (prerequisites cannot be met)
	if (!techNode) return false;

	// Iterate through all prerequisites of the technology node
	for (const auto& prereq : techNode->prerequisites) {
		// Check if the prerequisite is not known in the knownTechs map
		if (knownTechs.find(prereq) == knownTechs.end()) {
			return false; 
		}
	}

	// If all prerequisites are found in the knownTechs map, return true
	return true;
}
/////////////////////////////////



/////////////////////////////////
// GetAllTechNodeIDs - Returns a vector of strings containing the IDs of all registered technology nodes in the registry. This allows for easy iteration and management of all available technology nodes within the game.
std::vector<std::string> TechRegistry::GetAllTechNodeIDs() const {
	// Create a vector to hold the IDs of all registered technology nodes
	std::vector<std::string> ids;

	// Reserve space in the vector to avoid multiple reallocations during push_back
	ids.reserve(techNodes.size()); // Reserve space for efficiency

	// Iterate through the unordered_map of tech nodes and extract their IDs
	for (const auto& pair : techNodes) {
		ids.push_back(pair.first);
	}

	// Return the vector containing all tech node IDs
	return ids;
}
/////////////////////////////////



/////////////////////////////////
// LoadDefaults - Loads default tech nodes into the registry. This function initializes the registry with a predefined set of technology nodes, allowing for a consistent starting point for technology progression in the game.
void TechRegistry::LoadDefaults() {
	// --- Agriculture.Basic ---
	CTechNode basicAgri;
	basicAgri.id = "agriculture.basic";
	basicAgri.name = "Basic Agriculture";
	basicAgri.category = "agriculture";
	basicAgri.requiredKnowledge = 20.0f;
	basicAgri.baseDifficulty = 1.0f;
	basicAgri.mutationPotential = 0.05f;
	basicAgri.compatibilityTags = {"agriculture", "food"};
	basicAgri.prerequisites = {}; // none

	RegisterTechNode(basicAgri);

	// --- Agriculture.Irrigation ---
	CTechNode irrigation;
	irrigation.id = "agriculture.irrigation";
	irrigation.name = "Irrigation";
	irrigation.category = "agriculture";
	irrigation.requiredKnowledge = 60.0f;
	irrigation.baseDifficulty = 1.5f;
	irrigation.mutationPotential = 0.08f;
	irrigation.compatibilityTags = {"agriculture", "water"};
	irrigation.prerequisites = {"agriculture.basic"};

	RegisterTechNode(irrigation);

	// --- Agriculture.CropRotation ---
	CTechNode cropRotation;
	cropRotation.id = "agriculture.crop_rotation";
	cropRotation.name = "Crop Rotation";
	cropRotation.category = "agriculture";
	cropRotation.requiredKnowledge = 60.0f;
	cropRotation.baseDifficulty = 2.0f;
	cropRotation.mutationPotential = 0.12f;
	cropRotation.compatibilityTags = {"agriculture", "soil"};
	cropRotation.prerequisites = {"agriculture.irrigation"};

	RegisterTechNode(cropRotation);
}
/////////////////////////////////