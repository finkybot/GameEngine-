/////////////////////////////////
// SoundSystem.cpp - Implementation of the ECS-based sound effect management system
/////////////////////////////////



/////////////////////////////////
// Includes
#include "SoundSystem.h"
#include "CTransform.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <SFML/Audio.hpp>
/////////////////////////////////



/////////////////////////////////
// Destructor - Clean up buffer cache and sound pool
SoundSystem::~SoundSystem() {
	m_bufferCache.clear();
	m_soundEffectPool.clear();
}
/////////////////////////////////



/////////////////////////////////
// Initialize - Enumerate SFML 3.1 PlaybackDevice and set the default output device
void SoundSystem::Initialize() {
	std::cout << "[SoundSystem] Initializing audio system..." << std::endl;
	std::cout.flush();

	// SFML 3.1 requires explicit device selection via sf::PlaybackDevice
	auto devices = sf::PlaybackDevice::getAvailableDevices();
	std::cout << "[SoundSystem] Available playback devices (" << devices.size() << "):" << std::endl;
	
	for (const auto& d : devices) 
		std::cout << "  - " << d << std::endl;

	auto defaultDevice = sf::PlaybackDevice::getDefaultDevice();
	if (defaultDevice.has_value()) {
		std::cout << "[SoundSystem] Default device: " << *defaultDevice << std::endl;
		if (sf::PlaybackDevice::setDevice(*defaultDevice))
			std::cout << "[SoundSystem] Playback device set to: " << *defaultDevice << std::endl;
		else
			std::cerr << "[SoundSystem ERROR] Failed to set playback device: " << *defaultDevice << std::endl;
	} else {
		std::cerr << "[SoundSystem ERROR] No default playback device found!" << std::endl;
	}

	// Log the currently active device
	auto currentDevice = sf::PlaybackDevice::getDevice();
	if (currentDevice.has_value()) {
		std::cout << "[SoundSystem] Currently active device: " << *currentDevice << std::endl;
	} else {
		std::cout << "[SoundSystem] Currently active device: (none)" << std::endl;
	}

	std::cout.flush();
}
/////////////////////////////////



/////////////////////////////////
// InitializePool - Create a pool of pre-allocated sound entities for reuse
void SoundSystem::InitializePool(EntityManager& em, size_t poolSize) {
	m_poolSize = poolSize;
	m_entityManager = &em;

	std::cout << "[SoundSystem] Initializing sound effect pool with " << poolSize << " entities" << std::endl;

	// Pre-allocate sound entities from the pending entities
	// Note: We'll manually manage them rather than adding to EntityManager
	for (size_t i = 0; i < poolSize; ++i) {
		Entity* soundEntity = em.AddEntity(EntityType::Default);
		soundEntity->AddComponent<CSoundEffect>();
		m_soundEffectPool.push_back(soundEntity);
	}

	std::cout << "[SoundSystem] Sound pool initialized successfully" << std::endl;
}
/////////////////////////////////



/////////////////////////////////
// AcquirePooledSoundEntity - Get an available sound entity from the pool
Entity* SoundSystem::AcquirePooledSoundEntity() {
	if (m_soundEffectPool.empty()) {
		return nullptr; // Pool exhausted
	}

	Entity* entity = m_soundEffectPool.back();
	m_soundEffectPool.pop_back();

	// Reset component state for reuse
	if (auto* sound = entity->GetComponent<CSoundEffect>()) {
		sound->m_shouldPlay = false;
		sound->m_state = CSoundEffect::State::Stopped;
		sound->m_fadeOutDuration = 0.0f;
		sound->m_fadeOutElapsed = 0.0f;
	}

	return entity;
}
/////////////////////////////////



/////////////////////////////////
// ReturnSoundToPool - Return a sound entity to the pool for reuse
void SoundSystem::ReturnSoundToPool(Entity* entity) {
	if (!entity) return;

	// Stop the sound
	if (auto* sound = entity->GetComponent<CSoundEffect>()) {
		if (sound->m_sound) {
			sound->m_sound->stop();
		}
		sound->m_shouldPlay = false;
		sound->m_state = CSoundEffect::State::Stopped;
	}

	m_soundEffectPool.push_back(entity);
}
/////////////////////////////////



