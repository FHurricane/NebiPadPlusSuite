#pragma once

#include <functional>
#include <QString>

class QAction;
class QMainWindow;

void installGoogleDriveMenu(QMainWindow *window, QAction *before,
                            std::function<QString()> currentPath,
                            std::function<bool()> saveCurrent,
                            std::function<bool(const QString &)> openLocalFile);
