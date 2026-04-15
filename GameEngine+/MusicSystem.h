// ******* MusicSystem.h *******
#pragma once
#include <memory>
#include <unordered_map>
// we'll use SFML SoundBuffer for offline analysis
namespace sf {
class Music;
class SoundBuffer;
} // namespace sf
#include <mutex>
#include <vector>

// Forward declaration of SFML audio types used by this header
namespace sf {
class Music;
class SoundBuffer;
} // namespace sf

class Entity;
class EntityManager;

// MusicSystem class - manages the playback of music tracks in the game. It interacts with entities that have CMusic components, and uses the SFML Audio module to play music.
// The MusicSystem is responsible for starting, stopping, and updating music playback based on the state of CMusic components in the entities. It maintains a mapping of
// active music tracks to their corresponding sf::Music instances for efficient management and control of music playback.
class MusicSystem {
public:
	explicit MusicSystem(
		EntityManager&
			entityManager); // Constructor that takes a reference to the EntityManager, which is used to access entities and their components.
	~MusicSystem(); // Destructor to clean up any active music instances when the MusicSystem is destroyed.
	void Update(
		float
			deltaSeconds); // Update method that should be called every frame to manage music playback based on the state of CMusic components in the entities.

	void
	Process(); // Process method to handle starting, stopping, and updating music playback based on the state of CMusic components in the entities. This is where the main logic for managing music playback will be implemented.

	// Stop and clear all active music instances immediately
	void StopAllMusic();

	float
	GetLevel(size_t entityId) const; // Query latest measured RMS / level measured for a given entity id (0.0 .. 1.0)
	// Return whether offline analysis buffer is available for entity
	bool HasAnalysisBuffer(size_t entityId) const;

	// Query playback position (seconds) for a given entity. Returns 0.0 if not available.
	float GetPlayingOffset(size_t entityId) const;

	// Query total duration (seconds) for a given entity. Returns 0.0 if not available.
	float GetDuration(size_t entityId) const;

	// Seek to a given offset (seconds) for a given entity. No-op if music not available.
	void Seek(size_t entityId, float seconds);

	// Spectrum (Goertzel) accessors
	// Fills outSpectrum with band magnitudes (0..1). Returns true if spectrum is available.
	bool GetSpectrum(size_t entityId, std::vector<float>& outSpectrum) const;
	size_t GetSpectrumBandCount() const;
	void SetSpectrumBandCount(int count);
	void SetSpectrumSmoothing(float smoothing);
	float GetSpectrumSmoothing() const;

	// FFT-based analysis options
	void SetUseFFT(bool useFFT);
	bool GetUseFFT() const;
	void SetFFTSize(int size);
	int GetFFTSize() const;

private:
	// Latest measured audio levels (RMS) per entity id
	std::unordered_map<size_t, float> m_levels;
    mutable std::recursive_mutex m_levelsMutex; // protects map structure (recursive to avoid reentrant lock issues)

	// Optional decoded buffers used for analysis (loaded once when music opens)
	std::unordered_map<size_t, std::shared_ptr<sf::SoundBuffer>> m_buffers;
	EntityManager& m_entityManager; // Reference to the EntityManager for accessing entities and their components.
	std::unordered_map<size_t, std::unique_ptr<sf::Music>>
		m_activeMusic; // Map of entity ID to active sf::Music instance for entities that have a CMusic component currently playing. This allows us to manage multiple music tracks if needed, and ensures we can stop or update them as necessary.

	sf::Music* GetOrCreateMusic(
		Entity&
			entity); // Helper method to get the existing sf::Music instance for an entity with a CMusic component, or create a new one if it doesn't exist. This method will handle loading the music file and configuring the sf::Music instance based on the properties of the CMusic component.

	// Spectral analysis storage (Goertzel bands)
	std::unordered_map<size_t, std::vector<float>> m_spectra; // protected by m_levelsMutex
	int m_eqBandCount = 10; // default 10-band equalizer
	float m_spectrumSmoothing = 0.65f; // smoothing alpha (0..1) where higher is smoother
	std::vector<float> m_eqCenterFreqs; // center frequencies for bands (initialized on demand)
    // FFT options
	bool m_useFFT = false; // default: keep legacy Goertzel unless enabled
	int m_fftSize = 2048;  // FFT size to use when m_useFFT is true
};
