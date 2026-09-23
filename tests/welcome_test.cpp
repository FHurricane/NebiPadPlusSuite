#include "welcomewindow.h"
#include "recentfiles.h"
#include "localization.h"

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QFileInfo>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTemporaryDir directory;
    if (!directory.isValid()) return 1;
    QCoreApplication::setOrganizationName("NebiPadTest");
    QCoreApplication::setApplicationName("WelcomeTest");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
    NebiTranslator translator;
    if (translator.translate("WelcomeWindow", "Benvenuto in NebiPad+") != "Welcome to NebiPad+" ||
        translator.translate("SuiteWindow", "Inserisci tabella") != "Insert Table") return 10;
    const QString path = directory.filePath("example.cpp");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write("int main() {}") < 0) return 2;
    file.close();
    rememberRecentFile(path);

    WelcomeWindow welcome;
    if (argc > 1) {
        welcome.show();
        app.processEvents();
        if (!welcome.grab().save(QString::fromLocal8Bit(argv[1]))) return 5;
    }
    auto *recent = welcome.findChild<QListWidget *>("recentFilesList");
    auto *language = welcome.findChild<QComboBox *>("suiteLanguageCombo");
    auto *copyright = welcome.findChild<QLabel *>("welcomeCopyright");
    if (welcome.findChild<QPushButton *>("startButton") ||
        welcome.findChild<QPushButton *>("newDocumentButton") ||
        welcome.findChild<QPushButton *>("openButton") || !recent || !language || !copyright ||
        !copyright->text().contains("Costa Fabio") || !copyright->text().contains("2026") ||
        language->count() != 2 || language->findData("it") < 0 || language->findData("en") < 0 ||
        recent->count() != 1 ||
        recent->item(0)->data(Qt::UserRole).toString() != QFileInfo(path).canonicalFilePath()) return 3;
    if (!recent->item(0)->text().contains("(NebiPad+)")) return 6;
    int openedFiles = 0;
    int openedSuites = 0;
    welcome.setRecentFileHandler([&](const QString &candidate, const QString &application) {
        ++openedFiles;
        return candidate == QFileInfo(path).canonicalFilePath() && application == "NebiPad+";
    });
    welcome.setSuiteHandler([&](int) { ++openedSuites; });
    welcome.show();
    app.processEvents();
    recent->itemClicked(recent->item(0));
    for (int i = 0; i < 4; ++i) {
        auto *button = welcome.findChild<QPushButton *>(QString("suiteButton%1").arg(i));
        if (!button) return 8;
        button->click();
        if (!welcome.isVisible()) return 9;
    }
    return openedFiles == 1 && openedSuites == 4 && welcome.isVisible() ? 0 : 4;
}
