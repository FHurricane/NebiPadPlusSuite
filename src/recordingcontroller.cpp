#include "recordingcontroller.h"
#include "audioanalysis.h"
#include "audioringbuffer.h"
#include "recordingdsp.h"

#include <QAudioDevice>
#include <QAudioOutput>
#include <QAudioSource>
#include <QDataStream>
#include <QFile>
#include <QFileInfo>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QTimer>
#include <QThread>
#include <QUrl>
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
#include <QSpan>
#endif

#include <cmath>
#include <cstring>
#include <cstdint>
#include <chrono>

#if defined(Q_OS_MACOS)
#include <pthread.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <pthread.h>
#include <sched.h>
#endif

class RecordingController::CaptureDevice final : public QIODevice
{
public:
    explicit CaptureDevice(std::function<void(const char *, std::size_t)> callback)
        : callback_(std::move(callback)) { open(QIODevice::WriteOnly); }
    bool isSequential() const override { return true; }
protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *data, qint64 size) override
    {
        if (size <= 0) return 0;
        callback_(data, std::size_t(size));
        return size;
    }
private:
    std::function<void(const char *, std::size_t)> callback_;
};

RecordingController::RecordingController(QObject *parent) : QObject(parent),
    player_(new QMediaPlayer(this)), output_(new QAudioOutput(this))
{
    player_->setAudioOutput(output_);
    output_->setVolume(0.8);
    analysisTimer_.setInterval(40);
    connect(&analysisTimer_, &QTimer::timeout, this, [this] {
        if (writeFailed_.exchange(false, std::memory_order_acq_rel)) {
            stop();
            if (error_) error_(tr("La registrazione audio si è interrotta: impossibile scrivere il file."));
            return;
        }
        QVector<float> snapshot;
        {
            const std::lock_guard<std::mutex> lock(analysisMutex_);
            snapshot = analysisSamples_;
        }
        if (recording_.load(std::memory_order_acquire) && snapshot.size() >= 512 && levelsChanged_)
            levelsChanged_(spectralLevels(snapshot, format_.sampleRate()));
    });
    connect(player_, &QMediaPlayer::errorOccurred, this,
            [this](QMediaPlayer::Error, const QString &message) {
        if (error_ && !message.isEmpty()) error_(tr("Riproduzione audio: %1").arg(message));
    });
}

RecordingController::~RecordingController() { stop(); }

void RecordingController::writeHeader(quint32 dataBytes)
{
    if (!file_) return;
    file_->seek(0);
    QDataStream stream(file_.get());
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.writeRawData("RIFF", 4);
    stream << quint32(36 + dataBytes);
    stream.writeRawData("WAVEfmt ", 8);
    stream << quint32(16) << quint16(1) << quint16(format_.channelCount());
    stream << quint32(format_.sampleRate());
    const quint32 byteRate = quint32(format_.sampleRate() * format_.channelCount() * 2);
    stream << byteRate << quint16(format_.channelCount() * 2) << quint16(16);
    stream.writeRawData("data", 4);
    stream << dataBytes;
}

