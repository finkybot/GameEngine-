/////////////////////////////////
// SoundSystem.h - ECS-based sound effect management system with pooling
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "CSoundEffect.h"
#include "Entity.h"
#include "EntityManager.h"
#include "SFML/Audio.hpp"
#include <cmath>
/////////////////////////////////



/////////////////////////////////
// SoundSystem - Manages playback of all sound effects in the game.
// Features:
//   - Entity pooling for frequently-triggered sounds (footsteps, impacts, etc.)
//   - Buffer caching for efficient audio loading
//   - Spatial audio (3D positioning, distance attenuation, panning)
//   - Priority-based culling when max concurrent sounds exceeded
//   - Fade-in/fade-out support
//   - Data-driven (flag-based) design using m_shouldPlay
//								|
//								|_______________________________________________________________________
class SoundSystem {
	/////////////////////////////////
	// Private member variables
private:
	/////////////////////////////////
	// BUFFER CACHING (Path-based caching of sf::SoundBuffer)
	std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> m_bufferCache;
	/////////////////////////////////



	/////////////////////////////////
	// Sound effect pooling (Entity-based pooling)
	std::vector<Entity*> m_soundEffectPool;      // Pool of pre-allocated sound entities
	size_t m_poolSize = 64;                      // Default pool size
	EntityManager* m_entityManager = nullptr;    // Reference to EntityManager for pool creation
	/////////////////////////////////



	/////////////////////////////////
	// Audio configuration
	float m_masterVolume = 80.0f;  // Master volume (0-100 scale for SFML)
	size_t m_maxConcurrentSounds = 32;
	Vec2 m_listenerPosition = Vec2::Zero;
	float m_lastExplosionTime = -10.0f;  // Track when the last explosion sound started playing
	bool m_spatialAudioEnabled = true;   // Flag to disable spatial audio updates (e.g., in music visualizer)
	/////////////////////////////////



	/////////////////////////////////
	// Public methods
public:
	/////////////////////////////////
	// Constructor and destructor
	SoundSystem() = default;
	~SoundSystem();
	void Initialize();  // Initialize audio device and diagnostics
	/////////////////////////////////



	/////////////////////////////////
	// Pool management
	void InitializePool(EntityManager& em, size_t poolSize = 64);
	Entity* AcquirePooledSoundEntity();
	void ReturnSoundToPool(Entity* entity);
	size_t GetPoolAvailableCount() const { return m_soundEffectPool.size(); }
	size_t GetPoolTotalSize() const { return m_poolSize; }
	/////////////////////////////////



	/////////////////////////////////
	// Main processing and update methods
	void Process(EntityManager& em, float deltaTime);  // Process all CSoundEffect components
	void Update(float deltaTime);                       // Update sound states (fade, distance, etc.)
	/////////////////////////////////
	


	/////////////////////////////////
	// Configuration methods
	void SetMasterVolume(float volume);
	void SetMaxConcurrentSounds(size_t count);
	void SetListenerPosition(const Vec2& pos);
	Vec2 GetListenerPosition() const { return m_listenerPosition; }
	void SetSpatialAudioEnabled(bool enabled) { m_spatialAudioEnabled = enabled; }
	bool IsSpatialAudioEnabled() const { return m_spatialAudioEnabled; }
	/////////////////////////////////



	/////////////////////////////////
	// Control methods for sound effects
	void StopSoundEffect(Entity* entity, float fadeOutTime = 0.0f);
	void PauseSoundEffect(Entity* entity);
	void ResumeSoundEffect(Entity* entity);
	/////////////////////////////////



	/////////////////////////////////
	// Accessor methods
	float GetMasterVolume() const { return m_masterVolume; }
	size_t GetActiveSoundCount(EntityManager& em) const;
	bool CanPlayNewSound(EntityManager& em) const;  // Check if we're below max concurrent sounds
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods
private:
	/////////////////////////////////
	// CanPlayExplosion - Check if enough time has passed since the last explosion sound finished
	bool CanPlayExplosion(float currentTime, float explosionDuration);
	/////////////////////////////////

	/////////////////////////////////
	// GetOrLoadBuffer - Load or retrieve a cached sound buffer
	sf::SoundBuffer* GetOrLoadBuffer(const std::string& path);
	/////////////////////////////////



	/////////////////////////////////
	// ApplySpatialAudio - Apply spatial audio (distance attenuation, panning) to a sound
	void ApplySpatialAudio(sf::Sound& sound, const CSoundEffect& soundCmp, const Vec2& entityPos);
	/////////////////////////////////



	/////////////////////////////////
	// CullByPriority - Stop low-priority sounds when max concurrent sounds exceeded
	void CullByPriority(EntityManager& em);
	/////////////////////////////////



	/////////////////////////////////
	// CountActiveSounds - Count the number of currently active (playing) sounds
	size_t CountActiveSounds(EntityManager& em) const;
	/////////////////////////////////
};
/////////////////////////////////
