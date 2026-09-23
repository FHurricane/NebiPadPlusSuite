#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

#include <functional>
#include <memory>

struct SpeechCallbacks {
    std::function<void()> ready;
    std::function<void(const QString &)> partial;
    std::function<void(const QString &)> finalText;
    std::function<void(const QString &)> error;
};

class SpeechBackend
{
public:
    virtual ~SpeechBackend() = default;
    virtual void start(SpeechCallbacks callbacks) = 0;
    virtual void appendAudio(const QByteArray &pcm16) = 0;
    virtual void finish() = 0;
    virtual void cancel() = 0;
};

std::unique_ptr<SpeechBackend> createSpeechBackend(QObject *context);

QString whisperModelPath();
void setWhisperModelPath(const QString &path);
QString whisperExecutablePath();
void setWhisperExecutablePath(const QString &path);
