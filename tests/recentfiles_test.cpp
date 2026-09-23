#include "recentfiles.h"

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    if (!directory.isValid()) return 1;
    QCoreApplication::setOrganizationName("NebiPadTest");
    QCoreApplication::setApplicationName("RecentFilesTest");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());

    QStringList paths;
    for (int i = 0; i < 12; ++i) {
        const QString path = directory.filePath(QString("file%1.txt").arg(i));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write("text") != 4) return 2;
        file.close();
        paths << QFileInfo(path).canonicalFilePath();
        rememberRecentFile(path);
    }
    QStringList recent = recentFiles();
    if (recent.size() != 10 || recent.first() != paths[11] || recent.last() != paths[2]) return 3;
    rememberRecentFile(paths[5], "MindNet");
    recent = recentFiles();
    if (recent.size() != 10 || recent.first() != paths[5]) return 4;
    const QVector<RecentFileEntry> entries = recentFileEntries();
    if (entries.isEmpty() || entries.first().application != "MindNet" ||
        entries.first().path != paths[5]) return 7;
    QFile::remove(paths[5]);
    if (recentFiles().contains(paths[5])) return 5;
    forgetRecentFile(paths[4]);
    return recentFiles().contains(paths[4]) ? 6 : 0;
}
