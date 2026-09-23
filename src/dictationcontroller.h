#pragma once

#include "speechbackend.h"

#include <QAudioFormat>
#include <QObject>
#include <QVector>

#include <functional>
#include <memory>

class QAudioSource;
class QIODevice;
class AudioCaptureDevice;

class DictationController final : public QObject
{
public:
    enum class State { Idle, Preparing, Listening, Processing };

    explicit DictationController(QObject *parent = nullptr);
    ~DictationController() override;
    State state() const { return state_; }
    void toggle();
    void stop();
    void setCallbacks(std::function<void(State)> stateChanged,
                      std::function<void(const QVector<float> &)> levelsChanged,
                      std::function<void(const QString &)> partialChanged,
                      std::function<void(const QString &)> completed,
                      std::function<void(const QString &)> failed);

private:
    State state_ = State::Idle;
    QAudioFormat inputFormat_;
    std::unique_ptr<QAudioSource> audioSource_;
    std::unique_ptr<AudioCaptureDevice> audioDevice_;
    std::unique_ptr<SpeechBackend> backend_;
    QByteArray pendingInput_;
    QVector<float> analysisSamples_;
    double resamplePhase_ = 0.0;
    int generation_ = 0;
    qint64 capturedBytes_ = 0;
    std::function<void(State)> stateChanged_;
    std::function<void(const QVector<float> &)> levelsChanged_;
    std::function<void(const QString &)> partialChanged_;
    std::function<void(const QString &)> completed_;
    std::function<void(const QString &)> failed_;

    void setState(State state);
    void startCapture();
    void stopCapture();
    void processAudio(const QByteArray &bytes);
    void fail(const QString &message);
    void finish(const QString &text);
};
