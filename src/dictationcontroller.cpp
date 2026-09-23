#include "dictationcontroller.h"
#include "audioanalysis.h"

#include <QAudioDevice>
#include <QAudioSource>
#include <QIODevice>
#include <QMediaDevices>
#include <QThread>
#include <QTimer>

#include <cmath>
#include <cstring>

class AudioCaptureDevice final : public QIODevice
{
public:
    explicit AudioCaptureDevice(std::function<void(const QByteArray &)> received, QObject *parent = nullptr)
        : QIODevice(parent), received_(std::move(received))
    {
        open(QIODevice::WriteOnly);
    }

    bool isSequential() const override { return true; }

protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *data, qint64 size) override
    {
        if (size <= 0) return 0;
        const QByteArray copy(data, qsizetype(size));
        // Keep conversion, FFT and speech streaming out of the audio callback.
        QMetaObject::invokeMethod(this, [this, copy] { received_(copy); }, Qt::QueuedConnection);
        return size;
    }

private:
    std::function<void(const QByteArray &)> received_;
};

DictationController::DictationController(QObject *parent)
    : QObject(parent), backend_(createSpeechBackend(this)) {}

DictationController::~DictationController()
{
    stopCapture();
    backend_->cancel();
}

void DictationController::setCallbacks(std::function<void(State)> stateChanged,
                                      std::function<void(const QVector<float> &)> levelsChanged,
                                      std::function<void(const QString &)> partialChanged,
                                      std::function<void(const QString &)> completed,
                                      std::function<void(const QString &)> failed)
{
    stateChanged_ = std::move(stateChanged);
    levelsChanged_ = std::move(levelsChanged);
    partialChanged_ = std::move(partialChanged);
    completed_ = std::move(completed);
    failed_ = std::move(failed);
}

void DictationController::setState(State state)
{
    if (state_ == state) return;
    state_ = state;
    if (stateChanged_) stateChanged_(state);
}

void DictationController::toggle()
{
    if (state_ == State::Listening || state_ == State::Preparing) {
        stop();
        return;
    }
    if (state_ != State::Idle) return;
    const QAudioDevice microphone = QMediaDevices::defaultAudioInput();
    if (microphone.isNull()) {
        fail(QStringLiteral("Nessun microfono disponibile."));
        return;
    }
    ++generation_;
    pendingInput_.clear();
    analysisSamples_.clear();
    resamplePhase_ = 0.0;
    capturedBytes_ = 0;
    setState(State::Preparing);
    backend_->start({
        [this] { if (state_ == State::Preparing) startCapture(); },
        [this](const QString &text) { if (partialChanged_) partialChanged_(text); },
        [this](const QString &text) { finish(text); },
        [this](const QString &message) { fail(message); }
    });
}

void DictationController::startCapture()
{
    const QAudioDevice microphone = QMediaDevices::defaultAudioInput();
    if (microphone.isNull()) { fail(QStringLiteral("Il microfono non è più disponibile.")); return; }
    QAudioFormat preferred;
    preferred.setSampleRate(16000);
    preferred.setChannelCount(1);
    preferred.setSampleFormat(QAudioFormat::Int16);
    inputFormat_ = microphone.isFormatSupported(preferred) ? preferred : microphone.preferredFormat();
    if (inputFormat_.sampleRate() <= 0 || inputFormat_.channelCount() <= 0) {
        fail(QStringLiteral("Formato audio del microfono non valido."));
        return;
    }
    audioSource_ = std::make_unique<QAudioSource>(microphone, inputFormat_);
    audioSource_->setBufferSize(inputFormat_.bytesForDuration(500000));
    audioDevice_ = std::make_unique<AudioCaptureDevice>([this](const QByteArray &bytes) {
        if (state_ == State::Listening) {
            capturedBytes_ += bytes.size();
            processAudio(bytes);
        }
    });
    audioSource_->start(audioDevice_.get());
    if (audioSource_->error() != QAudio::NoError) {
        fail(QStringLiteral("Impossibile avviare il microfono."));
        return;
    }
    connect(audioSource_.get(), &QAudioSource::stateChanged, this, [this](auto state) {
        if (state_ == State::Listening && state == QAudio::StoppedState &&
            audioSource_ && audioSource_->error() != QAudio::NoError)
            QTimer::singleShot(0, this, [this] {
                if (state_ == State::Listening)
                    fail(QStringLiteral("Il microfono si è interrotto."));
            });
    });
    setState(State::Listening);
    const int currentGeneration = generation_;
    QTimer::singleShot(2500, this, [this, currentGeneration] {
        if (currentGeneration == generation_ && state_ == State::Listening && capturedBytes_ == 0)
            fail(QStringLiteral("Il microfono non sta inviando audio. Controlla i permessi del microfono e il dispositivo di ingresso nelle impostazioni di sistema."));
    });
}

