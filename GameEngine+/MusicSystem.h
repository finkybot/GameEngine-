/////////////////////////////////
// MusicSystem.h - header file for the MusicSystem class, which manages music playback in the game. It interacts with entities that have CMusic components and uses the SFML Audio module to play music tracks. The MusicSystem is responsible for starting, stopping, and updating music playback based on the state of CMusic components in the entities. 
// It maintains a mapping of active music tracks to their corresponding sf::Music instances for efficient management and control of music playback.
/////////////////////////////////



/////////////////////////////////
// Includes
#pragma once
#include <memory>
#include <unordered_map>
/////////////////////////////////



/////////////////////////////////
// sf::Music is used for streaming music playback, while sf::SoundBuffer is used for offline analysis of audio data (e.g., measuring levels, performing spectral analysis). The MusicSystem will manage sf::Music instances for active music tracks, and optionally load sf::SoundBuffer 
// instances for entities that have CMusic components to enable analysis features without affecting playback performance; we'll use SFML SoundBuffer for offline analysis
namespace sf {
	class Music;
	class SoundBuffer;
}
/////////////////////////////////



/////////////////////////////////
// Further Includes
#include <mutex>
#include <vector>
/////////////////////////////////



/////////////////////////////////
// Forward declarations for Entity and EntityManager classes, which are used by the MusicSystem to access entities with CMusic components and manage music playback based on their state.
class Entity;
class EntityManager;
/////////////////////////////////



/////////////////////////////////
// MusicSystem class - manages the playback of music tracks in the game. It interacts with entities that have CMusic components, and uses the SFML Audio module to play music. The MusicSystem is responsible for starting, stopping, and updating music playback based on the state of 
// CMusic components in the entities. It maintains a mapping of active music tracks to their corresponding sf::Music instances for efficient management and control of music playback.
class MusicSystem {
	/////////////////////////////////
	// Public interface
public:
	/////////////////////////////////
	// Constructor and destructor for the MusicSystem class. The constructor takes a reference to the EntityManager, which is used to access entities and their components. The destructor is responsible for cleaning up any active music instances when the MusicSystem is destroyed.
	explicit MusicSystem(EntityManager&	entityManager);
	~MusicSystem();
	/////////////////////////////////



	/////////////////////////////////
	// Update - Method that should be called every frame to manage music playback based on the state of CMusic components in the entities. This method will handle starting new music tracks, 
	// stopping tracks that are no longer active, and updating any necessary state for currently playing tracks (e.g., measuring levels, performing analysis).
	void Update(float deltaSeconds);
	/////////////////////////////////



	/////////////////////////////////
	// Process - Method that can be called to perform any necessary processing related to music playback, such as measuring audio levels, performing spectral analysis, or updating internal state based on the current playback status of music tracks. This method can be called from 
	// the main game loop or from specific events to ensure that music-related processing is performed at the appropriate times.
	void Process();
	/////////////////////////////////

	

	/////////////////////////////////
	// StopAllMusic - Method to stop all currently playing music tracks. This can be called when transitioning between scenes, when the player dies, or in any situation where you want to immediately stop all music playback. It will iterate through all active music instances 
	// and call their stop method, then clear the active music mapping.
	void StopAllMusic();
	/////////////////////////////////



	/////////////////////////////////
	// GetLevel - Method to query the latest measured RMS level for a given entity ID. This can be used for visualizations (e.g., volume meters) or gameplay mechanics that depend on the current audio level of a music track. It returns a float value between 0.0 and 1.0 
	// representing the normalized RMS level, where 0.0 means silence and 1.0 means maximum volume.
	float GetLevel(size_t entityId) const;
	/////////////////////////////////
	 
	

	/////////////////////////////////
	// HasAnalysisBuffer - Method to check if an analysis buffer (sf::SoundBuffer) is available for a given entity ID. This can be used to determine if spectral analysis or other audio processing features are available for a music track, 
	// based on whether the corresponding sound buffer was successfully loaded.
	bool HasAnalysisBuffer(size_t entityId) const;
	/////////////////////////////////



	/////////////////////////////////
	// GetPlayingOffset - Method to query the current playback position (in seconds) for a given entity ID. This can be used for visualizations (e.g., progress bars) or gameplay mechanics that depend on the current position of a music track. 
	// It returns a float value representing the playback offset in seconds, or 0.0 if the music is not available or not currently playing.
	float GetPlayingOffset(size_t entityId) const;
	/////////////////////////////////



	/////////////////////////////////
	// GetDuration - Method to query the total duration (in seconds) of the music track for a given entity ID. This can be used for visualizations (e.g., progress bars) or gameplay mechanics that depend on the total length of a music track.
	float GetDuration(size_t entityId) const;
	/////////////////////////////////



	/////////////////////////////////
	// Seek - Method to seek to a given offset (in seconds) for a given entity ID. This can be used to implement features like scrubbing through a music track or starting playback from a specific position.
	// No-op if music is not available.
	void Seek(size_t entityId, float seconds);
	/////////////////////////////////



	/////////////////////////////////
	// Spectrum (Goertzel) accessors -
	// GetSpectrum - Method to query the latest calculated spectrum values for a given entity ID. This can be used for visualizations (e.g., spectrum analyzers) or gameplay mechanics that depend on the current spectral content of a music track. 
	// It fills the provided vector with the latest spectrum values, where each value represents the magnitude of a specific frequency band.
	bool GetSpectrum(size_t entityId, std::vector<float>& outSpectrum) const;
	/////////////////////////////////



