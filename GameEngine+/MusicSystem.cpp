/////////////////////////////////
// MusicSystem.cpp : Implementation of the MusicSystem class for music playback management.
/////////////////////////////////



/////////////////////////////////
// Includes
#include "Entity.h"
#include "MusicSystem.h"
#include "EntityManager.h"
#include "CMusic.h"
#include "CTransform.h"
#include "DebugStack.h"

#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <cstring>
#include <unordered_set>
#include <complex>
#include <algorithm>
#include <vector>
#include <filesystem>
/////////////////////////////////



/////////////////////////////////
// Include kiss_fft implementation directly in the MusicSystem.cpp to avoid needing to link against an external library. This allows us 
// to keep the music system self-contained and simplifies the build process, as we don't need to worry about linking against a separate FFT library.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include "../third_party/kissfft/kiss_fft.h"
/////////////////////////////////



/////////////////////////////////
// UtfToPath - Converts a UTF-8 encoded std::string to a std::filesystem::path, ensuring that Unicode characters are correctly handled on 
// Windows. On Windows, we round-trip through a wide string (std::wstring) to ensure that the full Unicode path is preserved, 
// as the Windows API expects wide strings for file paths.
static std::filesystem::path Utf8ToPath(const std::string& utf8) {
#ifdef _WIN32
	// Round-trip through wstring so Windows sees the full Unicode path
	int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
	std::wstring wide(static_cast<size_t>(wlen > 0 ? wlen : 0), L'\0');
	if (wlen > 0) MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, wide.data(), wlen);
	return std::filesystem::path(wide);
#else
	return std::filesystem::u8path(utf8);
#endif
}
/////////////////////////////////



