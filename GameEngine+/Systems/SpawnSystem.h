/////////////////////////////////
// SpawnSystem.h - Header file for the SpawnSystem class, which manages the spawning of entities based on configurable patterns and triggers. The SpawnSystem allows for various spawn types (burst, continuous, periodic) and patterns (random, circular, spiral, etc.) that can be defined in a JSON configuration file. It also supports audio-reactive spawning based on music levels from the MusicSystem.
/////////////////////////////////



/////////////////////////////////
// Includes 
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>
/////////////////////////////////



/////////////////////////////////
// Forward declarations to avoid header cycles. We forward declare the EntityManager and MusicSystem classes, as well as the SFML RenderWindow class, which are used in the SpawnSystem but we don't need their full definitions in this header.
struct EntityManager;
struct MusicSystem;
namespace sf { 
	class RenderWindow; 
}
/////////////////////////////////



/////////////////////////////////
// Spawn namespace encapsulates all functionality related to spawning entities based on configurable patterns and triggers. It defines the SpawnSystem class, which manages the spawning logic, as well as the SpawnerConfig struct that holds the configuration for each spawner. 
// The SpawnSystem interacts with the EntityManager to create new entities and with the MusicSystem for audio-reactive spawning.
namespace Spawn {
/////////////////////////////////
// Enums for spawn types and patterns. The Type enum defines the different spawning modes (burst, continuous, periodic), while the Pattern enum defines various spawn patterns (random, circular, spiral, etc.) that can be used to determine how entities are spawned in the game world.
enum class Type { Burst, Continuous, Periodic };
enum class Pattern {
	Random,				 // Original random position/color
	Circular,			 // Spawn on circle, advance angle
	LevelScaledCircular, // Circular with size/brightness scaled by level
	Spiral,				 // Expanding/contracting spiral
	Firework,			 // Burst outward from center
	Figure8,			 // Lissajous figure-8 pattern
	Wave,				 // Sine wave pattern across screen
	MultiRing,			 // Multiple concentric rings
	Starburst,			 // Rays emanating from center
	Helix,				 // Double helix pattern
	Equalizer			 // Spawn columns based on audio spectrum
};
/////////////////////////////////



/////////////////////////////////
// SpawnerConfig struct holds the configuration for each spawner, including its ID, enabled state, spawn type, pattern, trigger threshold, spawn rate, burst count, probability, shape, size range, lifetime, spawn radius, and any additional settings needed for specific patterns.
struct SpawnerConfig {
	std::string id;
	bool enabled = true;
	Type type = Type::Burst;
	Pattern pattern = Pattern::Random;
	// trigger: currently only global level based
	float threshold = 0.02f;
	float rate = 1.0f; // spawns per second for continuous / cooldown for burst
	int burstCount = 4;
	float probability = 1.0f;
	std::string shape = "circle";
	float sizeMin = 6.0f;
	float sizeMax = 48.0f;
	float lifetime = 2.5f;
	float spawnRadius = 250.0f;
	// Circular/pattern settings
	float circularSpeed = 0.06f;  // radians per spawn
	float spiralExpansion = 2.0f; // how fast spiral expands
	int ringCount = 3;			  // for MultiRing pattern
};
/////////////////////////////////



/////////////////////////////////
// SpawnSystem class manages the spawning of entities based on the defined SpawnerConfig configurations. It updates spawn timers, checks trigger conditions, and spawns entities according to the specified patterns and types. 
// The SpawnSystem also provides methods for loading configurations from a JSON file, saving configurations, and managing the enabled state of the system.
class SpawnSystem {
	/////////////////////////////////
	// Public interface for the SpawnSystem class
public:
	/////////////////////////////////
	// Constructor and destructor for the SpawnSystem class. The constructor takes a pointer to the EntityManager and an optional pointer to the SFML RenderWindow, which can be used for certain spawn patterns that require screen dimensions. 
	// The destructor can be used to clean up any resources if needed.
	SpawnSystem(EntityManager* em, sf::RenderWindow* window);
	~SpawnSystem();
	/////////////////////////////////



	/////////////////////////////////
	// Update - Called every frame to update the spawn timers, check trigger conditions based on the provided level (e.g., music level for audio-reactive spawning), and spawn entities according to their configurations.
	void Update(float dt, float level = 0.0f);
	/////////////////////////////////



