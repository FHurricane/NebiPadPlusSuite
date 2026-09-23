#include "audiolinks.h"

#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QUrl>

#include <limits>

namespace {
const QRegularExpression &audioLink()
{
    static const QRegularExpression pattern(
        R"(\[Audio: [^\]]+\]\(((?:recording/[A-Za-z0-9_.-]+\.wav)|(?:file://[^\s)]+\.wav))\))");
    return pattern;
}

const QRegularExpression &recordingLabel()
{
    static const QRegularExpression pattern(R"(\[Registrazione_([1-9][0-9]*)\])");
    return pattern;
}

QString localIndexPath(const QString &document)
{
    const QFileInfo info(document);
    return info.absolutePath() + "/recording/" + info.fileName() + ".links.json";
}

QString fallbackIndexPath(const QString &document)
{
    const QByteArray key = QCryptographicHash::hash(
        QFileInfo(document).absoluteFilePath().toUtf8(), QCryptographicHash::Sha256).toHex();
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        "/recording/index/" + QString::fromLatin1(key) + ".json";
}

QJsonObject readIndex(const QString &document)
{
    if (document.isEmpty()) return {};
    for (const QString &path : {fallbackIndexPath(document), localIndexPath(document)}) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            const QJsonDocument json = QJsonDocument::fromJson(file.readAll());
            if (json.isObject()) return json.object();
        }
    }
    return {};
}

bool writeIndex(const QString &document, const QJsonObject &index, QString *error)
{
    const QByteArray data = QJsonDocument(index).toJson(QJsonDocument::Indented);
    for (const QString &path : {localIndexPath(document), fallbackIndexPath(document)}) {
        if (!QDir().mkpath(QFileInfo(path).absolutePath())) continue;
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly)) continue;
        if (file.write(data) == data.size() && file.commit()) {
            if (path == localIndexPath(document)) QFile::remove(fallbackIndexPath(document));
            return true;
        }
    }
    if (error) *error = QStringLiteral("Impossibile salvare l'indice delle registrazioni per %1")
        .arg(document);
    return false;
}
}

QString audioTargetAt(const QString &text, int position)
{
    auto labels = recordingLabel().globalMatch(text);
    while (labels.hasNext()) {
        const auto match = labels.next();
        if (position >= match.capturedStart() && position <= match.capturedEnd())
            return QStringLiteral("recording-id:%1").arg(match.captured(1));
    }
    auto matches = audioLink().globalMatch(text);
    while (matches.hasNext()) {
        const auto match = matches.next();
        if (position >= match.capturedStart() && position <= match.capturedEnd())
            return match.captured(1);
    }
    return {};
}

QString nearestAudioTarget(const QString &text, int position)
{
    QString nearest;
    int best = std::numeric_limits<int>::max();
    auto matches = audioLink().globalMatch(text);
    while (matches.hasNext()) {
        const auto match = matches.next();
        const int distance = position < match.capturedStart() ? match.capturedStart() - position :
            (position > match.capturedEnd() ? position - match.capturedEnd() : 0);
        if (distance < best) { best = distance; nearest = match.captured(1); }
    }
    auto labels = recordingLabel().globalMatch(text);
    while (labels.hasNext()) {
        const auto match = labels.next();
        const int distance = position < match.capturedStart() ? match.capturedStart() - position :
            (position > match.capturedEnd() ? position - match.capturedEnd() : 0);
        if (distance < best) {
            best = distance;
            nearest = QStringLiteral("recording-id:%1").arg(match.captured(1));
        }
    }
    return nearest;
}

QString resolveAudioTarget(const QString &documentPath, const QString &target)
{
    if (target.startsWith("recording-id:")) {
        bool valid = false;
        const int number = target.mid(13).toInt(&valid);
        if (!valid || number < 1) return {};
        const QString stored = readIndex(documentPath).value(QString::number(number)).toString();
        if (stored.startsWith("recording-id:")) return {};
        return resolveAudioTarget(documentPath, stored);
    }
    if (target.startsWith("recording/")) {
        const QString name = target.mid(10);
        if (name.contains('/') || name.contains('\\') || name == "." || name == ".." ||
            documentPath.isEmpty()) return {};
        return QFileInfo(documentPath).absolutePath() + "/recording/" + name;
    }
    const QUrl url(target);
    return url.scheme() == "file" && url.isLocalFile() ? url.toLocalFile() : QString();
}

int nextRecordingNumber(const QString &documentPath, const QString &text)
{
    int maximum = 0;
    auto labels = recordingLabel().globalMatch(text);
    while (labels.hasNext()) maximum = qMax(maximum, labels.next().captured(1).toInt());
    Q_UNUSED(documentPath);
    int legacyCount = 0;
    auto links = audioLink().globalMatch(text);
    while (links.hasNext()) { links.next(); ++legacyCount; }
    return qMax(maximum, legacyCount) + 1;
}

bool registerRecordingLink(const QString &documentPath, int number,
                           const QString &target, QString *error)
{
    if (documentPath.isEmpty() || number < 1 || resolveAudioTarget(documentPath, target).isEmpty())
        return false;
    QJsonObject index = readIndex(documentPath);
    index.insert(QString::number(number), target);
    return writeIndex(documentPath, index, error);
}

bool copyRecordingLinks(const QString &oldDocument, const QString &newDocument,
                        const QString &text, QString *error)
{
    const QJsonObject oldIndex = readIndex(oldDocument);
    if (oldIndex.isEmpty()) return true;
    QJsonObject newIndex;
    auto labels = recordingLabel().globalMatch(text);
    while (labels.hasNext()) {
        const QString number = labels.next().captured(1);
        const QString target = oldIndex.value(number).toString();
        if (target.isEmpty()) continue;
        if (target.startsWith("recording/")) {
            const QString oldFile = resolveAudioTarget(oldDocument, target);
            const QString newFile = resolveAudioTarget(newDocument, target);
            if (oldFile != newFile && QFileInfo::exists(oldFile) &&
                (!QDir().mkpath(QFileInfo(newFile).absolutePath()) ||
                 (!QFileInfo::exists(newFile) && !QFile::copy(oldFile, newFile)))) {
                if (error) *error = QStringLiteral("Impossibile copiare %1").arg(oldFile);
                return false;
            }
        }
        newIndex.insert(number, target);
    }
    return newIndex.isEmpty() || writeIndex(newDocument, newIndex, error);
}

bool migrateLegacyAudioLinks(const QString &documentPath, QString *text, QString *error)
{
    if (!text || documentPath.isEmpty()) return false;
    int number = 1;
    auto labels = recordingLabel().globalMatch(*text);
    while (labels.hasNext()) number = qMax(number, labels.next().captured(1).toInt() + 1);
    QString converted;
    int last = 0;
    auto links = audioLink().globalMatch(*text);
    while (links.hasNext()) {
        const auto match = links.next();
        if (!registerRecordingLink(documentPath, number, match.captured(1), error)) return false;
        converted += text->mid(last, match.capturedStart() - last);
        converted += QStringLiteral("[Registrazione_%1]").arg(number++);
        last = match.capturedEnd();
    }
    if (last > 0) {
        converted += text->mid(last);
        *text = converted;
    }
    return true;
}

QStringList relativeAudioFiles(const QString &text)
{
    QStringList files;
    auto matches = audioLink().globalMatch(text);
    while (matches.hasNext()) {
        const QString target = matches.next().captured(1);
        if (target.startsWith("recording/")) {
            const QString name = target.mid(10);
            if (!files.contains(name)) files.append(name);
        }
    }
    return files;
}
