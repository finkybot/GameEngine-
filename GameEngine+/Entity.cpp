#include "Entity.h"

Entity::Entity(EntityType type, size_t id): m_type(type), m_id(id)
{
}

EntityType Entity::GetType() const
{
	return m_type;
}

bool Entity::IsAlive() const
{
	return m_alive;
}

void Entity::Destroy()
{
	m_alive = false;
}

