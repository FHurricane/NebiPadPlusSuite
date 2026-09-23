#include "savepaths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    for (const bool html : {false, true}) {
        const QString path = initialDocumentSavePath(html);
        if (!QFileInfo(path).isAbsolute() ||
            QFileInfo(path).absolutePath() == QDir::rootPath() ||
            !QDir(QFileInfo(path).absolutePath()).exists()) return 1;
        if (!path.endsWith(html ? ".html" : ".txt")) return 2;
    }
    return 0;
}