bool RecordingController::start(const QString &path, QString *error, bool *storageError)
{
    if (storageError) *storageError = false;
    if (recording_.load(std::memory_order_acquire)) { if (error) *error = tr("Una registrazione è già in corso."); return false; }
    const QAudioDevice microphone = QMediaDevices::defaultAudioInput();
    if (microphone.isNull()) { if (error) *error = tr("Nessun microfono disponibile."); return false; }
    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    if (!microphone.isFormatSupported(format)) {
        format.setSampleRate(microphone.preferredFormat().sampleRate());
        format.setChannelCount(microphone.preferredFormat().channelCount());
    }
    if (!microphone.isFormatSupported(format)) format = microphone.preferredFormat();
    if (format.sampleRate() <= 0 || format.channelCount() <= 0 ||
        (format.sampleFormat() != QAudioFormat::Int16 &&
         format.sampleFormat() != QAudioFormat::Float &&
         format.sampleFormat() != QAudioFormat::UInt8)) {
        if (error) *error = tr("Formato audio del microfono non supportato.");
        return false;
    }
    format_ = format;
    file_ = std::make_unique<QFile>(path);
    if (!file_->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (storageError) *storageError = true;
        if (error) *error = tr("%1: %2").arg(path, file_->errorString());
        file_.reset();
        return false;
    }
    dataBytes_ = 0;
    { const std::lock_guard<std::mutex> lock(analysisMutex_); analysisSamples_.clear(); }
    pendingInput_.clear();
    pendingInput_.reserve(65536);
    droppedChunks_ = 0;
    const int inputBytesPerSample = format_.sampleFormat() == QAudioFormat::Float ? 4 :
        (format_.sampleFormat() == QAudioFormat::Int16 ? 2 : 1);
    // Eight seconds absorb temporary UI/disk stalls without blocking the
    // real-time audio callback or dropping discontinuous chunks.
    const std::size_t capacity = std::size_t(format_.sampleRate()) *
        std::size_t(format_.channelCount()) * std::size_t(inputBytesPerSample) * 8;
    ring_ = std::make_unique<AudioRingBuffer>(capacity);
    dsp_ = std::make_unique<RecordingDsp>();
    dsp_->reset(format_.channelCount(), format_.sampleRate());
    dsp_->setInputGain(gain_.load(std::memory_order_relaxed));
    writeHeader(0);
    source_ = std::make_unique<QAudioSource>(microphone, format_);
    connect(source_.get(), &QAudioSource::stateChanged, this, [this](auto state) {
        if (recording_.load(std::memory_order_acquire) && state == QAudio::StoppedState && source_ &&
            source_->error() != QAudio::NoError)
            QTimer::singleShot(0, this, [this] {
                if (!recording_.load(std::memory_order_acquire)) return;
                stop();
                if (error_) error_(tr("Il microfono si è interrotto durante la registrazione."));
            });
    });
    writerStop_.store(false, std::memory_order_release);
    writeFailed_.store(false, std::memory_order_release);
    recording_.store(true, std::memory_order_release);
    writerThread_ = std::thread(&RecordingController::writerLoop, this);
    // A generous backend queue absorbs scheduler jitter before the lock-free
    // application ring. The writer thread still drains continuously.
    source_->setBufferSize(format_.bytesForDuration(500000));
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
    if (format_.sampleFormat() == QAudioFormat::Int16)
        source_->start([this](QSpan<const std::int16_t> samples) {
            enqueueAudio(reinterpret_cast<const char *>(samples.data()), samples.size() * sizeof(std::int16_t));
        });
    else if (format_.sampleFormat() == QAudioFormat::Float)
        source_->start([this](QSpan<const float> samples) {
            enqueueAudio(reinterpret_cast<const char *>(samples.data()), samples.size() * sizeof(float));
        });
    else
        source_->start([this](QSpan<const std::uint8_t> samples) {
            enqueueAudio(reinterpret_cast<const char *>(samples.data()), samples.size());
        });
#else
    device_ = std::make_unique<CaptureDevice>([this](const char *data, std::size_t size) {
        enqueueAudio(data, size);
    });
    source_->start(device_.get());
#endif
    if (source_->error() != QAudio::NoError) {
        recording_.store(false, std::memory_order_release);
        writerStop_.store(true, std::memory_order_release);
        writerWake_.notify_one();
        if (writerThread_.joinable()) writerThread_.join();
        if (error) *error = tr("Impossibile avviare il microfono.");
        source_.reset(); device_.reset(); ring_.reset(); file_->close(); file_.reset();
        QFile::remove(path);
        return false;
    }
    analysisTimer_.start();
    if (stateChanged_) stateChanged_();
    return true;
}

void RecordingController::enqueueAudio(const char *data, std::size_t size) noexcept
{
    if (ring_ && !ring_->push(data, size)) droppedChunks_.fetch_add(1, std::memory_order_relaxed);
    else writerWake_.notify_one();
}

void RecordingController::writerLoop()
{
#if defined(Q_OS_MACOS)
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#elif defined(Q_OS_WIN)
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#elif defined(Q_OS_LINUX)
    sched_param parameter{}; parameter.sched_priority = 10;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &parameter);
#endif
    while (!writerStop_.load(std::memory_order_acquire) || (ring_ && ring_->available() > 0)) {
        drainAudio();
        std::unique_lock<std::mutex> lock(writerMutex_);
        writerWake_.wait_for(lock, std::chrono::milliseconds(4), [this] {
            return writerStop_.load(std::memory_order_acquire) || (ring_ && ring_->available() > 0);
        });
    }
    drainAudio();
}