void DictationController::processAudio(const QByteArray &bytes)
{
    pendingInput_ += bytes;
    int bytesPerSample = 0;
    switch (inputFormat_.sampleFormat()) {
    case QAudioFormat::UInt8: bytesPerSample = 1; break;
    case QAudioFormat::Int16: bytesPerSample = 2; break;
    case QAudioFormat::Float: bytesPerSample = 4; break;
    default: fail(QStringLiteral("Formato audio del microfono non supportato.")); return;
    }
    const int frameSize = bytesPerSample * inputFormat_.channelCount();
    const int frames = pendingInput_.size() / frameSize;
    QByteArray pcm16;
    pcm16.reserve(frames * 2);
    const unsigned char *data = reinterpret_cast<const unsigned char *>(pendingInput_.constData());
    for (int frame = 0; frame < frames; ++frame) {
        float mono = 0.0f;
        for (int channel = 0; channel < inputFormat_.channelCount(); ++channel) {
            const unsigned char *sample = data + frame * frameSize + channel * bytesPerSample;
            float value = 0.0f;
            if (inputFormat_.sampleFormat() == QAudioFormat::UInt8)
                value = (int(sample[0]) - 128) / 128.0f;
            else if (inputFormat_.sampleFormat() == QAudioFormat::Int16) {
                const quint16 bits = quint16(sample[0]) | (quint16(sample[1]) << 8);
                value = qint16(bits) / 32768.0f;
            } else {
                std::memcpy(&value, sample, sizeof(value));
            }
            mono += value;
        }
        mono = qBound(-1.0f, mono / inputFormat_.channelCount(), 1.0f);
        resamplePhase_ += 16000.0 / inputFormat_.sampleRate();
        while (resamplePhase_ >= 1.0) {
            resamplePhase_ -= 1.0;
            const qint16 signedSample = qint16(std::lround(mono * 32767.0f));
            const quint16 bits = quint16(signedSample);
            pcm16.append(char(bits & 0xff));
            pcm16.append(char((bits >> 8) & 0xff));
            analysisSamples_.append(mono);
            if (analysisSamples_.size() >= 512) {
                if (levelsChanged_) levelsChanged_(spectralLevels(analysisSamples_));
                analysisSamples_.clear();
            }
        }
    }
    pendingInput_.remove(0, frames * frameSize);
    if (!pcm16.isEmpty()) backend_->appendAudio(pcm16);
}

void DictationController::stopCapture()
{
    if (audioSource_) {
        audioSource_->stop();
        audioSource_.reset();
    }
    audioDevice_.reset();
    pendingInput_.clear();
    analysisSamples_.clear();
}

void DictationController::stop()
{
    if (state_ == State::Preparing) {
        backend_->cancel();
        setState(State::Idle);
        return;
    }
    if (state_ != State::Listening) return;
    stopCapture();
    setState(State::Processing);
    backend_->finish();
    const int currentGeneration = generation_;
    QTimer::singleShot(30000, this, [this, currentGeneration] {
        if (currentGeneration == generation_ && state_ == State::Processing)
            fail(QStringLiteral("La trascrizione ha impiegato troppo tempo."));
    });
}

void DictationController::finish(const QString &text)
{
    if (state_ == State::Idle) return;
    ++generation_;
    stopCapture();
    setState(State::Idle);
    if (completed_) completed_(text);
}

void DictationController::fail(const QString &message)
{
    ++generation_;
    stopCapture();
    backend_->cancel();
    setState(State::Idle);
    if (failed_) failed_(message);
}
