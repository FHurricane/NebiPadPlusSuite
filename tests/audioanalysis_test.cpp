#include "audioanalysis.h"

#include <QtGlobal>
#include <algorithm>
#include <cmath>

int main()
{
    constexpr int sampleRate = 16000;
    constexpr int sampleCount = 512;
    constexpr double pi = 3.14159265358979323846;
    const auto signal = [](double frequency) {
        QVector<float> samples;
        for (int i = 0; i < sampleCount; ++i)
            samples.append(float(0.8 * std::sin(2.0 * pi * frequency * i / sampleRate)));
        return spectralLevels(samples, sampleRate);
    };
    const auto low = signal(250.0);
    const auto high = signal(4000.0);
    if (low.size() != 24 || high.size() != 24) return 1;
    const auto lowPeak = std::max_element(low.begin(), low.end()) - low.begin();
    const auto highPeak = std::max_element(high.begin(), high.end()) - high.begin();
    if (lowPeak >= highPeak || lowPeak > 10 || highPeak < 17) return 2;
    const auto silence = spectralLevels(QVector<float>(sampleCount, 0.0f), sampleRate);
    if (std::any_of(silence.begin(), silence.end(), [](float value) { return value != 0.0f; })) return 3;
    return 0;
}
