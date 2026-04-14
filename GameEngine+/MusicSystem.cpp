// MusicSystem.cpp : This file contains the implementation of the MusicSystem class, which manages the music playback in the game engine.
#include "Entity.h"        // Move this to be first
#include "MusicSystem.h"
#include "EntityManager.h"
#include "CMusic.h"

#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <cstring>
#include <unordered_set>

//#ifdef _MSC_VER
//#pragma comment(lib, "sfml-audio.lib")
//#endif

MusicSystem::MusicSystem(EntityManager& entityManager) : m_entityManager(entityManager) {}

void MusicSystem::StopAllMusic() {
	for (auto &p : m_activeMusic) {
		if (p.second) p.second->stop();
	}

	m_activeMusic.clear();
	std::lock_guard<std::mutex> lk(m_levelsMutex);
	m_buffers.clear();
	m_levels.clear();
}

float MusicSystem::GetPlayingOffset(size_t entityId) const {
	auto it = m_activeMusic.find(entityId);
	if (it == m_activeMusic.end() || !it->second) return 0.0f;
	try {
		return it->second->getPlayingOffset().asSeconds();
	} catch (...) { return 0.0f; }
}

float MusicSystem::GetDuration(size_t entityId) const {
	auto it = m_activeMusic.find(entityId);
	if (it == m_activeMusic.end() || !it->second) return 0.0f;
	try {
		return it->second->getDuration().asSeconds();
	} catch (...) { return 0.0f; }
}

void MusicSystem::Seek(size_t entityId, float seconds) {
	auto it = m_activeMusic.find(entityId);
	if (it == m_activeMusic.end() || !it->second) return;
	try {
		it->second->setPlayingOffset(sf::seconds(seconds));
	} catch (...) {}
}

// Other methods remain unchanged
MusicSystem::~MusicSystem() {
	// Stop all music activity and clear the active music map
	for (auto& pair : m_activeMusic) {
		if (pair.second) {
			pair.second->stop(); // Stop the music if it's currently playing
		}
	}
	m_activeMusic.clear();

	// clear measured levels
	{
		std::lock_guard<std::mutex> lk(m_levelsMutex);
		m_levels.clear();
	}
}

float MusicSystem::GetLevel(size_t entityId) const {
	std::lock_guard<std::mutex> lk(m_levelsMutex);
	auto it = m_levels.find(entityId);
	if (it == m_levels.end()) return 0.0f;
	return it->second;
}

bool MusicSystem::HasAnalysisBuffer(size_t entityId) const {
	std::lock_guard<std::mutex> lk(m_levelsMutex);
	return m_buffers.find(entityId) != m_buffers.end();
}

void MusicSystem::Update(float deltaSeconds) {}


