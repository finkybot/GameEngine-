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


/// <summary>
/// Entity - A game object that holds dynamic components
/// Uses an ECS (Entity Component System) pattern for flexible composition
/// </summary>
class Entity
{
private:
	friend class EntityManager;
	const size_t m_id = 0;
	EntityType m_type = EntityType::Default;
	bool m_alive = true;
	
	/// <summary>
	/// Private constructor - entities should be created via EntityManager
	/// </summary>
	Entity(EntityType type, size_t id);

	// Position tracking for efficient quadtree updates
	Vec2 m_previousPosition = Vec2::Zero;

	// Dynamic component storage using type_index for polymorphic lookups
	std::map<std::type_index, std::unique_ptr<Component>> m_components;

public:
	/// <summary>
	/// Add a component to this entity
	/// </summary>
	/// <typeparam name="T">Component type to add</typeparam>
	/// <param name="args">Constructor arguments for the component</param>
	/// <returns>Pointer to the newly added component</returns>
	template<typename T, typename... Args>
	T* AddComponent(Args&&... args) {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		auto comp = std::make_unique<T>(std::forward<Args>(args)...);
		T* ptr = comp.get();
		m_components[std::type_index(typeid(T))] = std::move(comp);
		return ptr;
	}

	/// <summary>
	/// Add a pre-constructed component pointer (for storing derived types as base types)
	/// Useful for polymorphic components like CCircle stored as CShape
	/// </summary>
	/// <typeparam name="T">Base component type to store as</typeparam>
	/// <param name="comp">Unique pointer to the component</param>
	/// <returns>Pointer to the added component</returns>
	template<typename T>
	T* AddComponentPtr(std::unique_ptr<T> comp) {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		T* ptr = comp.get();
		m_components[std::type_index(typeid(T))] = std::move(comp);
		return ptr;
	}

	/// <summary>
	/// Get a component by type (non-const version)
	/// </summary>
	/// <typeparam name="T">Component type to retrieve</typeparam>
	/// <returns>Pointer to component or nullptr if not found</returns>
	template<typename T>
	T* GetComponent() {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		auto it = m_components.find(std::type_index(typeid(T)));
		if (it != m_components.end()) {
			return static_cast<T*>(it->second.get());
		}
		return nullptr;
	}

	/// <summary>
	/// Get a component by type (const version)
	/// </summary>
	/// <typeparam name="T">Component type to retrieve</typeparam>
	/// <returns>Const pointer to component or nullptr if not found</returns>
	template<typename T>
	const T* GetComponent() const {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		auto it = m_components.find(std::type_index(typeid(T)));
		if (it != m_components.end()) {
			return static_cast<const T*>(it->second.get());
		}
		return nullptr;
	}

	/// <summary>
	/// Check if entity has a component of the given type
	/// </summary>
	/// <typeparam name="T">Component type to check for</typeparam>
	/// <returns>True if component exists, false otherwise</returns>
	template<typename T>
	bool HasComponent() const {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		return m_components.find(std::type_index(typeid(T))) != m_components.end();
	}

	/// <summary>
	/// Remove a component from this entity
	/// </summary>
	/// <typeparam name="T">Component type to remove</typeparam>
	template<typename T>
	void RemoveComponent() {
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		m_components.erase(std::type_index(typeid(T)));
	}

	/// <summary>
	/// Convenience accessors for commonly used components
	/// </summary>
	CShape* GetShape() { return GetComponent<CShape>(); }
	CTransform* GetTransform() { return GetComponent<CTransform>(); }
	CName* GetName() { return GetComponent<CName>(); }

	/// <summary>
	/// Get the tag/type of this entity (e.g. "TeamEagle", "Explosion")
	/// </summary>
	EntityType GetType() const;
	
	/// <summary>
	/// Set the tag/type of this entity
	/// </summary>
	void SetType(EntityType type) { m_type = type; }
	
	/// <summary>
	/// Check if entity is alive (not destroyed)
	/// </summary>
	bool IsAlive() const; 
	
	/// <summary>
	/// Mark entity for destruction (actual removal happens in RemoveDeadEntities)
	/// Uses deferred deletion to maintain iterator validity during game logic
	/// </summary>
	void Destroy();

	/// <summary>
	/// Get the center point of the entity (delegates to CShape component)
	/// </summary>
	inline Vec2 GetCentrePoint() const { 
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetCentrePoint() : Vec2::Zero;
	}
	
	/// <summary>
	/// Get the mid-length of the entity (delegates to CShape component)
	/// </summary>
	inline float GetMidLength() const { 
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetMidLength() : 0.0f;
	}
	
	/// <summary>
	/// Get the width of the entity (delegates to CShape component)
	/// </summary>
	inline float GetWidth() const { 
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetWidth() : 0.0f;
	}
	
	/// <summary>
	/// Get the height of the entity (delegates to CShape component)
	/// </summary>
	inline float GetHeight() const { 
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetHeight() : 0.0f;
	}
	
	/// <summary>
	/// Get the position of the entity (delegates to CShape component)
	/// </summary>
	inline const Vec2& GetPosition() const noexcept { 
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetPosition() : Vec2::Zero;
	}
	
	/// <summary>
	/// Get the radius of the entity (delegates to CShape component)
	/// </summary>
	inline float GetRadius() const { 
		auto shape = GetComponent<CShape>();
		return shape ? shape->GetRadius() : 0.0f;
	}
	
	/// <summary>
	/// Static helper to get the center point of an entity
	/// </summary>
	static inline Vec2 GetCentre(Entity& e) { return e.GetCentrePoint(); }

	/// <summary>
	/// Set the mid-length of the entity (delegates to CShape component)
	/// </summary>
	void SetMidLength(float length) { 
		auto shape = GetComponent<CShape>();
		if (shape) shape->SetMidLength(length);
	}
};
