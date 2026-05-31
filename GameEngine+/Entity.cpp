/////////////////////////////////
// Entity.cpp - Implementation of the Entity class methods declared in Entity.h
/////////////////////////////////



/////////////////////////////////
// Includes. We include the Entity.h header to implement the methods of the Entity class.
#include "Entity.h"
/////////////////////////////////



/////////////////////////////////
// Constructor for the Entity class. Initializes the entity with a specified type and ID, setting the alive status to true by default. This constructor is private and can only be called by the EntityManager class, which is declared as a friend of Entity.
Entity::Entity(EntityType type, size_t id) : m_type(type), m_id(id) {}
/////////////////////////////////



/////////////////////////////////
// GetType - returns the type of the entity. This method allows external code to query the type of the entity, which can be used for various purposes such as rendering, collision handling, or game logic decisions based on entity type.
EntityType Entity::GetType() const {
	return m_type;
}
/////////////////////////////////



/////////////////////////////////
// IsAlive - returns whether the entity is currently alive. This method allows external code to check the alive status of the entity, which can be used to determine if the entity should be updated, rendered, or if it should be removed from the game world.
bool Entity::IsAlive() const {
	return m_alive;
}
/////////////////////////////////



/////////////////////////////////
// Destroy - marks the entity as destroyed by setting its alive status to false. This method can be called when the entity should be removed from the game world, such as when it is defeated, expires, or is otherwise no longer needed. 
// The EntityManager can then handle the cleanup of destroyed entities.
void Entity::Destroy() {
	m_alive = false;
}
/////////////////////////////////