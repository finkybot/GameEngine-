/////////////////////////////////
// ***** Entity.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the Entity class. We include necessary headers for string manipulation, memory management, component handling, and various component types used in the game.
#pragma once
#include <string>
#include <memory>
#include <map>
#include <typeindex>
#include "Component.h"
#include "CName.h"
#include "CShape.h"
#include "CTransform.h"
#include "CLayer.h"
#include "CCamera.h"
#include "CTileMap.h"
#include "CSoundEffect.h"
#include "CText.h"
#include "CColliderRect.h"
#include "Vec2.h"
#include "EntityType.h"
#include "ComponentTypeId.h"
/////////////////////////////////



/////////////////////////////////
//	|	Entity class declaration, representing a game entity in the ECS architecture. Each entity has a unique ID, type, and a collection of components that define its behavior and properties. The Entity class provides methods for adding, retrieving, and managing 
//	|	components, as well as handling entity state such as position, rendering layer, and ownership.
//	|___________________________________________________________________________________
class Entity {

private:
	friend class EntityManager;					// Only EntityManager can create and manage entities so we will make the constructor private 
	const size_t m_id = 0;						// Unique identifier for the entity, assigned by the EntityManager
	EntityType m_type = EntityType::Default;
	bool m_alive = true;
	std::map<std::type_index, std::unique_ptr<Component>> m_components;
	Vec2 m_previousPosition = Vec2::Zero;

	size_t m_ownerId =	0;						// Optional owner ID for the entity, can be used to track ownership or relationships between entities
	Entity(	EntityType type, size_t	id);		// Private constructor for the Entity class, only accessible by the EntityManager. Initializes the entity with a specific type and unique ID.



public:
	// Rendering layer enumeration for the entity, used to determine the order of rendering in the game. Entities in lower layers are rendered first, while entities in higher layers are rendered on top.
	enum class Layer {
		Background = 0,
		Mid = 1,
		Foreground = 2,
		Overlay = 3	
	}; 

	// Getter and Setter for the owner ID of the entity. This can be used to track ownership or relationships between entities, such as a projectile being owned by a player entity.
	size_t GetOwnerId() const { return m_ownerId; }
	void SetOwnerId(size_t ownerId) { m_ownerId = ownerId; }

	// Get the rendering layer for the entity. If a CLayer component exists, its layer will be returned; otherwise, the layer stored at the entity level will be returned for backward compatibility.
	Layer GetLayer() const {
		if (auto cl = GetComponent<CLayer>()) {
			return static_cast<Layer>(cl->m_layer);
		}
		return m_layer;
	}

	// Set the rendering layer for the entity. If a CLayer component exists, it will be updated; otherwise, the layer will be stored at the entity level for backward compatibility.
	void SetLayer(Layer layer) {
		if (auto cl = GetComponent<CLayer>()) {
			cl->m_layer = static_cast<CLayer::Layer>(layer);
		}
		m_layer = layer;
	}

	// Public member variable to track the creation time of the entity, used for time-based logic such as explosion lifespan
	std::chrono::high_resolution_clock::time_point m_creationTime;
	size_t GetId() const { return m_id; }


	
	
	
	// ---------------------------------
	// Template Methods for Component Management
	// ---------------------------------

	// Template method to add a component of type T to the entity, forwarding any constructor arguments. Returns a pointer to the added component for convenience.
	template <typename T, typename... Args>	T* AddComponent(Args&&... args) {
		// Ensure that T is derived from Component to maintain type safety in the ECS architecture. This static assertion will cause a compile-time error if T does not inherit from Component.
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		// Create a unique_ptr for the new component, forwarding the constructor arguments to T's constructor. This allows for flexible component construction with varying parameters.
		auto comp = std::make_unique<T>(std::forward<Args>(args)...);

		// Store the unique_ptr in the m_components map, using the type_index of T as the key. This allows for efficient retrieval of components by type.
		T* ptr = comp.get();

		// Move the unique_ptr into the map to transfer ownership of the component to the entity. This ensures that the component's lifetime is managed by the entity and will be automatically cleaned up when the entity is destroyed.
		m_components[std::type_index(typeid(T))] = std::move(comp);
		return ptr;
	}



	// Template method to add a component of type T to the entity using a unique_ptr. This allows for more complex component construction outside of the entity. Returns a pointer to the added component for convenience.
	template <typename T> T* AddComponentPtr(std::unique_ptr<T> comp) {
		// Ensure that T is derived from Component to maintain type safety in the ECS architecture. This static assertion will cause a compile-time error if T does not inherit from Component.
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		// Store the unique_ptr in the m_components map, using the type_index of T as the key. This allows for efficient retrieval of components by type.
		T* ptr = comp.get();

		//	Move the unique_ptr into the map to transfer ownership of the component to the entity. This ensures that the component's lifetime is managed by the entity and will be automatically cleaned up when the entity is destroyed.
		m_components[std::type_index(typeid(T))] = std::move(comp);
		return ptr;
	}