/////////////////////////////////
// Process - Iterate all entities with CSoundEffect components and manage playback
void SoundSystem::Process(EntityManager& em, float deltaTime) {
	EntityVector& entities = em.GetEntities();

	static float accumulatedTime = 0.0f;
	accumulatedTime += deltaTime;

	size_t soundEffectCount = 0;
	for (auto& entityPtr : entities) {
		Entity* entity = entityPtr.get();
		CSoundEffect* sound = entity->GetComponent<CSoundEffect>();
		if (!sound) continue;

		soundEffectCount++;

		// Handle m_shouldPlay flag (data-driven control)
		if (sound->m_shouldPlay && !sound->m_sound) {
			// Acquire or create backing sf::Sound
			if (sound->m_Path.empty()) {
				std::cerr << "[SoundSystem WARNING] Entity has m_shouldPlay=true but m_Path is empty" << std::endl;
				continue; // No audio file specified
			}

			sf::SoundBuffer* buffer = GetOrLoadBuffer(sound->m_Path);
			if (!buffer) {
				std::cerr << "[SoundSystem ERROR] Failed to load buffer for: " << sound->m_Path << std::endl;
				sound->m_shouldPlay = false;  // Disable further attempts
				continue; // Failed to load audio
			}

			sound->m_sound = std::make_unique<sf::Sound>(*buffer);
			// SFML 3 volume range is 1-100 (not 0-100)
			// Internal m_volume should be in range 1-100 for proper SFML playback
			float finalVolume = std::clamp(sound->m_volume, 1.0f, 100.0f);
			sound->m_sound->setVolume(finalVolume);
			// Enable/disable 3D spatialization based on component flag
			sound->m_sound->setSpatializationEnabled(sound->m_is3D);
		}

		// Start playback if flagged
		if (sound->m_shouldPlay && sound->m_sound && sound->m_state != CSoundEffect::State::Playing) {
			// Check if this is an explosion sound and prevent overlapping playback
			bool canPlay = true;
			// Disabled for now - entity keeps explosion alive until sound finishes
			//if (sound->m_Path.find("explosion") != std::string::npos) {
			//	const sf::SoundBuffer& buffer = sound->m_sound->getBuffer();
			//	if (!CanPlayExplosion(accumulatedTime, buffer.getDuration().asSeconds())) {
			//		canPlay = false;  // Don't play yet, wait for previous explosion to finish
			//	} else {
			//		m_lastExplosionTime = accumulatedTime;  // Update the last explosion time
			//	}
			//}

			if (canPlay) {
				sound->m_sound->play();
				sound->m_state = CSoundEffect::State::Playing;
			}
		}

		// Update sound status based on actual playback
		if (sound->m_sound && sound->m_state == CSoundEffect::State::Playing) {
			auto actualStatus = sound->m_sound->getStatus();
			if (actualStatus == sf::Sound::Status::Stopped) {
				sound->m_state = CSoundEffect::State::Stopped;
				sound->m_shouldPlay = false; // Mark as done
			}
		}

		// Handle pause/resume
		if (sound->m_state == CSoundEffect::State::Paused && sound->m_sound) {
			// SFML status check - verify it's actually paused
			if (sound->m_sound->getStatus() != sf::Sound::Status::Paused) {
				sound->m_sound->pause();
			}
		}

		// Handle explicit stop (only if m_shouldPlay was set to false AFTER creation)
		if (!sound->m_shouldPlay && sound->m_sound && sound->m_state != CSoundEffect::State::Stopped && sound->m_state != CSoundEffect::State::Playing) {
			sound->m_sound->stop();
			sound->m_state = CSoundEffect::State::Stopped;
		}

		// Update spatial audio if entity has a transform and 3D is enabled
		if (sound->m_is3D) {
			if (auto* transform = entity->GetComponent<CTransform>()) {
				if (sound->m_sound) {
					ApplySpatialAudio(*sound->m_sound, *sound, transform->m_position);
				}
			}
		}
	}

	if (soundEffectCount == 0 && entities.size() > 0) {
		// Don't spam this warning - most entities won't have sound effects
		// std::cout << "[SoundSystem] WARNING: Found " << entities.size() << " entities but 0 CSoundEffect components" << std::endl;
	}

	// Culling is now handled at creation time via CanPlayNewSound check in CollisionSystem
	// So we don't need to cull here anymore - this prevents cutting off sounds that were created
	/*
	// Check if we've exceeded max concurrent sounds and cull by priority
	size_t activeSounds = CountActiveSounds(em);
	if (activeSounds > m_maxConcurrentSounds) {
		CullByPriority(em);
	}
	*/
}
/////////////////////////////////



