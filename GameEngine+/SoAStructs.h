/////////////////////////////////
// SoAStructs.h
/////////////////////////////////



/////////////////////////////////
// Includes and forward declarations for the TransformSoA class. This class is used to store and manage the position, velocity, and acceleration of entities in a structure-of-arrays format for efficient processing in the game engine.
#pragma once
#include <vector>
#include <cstdint>
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

	// dirty flags (0 = clean, 1 = dirty)
	std::vector<uint8_t> dirty;

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

		dirty.push_back(1); // mark as dirty for the first frame after creation)

		return i;
	}


	// Remove - removes an entity and its transform data from the TransformSoA structure at the specified index. This method performs an out-of-bounds check and, if the index is valid, it moves the last element in each array to the position 
	// of the removed element, effectively overwriting it.
	void Remove(size_t index) {
		size_t last = posX.size() - 1;

		if (index != last) {
			posX[index] = posX[last];
			posY[index] = posY[last];
			velX[index] = velX[last];
			velY[index] = velY[last];
			owner[index] = owner[last];

			dirty[index] = dirty[last]; // NEW: keep dirty flags in sync

			if (owner[index])
				owner[index]->transformIndex = index;
		}

		posX.pop_back();
		posY.pop_back();
		velX.pop_back();
		velY.pop_back();
		owner.pop_back();
		dirty.pop_back(); // NEW: pop dirty flag
	}


	inline void MarkDirty(size_t idx) { dirty[idx] = 1; }

	inline bool IsDirty(size_t idx) const { return dirty[idx] != 0; }

	inline void ClearDirty(size_t idx) { dirty[idx] = 0; }

	inline void SetPosition(size_t idx, float x, float y) {
		posX[idx] = x;
		posY[idx] = y;
		dirty[idx] = 1; // NEW: mark dirty
	}
};
/////////////////////////////////



/////////////////////////////////
//	|	TileRenderSoA struct - defines a structure-of-arrays (SoA) representation for storing the rendering data of tiles in a game engine. This format allows for efficient processing of tile rendering by separating the data into contiguous arrays, which can improve cache locality and performance during rendering.
//	|_______________________________________________________________________
struct TileRenderSoA {
	std::vector<float> posX;  // world X
	std::vector<float> posY;  // world Y
	std::vector<float> sizeX; // width
	std::vector<float> sizeY; // height;

	std::vector<float> u0;
	std::vector<float> v0;
	std::vector<float> u1;
	std::vector<float> v1;

	std::vector<float> r;
	std::vector<float> g;
	std::vector<float> b;
	std::vector<float> a;

	std::vector<std::uint32_t> handleLo;
	std::vector<std::uint32_t> handleHi;

	void Clear() {
		posX.clear();
		posY.clear();
		sizeX.clear();
		sizeY.clear();
		u0.clear();
		v0.clear();
		u1.clear();
		v1.clear();
		r.clear();
		g.clear();
		b.clear();
		a.clear();
		handleLo.clear();
		handleHi.clear();
	}

	std::size_t Size() const { return posX.size(); }
};
/////////////////////////////////