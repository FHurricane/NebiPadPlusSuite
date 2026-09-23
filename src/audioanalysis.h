#pragma once

#include <QVector>

// Returns 24 logarithmically spaced bands from low (left) to high (right).
QVector<float> spectralLevels(const QVector<float> &samples, int sampleRate = 16000);
