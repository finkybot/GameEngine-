/////////////////////////////////
// FPSCounter.h
/////////////////////////////////



/////////////////////////////////
// Includes, there are no includes
#pragma once
/////////////////////////////////



/////////////////////////////////
// FPSCounter class - utility for tracking and smoothing frames per second (FPS) in the game engine. It calculates the instantaneous FPS based on the time elapsed between 
// frames and applies an exponential moving average to smooth out fluctuations in FPS for a more stable display. The smoothing factor can be adjusted to control how quickly the smoothed FPS responds to changes in the instantaneous FPS.
//								|
//								|_______________________________________________________________________
class FPSCounter {
	/////////////////////////////////
	// Public interface for the FPSCounter class
public:
	/////////////////////////////////
	// Constructor for the FPSCounter class. It takes an optional smoothing factor parameter that controls how much the smoothed FPS value is influenced by the instantaneous FPS. 
	// Enforce Explicit to prevent unintended implicit conversions when creating an FPSCounter instance with a single float argument for smoothing.
	explicit FPSCounter(float smoothing = 0.1f) : m_smoothing(smoothing), m_smoothedFps(0.0f) {}
	/////////////////////////////////



	/////////////////////////////////
	// Update - updates the FPS counter with the time elapsed since the last frame (deltaSeconds). It calculates the instantaneous FPS based on the delta time and updates the smoothed FPS using an exponential moving average.
	void Update(float deltaSeconds) {
		float fps = deltaSeconds > 0.0f ? 1.0f / deltaSeconds : 0.0f;
		m_instantFps = fps;
		if (m_smoothedFps == 0.0f)
			m_smoothedFps = fps;
		m_smoothedFps = (1.0f - m_smoothing) * m_smoothedFps + m_smoothing * fps;
	}
	/////////////////////////////////



	/////////////////////////////////
	// Accessor methods for retrieving the smoothed FPS and instantaneous FPS values, as well as setting the smoothing factor. GetFPS returns the smoothed FPS value, while GetInstantFPS returns the most recently calculated 
	// instantaneous FPS. SetSmoothing allows adjusting the smoothing factor to control how responsive the smoothed FPS is to changes in the instantaneous FPS.
	float GetFPS() const { return m_smoothedFps; }
	float GetInstantFPS() const { return m_instantFps; }
	void SetSmoothing(float s) { m_smoothing = s; }
	/////////////////////////////////



	/////////////////////////////////
	// Private member variables for the FPSCounter class, including the smoothing factor, smoothed FPS value, and instantaneous FPS value. The smoothing factor controls how much the smoothed FPS is influenced by the instantaneous FPS, 
	// while the smoothed FPS value is the result of applying the exponential moving average to the instantaneous FPS values over time.
private:
	float m_smoothing;
	float m_smoothedFps;
	float m_instantFps = 0.0f;
	/////////////////////////////////
};
/////////////////////////////////
