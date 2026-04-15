// MusicSystem.cpp : Implementation of the MusicSystem class for music playback management.
#include "Entity.h"
#include "MusicSystem.h"
#include "EntityManager.h"
#include "CMusic.h"
#include "DebugStack.h"

#include <SFML/Audio.hpp>
#include <iostream>
#include <cmath>
#include <cstring>
#include <unordered_set>
#include <complex>
#include <algorithm>
#include <vector>

MusicSystem::MusicSystem(EntityManager& entityManager) : m_entityManager(entityManager) {}

// Simple in-place radix-2 Cooley-Tukey FFT (complex)
static void fft_inplace(std::vector<std::complex<float>>& a) {
	const size_t n = a.size();
	if (n <= 1) return;
	// bit-reverse permutation
	size_t j = 0;
	for (size_t i = 1; i < n; ++i) {
		size_t bit = n >> 1;
		for (; j & bit; bit >>= 1) j ^= bit;
		j ^= bit;
		if (i < j) std::swap(a[i], a[j]);
	}
	// FFT
	for (size_t len = 2; len <= n; len <<= 1) {
		float angle = -2.0f * 3.14159265358979323846f / static_cast<float>(len);
		std::complex<float> wlen(std::cos(angle), std::sin(angle));
		for (size_t i = 0; i < n; i += len) {
			std::complex<float> w(1.0f, 0.0f);
			for (size_t j = 0; j < len/2; ++j) {
				std::complex<float> u = a[i + j];
				std::complex<float> v = a[i + j + len/2] * w;
				a[i + j] = u + v;
				a[i + j + len/2] = u - v;
				w *= wlen;
			}
		}
	}
}

