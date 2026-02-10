#pragma once

/// <summary>
/// EntityType enumeration for categorizing game entities
/// Replaces string-based tags with efficient enum values
/// </summary>
enum class EntityType
{
	TeamEagle = 0,
	TeamHawk = 1,
	TeamBoogaloo = 2,
	TeamRocket = 3,
	TeamMonkey = 4,
	Explosion = 5,
	Default = 6
};

/// <summary>
/// Convert EntityType enum to string representation (for debugging/logging)
/// </summary>
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
		case EntityType::Default: return "Default";
		default: return "Unknown";
	}
}
