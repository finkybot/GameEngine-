// ***** Component class declaration *****
#pragma once
#include <typeindex>
#include <map>
#include <memory>
#include <type_traits>

// Base Component class - all components will inherit from this. It is an empty class that serves as a common base for all components in the ECS architecture,
// allowing for polymorphic storage and retrieval of components within entities. The Component class itself does not contain any data or functionality,
// but it provides a common type for all components to enable type-safe management of components in the Entity class.
class Component {
public:
	virtual ~Component() = default;
};