	/////////////////////////////////
	// LoadFromFile - Load a simple json preset (very small parser, tolerant)
	bool LoadFromFile(const std::filesystem::path& path, std::string& outError);
	/////////////////////////////////



	/////////////////////////////////
	// SaveToFile - Save the current spawner configurations to a JSON file. This allows for easy editing and sharing of spawn presets.
	bool SaveToFile(const std::filesystem::path& path, std::string& outError) const;
	/////////////////////////////////



	/////////////////////////////////
	// LoadDefault - Load a default preset with some example spawners. This can be used to quickly set up the system with some basic configurations for testing or as a starting point for creating custom presets.
	void LoadDefault();
	/////////////////////////////////



	/////////////////////////////////
	// SetMusicEntityId - Associate a music entity ID to use for audio-reactive patterns (e.g., Equalizer). This allows the SpawnSystem to query the MusicSystem for the current audio levels of the specified music entity and spawn entities based on those levels.
	void SetMusicEntityId(size_t entityId) { m_musicEntityId = entityId; }
	/////////////////////////////////



	/////////////////////////////////
	// Accessor methods for the spawner configurations and enabled state. GetConfigs returns a const reference to the vector of SpawnerConfig, while GetConfigsMutable returns a non-const reference for editing. SetEnabled and IsEnabled manage the global enabled state of the spawn system.
	const std::vector<SpawnerConfig>& GetConfigs() const { return m_configs; }
	std::vector<SpawnerConfig>& GetConfigsMutable() { return m_configs; } // no const please, we're mutable!
	/////////////////////////////////



	/////////////////////////////////
	// Global enable/disable for entire system
	void SetEnabled(bool enabled) { m_enabled = enabled; }
	bool IsEnabled() const { return m_enabled; }
	/////////////////////////////////



	/////////////////////////////////
	// Update a specific spawner's config by id (returns false if not found)
	bool UpdateConfig(const std::string& id, const SpawnerConfig& cfg);
	/////////////////////////////////
		 
		

	/////////////////////////////////
	// Add a new config at runtime
	void AddConfig(const SpawnerConfig& cfg);
	/////////////////////////////////
		 
		

	/////////////////////////////////
	// Clear all configs
	void ClearConfigs() {
		m_configs.clear();
		m_spawnTimers.clear();
		m_circularAngles.clear();
		m_spiralRadius.clear();
		m_spawnCounter.clear();
	}
	/////////////////////////////////



	/////////////////////////////////
	// Private helper method 
private:
	/////////////////////////////////
	// SpawnEntity - Helper method to spawn an entity based on a given SpawnerConfig and the current level (e.g., music level for audio-reactive spawning). This method will handle the logic for determining the spawn position, color, size, lifetime, 
	// and any pattern-specific calculations needed to spawn the entity according to the specified configuration.
	void SpawnEntity(const SpawnerConfig& cfg, float level);
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the SpawnSystem class. These include a pointer to the EntityManager for spawning entities, a vector of SpawnerConfig for managing multiple spawners, and various maps for tracking spawn timers, angles, and counters for different patterns.
	EntityManager* m_entityManager;
	std::vector<SpawnerConfig> m_configs;
	std::unordered_map<std::string, float> m_spawnTimers;
	std::unordered_map<std::string, float> m_circularAngles; // per-spawner angle for circular patterns
	std::unordered_map<std::string, float> m_spiralRadius;	 // per-spawner radius for spiral patterns
	std::unordered_map<std::string, int> m_spawnCounter;	 // per-spawner spawn count for patterns
	sf::RenderWindow* m_window = nullptr;
	/////////////////////////////////
		 
		
	/////////////////////////////////
	// Global enabled flag for the entire spawn system, allowing for quick toggling of all spawning activity without modifying individual spawner configurations. The m_globalTime variable can be used to track accumulated time for animated effects or patterns that require timing, 
	// while m_musicEntityId stores the ID of the music entity used for audio-reactive patterns.
	bool m_enabled = false;
	float m_globalTime = 0.0f; 
	size_t m_musicEntityId = 0;
	/////////////////////////////////
};
/////////////////////////////////
}
/////////////////////////////////