// Process method is called every frame to update the music playback and perform offline analysis for each entity with a CMusic component. It checks for any entities 
// that no longer have a CMusic component and stops their music and removes them from the active music map. For entities with a CMusic component, it updates the 
// corresponding sf::Music instance's properties based on the component's state (play/pause/stop) and volume. It also performs offline analysis by computing a 
// short-window mean-square level around the current play position of the music if we have a loaded SoundBuffer for that entity, and stores the computed level 
// in the m_levels map for retrieval by GetLevel.
void MusicSystem::Process() {
    // Clean up any active music instances for entities that no longer have a CMusic component
	std::unordered_set<size_t> liveIds;
	for (const auto& entity : m_entityManager.getEntities()) {
		if (entity->HasComponent<CMusic>()) liveIds.insert(entity->GetId());
	}
		
	// Erase any active music entries that are no longer live
	for (auto it = m_activeMusic.begin(); it != m_activeMusic.end(); ) {
		if (liveIds.find(it->first) == liveIds.end()) {
			// stop and erase
			if (it->second) it->second->stop(); { // remove analysis buffer and level entry
				std::lock_guard<std::mutex> lk(m_levelsMutex);
				m_buffers.erase(it->first);
				m_levels.erase(it->first);
			}
			it = m_activeMusic.erase(it);
		} else {
			++it;
		}
	}
	
	// Get all entities with a CMusic component
	for (const auto& entity : m_entityManager.getEntities()) {
		// Check if the entity has a CMusic component
		if (entity->HasComponent<CMusic>()) {
			// Get the CMusic component and update the corresponding sf::Music instance
			CMusic* musicComp = entity->GetComponent<CMusic>();
			if (musicComp) {
				// Get or create the sf::Music instance for this entity and update its properties based on the CMusic component
				sf::Music* music = GetOrCreateMusic(*entity);
				if (music) {
					music->setVolume(musicComp->volume);
                    // Set looping on the underlying sf::Music
					music->setLooping(musicComp->loop);

					// Check if music has naturally finished (stopped by reaching end, not by user)
					// This happens when: sf::Music is Stopped, but CMusic state is still Playing, and loop is disabled
					if (music->getStatus() == sf::SoundSource::Status::Stopped && 
						musicComp->state == CMusic::State::Playing && 
						!musicComp->loop) {
						// Music finished naturally - update component state to Stopped
						// Don't restart it - user must press Play again
						musicComp->state = CMusic::State::Stopped;
					}

					// Handle play/pause/stop based on the state of the CMusic component
					switch (musicComp->state) {
                        case CMusic::State::Playing:
							// If the underlying sf::Music is stopped (e.g. reached end), rewind to start before playing.
							// If it's paused, music->play() will resume at current offset.
							if (music->getStatus() == sf::SoundSource::Status::Stopped) {
								try { music->setPlayingOffset(sf::seconds(0)); } catch(...) {}
							}
							if (music->getStatus() != sf::SoundSource::Status::Playing) {
								music->play();
							}
							break;

						case CMusic::State::Paused:
							if (music->getStatus() == sf::SoundSource::Status::Playing) { music->pause(); }
							break;

						case CMusic::State::Stopped:
							if (music->getStatus() != sf::SoundSource::Status::Stopped) { music->stop(); }
							break;
					}

                    // Offline analysis: if we loaded a SoundBuffer for this entity, compute a short-window mean-square around current play position
					size_t id = entity->GetId();
				
					std::shared_ptr<sf::SoundBuffer> buf; {
						std::lock_guard<std::mutex> lk(m_levelsMutex);
						auto itb = m_buffers.find(id);
						if (itb != m_buffers.end()) buf = itb->second;
					}
					
					if (buf) {
						// determine sample index from music playing offset
						float seconds = 0.0f;
						try {
							seconds = music->getPlayingOffset().asSeconds();
						} catch (...) { seconds = 0.0f; }
						
						// get sample data, channels, sample rate, total samples from the buffer for analysis
						unsigned int sampleRate = buf->getSampleRate();
						unsigned int channels = buf->getChannelCount();
                        const short* samples = buf->getSamples();
						size_t totalSamples = static_cast<size_t>(buf->getSampleCount());

						// compute center sample index (interleaved samples)
						size_t center = static_cast<size_t>(seconds * static_cast<float>(sampleRate)) * channels;
						// window size in samples (per channel)
						size_t windowPerChannel = 1024;
						size_t windowSamples = windowPerChannel * channels;
						
						if (windowSamples == 0 || !samples) {
							// do nothing
						} else {
							// clamp start/end
							size_t start = (center > windowSamples/2) ? (center - windowSamples/2) : 0;
							if (start + windowSamples > totalSamples) {
								if (totalSamples > windowSamples) start = totalSamples - windowSamples; else start = 0;
							}
							
							// compute mean square of the window samples (convert to float -1.0 .. 1.0, then square and average)
							double sum = 0.0;
							for (size_t i = 0; i < windowSamples && (start + i) < totalSamples; ++i) {
								float v = static_cast<float>(samples[start + i]) / 32768.0f;
								sum += static_cast<double>(v) * static_cast<double>(v);
							}

							// convert mean square to RMS level (0.0 .. 1.0)
							double mean = 0.0;
							if (windowSamples > 0) mean = sum / static_cast<double>(windowSamples);	
							
							// store the computed level in the m_levels map for retrieval by GetLevel
							std::lock_guard<std::mutex> lk(m_levelsMutex);
							m_levels[id] = static_cast<float>(mean);
						}
					}
				}
			}
		}
	}
}

sf::Music* MusicSystem::GetOrCreateMusic(Entity& entity)
{
	CMusic* musicComp = entity.GetComponent<CMusic>();
	if (!musicComp) return nullptr; // If the entity doesn't have a CMusic component, return nullptr

	size_t id = entity.GetId();
	auto it = m_activeMusic.find(id);
	if (it != m_activeMusic.end()) return it->second.get();

	// If music doesn't exist for this entity, create a new one
	auto newMusic = std::make_unique<sf::Music>();
	sf::Music* musicPtr = newMusic.get();
	m_activeMusic.emplace(id, std::move(newMusic));

	// If a path is specified, attempt to open it now
	if (!musicComp->path.empty()) {
		if (!musicPtr->openFromFile(musicComp->path)) {
			std::cerr << "MusicSystem: Failed to open audio file: " << musicComp->path << std::endl;
			// leave instance in map but it will be skipped during Process if unopened
		} else {
			std::cout << "MusicSystem: Opened audio file: " << musicComp->path << " for entity " << id << std::endl;
		}
	}

	// initialize level entry
	{
		std::lock_guard<std::mutex> lk(m_levelsMutex);
		m_levels[id] = 0.0f;
	}

	// Attempt to load an sf::SoundBuffer for offline analysis (non-realtime)
	// Note: This may fail for large files as SoundBuffer loads entire audio into memory
	try {
		auto buf = std::make_shared<sf::SoundBuffer>();
		if (buf->loadFromFile(musicComp->path)) {
			std::lock_guard<std::mutex> lk(m_levelsMutex);
			m_buffers[id] = buf;
			std::cout << "MusicSystem: Loaded buffer for analysis for entity " << id << std::endl;
		} else {
			// buffer failed (maybe large file), we'll fall back to no-analysis
			std::cerr << "MusicSystem: Failed to load buffer for analysis: " << musicComp->path << std::endl;
		}
	} catch (...) {
		// ignore - analysis optional
	}

	return musicPtr;
}
