/////////////////////////////////
// ComponentTypeId.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "CTransform.h"
#include "CCivilisationTech.h"
#include "CStatic.h"
#include "CShape.h"
#include "CRectangle.h"
#include "CCircle.h"
#include "CExplosion.h"
#include "CTileMap.h"
#include "CMusic.h"
//#include "CSound.h"
/////////////////////////////////



/////////////////////////////////
// ComponentTypeId enumeration - Defines unique identifiers for different component types used in the game engine. Each component type corresponds to a specific aspect of an entity's behavior or properties, allowing for modular and flexible entity composition.
enum class ComponentTypeId {
	Transform,
	CivilisationTech,
	Static,
	Shape,
	Rectangle,
	Circle,
	Explosion,
	TileMap,
	Music,
	//Sound,
	// Add more as needed
};

template <typename T>
constexpr ComponentTypeId GetComponentTypeId();

/////////////////////////////////
// Template specializations
/////////////////////////////////

template <>
constexpr ComponentTypeId GetComponentTypeId<CTransform>() {
	return ComponentTypeId::Transform;
}

template <>
constexpr ComponentTypeId GetComponentTypeId<CCivilisationTech>() {
	return ComponentTypeId::CivilisationTech;
}

template <>
constexpr ComponentTypeId GetComponentTypeId<CStatic>() {
	return ComponentTypeId::Static;
}

template <>
constexpr ComponentTypeId GetComponentTypeId<CShape>() {
	return ComponentTypeId::Shape;
}

template <>
constexpr ComponentTypeId GetComponentTypeId<CRectangle>() {
	return ComponentTypeId::Rectangle;
}

template <>
constexpr ComponentTypeId GetComponentTypeId<CCircle>() {
	return ComponentTypeId::Circle;
}

template <>
constexpr ComponentTypeId GetComponentTypeId<CExplosion>() {
	return ComponentTypeId::Explosion;
}

template <>
constexpr ComponentTypeId GetComponentTypeId<CTileMap>() {
	return ComponentTypeId::TileMap;
}

template <>
constexpr ComponentTypeId GetComponentTypeId<CMusic>() {
	return ComponentTypeId::Music;
}

//template <>
//constexpr ComponentTypeId GetComponentTypeId<CSound>() {
//	return ComponentTypeId::Sound;
//}

/////////////////////////////////
// End of ComponentTypeId.h
/////////////////////////////////