void RecordingController::drainAudio()
{
    if (!ring_ || !file_) return;
    char chunk[65536];
    for (;;) {
        const std::size_t count = ring_->pop(chunk, sizeof(chunk));
        if (count == 0) break;
        writeAudio(QByteArray(chunk, qsizetype(count)));
    }
}

void RecordingController::writeAudio(const QByteArray &data)
{
    if (!file_ || data.isEmpty()) return;
    pendingInput_ += data;
    const int channels = format_.channelCount();
    const int sampleBytes = format_.sampleFormat() == QAudioFormat::Float ? 4 :
        (format_.sampleFormat() == QAudioFormat::Int16 ? 2 : 1);
    const int frames = pendingInput_.size() / (channels * sampleBytes);
    QByteArray pcm;
    pcm.reserve(frames * channels * 2);
    QVector<float> analysisBatch;
    analysisBatch.reserve(frames);
    const auto *bytes = reinterpret_cast<const unsigned char *>(pendingInput_.constData());
    QVector<float> frameSamples(channels);
    for (int frame = 0; frame < frames; ++frame) {
        float mono = 0.0f;
        for (int channel = 0; channel < channels; ++channel) {
            const int offset = (frame * channels + channel) * sampleBytes;
            float value = 0.0f;
            if (format_.sampleFormat() == QAudioFormat::UInt8)
                value = (int(bytes[offset]) - 128) / 128.0f;
            else if (format_.sampleFormat() == QAudioFormat::Int16) {
                const quint16 bits = quint16(bytes[offset]) | (quint16(bytes[offset + 1]) << 8);
                value = qint16(bits) / 32768.0f;
            } else std::memcpy(&value, bytes + offset, sizeof(value));
            frameSamples[channel] = value;
        }
        dsp_->setInputGain(gain_.load(std::memory_order_relaxed));
        dsp_->processFrame(frameSamples.data(), channels);
        for (int channel = 0; channel < channels; ++channel) {
            const float value = frameSamples[channel];
            mono += value;
            const quint16 pcmBits = quint16(qint16(std::lround(value * 32767.0f)));
            pcm.append(char(pcmBits & 0xff));
            pcm.append(char((pcmBits >> 8) & 0xff));
        }
        analysisBatch.append(mono / channels);
    }
    {
        const std::lock_guard<std::mutex> lock(analysisMutex_);
        analysisSamples_.append(analysisBatch);
        if (analysisSamples_.size() > 2048)
            analysisSamples_.remove(0, analysisSamples_.size() - 2048);
    }
    pendingInput_.remove(0, frames * channels * sampleBytes);
    const qint64 written = file_->write(pcm);
    if (written > 0) dataBytes_ += quint32(written);
    if (written != pcm.size()) writeFailed_.store(true, std::memory_order_release);
}

void RecordingController::stop()
{
    if (recording_.exchange(false, std::memory_order_acq_rel)) {
        analysisTimer_.stop();
        if (source_) source_->stop();
        writerStop_.store(true, std::memory_order_release);
        writerWake_.notify_one();
        if (writerThread_.joinable()) writerThread_.join();
        source_.reset();
        device_.reset();
        ring_.reset();
        dsp_.reset();
        writeHeader(dataBytes_);
        if (file_) { file_->flush(); file_->close(); file_.reset(); }
        if (stateChanged_) stateChanged_();
        const int dropped = droppedChunks_.load(std::memory_order_relaxed);
        if (dropped > 0 && error_)
            error_(tr("Sono stati persi %1 blocchi audio durante la registrazione. La traccia può avere interruzioni.").arg(dropped));
    }
    if (player_->playbackState() != QMediaPlayer::StoppedState) player_->stop();
}

void RecordingController::playPause(const QString &path, QString *error)
{
    if (recording_.load(std::memory_order_acquire)) { if (error) *error = tr("Termina la registrazione prima di ascoltare."); return; }
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        if (error) *error = tr("Seleziona un riferimento audio esistente nel testo.");
        return;
    }
    if (playingPath_ == path && player_->playbackState() == QMediaPlayer::PlayingState) {
        player_->pause();
        return;
    }
    if (playingPath_ != path) {
        playingPath_ = path;
        player_->setSource(QUrl::fromLocalFile(path));
    }
    player_->play();
}
