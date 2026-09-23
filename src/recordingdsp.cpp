#include "recordingdsp.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float limiterCeiling = 0.89f; // about -1 dBFS headroom
constexpr float limiterKnee = 0.78f;
constexpr float dcPole = 0.995f;

float softCeiling(float value) noexcept
{
    const float magnitude = std::abs(value);
    if (magnitude <= limiterKnee) return value;
    const float range = limiterCeiling - limiterKnee;
    const float curved = limiterKnee + range *
        (1.0f - std::exp(-(magnitude - limiterKnee) / range));
    return std::copysign(std::min(curved, limiterCeiling), value);
}
}

void RecordingDsp::reset(int channels, int sampleRate)
{
    previousInput_.assign(std::max(channels, 1), 0.0f);
    previousOutput_.assign(std::max(channels, 1), 0.0f);
    limiterGain_ = 1.0f;
    // 80 ms release avoids pumping while recovering smoothly after a peak.
    releaseCoefficient_ = sampleRate > 0
        ? std::exp(-1.0f / (0.080f * float(sampleRate))) : 0.9995f;
}

void RecordingDsp::setInputGain(float gain)
{
    inputGain_ = std::clamp(gain, 0.1f, 1.0f);
}

void RecordingDsp::processFrame(float *samples, int channels) noexcept
{
    if (!samples || channels <= 0) return;
    if (int(previousInput_.size()) != channels) return;

    float peak = 0.0f;
    for (int channel = 0; channel < channels; ++channel) {
        const float input = std::isfinite(samples[channel]) ? samples[channel] : 0.0f;
        float filtered = input - previousInput_[channel] + dcPole * previousOutput_[channel];
        previousInput_[channel] = input;
        previousOutput_[channel] = filtered;
        filtered *= inputGain_;
        samples[channel] = filtered;
        peak = std::max(peak, std::abs(filtered));
    }

    const float requiredGain = peak > limiterCeiling ? limiterCeiling / peak : 1.0f;
    if (requiredGain < limiterGain_)
        limiterGain_ += (requiredGain - limiterGain_) * 0.45f;
    else
        limiterGain_ = 1.0f - (1.0f - limiterGain_) * releaseCoefficient_;

    for (int channel = 0; channel < channels; ++channel)
        samples[channel] = softCeiling(samples[channel] * limiterGain_);
}