/////////////////////////////////
// Update - Update sound states each frame (fade, distance calculations, etc.)
void SoundSystem::Update(float deltaTime) {
	if (!m_entityManager) return;

	EntityVector& entities = m_entityManager->GetEntities();

	for (auto& entityPtr : entities) {
		Entity* entity = entityPtr.get();
		CSoundEffect* sound = entity->GetComponent<CSoundEffect>();
		if (!sound || !sound->m_sound) continue;

		// Update fade-out
		if (sound->m_fadeOutDuration > 0.0f && sound->m_state == CSoundEffect::State::Playing) {
			sound->m_fadeOutElapsed += deltaTime;

			float progress = sound->m_fadeOutElapsed / sound->m_fadeOutDuration;
			if (progress >= 1.0f) {
				sound->m_sound->stop();
				sound->m_state = CSoundEffect::State::Stopped;
				sound->m_shouldPlay = false;
				sound->m_fadeOutDuration = 0.0f;
			} else {
				// Linearly interpolate volume during fade-out (clamp to 1-100 for SFML 3)
				float fadedVolume = std::max(1.0f, sound->m_volume * (1.0f - progress));
				sound->m_sound->setVolume(fadedVolume);
			}
		}

		// Update spatial audio distance
		if (auto* transform = entity->GetComponent<CTransform>()) {
			Vec2 delta = transform->m_position - m_listenerPosition;
			sound->m_currentDistance = delta.Mag();
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// SetMasterVolume - Set the global master volume (0-100 scale)
void SoundSystem::SetMasterVolume(float volume) {
	volume = std::clamp(volume, 0.0f, 100.0f);
	m_masterVolume = volume;
	std::cout << "[SoundSystem] Master volume set to: " << m_masterVolume << " (0-100)" << std::endl;

	// Update all active sounds
	if (m_entityManager) {
		EntityVector& entities = m_entityManager->GetEntities();

		for (auto& entityPtr : entities) {
			Entity* entity = entityPtr.get();
			CSoundEffect* sound = entity->GetComponent<CSoundEffect>();
			if (sound && sound->m_sound) {
				float finalVolume = std::clamp(sound->m_volume, 1.0f, 100.0f);
				sound->m_sound->setVolume(finalVolume);
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// SetMaxConcurrentSounds - Set the maximum number of concurrent sounds
void SoundSystem::SetMaxConcurrentSounds(size_t count) {
	m_maxConcurrentSounds = count;
}
/////////////////////////////////



/////////////////////////////////
// SetListenerPosition - Set the listener position for 3D audio
void SoundSystem::SetListenerPosition(const Vec2& pos) {
	m_listenerPosition = pos;
	// Only update SFML listener if spatial audio is enabled
	if (m_spatialAudioEnabled) {
		sf::Listener::setPosition(sf::Vector3f(pos.x, pos.y, 0.0f));
	}
}
/////////////////////////////////



/////////////////////////////////
// StopSoundEffect - Stop a sound with optional fade-out
void SoundSystem::StopSoundEffect(Entity* entity, float fadeOutTime) {
	if (!entity) return;

	CSoundEffect* sound = entity->GetComponent<CSoundEffect>();
	if (!sound) return;

	if (fadeOutTime > 0.0f) {
		sound->m_fadeOutDuration = fadeOutTime;
		sound->m_fadeOutElapsed = 0.0f;
	} else {
		if (sound->m_sound) {
			sound->m_sound->stop();
		}
		sound->m_state = CSoundEffect::State::Stopped;
		sound->m_shouldPlay = false;
	}
}
/////////////////////////////////



/////////////////////////////////
// PauseSoundEffect - Pause a sound effect
void SoundSystem::PauseSoundEffect(Entity* entity) {
	if (!entity) return;

	CSoundEffect* sound = entity->GetComponent<CSoundEffect>();
	if (sound && sound->m_sound) {
		sound->m_sound->pause();
		sound->m_state = CSoundEffect::State::Paused;
	}
}
/////////////////////////////////



/////////////////////////////////
// ResumeSoundEffect - Resume a paused sound effect
void SoundSystem::ResumeSoundEffect(Entity* entity) {
	if (!entity) return;

	CSoundEffect* sound = entity->GetComponent<CSoundEffect>();
	if (sound && sound->m_sound && sound->m_state == CSoundEffect::State::Paused) {
		sound->m_sound->play();
		sound->m_state = CSoundEffect::State::Playing;
	}
}
/////////////////////////////////



/////////////////////////////////
// GetActiveSoundCount - Get the number of currently active sounds
size_t SoundSystem::GetActiveSoundCount(EntityManager& em) const {
	return CountActiveSounds(em);
}
/////////////////////////////////



/////////////////////////////////
// GetOrLoadBuffer - Load or retrieve a cached sound buffer
sf::SoundBuffer* SoundSystem::GetOrLoadBuffer(const std::string& path) {
	auto it = m_bufferCache.find(path);
	if (it != m_bufferCache.end()) {
		return it->second.get();
	}

	std::cout << "[SoundSystem] Loading sound buffer from: " << path << std::endl;

	// Try the path as-is first
	std::ifstream file(path);
	std::string actualPath = path;

	if (!file.good()) {
		// If relative path fails, try prepending common asset root directories
		std::vector<std::string> searchPaths = {
			"GameEngine+/" + path,
			"GameEngine+\\" + path,
			"../" + path,
			"..\\" + path,
		};

		for (const auto& tryPath : searchPaths) {
			std::ifstream tryFile(tryPath);
			if (tryFile.good()) {
				actualPath = tryPath;
				std::cout << "[SoundSystem] Found file at: " << actualPath << std::endl;
				tryFile.close();
				break;
			}
		}
	} else {
		file.close();
	}

	auto newBuffer = std::make_unique<sf::SoundBuffer>();
	if (!newBuffer->loadFromFile(actualPath)) {
		std::cerr << "[SoundSystem ERROR] Failed to load audio file: " << actualPath << std::endl;
		std::cerr << "  Original path was: " << path << std::endl;
		std::cerr << "  Make sure the file exists and is in a supported format (WAV, OGG, FLAC, etc.)" << std::endl;
		return nullptr; // Failed to load
	}

	std::cout << "[SoundSystem] Successfully loaded: " << actualPath << std::endl;
	std::cout << "  Duration: " << newBuffer->getDuration().asSeconds() << "s" << std::endl;
	std::cout << "  Channels: " << newBuffer->getChannelCount() << std::endl;
	std::cout << "  Sample rate: " << newBuffer->getSampleRate() << " Hz" << std::endl;
	std::cout << "  Sample count: " << newBuffer->getSampleCount() << std::endl;
	sf::SoundBuffer* bufferPtr = newBuffer.get();
	m_bufferCache[path] = std::move(newBuffer);
	return bufferPtr;
}
/////////////////////////////////



/////////////////////////////////
// ApplySpatialAudio - Apply spatial audio (distance attenuation, panning) to a sound
void SoundSystem::ApplySpatialAudio(sf::Sound& sound, const CSoundEffect& soundCmp, const Vec2& entityPos) {
	// Only apply spatial audio if enabled
	if (!m_spatialAudioEnabled) {
		// If spatial audio is disabled, just set full volume and no panning
		float volume = soundCmp.m_volume;
		volume = std::clamp(volume, 1.0f, 100.0f);
		sound.setVolume(volume);
		sound.setPan(0.0f);  // Center pan
		return;
	}

	// Let SFML handle 3D audio positioning and spatialization
	// The sound position is set relative to the listener (which is at 0,0,0 by default)
	// so we set the sound position and SFML will apply attenuation and panning automatically

	// Convert 2D position to 3D (z=0)
	// Relative to listener: if listener is at m_listenerPosition, sound should be offset from that
	sf::Vector3f soundPos(entityPos.x - m_listenerPosition.x, entityPos.y - m_listenerPosition.y, 0.0f);
	sound.setPosition(soundPos);

	// Set the volume to the component's base volume
	// SFML's spatialization will handle attenuation based on distance
	float volume = soundCmp.m_volume;
	volume = std::clamp(volume, 1.0f, 100.0f);
	sound.setVolume(volume);

	// SFML 3D audio with setSpatializationEnabled(true) will automatically:
	// - Apply distance attenuation based on the sound position
	// - Apply panning based on the horizontal offset
	// - Use the min/max distance settings from the sound buffer if set
	// We just need to set the position and let SFML handle the rest
}
/////////////////////////////////



/////////////////////////////////
// CullByPriority - Stop low-priority sounds when max concurrent sounds exceeded
void SoundSystem::CullByPriority(EntityManager& em) {
	// Collect all active sounds with their priorities
	std::vector<std::pair<Entity*, SoundPriority>> activeSounds;

	EntityVector& entities = em.GetEntities();

	for (auto& entityPtr : entities) {
		Entity* entity = entityPtr.get();
		CSoundEffect* sound = entity->GetComponent<CSoundEffect>();
		if (sound && sound->m_state == CSoundEffect::State::Playing && sound->m_sound) {
			activeSounds.push_back({ entity, sound->m_priority });
		}
	}

	// Sort by priority (lower priority first for removal)
	std::sort(activeSounds.begin(), activeSounds.end(),
		[](const auto& a, const auto& b) {
			return static_cast<int>(a.second) < static_cast<int>(b.second);
		});

	// Stop lowest-priority sounds until we're under the limit
	size_t toRemove = activeSounds.size() - m_maxConcurrentSounds;
	for (size_t i = 0; i < toRemove && i < activeSounds.size(); ++i) {
		StopSoundEffect(activeSounds[i].first, 0.1f); // Quick fade-out
	}
}
/////////////////////////////////



/////////////////////////////////
// CountActiveSounds - Count the number of currently playing/active sounds
size_t SoundSystem::CountActiveSounds(EntityManager& em) const {
	size_t count = 0;

	const EntityVector& entities = em.GetEntities();

	for (const auto& entityPtr : entities) {
		Entity* entity = entityPtr.get();
		CSoundEffect* sound = entity->GetComponent<CSoundEffect>();
		if (sound && sound->m_state == CSoundEffect::State::Playing && sound->m_sound) {
			if (sound->m_sound->getStatus() == sf::Sound::Status::Playing) {
				++count;
			}
		}
	}

	return count;
}
/////////////////////////////////



/////////////////////////////////
// CanPlayNewSound - Check if we're below the max concurrent sound limit
bool SoundSystem::CanPlayNewSound(EntityManager& em) const {
	size_t activeCount = CountActiveSounds(em);
	return activeCount < m_maxConcurrentSounds;
}
/////////////////////////////////

/////////////////////////////////
// CanPlayExplosion - Check if enough time has passed since the last explosion sound finished
bool SoundSystem::CanPlayExplosion(float currentTime, float explosionDuration) {
	// If no explosion has played yet, allow it
	if (m_lastExplosionTime < 0.0f) {
		return true;
	}

	// Calculate when the last explosion will finish
	float lastExplosionEndTime = m_lastExplosionTime + explosionDuration;

	// Allow playback only if current time >= the end time of the last explosion
	return currentTime >= lastExplosionEndTime;
}
/////////////////////////////////
