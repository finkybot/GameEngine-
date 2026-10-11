/////////////////////////////////
// RaycastDispatcher.cpp
/////////////////////////////////



/////////////////////////////////
// Includes
#include "RaycastDispatcher.h"
/////////////////////////////////



/////////////////////////////////
// Global instance of RaycastDispatcher for managing raycast function registrations and dispatching. This allows for centralized management of raycast functions for different entity types, enabling dynamic dispatching based on entity type at runtime.
RaycastDispatcher g_RaycastDispatcher;
/////////////////////////////////



/////////////////////////////////
// Get - retrieves the registered raycast function for a given type hash code. If the type is not found in the registry, it returns nullptr. This allows for dynamic dispatching of raycast functions based on entity types at runtime.
RaycastFn RaycastDispatcher::Get(std::type_index type) const {
	// Look up the type in the registry
	auto it = registry.find(type);

	// Return the function pointer if found, otherwise return nullptr
	return (it != registry.end()) ? it->second : nullptr;
}
/////////////////////////////////
