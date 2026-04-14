// ****** CMusic.h ******
#pragma once
#include "Component.h"
#include <SFML/Audio.hpp>
#include <string>


// CMusic component - represents a music track that can be played in the game. It inherits from Component and encapsulates an sf::Music object from the SFML library
// Music component can be added to entities to allow playback of music tracks.
class CMusic : public Component
{
	// Public member variables
public:
	enum class State // Enumerator
	{
		Stopped,
		Playing,
		Paused
	};

	std::string path;				// file path to stream the music
	float volume = 50.f;			// volume level (0-100), I'm using 50 as a default
	bool loop = false;				// whether the music should loop when it reaches the end
	bool playOnStart = false;		// whether the music should start playing immediately when the component is added to an entity
	bool autoPlay = false;			// whether the music should automatically play when the component is added to an entity (this can be used in conjunction with playOnStart for more control over when the music starts)
    // Note: removal of restart flag - restart now handled by UI invoking Seek before Process.
	State state = State::Stopped;	// current playback state of the music (stopped, playing, or paused)

	// Public metheds (Constructors, etc.)
	CMusic() = default; // Default constructor
	explicit CMusic(const std::string& filePath, float vol = 50.f, bool looped = false, bool playOnStart = false) : path(filePath), volume(vol), loop(looped), playOnStart(playOnStart), autoPlay(playOnStart) {}
};

