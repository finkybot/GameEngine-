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


// Entity class - represents a game object with a unique ID, type, and a collection of components. It provides methods for adding, retrieving, and removing components, as well as managing the entity's lifecycle (alive/dead) and type/tag. The Entity class is designed
// to be managed by the EntityManager, which handles creation, destruction, and updates of all entities in the game.
class Entity
{
	// ****** Private Member Variables *****
private:
	friend class EntityManager;											// Only EntityManager can create entities, so it needs access to the private constructor and member variables.
	const size_t m_id = 0;												// Unique identifier for this entity, assigned by EntityManager. Immutable after creation.
	EntityType m_type = EntityType::Default;							// Tag/type of this entity (e.g. "TeamEagle", "Explosion"), can be used for grouping and logic decisions.
	bool m_alive = true;												// Alive status of the entity. True if the entity is active in the game, false if it has been destroyed. Managed by EntityManager for deferred deletion.
	std::map<std::type_index, std::unique_ptr<Component>> m_components; // Map of component type to component instance for this entity. Using unique_ptr for automatic memory management.

	// ****** Private Methods *****
	Entity(EntityType type, size_t id);									// Private constructor - only EntityManager can create entities
	Vec2 m_previousPosition = Vec2::Zero;								// For spatial hash updates - track previous position to detect movement

public:
	// ****** Public Member Variables *****
	std::chrono::high_resolution_clock::time_point m_creationTime;		// Track the creation time of this entity, used for explosion fading and other time-based logic

	// ****** Public Methods *****
public:
	
	// Variadic (now thats a programming term) template method to add a component of type T to this entity, forwarding constructor arguments. It creates a new component instance, stores it in the components map, and returns a pointer to the added component.
	template<typename T, typename... Args> T* AddComponent(Args&&... args) 
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		auto comp = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr = comp.get();
		m_components[std::type_index(typeid(T))] = std::move(comp);
		return ptr;
	}

	// Overload of AddComponent that takes a unique_ptr to an existing component instance. This allows for more flexible component creation (e.g. creating a shape component with specific properties before adding it to the entity). It stores the provided component in the components map and returns a pointer to it.
	template<typename T> T* AddComponentPtr(std::unique_ptr<T> comp) 
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		T* ptr = comp.get();
		m_components[std::type_index(typeid(T))] = std::move(comp);
		return ptr;
	}

	// Get a component by type
	template<typename T> T* GetComponent() 
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		auto it = m_components.find(std::type_index(typeid(T)));
		if (it != m_components.end()) {
			return static_cast<T*>(it->second.get());
		}
		return nullptr;
	}

	// Const version of GetComponent for read-only access to components. It returns a pointer to the component if it exists, or nullptr if the component is not found.
	template<typename T> const T* GetComponent() const 
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		auto it = m_components.find(std::type_index(typeid(T)));
		if (it != m_components.end()) {
			return static_cast<const T*>(it->second.get());
		}
		return nullptr;
	}

	// Check if this entity has a component of type T. It returns true if the component exists in the components map, or false if it does not.
	template<typename T> bool HasComponent() const 
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		return m_components.find(std::type_index(typeid(T))) != m_components.end();
	}

	// Remove a component of type T from this entity. It erases the component from the components map, effectively destroying it. If the component does not exist, this method does nothing.
	template<typename T> void RemoveComponent() 
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		m_components.erase(std::type_index(typeid(T)));
	}

	// Convenience getters for common components (these are not strictly necessary but provide easy access to frequently used components like CShape, CTransform, and CName without needing to specify the template type each time).
	CShape* GetShape() { return GetComponent<CShape>(); }				// Get the CShape component of this entity, or nullptr if it doesn't exist.
	CTransform* GetTransform() { return GetComponent<CTransform>(); }	// Get the CTransform component of this entity, or nullptr if it doesn't exist.
	CName* GetName() { return GetComponent<CName>(); }					// Get the CName component of this entity, or nullptr if it doesn't exist.
	EntityType GetType() const;											// Get the tag/type of this entity (e.g. "TeamEagle", "Explosion"). This is used for grouping entities and making logic decisions based on type.

	void SetType(EntityType type) { m_type = type; }	// Set the tag/type of this entity. This can be used to change the entity's group or category during its lifecycle.
	bool IsAlive() const;								// Check if this entity is alive (active in the game). Returns true if the entity is alive, or false if it has been destroyed. This is used by systems to determine whether to process this entity or skip it. 
	void Destroy();										// Mark this entity for destruction. This sets the entity's state to not alive, which will be handled by the game logic to remove the entity safely.

	// Convenience methods to delegate common shape properties to the CShape component. These methods check if the entity has a CShape component and, if so, call the corresponding method on it. If the CShape component does not exist, they return default values (e.g. zero vector for position, zero for dimensions). This allows for easy access to shape properties directly from the entity without needing to manually get the CShape component each time.
	inline Vec2 GetCentrePoint() const 
	{ 
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetCentrePoint() : Vec2::Zero;
	}
	
	// Get the mid-length of the entity (delegates to CShape component)
	inline float GetMidLength() const 
	{ 
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetMidLength() : 0.0f;
	}
	
	// Get the width of the entity (delegates to CShape component)
	inline float GetWidth() const 
	{ 
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetWidth() : 0.0f;
	}
	
	// Get the height of the entity (delegates to CShape component)
	inline float GetHeight() const 
	{ 
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetHeight() : 0.0f;
	}
	
	// Get the position of the entity (delegates to CShape component)
	inline const Vec2& GetPosition() const noexcept 
	{ 
		auto shape = GetComponent<CShape>();
		return shape ? shape->m_position : Vec2::Zero;
	}
	
	// Get the radius of the entity (delegates to CShape component)
	inline float GetRadius() const { 
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetRadius() : 0.0f;
	}
	
	// Get the center point of the entity (delegates to CShape component)
	static inline Vec2 GetCentre(Entity& e) { return e.GetCentrePoint(); }

	// Set the mid-length of the entity (delegates to CShape component)
	void SetMidLength(float length) 
	{ 
		auto shape = GetComponent<CShape>();
		if (shape) shape->SetMidLength(length);
	}
};
