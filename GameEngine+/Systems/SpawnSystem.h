#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>

struct EntityManager;
struct MusicSystem;
namespace sf {
class RenderWindow;
}

namespace Spawn {
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

class SpawnSystem {
public:
	SpawnSystem(EntityManager* em, sf::RenderWindow* window);
	~SpawnSystem();

	void Update(float dt, float level = 0.0f);

	// load a simple json preset (very small parser, tolerant)
	bool LoadFromFile(const std::filesystem::path& path, std::string& outError);
	bool SaveToFile(const std::filesystem::path& path, std::string& outError) const;
	void LoadDefault();

	// Associate a music entity id to use for audio-reactive patterns (Equalizer)
	void SetMusicEntityId(size_t entityId) { m_musicEntityId = entityId; }

	const std::vector<SpawnerConfig>& GetConfigs() const { return m_configs; }
	std::vector<SpawnerConfig>& GetConfigsMutable() { return m_configs; }

	// Global enable/disable for entire system
	void SetEnabled(bool enabled) { m_enabled = enabled; }
	bool IsEnabled() const { return m_enabled; }

	// Update a specific spawner's config by id (returns false if not found)
	bool UpdateConfig(const std::string& id, const SpawnerConfig& cfg);

	// Add a new config at runtime
	void AddConfig(const SpawnerConfig& cfg);

	// Clear all configs
	void ClearConfigs() {
		m_configs.clear();
		m_spawnTimers.clear();
		m_circularAngles.clear();
		m_spiralRadius.clear();
		m_spawnCounter.clear();
	}

private:
	void SpawnEntity(const SpawnerConfig& cfg, float level);

	EntityManager* m_entityManager;
	std::vector<SpawnerConfig> m_configs;
	std::unordered_map<std::string, float> m_spawnTimers;
	std::unordered_map<std::string, float> m_circularAngles; // per-spawner angle for circular patterns
	std::unordered_map<std::string, float> m_spiralRadius;	 // per-spawner radius for spiral patterns
	std::unordered_map<std::string, int> m_spawnCounter;	 // per-spawner spawn count for patterns
	sf::RenderWindow* m_window = nullptr;
    // default disabled to avoid surprising audio-reactive visuals on startup
	bool m_enabled = false;
	float m_globalTime = 0.0f; // accumulated time for animated effects
	size_t m_musicEntityId = 0; // entity id used for audio-reactive patterns
};
} // namespace Spawn
