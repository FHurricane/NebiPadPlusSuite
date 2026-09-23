#pragma once

#include <vector>

// Stateful capture processing. It removes microphone DC offset and applies a
// linked peak limiter, so loud transients cannot turn into hard digital clips.
class RecordingDsp
{
public:
    void reset(int channels, int sampleRate);
    void setInputGain(float gain);
    void processFrame(float *samples, int channels) noexcept;
    float limiterGain() const noexcept { return limiterGain_; }

private:
    std::vector<float> previousInput_;
    std::vector<float> previousOutput_;
    float inputGain_ = 0.75f;
    float limiterGain_ = 1.0f;
    float releaseCoefficient_ = 0.9995f;
};
