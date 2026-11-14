/////////////////////////////////
// CSoundEffect.h - Represents a sound effect in the game. This class can be used
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Component.h"
#include <string>
#include <memory>
#include <SFML/Audio.hpp>
/////////////////////////////////



/////////////////////////////////
//	|	SoundPriority enum - Define priority levels for audio culling when max sounds exceeded
//	|__________________________________________________________________________________
enum class SoundPriority {
	Background = 0,    // Ambient sounds (wind, rain)
	UI = 1,            // Menu clicks
	SFX = 2,           // Normal game sounds
	Dialogue = 3,      // Voice lines
	Critical = 4       // Important warnings/alerts
};
/////////////////////////////////



/////////////////////////////////
// 
//	|	CSoundEffect Component -	Represents a sound effect in the game, with properties for sound file, looping, 3D audio, spatialization, play on awake, volume, and pitch.
//	|___________________________________________________________________________________
class CSoundEffect : public Component {
public:
	// Constructors - Default constructor
	CSoundEffect() = default;

	// Parameterized constructor to initialize the sound effect with specific properties
	CSoundEffect(const std::string& soundFile, const std::string& path, bool loop = false, bool is3D = false, bool isSpatialized = false, bool playOnAwake = false, float volume = 1.0f, float pitch = 1.0f)
		: soundFile(soundFile), Path(path), loop(loop), is3D(is3D), isSpatialized(isSpatialized), PlayOnAwake(playOnAwake), volume(volume), pitch(pitch) {}



	// enum class State - Represents the current state of the sound effect, which can be Playing, Stopped, Paused, or FadingOut. 
	enum class State { 
		Playing, 
		Stopped, 
		Paused, 
		FadingOut 
	};


	// PLAYBACK STATE 
	State state = State::Stopped;					// current state of the sound effect
	bool shouldPlay = false;						// flag to request playback (deferred, system-driven)


	// SOUND FILE AND PATH
	std::string soundFile;							// name of the sound file (without path, e.g. "explosion.wav")
	std::string Path;								// path to the sound file (relative to assets/sounds/)


	// PLAYBACK PROPERTIES
	bool loop = false;								// looping state of the sound effect
	bool is3D = false;								// 3D state (positional audio) of the sound effect
	bool isSpatialized = false;						// spatialization state of the sound effect
	bool PlayOnAwake = false;						// play on awake state of the sound effect


	// VOLUME AND PITCH
	float volume = 1.0f;							// volume of the sound effect (1.0 = full volume)
	float pitch = 1.0f; 							// pitch of the sound effect (1.0 = normal pitch)


	// FADE OUT PROPERTIES
	float fadeOutDuration = 0.0f;					// duration of fade-out effect in seconds (0 means no fade-out)
	float fadeOutElapsed = 0.0f;					// elapsed time since fade-out started, used to track progress of fade-out effect	


	// SPATIAL AUDIO (3D POSITIONING)
	float _3DMinDistance = 500.0f;					// minimum distance for 3D sound attenuation (distance at which the sound is at full volume)
	float _3DMaxDistance = 10000.0f;				// maximum distance for 3D sound attenuation (distance beyond which the sound is inaudible)
	float currentDistance = 0.0f;					// current distance from listener (calculated by system each frame)
	float pan = 0.0f;								// pan of the sound effect (-1.0 = full left, 0.0 = center, 1.0 = full right)


	// PRIORITY SYSTEM
	SoundPriority priority = SoundPriority::SFX;	// priority of the sound effect (used for culling when max concurrent sounds exceeded)


	// BACKING AUDIO OBJECT (owned by component, managed by system)
	std::unique_ptr<sf::Sound> sound = nullptr;		// the actual SFML sound object for playback (created/managed by SoundSystem)
};
/////////////////////////////////
