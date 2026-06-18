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
// SoundPriority enum - Define priority levels for audio culling when max sounds exceeded
enum class SoundPriority {
	Background = 0,    // Ambient sounds (wind, rain)
	UI = 1,            // Menu clicks
	SFX = 2,           // Normal game sounds
	Dialogue = 3,      // Voice lines
	Critical = 4       // Important warnings/alerts
};
/////////////////////////////////



/////////////////////////////////
// CSoundEffect Component -	|	Represents a sound effect in the game, with properties for sound file, looping, 3D audio, spatialization, 
//							|	play on awake, volume, and pitch.
// 							|___________________________________________________________________________________
class CSoundEffect : public Component {
	/////////////////////////////////
	// Public Data Members and enums
public:
	/////////////////////////////////
	// Constructors - Default constructor
	CSoundEffect() = default;
	/////////////////////////////////



	/////////////////////////////////
	// Parameterized constructor to initialize the sound effect with specific properties
	CSoundEffect(const std::string& soundFile, const std::string& path, bool loop = false, bool is3D = false, bool isSpatialized = false, bool playOnAwake = false, float volume = 1.0f, float pitch = 1.0f)
		: m_soundFile(soundFile), m_Path(path), m_loop(loop), m_is3D(is3D), m_isSpatialized(isSpatialized), m_PlayOnAwake(playOnAwake), m_volume(volume), m_pitch(pitch) {}



	/////////////////////////////////
	// enum class State - Represents the current state of the sound effect, which can be Playing, Stopped, Paused, or FadingOut. 
	enum class State { Playing, Stopped, Paused, FadingOut };
	/////////////////////////////////



	/////////////////////////////////
	// Public properties for sound effect class
	/////////////////////////////////
	// PLAYBACK STATE 
	State m_state = State::Stopped;					// current state of the sound effect
	bool m_shouldPlay = false;						// flag to request playback (deferred, system-driven)
	/////////////////////////////////
	


	/////////////////////////////////
	// SOUND FILE AND PATH
	std::string m_soundFile;						// name of the sound file (without path, e.g. "explosion.wav")
	std::string m_Path;								// path to the sound file (relative to assets/sounds/)
	/////////////////////////////////



	/////////////////////////////////
	// PLAYBACK PROPERTIES
	bool m_loop = false;							// looping state of the sound effect
	bool m_is3D = false;							// 3D state (positional audio) of the sound effect
	bool m_isSpatialized = false;					// spatialization state of the sound effect
	bool m_PlayOnAwake = false;						// play on awake state of the sound effect
	/////////////////////////////////



	/////////////////////////////////
	// VOLUME AND PITCH
	float m_volume = 1.0f;							// volume of the sound effect (1.0 = full volume)
	float m_pitch = 1.0f; 							// pitch of the sound effect (1.0 = normal pitch)
	/////////////////////////////////



	/////////////////////////////////
	// FADE OUT PROPERTIES
	float m_fadeOutDuration = 0.0f;					// duration of fade-out effect in seconds (0 means no fade-out)
	float m_fadeOutElapsed = 0.0f;					// elapsed time since fade-out started, used to track progress of fade-out effect	
	/////////////////////////////////



	/////////////////////////////////
	// SPATIAL AUDIO (3D POSITIONING)
	float m_3DMinDistance = 500.0f;					// minimum distance for 3D sound attenuation (distance at which the sound is at full volume)
	float m_3DMaxDistance = 10000.0f;				// maximum distance for 3D sound attenuation (distance beyond which the sound is inaudible)
	float m_currentDistance = 0.0f;					// current distance from listener (calculated by system each frame)
	float m_pan = 0.0f;								// pan of the sound effect (-1.0 = full left, 0.0 = center, 1.0 = full right)
	/////////////////////////////////



	/////////////////////////////////
	// PRIORITY SYSTEM
	SoundPriority m_priority = SoundPriority::SFX;  // priority of the sound effect (used for culling when max concurrent sounds exceeded)
	/////////////////////////////////



	/////////////////////////////////
	// BACKING AUDIO OBJECT (owned by component, managed by system)
	std::unique_ptr<sf::Sound> m_sound = nullptr;  // the actual SFML sound object for playback (created/managed by SoundSystem)
	/////////////////////////////////
};
/////////////////////////////////
