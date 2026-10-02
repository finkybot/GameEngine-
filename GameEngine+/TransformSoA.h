/////////////////////////////////
// TransformSoA.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the TransformSoA class. This class is used to store and manage the position, velocity, and acceleration of entities in a structure-of-arrays format for efficient processing in the game engine.
#pragma once
#include <vector>
#include "Entity.h"
/////////////////////////////////



/////////////////////////////////
// |	TransformSoA struct - defines a structure-of-arrays (SoA) representation for storing the position, velocity, and acceleration of entities in a game engine. This format allows for efficient processing of entity transformations 
// |	by separating the data into contiguous arrays, which can improve cache locality and performance during updates.
// |_______________________________________________________________________
struct TransformSoA {
	std::vector<float> posX, posY;	// Arrays for storing the X and Y positions of entities
	std::vector<float> velX, velY;	// Arrays for storing the X and Y velocities of entities
	std::vector<float> rot;			
	std::vector<float> scale;

	std::vector<Entity*> owner;


	// Add - adds a new entity and its transform data to the TransformSoA structure. This method takes a pointer to an Entity and a CTransform object, extracts the position and velocity data, and appends it to the corresponding arrays. 
	// It returns the index of the newly added entity in the arrays.
	size_t Add(Entity* e, const CTransform& transform) { 
		size_t i = posX.size();

		posX.push_back(transform.position.x);
		posY.push_back(transform.position.y);
		velX.push_back(transform.velocity.x);
		velY.push_back(transform.velocity.y);
		
		//rot.push_back(transform.rotation); currently not used as CTransform does not have a rotation member or scale member (yet)
		//scale.push_back(transform.scale);
		owner.push_back(e);

		return i;
	}


	// Remove - removes an entity and its transform data from the TransformSoA structure at the specified index. This method performs an out-of-bounds check and, if the index is valid, it moves the last element in each array to the position 
	// of the removed element, effectively overwriting it.
	void Remove(size_t index) { 
		size_t last = posX.size() - 1;

		// 
		if (index != last) {
			posX[index] = posX[last];
			posY[index] = posY[last];
			velX[index] = velX[last];
			velY[index] = velY[last];

			// rot[index] = rot[last]; // currently not used as CTransform does not have a rotation member or scale member (yet)
			// scale[index] = scale[last];

			owner[index] = owner[last];

			// The moved entity MUST update its transformIndex
			if (owner[index])
				owner[index]->transformIndex = index;
		}
		posX.pop_back();
		posY.pop_back();
		velX.pop_back();
		velY.pop_back();

		// rot.pop_back(); // currently not used as CTransform does not have a rotation member or scale member (yet)
		// scale.pop_back();

		owner.pop_back();
	}
};
