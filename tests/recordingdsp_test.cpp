#include "recordingdsp.h"

#include <algorithm>
#include <cmath>

int main()
{
    RecordingDsp dsp;
    dsp.reset(2, 48000);
    dsp.setInputGain(1.0f);

    // A deliberately overloaded transient must stay below the WAV ceiling,
    // keep its polarity and preserve the stereo ratio through linked limiting.
    float transient[2] = {2.4f, -1.2f};
    dsp.processFrame(transient, 2);
    if (std::abs(transient[0]) > 0.891f || std::abs(transient[1]) > 0.891f) return 1;
    if (transient[0] <= 0.0f || transient[1] >= 0.0f) return 2;
    if (std::abs(transient[0]) <= std::abs(transient[1])) return 3;

    // Sustained overload may not create flat-topped samples (hard clipping).
    float previous = 0.0f;
    int equalPeaks = 0;
    for (int i = 0; i < 4800; ++i) {
        float frame[2] = {
            1.8f * std::sin(float(i) * 0.071f),
            1.8f * std::sin(float(i) * 0.071f)
        };
        dsp.processFrame(frame, 2);
        if (std::abs(frame[0]) > 0.891f) return 4;
        if (std::abs(frame[0] - previous) < 1.0e-7f && std::abs(frame[0]) > 0.88f)
            ++equalPeaks;
        previous = frame[0];
    }
    if (equalPeaks > 2) return 5;

    // Non-finite backend input must never reach the PCM encoder.
    float invalid[2] = {NAN, INFINITY};
    dsp.processFrame(invalid, 2);
    if (!std::isfinite(invalid[0]) || !std::isfinite(invalid[1])) return 6;
    return 0;
}
