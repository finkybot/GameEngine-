// ***** Entity.h - Entity class definition *****
#pragma once
#include <string>
#include <memory>
#include <map>
#include <typeindex>
#include "Component.h"
#include "CName.h"
#include "CShape.h"
#include "CTransform.h"
#include "Vec2.h"
#include "EntityType.h"

// Entity class - represents a game object with a unique ID, type, and a collection of components.
class Entity {
private:
	friend class EntityManager; // Only EntityManager can create and manage entities, so constructor is private.

	// Private member variables for entity state management, tagged with m_ prefix to indicate member variables
	const size_t m_id = 0;
	EntityType m_type = EntityType::Default;
	bool m_alive = true;
	std::map<std::type_index, std::unique_ptr<Component>> m_components;
	Vec2 m_previousPosition = Vec2::Zero;

	// Private constructor, only callable by EntityManager
	Entity(EntityType type, size_t id);

public:
	// Public member variable to track the creation time of the entity, used for time-based logic such as explosion lifespan
	std::chrono::high_resolution_clock::time_point m_creationTime;
	size_t GetId() const { return m_id; }

	// Template method to add a component of type T to the entity, forwarding any constructor arguments. Returns a pointer to the added component for convenience.
	template<typename T, typename... Args>
	T* AddComponent(Args&&... args) {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		auto comp = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr = comp.get();
		m_components[std::type_index(typeid(T))] = std::move(comp);
		return ptr;
	}

	// Template method to add a component of type T to the entity using a unique_ptr. This allows for more complex component construction outside of the entity. Returns a pointer to the added component for convenience.
	template<typename T>
	T* AddComponentPtr(std::unique_ptr<T> comp) {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		T* ptr = comp.get();
		m_components[std::type_index(typeid(T))] = std::move(comp);
		return ptr;
	}

	// Template method to get a pointer to a component of type T. Returns nullptr if the component does not exist on the entity.
	template<typename T>
	T* GetComponent() {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		auto it = m_components.find(std::type_index(typeid(T)));
		if (it != m_components.end()) {
			return static_cast<T*>(it->second.get());
		}
		return nullptr;
	}

	// Const version of GetComponent to allow access to components on const entities. Returns nullptr if the component does not exist on the entity.
	template<typename T>
	const T* GetComponent() const {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		auto it = m_components.find(std::type_index(typeid(T)));
		if (it != m_components.end()) {
			return static_cast<const T*>(it->second.get());
		}
		return nullptr;
	}

	// Template method to check if the entity has a component of type T. Returns true if the component exists, false otherwise.
	template<typename T>
	bool HasComponent() const {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		return m_components.find(std::type_index(typeid(T))) != m_components.end();
	}

	// Template method to remove a component of type T from the entity. If the component does not exist, this method does nothing.
	template<typename T>
	void RemoveComponent() {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		m_components.erase(std::type_index(typeid(T)));
	}

	// Convenience methods to get specific components commonly used in the game. These methods return nullptr if the component does not exist on the entity.
	CShape* GetShape() { return GetComponent<CShape>(); } // Get the CShape component of the entity
	CTransform* GetTransform() { return GetComponent<CTransform>(); } // Get the CTransform component of the entity
	CName* GetName() { return GetComponent<CName>(); } // Get the CName component of the entity

	EntityType GetType() const; // Get the type of the entity
	void SetType(EntityType type) { m_type = type; } // Set the type of the entity
	bool IsAlive() const; // Check if the entity is alive (not marked for destruction)
	void Destroy(); // Mark the entity for destruction

	// Convenience methods to get properties from the CShape component if it exists, or return default values if the component is not present.
	inline Vec2 GetCentrePoint() const {
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetCentrePoint() : Vec2::Zero;
	}

	// Get the mid-length property from the CShape component, which is used for collision detection and quadtree inclusion (I dont use the quadtree anymore).
	inline float GetMidLength() const {
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetMidLength() : 0.0f;
	}

	// Get the width of the bounding box from the CShape component, or return 0 if the component does not exist.
	inline float GetWidth() const {
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetWidth() : 0.0f;
	}
	
	// Get the height of the bounding box from the CShape component, or return 0 if the component does not exist.
	inline float GetHeight() const {
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetHeight() : 0.0f;
	}

	// Get the position from the CTransform component, or return a reference to a default zero vector if the component does not exist.
	inline const Vec2& GetPosition() const noexcept {
		auto transform = GetComponent<CTransform>();
		if (transform) return transform->m_position;
	}

	// Get the radius from the CShape component, or return 0 if the component does not exist.
	inline float GetRadius() const {
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetRadius() : 0.0f;
	}

	// Get the center point of the shape from the CShape component, or return a zero vector if the component does not exist.
	static inline Vec2 GetCentre(Entity& e) { return e.GetCentrePoint(); }

	// Set the mid-length property on the CShape component if it exists
	void SetMidLength(float length) {
		auto shape = GetComponent<CShape>();
		if (shape) shape->SetMidLength(length);
	}
};

