#include "audioanalysis.h"

#include <QtGlobal>
#include <cmath>

QVector<float> spectralLevels(const QVector<float> &samples, int sampleRate)
{
    constexpr int count = 24;
    QVector<float> levels(count, 0.0f);
    if (samples.size() < 64 || sampleRate <= 0) return levels;
    const int length = qMin(samples.size(), 512);
    const int offset = samples.size() - length;
    constexpr double pi = 3.14159265358979323846;
    for (int band = 0; band < count; ++band) {
        const double fraction = double(band) / (count - 1);
        const double frequency = 80.0 * std::pow(7500.0 / 80.0, fraction);
        const int bin = qBound(1, int(std::round(frequency * length / sampleRate)), length / 2 - 1);
        const double increment = 2.0 * pi * bin / length;
        double real = 0.0;
        double imaginary = 0.0;
        for (int i = 0; i < length; ++i) {
            const double window = 0.5 - 0.5 * std::cos(2.0 * pi * i / (length - 1));
            const double value = samples[offset + i] * window;
            real += value * std::cos(increment * i);
            imaginary -= value * std::sin(increment * i);
        }
        const double amplitude = 2.0 * std::hypot(real, imaginary) / length;
        levels[band] = float(qBound(0.0, std::sqrt(amplitude) * 1.8, 1.0));
    }
    return levels;
}
