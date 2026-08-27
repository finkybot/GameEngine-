/////////////////////////////////
// SystemManager.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations
#pragma once
#include <vector>
#include "System.h"
#include "EntityManager.h"
/////////////////////////////////


/////////////////////////////////
// SystemManager class - 
//								|
//								|_______________________________________________________________________
class SystemManager {
	/////////////////////////////////
	// Public interface for the SystemManager class
public:
	/////////////////////////////////
	// AddSystem - Adds a system to the manager. This method takes a pointer to a System object and appends it to the internal vector of systems.
	void AddSystem(System* system) { m_systems.push_back(system); }
	/////////////////////////////////



	/////////////////////////////////
	// RemoveSystem - Removes a system from the manager if it exists. This method searches for the specified system in the internal vector and erases it if found.
	void RemoveSystem(System* system) {
		auto it = std::find(m_systems.begin(), m_systems.end(), system);
		if (it != m_systems.end()) {
			m_systems.erase(it);
		}
	}
	/////////////////////////////////



	/////////////////////////////////
	// Update - Updates all systems managed by the SystemManager. This method iterates through each system and calls its Update method, passing in the delta time and a reference to the EntityManager.
	void Update(float dt, EntityManager& entityManager) {
		for (auto system : m_systems) {
			if (system->enabled) system->Update(dt, entityManager);
		}
	}
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the SystemManager class. This vector holds pointers to all systems managed by the SystemManager.
private:
	/////////////////////////////////
	std::vector<System*> m_systems;
	/////////////////////////////////
};
/////////////////////////////////