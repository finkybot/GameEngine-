/////////////////////////////////
// CSoundEffect.h - Represents a sound effect in the game. This class can be used
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include "Component.h"
#include <string>
/////////////////////////////////



/////////////////////////////////
// CSoundEffect Component -	| Represents a sound effect in the game, with properties for sound file, looping, 3D audio, spatialization, 
//							|play on awake, volume, and pitch.
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
	State m_state = State::Stopped;			// current state of the sound effect
	
	std::string m_soundFile;				// name of the sound file (without path, e.g. "explosion.wav")
	std::string m_Path;						// path to the sound file (relative to assets/sounds/)

	bool m_loop = false;					// looping state of the sound effect
	bool m_is3D = false;					// 3D state (positional audio) of the sound effect
	bool m_isSpatialized = false;			// spatialization state of the sound effect
	bool m_PlayOnAwake = false;				// play on awake state of the sound effect

	float m_volume = 1.0f;					// volume of the sound effect (1.0 = full volume)
	float m_pitch = 1.0f; 					// pitch of the sound effect (1.0 = normal pitch)
	/////////////////////////////////
};
/////////////////////////////////
