/////////////////////////////////
// System.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the System base class.
#pragma once
class EntityManager;



/////////////////////////////////
// System base class -
// 								|
//								|_______________________________________________________________________
class System {
	/////////////////////////////////
	// Public interface for the System base class
public:
	bool enabled = true;		 // Flag to enable or disable the system
	virtual ~System() = default; // Virtual destructor for proper cleanup of derived classes
	virtual void Update(float dt, EntityManager& entityManager) = 0; // Pure virtual method to be implemented by derived systems for updating logic
};