/////////////////////////////////
// Provide simple kiss_fft implementation inline so it is compiled into the binary
extern "C" {
/////////////////////////////////
// ks_internal_fft - This function implements a simple radix-2 Cooley-Tukey FFT algorithm. It takes an array of complex 
// numbers (kiss_fft_cpx) and its size, and performs the FFT in-place. The function first performs a bit-reversal permutation on the input data,
static void ks_internal_fft(kiss_fft_cpx* data, int size) {
	// require power of two size, but we won't enforce it here since the config allocator will round up to the next 
	// power of two. Just do the best we can with the given size and ignore any extra samples if not a power of two
	if (size <= 1) return;
	
	// bit reverse
	int i, j = 0;
	for (i = 1; i < size; ++i) { // start at 1 since bit reverse of 0 is 0 and we can skip that swap
		int bit = size >> 1;

		// This bit reversal code is a common pattern in radix-2 FFT implementations. It effectively computes the bit-reversed 
		// index for each position i and swaps the elements accordingly. The inner loop shifts the bit variable right 
		// until it finds a bit that is not set in j, at which point it toggles that bit in j. 
		// This process generates the correct bit-reversed indices for the FFT algorithm.
		for (; j & bit; bit >>= 1) j ^= bit;
		j ^= bit;
		if (i < j) {
			kiss_fft_cpx tmp = data[i];
			data[i] = data[j];
			data[j] = tmp; // swap
		}
	}
	// Cooley-Tukey radix-2 FFT (find those soviet nuke tests)
	for (int len = 2; len <= size; len <<= 1) { // len is the size of the sub-FFTs we are combining at this stage
		float angle = -2.0f * 3.14159265358979323846f / (float)len;
		float cosv = cosf(angle);
		float sinv = sinf(angle);
		for (i = 0; i < size; i += len) {
			float wr = 1.0f, wi = 0.0f;
			for (j = 0; j < len/2; ++j) {
				float ur = data[i+j].r;
				float ui = data[i+j].i;
				float vr = data[i+j+len/2].r * wr - data[i+j+len/2].i * wi;
				float vi = data[i+j+len/2].r * wi + data[i+j+len/2].i * wr;
				data[i+j].r = ur + vr;
				data[i+j].i = ui + vi;
				data[i+j+len/2].r = ur - vr;
				data[i+j+len/2].i = ui - vi;
				float nwr = wr * cosv - wi * sinv;
				float nwi = wr * sinv + wi * cosv;
				wr = nwr; wi = nwi;
			}
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// kiss_fft_alloc - This function allocates and initializes a configuration structure for the FFT. It takes the desired FFT size 
// (nfft), a flag indicating whether to perform an inverse FFT, and optional memory parameters which we ignore in this simple implementation.
kiss_fft_cfg* kiss_fft_alloc(int nfft, int inverse_fft, void* mem, size_t* lenmem) {
	(void)mem; (void)lenmem;
	if (nfft <= 0) return NULL;
	int p = 1; while (p < nfft) p <<= 1;
	kiss_fft_cfg* cfg = (kiss_fft_cfg*)malloc(sizeof(kiss_fft_cfg));
	if (!cfg) return NULL;
	cfg->nfft = p;
	cfg->inverse = inverse_fft ? 1 : 0;
	return cfg;
}
/////////////////////////////////



/////////////////////////////////
// kiss_fft_free - This function frees the configuration structure allocated by kiss_fft_alloc. It simply checks if the pointer is 
// not null and then frees the memory.
void kiss_fft_free(kiss_fft_cfg* cfg) {
	if (cfg) free(cfg);
}
/////////////////////////////////



/////////////////////////////////
// kiss_fft - This function performs the FFT using the provided configuration and input/output buffers. It first checks for null 
// pointers, then allocates a temporary buffer to hold the input data. It copies the input data into the buffer, zero-padding if necessary, 
// and then calls the internal FFT function to perform the transformation in-place. Finally, it copies the result to the output buffer 
// and frees the temporary buffer.
void kiss_fft(const kiss_fft_cfg* cfg, const kiss_fft_cpx* fin, kiss_fft_cpx* fout) {
	if (!cfg || !fin || !fout) return;
	int n = cfg->nfft;
	kiss_fft_cpx* buf = (kiss_fft_cpx*)malloc(sizeof(kiss_fft_cpx) * n);
	if (!buf) return;
	for (int i = 0; i < n; ++i) {
		if (i < cfg->nfft) buf[i] = fin[i]; else { buf[i].r = 0.0f; buf[i].i = 0.0f; }
	}
	ks_internal_fft(buf, n);
	for (int i = 0; i < n; ++i) fout[i] = buf[i];
	free(buf);
}
/////////////////////////////////
} // extern "C"
/////////////////////////////////



/////////////////////////////////
// MusicSystem implementation
MusicSystem::MusicSystem(EntityManager& entityManager) : m_entityManager(entityManager) {}
/////////////////////////////////



/////////////////////////////////
// StopAllMusic - This method is responsible for stopping all currently playing music tracks. It first signals the analysis 
// thread to stop and waits for it to finish, ensuring that no analysis is running while we clear the active music state. 
// Then, it iterates through all active music instances and calls their stop method, effectively halting playback. 
// Finally, it clears the active music mapping and any associated analysis buffers and levels to reset the state of 
// the MusicSystem.
void MusicSystem::StopAllMusic() {
	// Pause the analysis thread while we clear state
	m_analysisStop.store(true);
	m_analysisCv.notify_all();
	if (m_analysisThread.joinable())
		m_analysisThread.join();

	for (auto& p : m_activeMusic) {
		if (p.second)
			p.second->stop();
	}
	m_activeMusic.clear();
	{
		std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
		m_buffers.clear();
		m_levels.clear();
	}
}
/////////////////////////////////



/////////////////////////////////
// GetSpectrum - This method retrieves the latest computed spectrum data for a given entity ID. It locks the levels mutex to 
// ensure thread safety while accessing the spectra mapping, then checks if there is an entry for the specified entity ID. 
// If an entry exists, it copies the spectrum data to the output parameter and returns true; otherwise, it returns false to 
// indicate that no spectrum data is available for that entity.
bool MusicSystem::GetSpectrum(size_t entityId, std::vector<float>& outSpectrum) const {
	std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	auto it = m_spectra.find(entityId);
	if (it == m_spectra.end()) return false;
	outSpectrum = it->second;
	return true;
}
/////////////////////////////////



/////////////////////////////////
// GetSpectrumBandCount - This method returns the number of frequency bands that are being analyzed in the spectrum. It simply 
// returns the value of m_eqBandCount, which is the current number of bands configured for spectral analysis. 
// This value can be used by callers to understand how many frequency bins are present in the spectrum data.
size_t MusicSystem::GetSpectrumBandCount() const {
	return static_cast<size_t>(m_eqBandCount);
}
/////////////////////////////////



/////////////////////////////////
// SetSpectrumBandCount - This method sets the number of frequency bands to analyze in the spectrum. It takes an integer count 
// as a parameter, which specifies how many bands should be used for spectral analysis. 
// The method first checks if the count is positive; if not, it returns without making changes.
void MusicSystem::SetSpectrumBandCount(int count) {
	if (count <= 0) return;
    std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	m_eqBandCount = count;
	// resize center freqs if needed; keep existing for lower indices
	if ((int)m_eqCenterFreqs.size() < m_eqBandCount) {
		// extend with octave multiples starting at 31Hz
		float base = 31.0f;
		while ((int)m_eqCenterFreqs.size() < m_eqBandCount) {
			m_eqCenterFreqs.push_back(base);
			base *= 2.0f;
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// GetSpectrumBandCount - This method returns the current number of frequency bands that are being analyzed in the spectrum. It simply 
// returns the value of m_eqBandCount, which is the current configuration for spectral analysis.
void MusicSystem::SetSpectrumSmoothing(float smoothing) {
	if (smoothing < 0.0f) smoothing = 0.0f;
	if (smoothing > 0.99f) smoothing = 0.99f;
                        std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	m_spectrumSmoothing = smoothing;
}
/////////////////////////////////



/////////////////////////////////
// GetSpectrumSmoothing - This method returns the current smoothing factor applied to the spectrum data. The smoothing factor is a value between 
// 0.0 and 0.99 that determines how much the spectrum values are smoothed over time, with higher values resulting in smoother but less 
// responsive spectrum data.
float MusicSystem::GetSpectrumSmoothing() const {
            std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	return m_spectrumSmoothing;
}
/////////////////////////////////



/////////////////////////////////
// SetUseFFT - This method enables or disables the use of FFT analysis for the music tracks. It takes a boolean parameter useFFT, which 
// indicates whether FFT analysis should be used. The method locks the levels mutex to ensure thread safety while modifying the m_useFFT 
// member variable, which controls whether FFT analysis is performed in the background thread.
void MusicSystem::SetUseFFT(bool useFFT) {
	std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	m_useFFT = useFFT;
}
/////////////////////////////////



/////////////////////////////////
// GetUseFFT - This method returns whether FFT analysis is currently enabled for the music tracks. It locks the levels mutex to ensure thread 
// safety while accessing the m_useFFT member variable, which indicates whether FFT analysis is being used.
bool MusicSystem::GetUseFFT() const {
	std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	return m_useFFT;
}
/////////////////////////////////



/////////////////////////////////
// SetFFTSize - This method sets the size of the FFT to be used for spectral analysis. It takes an integer size as a parameter, which specifies 
// the desired FFT size. The method first checks if the size is positive; if not, it returns without making changes.
void MusicSystem::SetFFTSize(int size) {
	if (size <= 0) return;
	// ensure power of two
	int p = 1;
	while (p < size) p <<= 1;
	std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	m_fftSize = p;
}
/////////////////////////////////



/////////////////////////////////
// GetFFTSize - This method returns the current size of the FFT being used for spectral analysis. It locks the levels mutex to ensure thread 
// safety while accessing the m_fftSize member variable, which holds the current FFT size configuration.
int MusicSystem::GetFFTSize() const {
	std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	return m_fftSize;
}
/////////////////////////////////



/////////////////////////////////
// GetPlayingOffset - This method retrieves the current playback position (in seconds) for a given entity ID. It first checks if there is an 
// active music instance for the specified entity ID; if not, it returns 0.0f. If an active music instance exists, 
// it attempts to get the playing offset using the SFML Music API,
float MusicSystem::GetPlayingOffset(size_t entityId) const {
	auto it = m_activeMusic.find(entityId);
	if (it == m_activeMusic.end() || !it->second)
		return 0.0f;
	try {
		return it->second->getPlayingOffset().asSeconds();
	} catch (...) {
		return 0.0f;
	}
}
/////////////////////////////////



/////////////////////////////////
// GetDuration - This method retrieves the total duration (in seconds) of the music track associated with a given entity ID. It first checks 
// if there is an active music instance for the specified entity ID; if not, it returns 0.0f. If an active music instance exists, it attempts 
// to get the duration using the SFML Music API, and returns it in seconds. If any exceptions occur during this process, it catches them and 
// returns 0.0f as a fallback.
float MusicSystem::GetDuration(size_t entityId) const {
	auto it = m_activeMusic.find(entityId);
	if (it == m_activeMusic.end() || !it->second)
		return 0.0f;
	try {
		return it->second->getDuration().asSeconds();
	} catch (...) {
		return 0.0f;
	}
}
/////////////////////////////////



/////////////////////////////////
// Seek - This method seeks to a specific position (in seconds) in the music track associated with a given entity ID. It first checks if there is an active 
// music instance for the specified entity ID; if not, it returns without doing anything. If an active music instance exists,
void MusicSystem::Seek(size_t entityId, float seconds) {
	auto it = m_activeMusic.find(entityId);
	if (it == m_activeMusic.end() || !it->second)
		return;
	try {
		it->second->setPlayingOffset(sf::seconds(seconds));
	} catch (...) {}
}
/////////////////////////////////



/////////////////////////////////
// Destructor - This method stops the analysis thread and cleans up all active music instances and associated resources.
MusicSystem::~MusicSystem() {
	// Stop the analysis thread first
	m_analysisStop.store(true);
	m_analysisCv.notify_all();
	if (m_analysisThread.joinable())
		m_analysisThread.join();

	for (auto& pair : m_activeMusic) {
		if (pair.second)
			pair.second->stop();
	}
	m_activeMusic.clear();
	{
                        std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
		m_levels.clear();
	}
}
/////////////////////////////////



/////////////////////////////////
// GetLevel - This method retrieves the latest measured RMS level for a given entity ID. It locks the levels mutex to ensure 
// thread safety while accessing the m_levels mapping, then checks if there is an entry for the specified entity ID. If an 
// entry exists, it returns the RMS level;
float MusicSystem::GetLevel(size_t entityId) const {
        std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	auto it = m_levels.find(entityId);
	if (it == m_levels.end())
		return 0.0f;
	return it->second;
}
/////////////////////////////////



/////////////////////////////////
// HasAnalysisBuffer - This method checks if an analysis buffer (sf::SoundBuffer) is available for a given entity ID. It 
// locks the levels mutex to ensure thread safety while accessing the m_buffers mapping, then checks if there is an entry 
// or the specified entity ID. If an entry exists, it returns true; otherwise, it returns false.
bool MusicSystem::HasAnalysisBuffer(size_t entityId) const {
        std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	return m_buffers.find(entityId) != m_buffers.end();
}
/////////////////////////////////



/////////////////////////////////
// Update - This method is called every frame to update the music playback state and manage the analysis thread. It currently 
// does sweet FA, as the main processing is handled in the Process() method. However, it can be used in 
// the future to implement additional per-frame logic related to music playback or analysis.
void MusicSystem::Update(float deltaSeconds) {}
/////////////////////////////////



/////////////////////////////////
// Process - This method is called every frame to update the music playback state and manage the analysis thread. It starts 
// the analysis thread if it hasn't been started yet, cleans up active music instances for entities that no longer have a 
// CMusic component, and processes all entities with a CMusic component to control playback and update playhead snapshots 
// for analysis. Audio analysis runs on a background thread (AnalysisThreadFunc) at ~60 Hz.
void MusicSystem::Process() {
	// Start the analysis thread the first time Process()e.g. run the thread if it hasn't been started yet.
	// The analysis thread will run in the background at ~30 Hz, reading playhead snapshots written by Process(),
	if (!m_analysisThread.joinable()) { 
		m_analysisStop.store(false);
		m_analysisThread = std::thread(&MusicSystem::AnalysisThreadFunc, this);
	}

	// Clean up active music instances for entities that no longer have a CMusic component
	std::unordered_set<size_t> liveIds;
	for (const auto& entity : m_entityManager.GetEntities()) {
		if (entity->HasComponent<CMusic>())
			liveIds.insert(entity->GetId());
	}

	// Erase active music entries that are no longer live
	for (auto it = m_activeMusic.begin(); it != m_activeMusic.end();) {
		if (liveIds.find(it->first) == liveIds.end()) {
			if (it->second)
				it->second->stop(); // Stop the music if it's still playing

			// Remove the associated analysis buffer and level
			std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
			m_buffers.erase(it->first);
			m_levels.erase(it->first);
			it = m_activeMusic.erase(it);
		} else {
			++it;
		}
	}

	// Process all entities with a CMusic component
	for (const auto& entity : m_entityManager.GetEntities()) {
		if (!entity->HasComponent<CMusic>())
			continue;

		CMusic* musicComp = entity->GetComponent<CMusic>();
		if (!musicComp)
			continue;

		sf::Music* music = GetOrCreateMusic(*entity);
		if (!music)
			continue;

		music->setVolume(musicComp->volume);
		music->setLooping(musicComp->loop);

		// Apply spatial audio if the entity has a transform component
		if (auto* transform = entity->GetComponent<CTransform>()) {
			ApplySpatialAudioToMusic(*music, *musicComp, transform->m_position);
		}

		//std::cout << "Time left to play for music " << music->getDuration().asSeconds() - music->getPlayingOffset().asSeconds() << ": " << std::endl;
		// Check if music has naturally finished (stopped by reaching end, not by user)
		if (!musicComp->loop && music->getDuration().asSeconds() - music->getPlayingOffset().asSeconds() < 0.01f) {
			musicComp->state = CMusic::State::Stopped;
		}

		// Handle play/pause/stop based on the state of the CMusic component
		switch (musicComp->state) {
		case CMusic::State::Playing:
			if (music->getStatus() == sf::SoundSource::Status::Stopped) {
				try {
					std::cout << "Playing music " << std::endl;
					music->setPlayingOffset(sf::seconds(0));
				} catch (...) {}
			}
			if (music->getStatus() != sf::SoundSource::Status::Playing)
				music->play();
			break;
		case CMusic::State::Paused:
			if (music->getStatus() == sf::SoundSource::Status::Playing)
				music->pause();
			break;
		case CMusic::State::Stopped:
			if (music->getStatus() != sf::SoundSource::Status::Stopped)
				music->stop();
			break;
		}

		// Snapshot the current playhead for the analysis thread
		float seconds = 0.0f;
		try { seconds = music->getPlayingOffset().asSeconds(); } catch (...) {}
		{
			std::lock_guard<std::mutex> lk(m_playheadMutex);
			m_playheadSnapshot[entity->GetId()] = seconds;
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// AnalysisThreadFunc - runs on a dedicated thread at ~30 Hz. Reads playhead snapshots written by Process(), seeks/reads from InputSoundFile, 
// performs FFT or Goertzel analysis, and stores results in m_levels / m_spectra.
void MusicSystem::AnalysisThreadFunc() {
	using namespace std::chrono_literals;
	std::unordered_map<size_t, float> lastAnalyzedSeconds;
	std::unordered_map<size_t, long long> lastReadCursorSamples;
	while (!m_analysisStop.load(std::memory_order_relaxed)) {
		// Wait up to ~16 ms (~60 Hz) for a wake signal or timeout
		{
			std::unique_lock<std::mutex> lk(m_analysisCvMutex);
			m_analysisCv.wait_for(lk, 16ms);
		}
		if (m_analysisStop.load(std::memory_order_relaxed))
			break;

		// Snapshot the buffers and playhead map under their respective locks
		std::unordered_map<size_t, std::shared_ptr<sf::InputSoundFile>> bufSnap;
		{
			std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
			bufSnap = m_buffers;
		}
		std::unordered_map<size_t, float> playheadSnap;
		{
			std::lock_guard<std::mutex> lk(m_playheadMutex);
			playheadSnap = m_playheadSnapshot;
		}

		// Read config under the levels mutex
		int eqBandCount;
		float spectrumSmoothing;
		bool useFFT;
		int fftSize;
		
		// Make a local copy of the EQ center frequencies for use in this analysis iteration. This allows the main thread to modify the center 
		// frequencies without affecting the analysis thread until the next iteration, and avoids holding the levels mutex while performing analysis.
		std::vector<float> eqCenterFreqs;
		
		std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
		eqBandCount = m_eqBandCount;
		spectrumSmoothing = m_spectrumSmoothing;
		useFFT = m_useFFT;
		fftSize = m_fftSize;
		eqCenterFreqs = m_eqCenterFreqs;
		
		// For each active music track, read a window of audio samples around the playhead position, compute the RMS level and spectrum, and store the results. 
		// I use the playhead snapshot to know where to read from each track's sound file, while the buffer snapshot is used to access the sound files without 
		// holding locks during analysis.
		for (auto& [id, soundFile] : bufSnap) {
			if (!soundFile) continue;
			auto pit = playheadSnap.find(id);
			if (pit == playheadSnap.end()) continue;
			float seconds = pit->second;

			// If playhead hasn't moved meaningfully since last analysis, skip this pass.
			// This avoids repeated seek/read/decode work (especially noticeable on very large files or paused playback).
			auto lastIt = lastAnalyzedSeconds.find(id);
			if (lastIt != lastAnalyzedSeconds.end()) {
				if (std::fabs(seconds - lastIt->second) < 0.002f) {
					continue;
				}
			}
			lastAnalyzedSeconds[id] = seconds;

			unsigned int sampleRate = soundFile->getSampleRate();
			unsigned int channels = soundFile->getChannelCount();
			size_t totalSamples = static_cast<size_t>(soundFile->getSampleCount());

			if (sampleRate == 0 || channels == 0) continue;

			// Keep analysis window close to the analysis tick duration so decode cursor stays aligned
			// with playback progression and avoids recurring deep re-seeks on long tracks.
			const float analysisHz = 60.0f;
			size_t windowPerChannel = static_cast<size_t>(std::max(256.0f, std::round(static_cast<float>(sampleRate) / analysisHz)));
			size_t windowSamples = windowPerChannel * channels;
			if (windowSamples == 0) continue;

			// Compute analysis window around playhead
			long long center = static_cast<long long>(seconds * static_cast<float>(sampleRate)) * static_cast<long long>(channels);
			long long start = (center > static_cast<long long>(windowSamples / 2)) ? (center - static_cast<long long>(windowSamples / 2)) : 0;
			if (totalSamples > 0 && static_cast<size_t>(start) + windowSamples > totalSamples)
				start = totalSamples > windowSamples ? static_cast<long long>(totalSamples - windowSamples) : 0;
			if (start < 0) start = 0;

			// Avoid expensive deep seeks every analysis tick: read sequentially when playhead movement is continuous,
			// and only seek on startup / large jumps (e.g., user scrub).
			bool needSeek = true;
			auto cursorIt = lastReadCursorSamples.find(id);
			if (cursorIt != lastReadCursorSamples.end()) {
				long long expected = cursorIt->second;
				long long delta = std::llabs(start - expected);
				if (delta <= static_cast<long long>(windowSamples * 4)) {
					needSeek = false;
				}
			}

			if (needSeek) {
				soundFile->seek(static_cast<size_t>(start));
				lastReadCursorSamples[id] = start;
			} else if (cursorIt != lastReadCursorSamples.end() && cursorIt->second < start) {
				// Advance by reading/discarding a small amount to catch up without expensive absolute seek.
				long long toSkip = start - cursorIt->second;
				const size_t scratchCap = 8192;
				std::vector<short> scratch(static_cast<size_t>(std::min<long long>(toSkip, scratchCap)), 0);
				while (toSkip > 0) {
					size_t chunk = static_cast<size_t>(std::min<long long>(toSkip, static_cast<long long>(scratch.size())));
					size_t skipped = soundFile->read(scratch.data(), chunk);
					if (skipped == 0) break;
					toSkip -= static_cast<long long>(skipped);
					cursorIt->second += static_cast<long long>(skipped);
				}
			}

			std::vector<short> sampleBuf(windowSamples, 0);
			size_t readCount = soundFile->read(sampleBuf.data(), windowSamples);
			const short* samples = sampleBuf.data();
			lastReadCursorSamples[id] += static_cast<long long>(readCount);

			// RMS level
			double rmsSum = 0.0;
			for (size_t i = 0; i < readCount; ++i) {
				float v = static_cast<float>(samples[i]) / 32768.0f;
				rmsSum += static_cast<double>(v) * static_cast<double>(v);
			}
			float level = readCount > 0 ? static_cast<float>(rmsSum / static_cast<double>(readCount)) : 0.0f;

			// Band analysis
			std::vector<float> mags(static_cast<size_t>(eqBandCount), 0.0f);

			// FFT analysis
			if (useFFT && fftSize > 0) {
				int N = fftSize;
				std::vector<float> mono(static_cast<size_t>(N), 0.0f);
				long long centerSample = static_cast<long long>(seconds * static_cast<float>(sampleRate));
				long long startSample  = centerSample - static_cast<long long>(N / 2);
				long long bufStartSample = static_cast<long long>(start / channels);
				
				// Convert to mono
				for (int i = 0; i < N; ++i) {
					long long localIdx = (startSample + i) - bufStartSample;
					if (localIdx < 0 || static_cast<size_t>(localIdx) >= readCount / channels) continue;
					size_t base = static_cast<size_t>(localIdx) * channels;
					float acc = 0.0f;
					for (unsigned int c = 0; c < channels; ++c)
						acc += static_cast<float>(samples[base + c]) / 32768.0f;
					mono[i] = acc / static_cast<float>(channels);
				}

				// Apply Hanning window
				for (int i = 0; i < N; ++i) {
					float w = 0.5f * (1.0f - cosf(2.0f * 3.14159265358979323846f * static_cast<float>(i) / static_cast<float>(N)));
					mono[i] *= w;
				}

				// Perform FFT using kiss_fft
				int fftN = N, half = 0;
				std::vector<float> magsBins;
				kiss_fft_cfg* cfg = kiss_fft_alloc(N, 0, NULL, NULL);
				if (cfg) {
					int nfft = cfg->nfft;
					std::vector<kiss_fft_cpx> fin(nfft), fout(nfft);
					for (int i = 0; i < nfft; ++i) {
						fin[i].r = (i < N) ? mono[i] : 0.0f;
						fin[i].i = 0.0f;
					}
					kiss_fft(cfg, fin.data(), fout.data());
					kiss_fft_free(cfg);
					fftN = nfft; half = fftN / 2;
					magsBins.assign(static_cast<size_t>(half) + 1, 0.0f);
					for (int k = 0; k <= half; ++k)
						magsBins[k] = sqrtf(fout[k].r * fout[k].r + fout[k].i * fout[k].i) / static_cast<float>(fftN);
				} else {
					fftN = N; half = fftN / 2;
					magsBins.assign(static_cast<size_t>(half) + 1, 0.0f);
				}
				float nyq = static_cast<float>(sampleRate) * 0.5f;
				float minF = 20.0f; if (nyq <= minF) minF = 1.0f;
				float dbFloor = -80.0f;
				
				// Average bins into bands
				for (int b = 0; b < eqBandCount; ++b) {
					float lowF, highF;
					if (eqBandCount == 1) { lowF = minF; highF = nyq; }
					else {
						float L = log10f(minF), H = log10f(nyq);
						lowF  = powf(10.0f, L + (H - L) * (static_cast<float>(b)     / static_cast<float>(eqBandCount)));
						highF = powf(10.0f, L + (H - L) * (static_cast<float>(b + 1) / static_cast<float>(eqBandCount)));
					}
					
					// Average all FFT bins whose center frequency falls within the band's low/high range. If no bins fall within the band, 
					// use the single bin closest to the band's center frequency.
					int cnt = 0; double bsum = 0.0;
					for (int k = 0; k <= half; ++k) {
						float freq = static_cast<float>(k) * static_cast<float>(sampleRate) / static_cast<float>(fftN);
						if (freq >= lowF && freq <= highF) { bsum += magsBins[k]; cnt++; }
					}

					// Compute the average magnitude for the band, convert to dB, and normalize to [0, 1] based on a floor value. 
					// If no bins were averaged, use the single closest bin's magnitude as a fallback.
					float avg = 0.0f;
					if (cnt > 0) avg = static_cast<float>(bsum / static_cast<double>(cnt));
					else {
						float cf = sqrtf(lowF * highF);
						int k = static_cast<int>(cf * static_cast<float>(fftN) / static_cast<float>(sampleRate) + 0.5f);
						if (k < 0) k = 0; if (k > half) k = half;
						avg = magsBins[k];
					}

					// Convert to dB and normalize
					float db = 20.0f * log10f(std::max(avg, 1e-20f));
					float v  = (db - dbFloor) / (-dbFloor);
					if (!std::isfinite(v)) v = 0.0f;
					mags[b] = std::max(0.0f, std::min(1.0f, v));
				}
			} else {
				// Goertzel fallback
				if (eqCenterFreqs.empty()) {
					eqCenterFreqs = {31.0f, 62.0f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};
				}

				// Convert to mono and apply Hanning window
				const size_t N = windowPerChannel;
				std::vector<float> windowed(N, 0.0f);
				for (size_t i = 0; i < N; ++i) {
					size_t idx = i * channels;
					if (idx >= readCount) continue;
					float acc = 0.0f;
					for (unsigned int c = 0; c < channels; ++c)
						acc += static_cast<float>(samples[idx + c]) / 32768.0f;
					float w = 0.5f * (1.0f - cosf(2.0f * 3.14159265358979323846f * static_cast<float>(i) / static_cast<float>(N)));
					windowed[i] = (acc / static_cast<float>(channels)) * w;
				}

				// For each band, run a Goertzel analysis to compute the magnitude at the band's center frequency. 
				// This is less accurate than FFT binning but much cheaper to compute, and allows for arbitrary center frequencies.
				for (int b = 0; b < eqBandCount; ++b) {
					float freq = (b < (int)eqCenterFreqs.size()) ? eqCenterFreqs[b] : (1000.0f * (b + 1));
					if (freq <= 0.0f || freq >= static_cast<float>(sampleRate) * 0.5f) continue;
					float omega = 2.0f * 3.14159265358979323846f * freq / static_cast<float>(sampleRate);
					float coeff = 2.0f * cosf(omega), s1 = 0.0f, s2 = 0.0f;
					for (size_t n = 0; n < N; ++n) { float s = windowed[n] + coeff * s1 - s2; s2 = s1; s1 = s; }
					float real = s1 - s2 * cosf(omega), imag = s2 * sinf(omega);
					mags[b] = sqrtf(real * real + imag * imag) / static_cast<float>(N);
				}
				float maxv = 1e-9f;
				for (float v : mags) if (v > maxv) maxv = v;
				if (maxv > 0.0f) for (float& v : mags) v /= maxv;
			}

			// Write results under the levels mutex
			std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
			m_levels[id] = level;
			auto& store = m_spectra[id];
			if ((int)store.size() != eqBandCount) store.assign(static_cast<size_t>(eqBandCount), 0.0f);
			for (int b = 0; b < eqBandCount; ++b)
				store[b] = store[b] * spectrumSmoothing + mags[b] * (1.0f - spectrumSmoothing);
		}
	}
}
/////////////////////////////////



/////////////////////////////////
// GetOrCreateMusic - This method retrieves the sf::Music instance associated with a given entity. If an instance already exists in 
// the m_activeMusic mapping, it returns it. If not, it attempts to create a new sf::Music instance based on the file path 
// specified in the CMusic component of the entity.
sf::Music* MusicSystem::GetOrCreateMusic(Entity& entity) {
	CMusic* musicComp = entity.GetComponent<CMusic>();
	if (!musicComp)
		return nullptr;

	// If music pointer already exists, return it
	if (musicComp->m_music)
		return musicComp->m_music;

	size_t id = entity.GetId();
	auto it = m_activeMusic.find(id);
	if (it != m_activeMusic.end()) {
		// Already in map, just set the component pointer and return
		musicComp->m_music = it->second.get();
		return musicComp->m_music;
	}

	auto newMusic = std::make_unique<sf::Music>();
	sf::Music* musicPtr = newMusic.get();
	m_activeMusic.emplace(id, std::move(newMusic));

	// Store pointer in component for direct access
	musicComp->m_music = musicPtr;

	if (!musicComp->path.empty()) {
		if (!musicPtr->openFromFile(Utf8ToPath(musicComp->path))) {
			std::cerr << "MusicSystem: Failed to open audio file: " << musicComp->path << std::endl;
		} else {
			std::cout << "MusicSystem: Opened audio file: " << musicComp->path << " for entity " << id 
				<< " | Sample Rate: " << musicPtr->getSampleRate() 
				<< " | Channels: " << musicPtr->getChannelCount()
				<< " | Duration: " << musicPtr->getDuration().asSeconds() << "s" << std::endl;
		}
	}

	{
		std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
		m_levels[id] = 0.0f;
	}

	// Open sf::InputSoundFile for streaming analysis (reads only the required sample window per frame, avoids full decode into RAM)
	try {
		auto soundFile = std::make_shared<sf::InputSoundFile>();
		if (soundFile->openFromFile(Utf8ToPath(musicComp->path))) {
			std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
			m_buffers[id] = soundFile;
			std::cout << "MusicSystem: Opened sound file for analysis for entity " << id << std::endl;
		} else {
			std::cerr << "MusicSystem: Failed to open sound file for analysis: " << musicComp->path << std::endl;
		}
	} catch (...) {}

	return musicPtr;
}
/////////////////////////////////



/////////////////////////////////
// ApplySpatialAudioToMusic - Apply spatial audio (distance attenuation, panning) to music
void MusicSystem::ApplySpatialAudioToMusic(sf::Music& music, const CMusic& musicCmp, const Vec2& entityPos) {
	// Apply spatial positioning to the music track
	// Convert 2D position to 3D (z=0), relative to listener at origin
	sf::Vector3f soundPos(entityPos.x, entityPos.y, 0.0f);
	music.setPosition(soundPos);

	// Set distance parameters for attenuation curve
	music.setMinDistance(musicCmp.m_3DMinDistance);
	music.setMaxDistance(musicCmp.m_3DMaxDistance);

	// Set the volume to the component's base volume
	// SFML's spatialization will handle attenuation based on distance
	float volume = musicCmp.volume;
	volume = std::clamp(volume, 1.0f, 100.0f);
	music.setVolume(volume);

	// SFML 3D audio will automatically:
	// - Apply distance attenuation based on the music position using min/max distance
	// - Apply panning based on the horizontal offset
	// We set the position, distance parameters, and let SFML handle the rest
}
/////////////////////////////////

