// MusicSystem.cpp : This file contains the implementation of the MusicSystem class, which manages the music playback in the game engine.
#include "Entity.h"        // Move this to be first
#include "MusicSystem.h"
#include "EntityManager.h"
#include "CMusic.h"

#include <SFML/Audio.hpp>
#include <iostream>

//#ifdef _MSC_VER
//#pragma comment(lib, "sfml-audio.lib")
//#endif



MusicSystem::MusicSystem(EntityManager& entityManager) : m_entityManager(entityManager) {}

MusicSystem::~MusicSystem()
{
	// Stop all music activity and clear the active music map
	for (auto& pair : m_activeMusic) {
		if (pair.second) {
			pair.second->stop(); // Stop the music if it's currently playing
		}
	}
}

void MusicSystem::Update(float deltaSeconds)
{}

void MusicSystem::Process()
{
	// Get all entities with a CMusic component
	for (const auto& entity : m_entityManager.getEntities()) {
		if (entity->HasComponent<CMusic>()) {
			CMusic* musicComp = entity->GetComponent<CMusic>();
			if (musicComp) 
			{
				sf::Music* music = GetOrCreateMusic(*entity);
				if (music)
				{
					// Update music properties based on the CMusic component
					music->setVolume(musicComp->volume);
                    // Some SFML variants expose looping under different names; try setLooping first
					music->setLooping(musicComp->loop);
					// Handle play/pause/stop based on the state of the CMusic component
					switch (musicComp->state) {
                        case CMusic::State::Playing:
							if (music->getStatus() != sf::SoundSource::Status::Playing) {
								music->play();
							}
							break;
                        case CMusic::State::Paused:
							if (music->getStatus() == sf::SoundSource::Status::Playing) {
								music->pause();
							}
							break;
                        case CMusic::State::Stopped:
							if (music->getStatus() != sf::SoundSource::Status::Stopped) {
								music->stop();
							}
							break;
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
		}
	}

	return musicPtr;
}
