/////////////////////////////////
// Component.h
/////////////////////////////////



/////////////////////////////////
// Include guards and necessary headers for the Component class. We include typeindex for runtime type information, map and memory for component storage, and type_traits for static assertions to ensure type safety when managing components.
#pragma once
#include <typeindex>
#include <map>
#include <memory>
#include <type_traits>
/////////////////////////////////



/////////////////////////////////
// Component class - all components will inherit from this. It is an empty class that serves as a common base for all components in the ECS architecture, allowing for polymorphic storage and retrieval of components within entities. 
// The Component class itself does not contain any data or functionality, but it provides a common type for all components to enable type-safe management of components in the Entity class.
class Component {
	/////////////////////////////////
	// Public distructor for the Component class.
public:
	/////////////////////////////////
	// Virtual destructor to allow for proper cleanup of derived component classes when deleted through a base class pointer.
	virtual ~Component() = default;
	/////////////////////////////////
};
/////////////////////////////////