void MusicSystem::StopAllMusic() {
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

bool MusicSystem::GetSpectrum(size_t entityId, std::vector<float>& outSpectrum) const {
	std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	auto it = m_spectra.find(entityId);
	if (it == m_spectra.end()) return false;
	outSpectrum = it->second;
	return true;
}

size_t MusicSystem::GetSpectrumBandCount() const {
	return static_cast<size_t>(m_eqBandCount);
}

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

void MusicSystem::SetSpectrumSmoothing(float smoothing) {
	if (smoothing < 0.0f) smoothing = 0.0f;
	if (smoothing > 0.99f) smoothing = 0.99f;
                        std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	m_spectrumSmoothing = smoothing;
}

float MusicSystem::GetSpectrumSmoothing() const {
            std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	return m_spectrumSmoothing;
}

void MusicSystem::SetUseFFT(bool useFFT) {
	std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	m_useFFT = useFFT;
}

bool MusicSystem::GetUseFFT() const {
	std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	return m_useFFT;
}

void MusicSystem::SetFFTSize(int size) {
	if (size <= 0) return;
	// ensure power of two
	int p = 1;
	while (p < size) p <<= 1;
	std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	m_fftSize = p;
}

int MusicSystem::GetFFTSize() const {
	std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	return m_fftSize;
}


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

void MusicSystem::Seek(size_t entityId, float seconds) {
	auto it = m_activeMusic.find(entityId);
	if (it == m_activeMusic.end() || !it->second)
		return;
	try {
		it->second->setPlayingOffset(sf::seconds(seconds));
	} catch (...) {}
}

MusicSystem::~MusicSystem() {
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

float MusicSystem::GetLevel(size_t entityId) const {
        std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	auto it = m_levels.find(entityId);
	if (it == m_levels.end())
		return 0.0f;
	return it->second;
}

bool MusicSystem::HasAnalysisBuffer(size_t entityId) const {
        std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
	return m_buffers.find(entityId) != m_buffers.end();
}

void MusicSystem::Update(float deltaSeconds) {}

// Process is called every frame to update music playback and perform offline analysis
// for each entity with a CMusic component.
void MusicSystem::Process() {
	// Clean up active music instances for entities that no longer have a CMusic component
	std::unordered_set<size_t> liveIds;
	for (const auto& entity : m_entityManager.getEntities()) {
		if (entity->HasComponent<CMusic>())
			liveIds.insert(entity->GetId());
	}

	// Erase active music entries that are no longer live
	for (auto it = m_activeMusic.begin(); it != m_activeMusic.end();) {
		if (liveIds.find(it->first) == liveIds.end()) {
			if (it->second)
				it->second->stop();
			{
    std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
				m_buffers.erase(it->first);
				m_levels.erase(it->first);
			}
			it = m_activeMusic.erase(it);
		} else {
			++it;
		}
	}

	// Process all entities with a CMusic component
	for (const auto& entity : m_entityManager.getEntities()) {
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

		// Check if music has naturally finished (stopped by reaching end, not by user)
		if (music->getStatus() == sf::SoundSource::Status::Stopped && musicComp->state == CMusic::State::Playing &&
			!musicComp->loop) {
			musicComp->state = CMusic::State::Stopped;
		}

		// Handle play/pause/stop based on the state of the CMusic component
		switch (musicComp->state) {
		case CMusic::State::Playing:
			if (music->getStatus() == sf::SoundSource::Status::Stopped) {
				try {
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

		// Offline analysis: compute short-window mean-square around current play position
		size_t id = entity->GetId();
		std::shared_ptr<sf::SoundBuffer> buf;
		{
            std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
			auto itb = m_buffers.find(id);
			if (itb != m_buffers.end())
				buf = itb->second;
		}

		if (!buf)
			continue;

		float seconds = 0.0f;
		try {
			seconds = music->getPlayingOffset().asSeconds();
		} catch (...) {
			seconds = 0.0f;
		}

		unsigned int sampleRate = buf->getSampleRate();
		unsigned int channels = buf->getChannelCount();
		const short* samples = buf->getSamples();
		size_t totalSamples = static_cast<size_t>(buf->getSampleCount());

		size_t center = static_cast<size_t>(seconds * static_cast<float>(sampleRate)) * channels;
		size_t windowPerChannel = 1024;
		size_t windowSamples = windowPerChannel * channels;

		if (windowSamples == 0 || !samples)
			continue;

		size_t start = (center > windowSamples / 2) ? (center - windowSamples / 2) : 0;
		if (start + windowSamples > totalSamples) {
			if (totalSamples > windowSamples)
				start = totalSamples - windowSamples;
			else
				start = 0;
		}

		double sum = 0.0;
		for (size_t i = 0; i < windowSamples && (start + i) < totalSamples; ++i) {
			float v = static_cast<float>(samples[start + i]) / 32768.0f;
			sum += static_cast<double>(v) * static_cast<double>(v);
		}

		double mean = 0.0;
		if (windowSamples > 0)
			mean = sum / static_cast<double>(windowSamples);

        std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
		m_levels[id] = static_cast<float>(mean);

   // --- Band analysis: FFT-based (preferred) or fallback Goertzel ---
	std::vector<float> mags;
	mags.resize((size_t)m_eqBandCount);
	if (m_useFFT && m_fftSize > 0) {
		int N = m_fftSize;
		// prepare mono buffer of size N centered at playhead
		std::vector<float> mono;
		mono.resize((size_t)N);
		long long centerSample = static_cast<long long>(seconds * static_cast<float>(sampleRate));
		long long startSample = centerSample - static_cast<long long>(N / 2);
		for (int i = 0; i < N; ++i) {
			long long sidx = startSample + i;
			if (sidx < 0 || static_cast<size_t>(sidx) >= static_cast<size_t>(totalSamples / channels)) {
				mono[i] = 0.0f;
				continue;
			}
			size_t base = static_cast<size_t>(sidx) * channels;
			float acc = 0.0f;
			for (unsigned int c = 0; c < channels; ++c) {
				acc += static_cast<float>(samples[base + c]) / 32768.0f;
			}
			mono[i] = acc / static_cast<float>(channels);
		}
		// window (Hann)
		for (int i = 0; i < N; ++i) {
			float w = 0.5f * (1.0f - cosf(2.0f * 3.14159265358979323846f * static_cast<float>(i) / static_cast<float>(N)));
			mono[i] *= w;
		}
		// complex buffer
		std::vector<std::complex<float>> a;
		a.resize((size_t)N);
		for (int i = 0; i < N; ++i) a[i] = std::complex<float>(mono[i], 0.0f);
		fft_inplace(a);
		int half = N/2;
		std::vector<float> magsBins((size_t)half + 1);
		for (int k = 0; k <= half; ++k) {
			float mag = std::abs(a[k]) / static_cast<float>(N);
			magsBins[k] = mag;
		}
		float nyq = static_cast<float>(sampleRate) * 0.5f;
		float dbFloor = -80.0f;
		// map bands using log spacing between 20Hz and nyquist
		float minF = 20.0f;
		if (nyq <= minF) minF = 1.0f;
		for (int b = 0; b < m_eqBandCount; ++b) {
			float lowF, highF;
			if (m_eqBandCount == 1) {
				lowF = minF; highF = nyq;
			} else {
				float L = log10f(minF);
				float H = log10f(nyq);
				float lb = L + (H - L) * (static_cast<float>(b) / static_cast<float>(m_eqBandCount));
				float hb = L + (H - L) * (static_cast<float>(b + 1) / static_cast<float>(m_eqBandCount));
				lowF = powf(10.0f, lb);
				highF = powf(10.0f, hb);
			}
          int count = 0;
			double sum = 0.0;
			for (int k = 0; k <= half; ++k) {
				float freq = static_cast<float>(k) * static_cast<float>(sampleRate) / static_cast<float>(N);
				// include upper boundary to avoid gaps between adjacent bands
				if (freq >= lowF && freq <= highF) {
					sum += magsBins[k];
					count++;
				}
			}
			float avg = 0.0f;
			if (count > 0) {
				avg = static_cast<float>(sum / static_cast<double>(count));
			} else {
				// No FFT bins fell into this band's range (can happen for very narrow bands or low FFT resolution)
				// Fallback: sample nearest FFT bin at the band's center frequency
				float centerF = sqrtf(lowF * highF);
				int k = static_cast<int>(centerF * static_cast<float>(N) / static_cast<float>(sampleRate) + 0.5f);
				if (k < 0) k = 0;
				if (k > half) k = half;
				avg = magsBins[k];
			}
			// convert to dB and normalize
			float db = 20.0f * log10f(std::max(avg, 1e-20f));
			float v = (db - dbFloor) / (-dbFloor);
			if (!std::isfinite(v)) v = 0.0f;
			if (v < 0.0f) v = 0.0f;
			if (v > 1.0f) v = 1.0f;
			mags[b] = v;
		}
	} else {
		// fallback: existing Goertzel-based approach (keep previous behavior)
		const size_t N = windowPerChannel; // number of samples per channel
		std::vector<float> mono;
		mono.reserve(N);
		for (size_t i = 0; i < N; ++i) {
			size_t idx = start + i * channels;
			if (idx >= totalSamples) {
				mono.push_back(0.0f);
				continue;
			}
			float acc = 0.0f;
			for (unsigned int c = 0; c < channels; ++c) {
				acc += static_cast<float>(samples[idx + c]) / 32768.0f;
			}
			mono.push_back(acc / static_cast<float>(channels));
		}

		// Initialize default center freqs if needed
		if (m_eqCenterFreqs.empty()) {
			m_eqCenterFreqs = {31.0f, 62.0f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f};
		}

		// Build windowed samples (Hann)
		std::vector<float> windowed(N);
		for (size_t n = 0; n < N; ++n) {
			float w = 0.5f * (1.0f - cosf(2.0f * 3.14159265358979323846f * static_cast<float>(n) / static_cast<float>(N)));
			windowed[n] = mono[n] * w;
		}

		// Compute Goertzel per band
		for (int b = 0; b < m_eqBandCount; ++b) {
			float freq = (b < (int)m_eqCenterFreqs.size()) ? m_eqCenterFreqs[b] : (1000.0f * (b + 1));
			if (freq <= 0.0f || freq >= static_cast<float>(sampleRate) * 0.5f) {
				mags[b] = 0.0f;
				continue;
			}
			float omega = 2.0f * 3.14159265358979323846f * freq / static_cast<float>(sampleRate);
			float coeff = 2.0f * cosf(omega);
			float s_prev = 0.0f, s_prev2 = 0.0f;
			for (size_t n = 0; n < N; ++n) {
				float s = windowed[n] + coeff * s_prev - s_prev2;
				s_prev2 = s_prev;
				s_prev = s;
			}
			float real = s_prev - s_prev2 * cosf(omega);
			float imag = s_prev2 * sinf(omega);
			float power = real * real + imag * imag;
			float magnitude = sqrtf(power) / static_cast<float>(N);
			mags[b] = magnitude;
		}
		// normalize
		float maxv = 1e-9f;
		for (float v : mags) if (v > maxv) maxv = v;
		if (maxv > 0.0f) {
			for (float &v : mags) v = v / maxv;
		}
	}

	// Store into m_spectra with smoothing
	{
		std::lock_guard<std::recursive_mutex> lk2(m_levelsMutex);
		auto &store = m_spectra[id];
		if ((int)store.size() != m_eqBandCount) store.assign(m_eqBandCount, 0.0f);
		for (int b = 0; b < m_eqBandCount; ++b) {
			float prev = store[b];
			float nv = mags[b];
			store[b] = prev * m_spectrumSmoothing + nv * (1.0f - m_spectrumSmoothing);
		}
	}
	}
}

sf::Music* MusicSystem::GetOrCreateMusic(Entity& entity) {
	CMusic* musicComp = entity.GetComponent<CMusic>();
	if (!musicComp)
		return nullptr;

	size_t id = entity.GetId();
	auto it = m_activeMusic.find(id);
	if (it != m_activeMusic.end())
		return it->second.get();

	auto newMusic = std::make_unique<sf::Music>();
	sf::Music* musicPtr = newMusic.get();
	m_activeMusic.emplace(id, std::move(newMusic));

	if (!musicComp->path.empty()) {
		if (!musicPtr->openFromFile(musicComp->path)) {
			std::cerr << "MusicSystem: Failed to open audio file: " << musicComp->path << std::endl;
		} else {
			std::cout << "MusicSystem: Opened audio file: " << musicComp->path << " for entity " << id << std::endl;
		}
	}

	{
        std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
		m_levels[id] = 0.0f;
	}

	// Attempt to load sf::SoundBuffer for offline analysis (may fail for large files)
	try {
		auto buf = std::make_shared<sf::SoundBuffer>();
		if (buf->loadFromFile(musicComp->path)) {
        std::lock_guard<std::recursive_mutex> lk(m_levelsMutex);
			m_buffers[id] = buf;
			std::cout << "MusicSystem: Loaded buffer for analysis for entity " << id << std::endl;
		} else {
			std::cerr << "MusicSystem: Failed to load buffer for analysis: " << musicComp->path << std::endl;
		}
	} catch (...) {}

	return musicPtr;
}
