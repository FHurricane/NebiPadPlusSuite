#pragma once

#include <QAudioFormat>
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>

#include <functional>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class QAudioSource;
class QAudioOutput;
class QMediaPlayer;
class QFile;
class QIODevice;
class AudioRingBuffer;
class RecordingDsp;

class RecordingController final : public QObject
{
public:
    explicit RecordingController(QObject *parent = nullptr);
    ~RecordingController() override;
    bool isRecording() const { return recording_.load(std::memory_order_acquire); }
    bool start(const QString &path, QString *error, bool *storageError = nullptr);
    void stop();
    void playPause(const QString &path, QString *error);
    void setGainPercent(int percent) { gain_.store(qBound(0.25f, percent / 100.0f, 1.0f), std::memory_order_relaxed); }
    void setStateChanged(std::function<void()> callback) { stateChanged_ = std::move(callback); }
    void setLevelsChanged(std::function<void(const QVector<float> &)> callback) { levelsChanged_ = std::move(callback); }
    void setError(std::function<void(const QString &)> callback) { error_ = std::move(callback); }

private:
    class CaptureDevice;
    std::unique_ptr<QAudioSource> source_;
    std::unique_ptr<CaptureDevice> device_;
    std::unique_ptr<QFile> file_;
    QAudioFormat format_;
    QMediaPlayer *player_;
    QAudioOutput *output_;
    QString playingPath_;
    quint32 dataBytes_ = 0;
    std::atomic<bool> recording_{false};
    std::function<void()> stateChanged_;
    std::function<void(const QVector<float> &)> levelsChanged_;
    std::function<void(const QString &)> error_;
    QVector<float> analysisSamples_;
    mutable std::mutex analysisMutex_;
    QByteArray pendingInput_;
    QTimer analysisTimer_;
    std::unique_ptr<AudioRingBuffer> ring_;
    std::unique_ptr<RecordingDsp> dsp_;
    std::atomic<int> droppedChunks_{0};
    std::atomic<float> gain_{0.75f};
    std::thread writerThread_;
    std::mutex writerMutex_;
    std::condition_variable writerWake_;
    std::atomic<bool> writerStop_{false};
    std::atomic<bool> writeFailed_{false};

    void writeAudio(const QByteArray &data);
    void enqueueAudio(const char *data, std::size_t size) noexcept;
    void drainAudio();
    void writerLoop();
    void writeHeader(quint32 dataBytes);
};
