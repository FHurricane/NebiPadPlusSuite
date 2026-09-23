#include "speechbackend.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryFile>

QString whisperModelPath()
{
    const QString env = qEnvironmentVariable("NEBIPAD_WHISPER_MODEL");
    if (QFileInfo(env).isFile()) return env;
    const QString saved = QSettings().value("speech/whisperModel").toString();
    if (QFileInfo(saved).isFile()) return saved;
    const QString local = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                          "/models/ggml-base.bin";
    return QFileInfo(local).isFile() ? local : QString();
}

void setWhisperModelPath(const QString &path)
{
    QSettings settings;
    settings.setValue("speech/whisperModel", path);
}

QString whisperExecutablePath()
{
    const QString env = qEnvironmentVariable("NEBIPAD_WHISPER_CLI");
    if (QFileInfo(env).isExecutable()) return env;
    const QString saved = QSettings().value("speech/whisperExecutable").toString();
    if (QFileInfo(saved).isExecutable()) return saved;
    const QString found = QStandardPaths::findExecutable("whisper-cli");
    if (!found.isEmpty()) return found;
#ifdef Q_OS_MACOS
    for (const QString &directory : {QStringLiteral("/opt/homebrew/bin"), QStringLiteral("/usr/local/bin")}) {
        const QString path = directory + "/whisper-cli";
        if (QFileInfo(path).isExecutable()) return path;
    }
#endif
    return {};
}

void setWhisperExecutablePath(const QString &path)
{
    QSettings settings;
    settings.setValue("speech/whisperExecutable", path);
}

namespace {
class WhisperBackend final : public SpeechBackend
{
public:
    explicit WhisperBackend(QObject *context) : context_(context)
    {
        QObject::connect(&process_, &QProcess::finished, context_,
                         [this](int exitCode, QProcess::ExitStatus status) {
            if (!callbacks_.finalText && !callbacks_.error) return;
            QString text;
            QFile result(outputPath_);
            if (result.open(QIODevice::ReadOnly)) text = QString::fromUtf8(result.readAll()).trimmed();
            QFile::remove(outputPath_);
            file_.reset();
            if (status != QProcess::NormalExit || exitCode != 0) {
                if (callbacks_.error)
                    callbacks_.error(QStringLiteral("La trascrizione Whisper non è riuscita: %1")
                                     .arg(QString::fromUtf8(process_.readAllStandardError()).trimmed()));
            } else if (callbacks_.finalText) {
                callbacks_.finalText(text);
            }
            callbacks_ = {};
        });
        QObject::connect(&process_, &QProcess::errorOccurred, context_,
                         [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart && callbacks_.error) {
                callbacks_.error(QStringLiteral("Impossibile avviare whisper-cli."));
                callbacks_ = {};
            }
        });
    }

    ~WhisperBackend() override { cancel(); }

    void start(SpeechCallbacks callbacks) override
    {
        callbacks_ = std::move(callbacks);
        pcm_.clear();
        if (whisperExecutablePath().isEmpty()) {
            if (callbacks_.error)
                callbacks_.error(QStringLiteral("whisper-cli non è installato o non è nel PATH."));
            return;
        }
        if (whisperModelPath().isEmpty()) {
            if (callbacks_.error)
                callbacks_.error(QStringLiteral("Seleziona un modello Whisper multilingue in Strumenti."));
            return;
        }
        if (callbacks_.ready) callbacks_.ready();
    }

    void appendAudio(const QByteArray &pcm16) override { pcm_ += pcm16; }

    void finish() override
    {
        if (pcm_.isEmpty()) {
            if (callbacks_.finalText) callbacks_.finalText({});
            callbacks_ = {};
            return;
        }
        file_ = std::make_unique<QTemporaryFile>(QDir::tempPath() + "/nebipad-speech-XXXXXX.wav");
        if (!file_->open()) {
            if (callbacks_.error) callbacks_.error(QStringLiteral("Impossibile creare il file audio temporaneo."));
            return;
        }
        QDataStream stream(file_.get());
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.writeRawData("RIFF", 4);
        stream << quint32(36 + pcm_.size());
        stream.writeRawData("WAVEfmt ", 8);
        stream << quint32(16) << quint16(1) << quint16(1);
        stream << quint32(16000) << quint32(32000) << quint16(2) << quint16(16);
        stream.writeRawData("data", 4);
        stream << quint32(pcm_.size());
        stream.writeRawData(pcm_.constData(), pcm_.size());
        file_->flush();
        outputPath_ = file_->fileName() + ".result.txt";
        const QString prefix = file_->fileName() + ".result";
        const QStringList arguments = {"-m", whisperModelPath(), "-f", file_->fileName(),
                                       "-l", "auto", "-otxt", "-of", prefix, "-nt"};
        process_.start(whisperExecutablePath(), arguments);
        pcm_.clear();
    }

    void cancel() override
    {
        callbacks_ = {};
        if (process_.state() != QProcess::NotRunning) {
            process_.kill();
            process_.waitForFinished(250);
        }
        QFile::remove(outputPath_);
        file_.reset();
        pcm_.clear();
    }

private:
    QObject *context_;
    SpeechCallbacks callbacks_;
    QByteArray pcm_;
    QProcess process_;
    std::unique_ptr<QTemporaryFile> file_;
    QString outputPath_;
};
}

#ifndef Q_OS_MACOS
std::unique_ptr<SpeechBackend> createSpeechBackend(QObject *context)
{
    return std::make_unique<WhisperBackend>(context);
}
#endif
