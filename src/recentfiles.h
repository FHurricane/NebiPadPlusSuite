#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct RecentFileEntry {
    QString path;
    QString application;
};

QStringList recentFiles();
QVector<RecentFileEntry> recentFileEntries();
void rememberRecentFile(const QString &path, const QString &application = QStringLiteral("NebiPad+"));
void forgetRecentFile(const QString &path);
