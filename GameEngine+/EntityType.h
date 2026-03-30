// ***** EntityType.h - EntityType enum class definition *****
#pragma once

// EntityType.h (Defines the EntityType enum class representing different types of entities in the game, such as teams and explosions.)
enum class EntityType
{
	TeamEagle = 0,
	TeamHawk = 1,
	TeamBoogaloo = 2,
	TeamRocket = 3,
	TeamMonkey = 4,
	Explosion = 5,
 TileMap = 6,
	Tile = 7,

	Default = 8		
};

// Utility function to convert EntityType enum values to human-readable strings for debugging and logging purposes.
inline const char* EntityTypeToString(EntityType type)
{
	switch (type)
	{
		case EntityType::TeamEagle: return "TeamEagle";
		case EntityType::TeamHawk: return "TeamHawk";
		case EntityType::TeamBoogaloo: return "TeamBoogaloo";
		case EntityType::TeamRocket: return "TeamRocket";
		case EntityType::TeamMonkey: return "TeamMonkey";
		case EntityType::Explosion: return "Explosion";
		case EntityType::Tile: return "Tile";
		case EntityType::Default: return "Default";
		default: return "Unknown";
	}
}
