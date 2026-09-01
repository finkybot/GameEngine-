/////////////////////////////////
// SpatialLayerRegistry.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <unordered_map>
#include <string>
#include "SpatialHashGrid.h"
#include "Entity.h"
/////////////////////////////////



/////////////////////////////////
struct SpatialLayerFilter {
	std::vector<ComponentTypeId> requiredComponents;
	std::vector<ComponentTypeId> forbiddenComponents;
};
/////////////////////////////////



/////////////////////////////////
// SpatialLayerRegistry class declaration. This class is intended to manage spatial layers in the game engine, but currently has no implementation details provided.
//								|
// 								|_______________________________________________________________________
class SpatialLayerRegistry {
	/////////////////////////////////
	// Public methods for the SpatialLayerRegistry class.
public:
	/////////////////////////////////
	// CreateLayer - Creates a new spatial layer with the specified name, cell size, and optional filter. If a layer with the same name already exists, it returns the existing grid. Otherwise, it creates a new LayerEntry with a 
	// SpatialHashGrid and the provided filter, and adds it to the m_layers map. This allows for efficient management of spatial layers by name.
	SpatialHashGrid<Entity>& CreateLayer(const std::string& name, float cellSize, const SpatialLayerFilter& filter = {}) {
		// If layer already exists, return existing grid
		auto it = m_layers.find(name);
		
		// If the layer is found, return the existing grid associated with that layer name
		if (it != m_layers.end()) return it->second.grid;

		// Otherwise create a new layer entry
		LayerEntry entry;
		entry.grid = SpatialHashGrid<Entity>(cellSize);
		entry.filter = filter;

		// Add the new layer entry to the m_layers map with the specified name
		m_layers.emplace(name, std::move(entry));
		return m_layers[name].grid;
	}
	/////////////////////////////////



	/////////////////////////////////
	// HasLayer - Checks if a spatial layer with the specified name exists in the registry. Returns true if the layer exists, false otherwise. This allows for efficient checking of spatial layers by name.
	bool HasLayer(const std::string& layerName) const { return m_layers.find(layerName) != m_layers.end(); }
	/////////////////////////////////


	/////////////////////////////////
	// Retrieve a named layer
	SpatialHashGrid<Entity>& GetLayer(const std::string& name) { return m_layers.at(name).grid; }
	/////////////////////////////////
	 
	 
	
	/////////////////////////////////
	// Retrieve filter for a named layer
	const SpatialLayerFilter& GetFilter(const std::string& name) const { return m_layers.at(name).filter; }
	/////////////////////////////////




	/////////////////////////////////
	// GetLayer - Retrieves a reference to the SpatialHashGrid instance associated with the specified layer name. If the layer does not exist, it throws an exception. This allows for efficient retrieval of spatial layers by name.
	void ClearAllLayers() { 
		for (auto& [name, layer] : m_layers) {
			layer.grid.Clear();
		}
	}
	/////////////////////////////////



	/////////////////////////////////
	// Private member variable to store the mapping of layer names to their corresponding SpatialHashGrid instances. This allows for efficient management and retrieval of spatial layers based on their names.
private:
	/////////////////////////////////
	// LayerEntry struct is a helper structure that encapsulates a SpatialHashGrid<Entity> instance and a SpatialLayerFilter. It is used to store information about each spatial layer in the registry, including the grid for spatial queries and the filter for component requirements.
	struct LayerEntry {
		SpatialHashGrid<Entity> grid;
		SpatialLayerFilter filter;
	};
	/////////////////////////////////



	/////////////////////////////////
	// m_layers is an unordered_map that associates layer names (as strings) with their corresponding SpatialHashGrid<Entity> instances. This allows for efficient management and retrieval of spatial layers based on their names.
	std::unordered_map<std::string, LayerEntry> m_layers;
	/////////////////////////////////



	/////////////////////////////////
	public:
	/////////////////////////////////
		// GetAllLayers - Retrieves a const reference to the unordered_map containing all spatial layers in the registry. This allows for iteration and management of all available spatial layers by name.
	const std::unordered_map<std::string, LayerEntry>& GetAllLayers() const { return m_layers; }

	// GetAllLayers (non-const) - Retrieves a non-const reference to the unordered_map containing all spatial layers in the registry. This allows for modification of the spatial layers during iteration (e.g., inserting entities into grids).
	std::unordered_map<std::string, LayerEntry>& GetAllLayers() { return m_layers; }
	/////////////////////////////////
};
