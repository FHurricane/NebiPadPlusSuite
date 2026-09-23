#include "audiolinks.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QUrl>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QString document = "/tmp/NebiPad note.txt";
    const QString external = QUrl::fromLocalFile("/tmp/Audio Folder/recording/voice.wav")
        .toString(QUrl::FullyEncoded);
    const QString text = QString("uno [Audio: a](recording/a.wav) due [Audio: voice](%1)").arg(external);
    if (audioTargetAt(text, 14) != "recording/a.wav") return 1;
    if (nearestAudioTarget(text, text.size()) != external) return 2;
    if (resolveAudioTarget(document, "recording/a.wav") != "/tmp/recording/a.wav") return 3;
    if (resolveAudioTarget(document, external) != "/tmp/Audio Folder/recording/voice.wav") return 4;
    if (relativeAudioFiles(text) != QStringList{"a.wav"}) return 5;
    if (!resolveAudioTarget(document, "recording/../other.wav").isEmpty()) return 6;
    QTemporaryDir temporary;
    if (!temporary.isValid()) return 7;
    const QString first = temporary.filePath("first/notes.txt");
    const QString second = temporary.filePath("second/notes-copy.txt");
    const QString wav = QFileInfo(first).absolutePath() + "/recording/audio-long-name.wav";
    if (!QDir().mkpath(QFileInfo(wav).absolutePath())) return 8;
    QFile audio(wav);
    if (!audio.open(QIODevice::WriteOnly) || audio.write("RIFF", 4) != 4) return 9;
    audio.close();
    QString error;
    if (!registerRecordingLink(first, 1, "recording/audio-long-name.wav", &error)) return 10;
    if (nextRecordingNumber(first, "test [Registrazione_1]") != 2) return 11;
    if (audioTargetAt("test [Registrazione_1]", 8) != "recording-id:1") return 12;
    if (resolveAudioTarget(first, "recording-id:1") != wav) return 13;
    if (!copyRecordingLinks(first, second, "test [Registrazione_1]", &error)) return 14;
    if (!QFileInfo::exists(resolveAudioTarget(second, "recording-id:1"))) return 15;
    if (nearestAudioTarget("test [Registrazione_1]", 22) != "recording-id:1") return 16;
    QString oldText = "prima [Audio: audio-long-name](recording/audio-long-name.wav) dopo";
    const QString legacy = temporary.filePath("legacy/old.txt");
    if (!migrateLegacyAudioLinks(legacy, &oldText, &error)) return 17;
    if (oldText != "prima [Registrazione_1] dopo") return 18;
    if (resolveAudioTarget(legacy, "recording-id:1") !=
        QFileInfo(legacy).absolutePath() + "/recording/audio-long-name.wav") return 19;
    return 0;
}
