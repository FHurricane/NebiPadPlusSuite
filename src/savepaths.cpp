#include "savepaths.h"

#include <QDir>
#include <QStandardPaths>

QString initialDocumentSavePath(bool html)
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (directory.isEmpty() || !QDir(directory).exists()) directory = QDir::homePath();
    return QDir(directory).filePath(html ? "Untitled.html" : "Untitled.txt");
}