	// Template method to get a pointer to a component of type T. Returns nullptr if the component does not exist on the entity.
	template <typename T> T* GetComponent() {
		// Ensure that T is derived from Component to maintain type safety in the ECS architecture. This static assertion will cause a compile-time error if T does not inherit from Component.
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		// Find the component in the m_components map using the type_index of T as the key. If found, return a pointer to the component; otherwise, return nullptr.
		auto it = m_components.find(std::type_index(typeid(T)));

		// If the component is found, return a pointer to it; otherwise, return nullptr.
		if (it != m_components.end()) {
			return static_cast<T*>(it->second.get());
		}
		return nullptr;
	}



	// Const version of GetComponent to allow access to components on const entities. Returns nullptr if the component does not exist on the entity.
	template <typename T> const T* GetComponent() const {
		// Ensure that T is derived from Component to maintain type safety in the ECS architecture. This static assertion will cause a compile-time error if T does not inherit from Component.
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		// Find the component in the m_components map using the type_index of T as the key. If found, return a pointer to the component; otherwise, return nullptr.
		auto it = m_components.find(std::type_index(typeid(T)));

		// If the component is found, return a pointer to it; otherwise, return nullptr.
		if (it != m_components.end()) {
			return static_cast<const T*>(it->second.get());
		}
		return nullptr;
	}



	// Template method to check if the entity has a component of type T. Returns true if the component exists, false otherwise.
	template <typename T> bool HasComponent() const {
		//	Ensure that T is derived from Component to maintain type safety in the ECS architecture. This static assertion will cause a compile-time error if T does not inherit from Component.
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		// Check if the component exists in the m_components map using the type_index of T as the key. Returns true if found, false otherwise.
		return m_components.find(std::type_index(typeid(T))) != m_components.end();
	}



	// Method to check if the entity has a component of a specific type based on the ComponentTypeId enum. This allows for checking components without needing to know the exact C++ type.
	bool HasComponent(ComponentTypeId id) const {
		switch (id) {
		case ComponentTypeId::Transform:
			return HasComponent<CTransform>();
		case ComponentTypeId::CivilisationTech:
			return HasComponent<CCivilisationTech>();
		case ComponentTypeId::Static:
			return HasComponent<CStatic>();
		case ComponentTypeId::Shape:
			return HasComponent<CShape>();
		case ComponentTypeId::Rectangle:
			return HasComponent<CRectangle>();
		case ComponentTypeId::Circle:
			return HasComponent<CCircle>();
		case ComponentTypeId::Explosion:
			return HasComponent<CExplosion>();
		case ComponentTypeId::TileMap:
			return HasComponent<CTileMap>();
		case ComponentTypeId::Music:
			return HasComponent<CMusic>();
		//case ComponentTypeId::Sound:
		//	return HasComponent<CSound>();
		default:
			return false;
		}
	}

	// Template method to remove a component of type T from the entity. If the component does not exist, this method does nothing.
	template <typename T> void RemoveComponent() {
		//	Ensure that T is derived from Component to maintain type safety in the ECS architecture. This static assertion will cause a compile-time error if T does not inherit from Component.
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		// Remove the component from the m_components map using the type_index of T as the key. If the component does not exist, this operation has no effect.
		m_components.erase(std::type_index(typeid(T)));
	}

	// Helper methods to get specific components commonly used in the game. These methods return nullptr if the component does not exist on the entity.
	CShape* GetShape() { return GetComponent<CShape>(); }				// Get the CShape component of the entity
	CTransform* GetTransform() { return GetComponent<CTransform>(); }	// Get the CTransform component of the entity
	CName* GetName() { return GetComponent<CName>(); }					// Get the CName component of the entity


	EntityType GetType() const;											// Get the type of the entity
	void SetType(EntityType type) { m_type = type; }					// Set the type of the entity
	bool IsAlive() const;												// Check if the entity is alive (not marked for destruction)
	void Destroy();														// Mark the entity for destruction


	// Convenience methods to get properties from the CTransform component first, since transform is the source of truth for motion and collision.
	// If no transform exists, fall back to the shape center, then zero.
	inline Vec2 GetCentrePoint() const {
		auto transform = GetComponent<CTransform>();
		if (transform) return transform->position;
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
		if (transform)
			return transform->position;
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
		if (shape)
			shape->SetMidLength(length);
	}

	// GetBucketId - Get the bucket ID for the entity, which indicates which layer bucket it belongs to for rendering optimization. Returns -1 if the entity is not currently in a bucket.
	int GetBucketId() const { return m_bucketId; }

	// GetBucketPos - Get the bucket position for the entity, which indicates its position within its layer bucket for rendering optimization. Returns -1 if the entity is not currently in a bucket.
	int GetBucketPos() const { return m_bucketPos; }

	// SetBucketInfo - Set the bucket information for the entity, indicating which layer bucket it belongs to and its position within that bucket. This is used for incremental rendering maintenance to optimize drawing performance.
	void SetBucketInfo(int bucketId, int bucketPos) { m_bucketId = bucketId; m_bucketPos = bucketPos; }

	bool IsActive() const { return m_active; }
	void SetActive(bool active) { m_active = active; }

	// Private member variables and methods for internal state management. These are not accessible outside of the Entity class and are used to manage the entity's rendering layer and bucket metadata for rendering optimization.
private:
	// Backing field for render layer (default Mid)
	Layer m_layer = Layer::Mid;
    // per-entity bucket metadata for EntityManager layer buckets
	int m_bucketId = -1;
	int m_bucketPos = -1;
	bool m_active = true;
};
/////////////////////////////////