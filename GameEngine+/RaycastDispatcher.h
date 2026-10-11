/////////////////////////////////
// RaycastDispatcher.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <unordered_map>
#include <cstddef>
#include <typeinfo>
#include <typeindex>
/////////////////////////////////



/////////////////////////////////
// Forward declarations
class Entity;
struct Vec2;
/////////////////////////////////



/////////////////////////////////
// Using declarations
using RaycastFn = bool (*)(const Vec2& origin, const Vec2& direction, float maxDistance, const Entity& entity, float& outT, Vec2& outNormal);
/////////////////////////////////



/////////////////////////////////
//	|	RaycastDispatcher class is a placeholder for a future implementation that will handle the dispatching of raycast events or requests within the game engine. This class is currently empty and serves as a stub for future development.
//	|_______________________________________________________________________
class RaycastDispatcher {
public:
	template <typename T>
	void Register(RaycastFn fn) { registry[typeid(T)] = fn; }

	RaycastFn Get(std::type_index type) const;

	const std::unordered_map<std::type_index, RaycastFn>& GetRegistry() const { return registry; }

private:
	std::unordered_map<std::type_index, RaycastFn> registry;
};
/////////////////////////////////


// GLOBAL DISPATCHER DECLARATION
extern RaycastDispatcher g_RaycastDispatcher;