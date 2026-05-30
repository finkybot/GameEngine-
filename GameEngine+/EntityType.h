/////////////////////////////////
// EntityType.h
/////////////////////////////////



/////////////////////////////////
// Includes???????
#pragma once
/////////////////////////////////



/////////////////////////////////
// EntityType enum defines the various types of entities that can exist in the game. This includes different teams, explosions, tile maps, and 
// a default type for generic entities. The enum values are explicitly assigned for clarity and potential use in serialization or debugging.
enum class EntityType {
	TeamEagle = 0,
	TeamHawk = 1,
	TeamBoogaloo = 2,
	TeamRocket = 3,
	TeamMonkey = 4,
	Explosion = 5,
	TileMap = 6,
	Tile = 7,

	Default = 8,
	Equalizer = 9
};
/////////////////////////////////



/////////////////////////////////
// Utility function to convert EntityType enum values to human-readable strings for debugging and logging purposes.
inline const char* EntityTypeToString(EntityType type) {
	switch (type) {
	case EntityType::TeamEagle:
		return "TeamEagle";
	case EntityType::TeamHawk:
		return "TeamHawk";
	case EntityType::TeamBoogaloo:
		return "TeamBoogaloo";
	case EntityType::TeamRocket:
		return "TeamRocket";
	case EntityType::TeamMonkey:
		return "TeamMonkey";
	case EntityType::Explosion:
		return "Explosion";
	case EntityType::Tile:
		return "Tile";
   case EntityType::Equalizer:
		return "Equalizer";
	case EntityType::Default:
		return "Default";
	default:
		return "Unknown";
	}
}
/////////////////////////////////
