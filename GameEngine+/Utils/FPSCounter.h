#pragma once

class FPSCounter
{
public:
    explicit FPSCounter(float smoothing = 0.1f) : m_smoothing(smoothing), m_smoothedFps(0.0f) {}

    void Update(float deltaSeconds)
    {
        float fps = deltaSeconds > 0.0f ? 1.0f / deltaSeconds : 0.0f;
        m_instantFps = fps;
        if (m_smoothedFps == 0.0f) m_smoothedFps = fps;
        m_smoothedFps = (1.0f - m_smoothing) * m_smoothedFps + m_smoothing * fps;
    }

    float GetFPS() const { return m_smoothedFps; }
    float GetInstantFPS() const { return m_instantFps; }
    void SetSmoothing(float s) { m_smoothing = s; }

private:
    float m_smoothing;
    float m_smoothedFps;
    float m_instantFps = 0.0f;
};
