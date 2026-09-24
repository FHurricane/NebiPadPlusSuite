#include "nebimagewindow.h"
#include "recentfiles.h"

#include <QApplication>
#include <QImage>
#include <QListWidget>
#include <QSettings>
#include <QTemporaryDir>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir dir;
    if (!dir.isValid()) return 1;
    QCoreApplication::setOrganizationName("NebiPadTest");
    QCoreApplication::setApplicationName("NebImageTest");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
    const QString path = dir.filePath("sample.png");
    QImage image(320, 180, QImage::Format_RGB32);
    image.fill(QColor("#7647bd"));
    if (!image.save(path)) return 2;
    NebImageWindow window;
    if (!window.openFile(path)) return 3;
    auto *filmstrip = window.findChild<QListWidget *>("imageFilmstrip");
    if (!filmstrip || filmstrip->count() != 1 || !window.windowTitle().contains("sample.png")) return 4;
    const auto recent = recentFileEntries();
    if (recent.isEmpty() || recent.first().application != "NebImage+") return 5;
    return 0;
}
