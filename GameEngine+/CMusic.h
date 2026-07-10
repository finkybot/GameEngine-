/////////////////////////////////
// CMusic.h - Component for music playback in the game engine
// ///////////////////////////////



/////////////////////////////////
// Includes and necessary headers for the CMusic component.
#pragma once
#include "Component.h"
#include <SFML/Audio.hpp>
#include <string>
/////////////////////////////////



/////////////////////////////////
// CMusic component - represents a music playback component with properties for file path, volume, looping, and playback state. This component can be added to an entity to enable music playback functionality in the game
class CMusic : public Component {
	/////////////////////////////////
	// Public member variables including an enumerator for music playback state
public:
	/////////////////////////////////
	// Enumerator for music playback state, indicating whether the music is stopped, playing, or paused.
	enum class State // Enumerator
	{
		Stopped,
		Playing,
		Paused
	};
	/////////////////////////////////



	/////////////////////////////////
	// member variables for music properties and state management. These include the file path for the music, volume level, looping behavior, autoplay settings, and the current playback state.
	std::string path;	 // file path to stream the music
	float volume = 50.f; // volume level (0-100), I'm using 50 as a default
	bool loop = false;	 // whether the music should loop when it reaches the end
	bool playOnStart =	false; // start  state of the music
	bool autoPlay =		false; // whether the music should automatically play when the component is added to an entity (this can be used in conjunction with playOnStart for more control over when the music starts)
	State state = State::Stopped; // current playback state of the music (stopped, playing, or paused)

	// SFML music object pointer (owned and managed by MusicSystem)
	sf::Music* m_music = nullptr;  // pointer to the actual SFML music object for playback

	// 3D spatial audio parameters (for experimenting with audio positioning in visualizer)
	float m_3DMinDistance = 500.0f;		// minimum distance for 3D audio attenuation (distance at which the music is at full volume)
	float m_3DMaxDistance = 5000.0f;	// maximum distance for 3D audio attenuation (distance beyond which the music is inaudible)
	/////////////////////////////////



	/////////////////////////////////
	// Public methods (Constructors, etc.)
	CMusic() = default; // Default constructor
	/////////////////////////////////



	/////////////////////////////////
	// Constructor with parameters for initializing the music component with specific properties. This constructor allows setting the file path, volume, looping behavior, and autoplay settings when creating a CMusic component.
	explicit CMusic(const std::string& filePath, float vol = 50.f, bool looped = false, bool playOnStart = false)
		: path(filePath), volume(vol), loop(looped), playOnStart(playOnStart), autoPlay(playOnStart) {}
	/////////////////////////////////
};
/////////////////////////////////