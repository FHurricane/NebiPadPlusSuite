#include "recentfiles.h"

#include <QFileInfo>
#include <QFile>
#include <QSettings>
#include <QVariantMap>

#include <algorithm>

namespace {
QString normalizedPath(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? info.absoluteFilePath() : canonical;
}

QString inferredApplication(const QString &path)
{
    const QString lower = path.toLower();
    if (lower.endsWith(".mindnet.json")) return QStringLiteral("MindNet");
    if (lower.endsWith(".math.html")) return QStringLiteral("Math");
    if (lower.endsWith(".pdf")) return QStringLiteral("Impagination");
    if (lower.endsWith(".html") || lower.endsWith(".htm")) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly) && file.peek(64).startsWith("<!--NEBIPAD_LAYOUT:"))
            return QStringLiteral("Impagination");
    }
    return QStringLiteral("NebiPad+");
}
}

QVector<RecentFileEntry> recentFileEntries()
{
    QSettings settings;
    QVector<RecentFileEntry> saved;
    const QVariantList records = settings.value("recentFileEntries").toList();
    for (const QVariant &record : records) {
        const QVariantMap map = record.toMap();
        if (!map.value("path").toString().isEmpty())
            saved.append({map.value("path").toString(), map.value("application").toString()});
    }
    if (saved.isEmpty())
        for (const QString &path : settings.value("recentFiles").toStringList())
            saved.append({path, inferredApplication(path)});

    QVector<RecentFileEntry> valid;
    QStringList seen;
    for (RecentFileEntry entry : saved) {
        const QString path = normalizedPath(entry.path);
        const QFileInfo info(path);
        if (info.isFile() && !seen.contains(path)) {
            entry.path = path;
            if (entry.application.isEmpty()) entry.application = inferredApplication(path);
            valid.append(entry);
            seen.append(path);
        }
        if (valid.size() == 10) break;
    }
    QVariantList stored;
    for (const RecentFileEntry &entry : valid)
        stored.append(QVariantMap{{"path", entry.path}, {"application", entry.application}});
    settings.setValue("recentFileEntries", stored);
    settings.setValue("recentFiles", seen);
    return valid;
}

QStringList recentFiles()
{
    QStringList paths;
    for (const RecentFileEntry &entry : recentFileEntries()) paths.append(entry.path);
    return paths;
}

void rememberRecentFile(const QString &path, const QString &application)
{
    if (path.isEmpty()) return;
    const QString normalized = normalizedPath(path);
    if (!QFileInfo(normalized).isFile()) return;
    QVector<RecentFileEntry> entries = recentFileEntries();
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const RecentFileEntry &entry) {
        return entry.path == normalized;
    }), entries.end());
    entries.prepend({normalized, application.isEmpty() ? inferredApplication(normalized) : application});
    while (entries.size() > 10) entries.removeLast();
    QVariantList stored;
    QStringList paths;
    for (const RecentFileEntry &entry : entries) {
        stored.append(QVariantMap{{"path", entry.path}, {"application", entry.application}});
        paths.append(entry.path);
    }
    QSettings settings;
    settings.setValue("recentFileEntries", stored);
    settings.setValue("recentFiles", paths);
}

void forgetRecentFile(const QString &path)
{
    const QString normalized = normalizedPath(path);
    QVector<RecentFileEntry> entries = recentFileEntries();
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const RecentFileEntry &entry) {
        return entry.path == normalized;
    }), entries.end());
    QVariantList stored;
    QStringList paths;
    for (const RecentFileEntry &entry : entries) {
        stored.append(QVariantMap{{"path", entry.path}, {"application", entry.application}});
        paths.append(entry.path);
    }
    QSettings settings;
    settings.setValue("recentFileEntries", stored);
    settings.setValue("recentFiles", paths);
}
