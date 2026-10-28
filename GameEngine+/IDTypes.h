/////////////////////////////////
// IDTypes.h - Defines type aliases and constants for entity, scene, and bucket IDs used in the game engine. These types are used to uniquely identify entities, scenes, and buckets within the engine's systems.
/////////////////////////////////



/////////////////////////////////
#pragma once
#include <cstdint>
#include <limits>
/////////////////////////////////



/////////////////////////////////
using EntityID = uint64_t;
using SceneID = uint32_t;
using BucketID = uint32_t;

constexpr EntityID INVALID_ENTITY = std::numeric_limits<EntityID>::max();

constexpr SceneID INVALID_SCENE = std::numeric_limits<SceneID>::max();

constexpr BucketID INVALID_BUCKET = std::numeric_limits<BucketID>::max();
/////////////////////////////////