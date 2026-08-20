/////////////////////////////////
// CName.h - Component for storing the name of an entity
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Component.h"
/////////////////////////////////



/////////////////////////////////
// CName component - stores the name of an entity for identification (or at least it will eventually, for now it does nothing, but I want to have it in place for future use)
struct CName : public Component {
	std::string name;
	CName() = default;
	CName(const std::string& name) : name(name) {}
};
/////////////////////////////////