	/////////////////////////////////
	// GetSpectrumBandCount - Method to get the current number of spectrum bands being calculated for spectral analysis. This can be used to determine the resolution of the spectrum data and adjust visualizations or gameplay mechanics accordingly.
	size_t GetSpectrumBandCount() const;
	/////////////////////////////////



	/////////////////////////////////
	// SetSpectrumBandCount - Method to set the number of spectrum bands to calculate for spectral analysis. This can be used to adjust the resolution of the spectrum data, where a higher band count provides more detailed frequency information but may require more processing power.
	void SetSpectrumBandCount(int count);
	/////////////////////////////////



	/////////////////////////////////
	// SetSpectrumSmoothing - Method to set the smoothing factor for spectrum values. This can be used to adjust the responsiveness of spectrum visualizations or gameplay mechanics that depend on spectral data, where a higher smoothing factor results in smoother but less responsive spectrum values.
	void SetSpectrumSmoothing(float smoothing);
	/////////////////////////////////



	/////////////////////////////////
	// GetSpectrumSmoothing - Method to get the current smoothing factor for spectrum values. This can be used to query the current responsiveness of spectrum visualizations or gameplay mechanics that depend on spectral data.
	float GetSpectrumSmoothing() const;
	/////////////////////////////////



	/////////////////////////////////
	// FFT options - Methods to enable or disable FFT-based spectral analysis, and to set the FFT size when enabled. This allows for more detailed spectral analysis using FFT instead of Goertzel, at the cost of increased processing requirements.
	void SetUseFFT(bool useFFT);
	bool GetUseFFT() const;
	void SetFFTSize(int size);
	int GetFFTSize() const;
	/////////////////////////////////



	/////////////////////////////////
	// Private helper methods for the MusicSystem class. These methods include logic for managing sf::Music instances for entities with CMusic components, performing audio analysis (e.g., measuring RMS levels, calculating spectra), 
	// and any other internal functionality needed to support the public interface of the MusicSystem.
private:
	/////////////////////////////////
	// Map of entity ID to latest measured RMS level for that entity's music track. This allows us to provide level information for visualizations or gameplay mechanics that depend on the current audio level of a music track. 
	// The levels are normalized to a range of 0.0 to 1.0, where 0.0 means silence and 1.0 means maximum volume.
	std::unordered_map<size_t, float> m_levels;
	/////////////////////////////////



	/////////////////////////////////
	// Mutex to protect access to m_levels and m_spectra, which are updated in the Process method and read in the GetLevel and GetSpectrum methods. This ensures thread safety when accessing these data structures from different threads (e.g., main game loop vs audio processing thread).
    mutable std::recursive_mutex m_levelsMutex;
	/////////////////////////////////



	/////////////////////////////////
	// Map of entity ID to loaded sf::SoundBuffer for that entity's music track. This allows us to perform offline analysis (e.g., measuring levels, calculating spectra) on the music data without affecting playback performance, since sf::Music does not provide direct access to audio samples.
	std::unordered_map<size_t, std::shared_ptr<sf::SoundBuffer>> m_buffers;
	/////////////////////////////////



	/////////////////////////////////
	// Reference to the EntityManager for accessing entities and their components.
	EntityManager& m_entityManager;
	/////////////////////////////////



	/////////////////////////////////
	// Map of entity ID to active sf::Music instance for entities that have a CMusic component currently playing. This allows us to manage multiple music tracks if needed, and ensures we can stop or update them as necessary.
	std::unordered_map<size_t, std::unique_ptr<sf::Music>> m_activeMusic;
	/////////////////////////////////



	/////////////////////////////////
	// GetOrCreateMusic - Helper method to get the existing sf::Music instance for an entity with a CMusic component, or create a new one if it doesn't exist. This method will handle loading the music file and configuring the sf::Music instance based on the properties of the CMusic component.
	sf::Music* GetOrCreateMusic(Entity&	entity);
	/////////////////////////////////
	 
	
	/////////////////////////////////
	// Spectral analysis storage (Goertzel bands)
	// Map of entity ID to vector of latest calculated spectrum values for that entity's music track. This allows us to provide spectrum information for visualizations or gameplay mechanics that depend on the current spectral content of a music track, protected by m_levelsMutex.
	std::unordered_map<size_t, std::vector<float>> m_spectra;
	/////////////////////////////////



	/////////////////////////////////
	// Configuration parameters for spectral analysis. These include the number of spectrum bands to calculate (defaulting to 10 for a 10-band equalizer), the smoothing factor for spectrum values (defaulting to 0.65 for a balance between responsiveness and smoothness), 
	// and the center frequencies for each band (initialized on demand based on the sample rate and FFT size).
	int m_eqBandCount = 10; // default 10-band equalizer
	float m_spectrumSmoothing = 0.65f; // smoothing alpha (0..1) where higher is smoother
	std::vector<float> m_eqCenterFreqs; // center frequencies for bands (initialized on demand)
	/////////////////////////////////
	


	/////////////////////////////////
    // FFT options
	bool m_useFFT = false; // default: keep legacy Goertzel unless enabled
	int m_fftSize = 2048;  // FFT size to use when m_useFFT is true
	/////////////////////////////////
};
/////////////////////////////////
