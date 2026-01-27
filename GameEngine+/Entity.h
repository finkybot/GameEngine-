#pragma once
#include <string>
#include <memory>
#include "CName.h"
#include "CShape.h"
#include "CTransform.h"
#include "Vec2.h"


class Entity
{
private:
	friend class EntityManager;
	const size_t		m_id	= 0;
	std::string	m_tag	= "Default";
	bool				m_alive	= true;
	
	Entity(const std::string& tag, size_t id); // private

	// Position tracking for efficient quadtree updates
	Vec2 m_previousPosition = Vec2::Zero;

public:
	std::unique_ptr<CTransform> cTransform;
	std::unique_ptr<CName>		cName;
	std::unique_ptr<CShape>		cShape;


	const std::string& GetTag();
	void SetTag(const std::string& tag) { m_tag = tag; }
	
	bool IsAlive() const; 
	void Destroy();
	void Bounce(const Entity* pItr) const;
	bool Intersects(const Entity* pItr) const;

	inline Vec2 GetCentrePoint() const { return cShape->GetCentrePoint(); }
	inline float GetMidLength() const { return cShape->GetMidLength(); }
	inline float GetWidth() const { return cShape->GetWidth(); }
	inline float GetHeight() const { return cShape->GetHeight(); }
	inline const Vec2& GetPosition() const noexcept { return cShape->GetPosition(); }
	inline float GetRadius() const { return cShape->GetRadius(); }
	static inline Vec2 GetCentre(Entity& e) { return e.cShape->GetCentrePoint(); }

	void SetMidLength(float length) const { cShape->SetMidLength(length); }
};


