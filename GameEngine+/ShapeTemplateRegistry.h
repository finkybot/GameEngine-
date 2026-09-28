/////////////////////////////////
// ShapeTemplateRegistry.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <unordered_map>
#include "EntityType.h"
#include "Vec2.h"
/////////////////////////////////



/////////////////////////////////
//	|	ShapeTemplate struct - defines the properties of a shape template, including its type, radius, color, and velocity. This struct is used to store the characteristics of different shapes that can be spawned in the game.
//	|_______________________________________________________________________
struct ShapeTemplate {
	float radius; // The radius of the shape, used for rendering and collision detection.
	Vec3 color;	  // The color of the shape, represented as a Vec3 (RGB) for rendering purposes.
};
/////////////////////////////////



/////////////////////////////////
// | ShapeTemplateRegistry class - manages a registry of shape templates, allowing for the addition and retrieval of shape templates based on their EntityType. This class provides a convenient way to store and access predefined shape templates for different entity types in the game.
// |_______________________________________________________________________
class ShapeTemplateRegistry {
public:
	// Registers a shape template for a specific EntityType. This method takes an EntityType and a ShapeTemplate as parameters and adds the shape template to the registry, associating it with the specified EntityType. If a shape template for the given EntityType already exists, it will be overwritten with the new one.
	void RegisterShapeTemplate(EntityType type, float radius, Vec3 color) {
		m_shapeTemplates[type] = ShapeTemplate{ radius, color };
	}


	const ShapeTemplate& GetShapeTemplate(EntityType type) const {
		return m_shapeTemplates.at(type); // Retrieves the shape template associated with the specified EntityType. If the EntityType does not exist in the registry, this will throw an exception.
	}

private: std::unordered_map<EntityType, ShapeTemplate> m_shapeTemplates; // A map that associates EntityType with their corresponding ShapeTemplate for easy retrieval
};
/////////////////////////////////