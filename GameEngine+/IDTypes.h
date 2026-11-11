/////////////////////////////////
// IDTypes.h
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once

#include <cstdint>
#include <limits>
/////////////////////////////////



/////////////////////////////////
//	|	EntityID
//	|_______________________________________________________________________
struct EntityID {
	uint32_t index = 0;
	uint32_t generation = 0;

	constexpr bool operator==(const EntityID& other) const {
		return index == other.index && generation == other.generation;
	}

	constexpr bool operator!=(const EntityID& other) const { return !(*this == other); }

	constexpr bool IsValid() const { return index != 0 || generation != 0; }
};
/////////////////////////////////



/////////////////////////////////
// Other ID types
using SceneID = uint32_t;
using BucketID = uint32_t;
/////////////////////////////////



/////////////////////////////////
// Invalid constants
constexpr EntityID INVALID_ENTITY{};

constexpr SceneID INVALID_SCENE = std::numeric_limits<SceneID>::max();

constexpr BucketID INVALID_BUCKET = std::numeric_limits<BucketID>::max();
/////////////////////////////////