#include "mainwindow.h"
#include "editor.h"
#include "language.h"
#include "recentfiles.h"

#include <QApplication>
#include <QAction>
#include <QCompleter>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QRegularExpression>
#include <QTabWidget>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTemporaryDir settingsDirectory;
    if (!settingsDirectory.isValid()) return 30;
    QCoreApplication::setOrganizationName("NebiPadTest");
    QCoreApplication::setApplicationName("MenuVisibilityTest");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    MainWindow window;
    window.show();
    app.processEvents();
    if (!window.findChild<QAction *>("mainExportPdfAction")) return 47;
    auto *dictationAction = window.findChild<QAction *>("dictationAction");
    auto *dictationOverlay = window.findChild<QWidget *>("dictationOverlay");
    auto *recordAction = window.findChild<QAction *>("recordAudioAction");
    auto *playPauseAction = window.findChild<QAction *>("playPauseAudioAction");
    auto *stopAudioAction = window.findChild<QAction *>("stopAudioAction");
    auto *recordingGain = window.findChild<QSpinBox *>("recordingGainControl");
    if (!dictationAction || dictationAction->shortcut() != QKeySequence(Qt::Key_F12) ||
        !dictationOverlay || dictationOverlay->isVisible()) return 32;
    if (!recordAction || !playPauseAction || !stopAudioAction ||
        recordAction->icon().isNull() || playPauseAction->icon().isNull() ||
        stopAudioAction->icon().isNull() || !recordingGain ||
        recordingGain->value() != 75) return 33;
    auto *cloudMenu = window.findChild<QMenu *>("googleDriveMenu");
    auto *cloudOpen = window.findChild<QAction *>("googleDriveOpenAction");
    auto *cloudSave = window.findChild<QAction *>("googleDriveSaveAction");
    auto *cloudAvatar = window.findChild<QToolButton *>("googleDriveAvatar");
    auto *cloudAccount = window.findChild<QWidget *>("googleDriveAccountWidget");
    if (!cloudMenu || !cloudOpen || !cloudSave || !cloudAvatar || !cloudAvatar->menu() ||
        !cloudAccount || cloudAccount->isVisible()) return 45;
    if (!window.menuBar()->actions().first()->menu()->actions().contains(cloudOpen) ||
        !window.menuBar()->actions().first()->menu()->actions().contains(cloudSave)) return 46;
    auto *macroStart = window.findChild<QAction *>("macroStartRecordingAction");
    auto *macroStop = window.findChild<QAction *>("macroStopRecordingAction");
    auto *macroPlay = window.findChild<QAction *>("macroPlayRecordedAction");
    auto *macroSave = window.findChild<QAction *>("macroSaveRecordedAction");
    auto *savedMacros = window.findChild<QMenu *>("savedMacrosMenu");
    auto *macroEditor = dynamic_cast<Editor *>(window.findChild<QTextEdit *>());
    if (!macroStart || !macroStop || !macroPlay || !macroSave || !savedMacros || !macroEditor)
        return 48;
    macroEditor->clear(); macroEditor->setFocus();
    macroStart->trigger();
    QKeyEvent macroA(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "a");
    QKeyEvent macroB(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier, "b");
    QApplication::sendEvent(macroEditor, &macroA);
    QApplication::sendEvent(macroEditor, &macroB);
    macroStop->trigger();
    macroEditor->clear(); macroPlay->trigger();
    if (macroEditor->toPlainText() != "ab" || !macroSave->isEnabled()) return 49;
    QTimer::singleShot(0, [] {
        if (auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget())) {
            dialog->setTextValue("Macro test"); dialog->accept();
        }
    });
    macroSave->trigger();
    bool foundSavedMacro = false;
    for (QAction *action : savedMacros->actions())
        if (action->text() == "Macro test") foundSavedMacro = true;
    if (!foundSavedMacro) return 50;
    macroEditor->clear();
    auto *hashAction = window.findChild<QAction *>("hashGeneratorAction");
    if (!hashAction) return 34;
    bool hashDialogVerified = false;
    QTimer::singleShot(0, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog || dialog->objectName() != "hashGeneratorDialog") return;
        auto *algorithm = dialog->findChild<QComboBox *>("hashAlgorithm");
        auto *input = dialog->findChild<QPlainTextEdit *>("hashTextInput");
        auto *result = dialog->findChild<QLineEdit *>("hashResult");
        auto *calculate = dialog->findChild<QPushButton *>("hashCalculateAction");
        auto *copy = dialog->findChild<QPushButton *>("hashCopyAction");
        if (!algorithm || !input || !result || !calculate || !copy) { dialog->reject(); return; }
        input->setPlainText("abc");
        calculate->click();
        if (result->text() != "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") {
            dialog->reject(); return;
        }
        algorithm->setCurrentIndex(1);
        calculate->click();
        if (result->text() != "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                              "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f") {
            dialog->reject(); return;
        }
        copy->click();
        hashDialogVerified = QApplication::clipboard()->text() == result->text();
        dialog->reject();
    });
    hashAction->trigger();
    if (!hashDialogVerified) return 35;
    auto *passwordAction = window.findChild<QAction *>("passwordGeneratorAction");
    if (!passwordAction) return 36;
    bool passwordDialogVerified = false;
    QTimer::singleShot(0, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog || dialog->objectName() != "passwordGeneratorDialog") return;
        auto *length = dialog->findChild<QSpinBox *>("passwordLength");
        auto *count = dialog->findChild<QSpinBox *>("passwordCount");
        auto *result = dialog->findChild<QPlainTextEdit *>("passwordResult");
        auto *generate = dialog->findChild<QPushButton *>("passwordGenerateAction");
        auto *copy = dialog->findChild<QPushButton *>("passwordCopyAction");
        auto *strength = dialog->findChild<QProgressBar *>("passwordStrength");
        if (!length || !count || !result || !generate || !copy || !strength) {
            dialog->reject(); return;
        }
        length->setValue(32); count->setValue(3); generate->click();
        const QStringList passwords = result->toPlainText().split('\n');
        if (passwords.size() != 3 || strength->value() < 80) { dialog->reject(); return; }
        for (const QString &password : passwords) {
            if (password.size() != 32 || password.contains(QRegularExpression("[Il1O0o|]")) ||
                !password.contains(QRegularExpression("[a-z]")) ||
                !password.contains(QRegularExpression("[A-Z]")) ||
                !password.contains(QRegularExpression("[0-9]")) ||
                !password.contains(QRegularExpression("[^A-Za-z0-9]"))) {
                dialog->reject(); return;
            }
        }
        copy->click();
        passwordDialogVerified = QApplication::clipboard()->text() == result->toPlainText();
        dialog->reject();
    });
    passwordAction->trigger();
    if (!passwordDialogVerified) return 37;
    auto *pgpEncrypt = window.findChild<QAction *>("pgpEncryptAction");
    auto *pgpDecrypt = window.findChild<QAction *>("pgpDecryptAction");
    auto *pgpMenu = window.findChild<QMenu *>("openPgpMenu");
    auto *selectGpg = window.findChild<QAction *>("selectGpgExecutableAction");
    if (!pgpEncrypt || !pgpDecrypt || !pgpMenu || !selectGpg ||
        pgpEncrypt->icon().isNull() || pgpDecrypt->icon().isNull() ||
        !pgpMenu->actions().contains(pgpEncrypt) || !pgpMenu->actions().contains(pgpDecrypt)) return 44;
    QMenuBar *bar = window.menuBar();
    if (bar->isNativeMenuBar() || !bar->isVisible()) return 1;
    bool windowVisible = false;
    for (auto *action : bar->actions()) {
        if (action->text().remove('&') == "Finestra") {
            const QRect bounds = bar->actionGeometry(action);
            windowVisible = bounds.isValid() && bounds.right() < bar->width();
        }
    }
    if (!windowVisible) return 2;

    auto *languageMenu = window.findChild<QMenu *>("languageMenu");
    auto *status = window.findChild<QLabel *>("languageStatusLabel");
    auto *tabs = qobject_cast<QTabWidget *>(window.centralWidget());
    if (!languageMenu || !status || !tabs) return 3;
    auto *initialEditor = dynamic_cast<Editor *>(tabs->currentWidget());
    auto *readOnlyButton = qobject_cast<QToolButton *>(
        tabs->tabBar()->tabButton(tabs->currentIndex(), QTabBar::LeftSide));
    auto *tabControls = tabs->tabBar()->tabButton(tabs->currentIndex(), QTabBar::RightSide);
    auto *pinButton = tabControls ? tabControls->findChild<QToolButton *>("tabPinButton") : nullptr;
    auto *tabCloseButton = tabControls ? tabControls->findChild<QToolButton *>("tabCloseButton") : nullptr;
    if (!initialEditor || !readOnlyButton || !pinButton || !tabCloseButton) return 38;
    if (pinButton->x() >= tabCloseButton->x()) return 39;
    readOnlyButton->click();
    if (!initialEditor->isReadOnly() || !readOnlyButton->isChecked()) return 40;
    readOnlyButton->click();
    if (initialEditor->isReadOnly() || readOnlyButton->isChecked()) return 41;
    pinButton->click();
    const int pinnedCount = tabs->count();
    tabCloseButton->click();
    if (tabs->count() != pinnedCount || !initialEditor->property("tabPinned").toBool()) return 42;
    pinButton->click();
    if (initialEditor->property("tabPinned").toBool()) return 43;
    if (!(status->alignment() & Qt::AlignLeft)) return 18;
    int groupedCount = 0;
    for (auto *group : languageMenu->actions()) {
        if (!group->menu()) continue;
        if (group->text().size() != 1) return 15;
        for (auto *action : group->menu()->actions()) {
            if (action->text().at(0).toUpper() != group->text().at(0)) return 16;
            ++groupedCount;
        }
    }
    if (groupedCount != languages().size()) return 17;
    const auto languageAction = [languageMenu](const QString &name) -> QAction * {
        for (auto *group : languageMenu->actions()) {
            if (!group->menu()) continue;
            for (auto *action : group->menu()->actions())
                if (action->text() == name) return action;
        }
        return nullptr;
    };
    QMenu *aMenu = nullptr;
    for (auto *action : languageMenu->actions())
        if (action->text() == "A") aMenu = action->menu();
    if (!aMenu) return 4;
    const QStringList expectedA = {"ActionScript", "Ada", "ASP", "Assembly", "AutoIt", "AviSynth"};
    QStringList actualA;
    for (auto *action : aMenu->actions()) actualA << action->text();
    for (const QString &name : expectedA)
        if (!actualA.contains(name)) return 5;

    auto *python = languageAction("Python");
    auto *bash = languageAction("Bash");
    auto *html = languageAction("HTML");
    auto *cpp = languageAction("C++");
    if (!python || !bash || !html || !cpp) return 6;
    python->trigger();
    auto *editor = dynamic_cast<Editor *>(tabs->currentWidget());
    if (!editor || status->text() != "Python" ||
        editor->toPlainText() != "#!/usr/bin/env python3\n") return 7;
    editor->insertPlainText("def example():");
    QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "\n");
    QApplication::sendEvent(editor, &enter);
    if (!editor->toPlainText().endsWith("def example():\n    ")) return 8;
    bash->trigger();
    if (status->text() != "Bash" ||
        !editor->toPlainText().startsWith("#!/usr/bin/env bash\n") ||
        editor->toPlainText().contains("python3")) return 9;
    html->trigger();
    if (status->text() != "HTML" || editor->toPlainText().contains("#!") ||
        !editor->toPlainText().contains("def example():")) return 10;
    python->trigger();
    if (editor->toPlainText().count("#!/usr/bin/env python3") != 1) return 11;

    bar->actions().first()->menu()->actions().first()->trigger();
    if (tabs->count() != 2 || status->text() != "Normal Text File") return 12;
    cpp->trigger();
    if (status->text() != "C++") return 13;
    tabs->setCurrentIndex(0);
    if (status->text() != "Python") return 14;
    editor = dynamic_cast<Editor *>(tabs->currentWidget());
    editor->setPlainText("ret");
    if (editor->extraSelections().size() != 1 ||
        editor->extraSelections().first().format.background().color().green() <=
            editor->extraSelections().first().format.background().color().red()) return 21;
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    editor->setTextCursor(cursor);
    QKeyEvent typeU(QEvent::KeyPress, Qt::Key_U, Qt::NoModifier, "u");
    QApplication::sendEvent(editor, &typeU);
    auto *completer = editor->findChild<QCompleter *>();
    if (!completer || completer->currentCompletion() != "return") return 19;
    QApplication::sendEvent(editor, &enter);
    if (editor->toPlainText() != "return") return 20;
    editor->setPlainText("def calculate(value):\n    pass\ncalc");
    cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    editor->setTextCursor(cursor);
    QKeyEvent complete(QEvent::KeyPress, Qt::Key_Space, Qt::ControlModifier, " ");
    QApplication::sendEvent(editor, &complete);
    if (completer->currentCompletion() != "calculate") return 22;
    QApplication::sendEvent(editor, &enter);
    if (!editor->toPlainText().endsWith("calculate()") ||
        editor->textCursor().position() != editor->toPlainText().size() - 1) return 23;
    editor->setPlainText("pri");
    cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    editor->setTextCursor(cursor);
    QApplication::sendEvent(editor, &complete);
    if (completer->currentCompletion() != "print") return 24;
    QApplication::sendEvent(editor, &enter);
    if (editor->toPlainText() != "print()" || editor->textCursor().position() != 6) return 25;
    MainWindow emptyWindow(nullptr, true);
    auto *emptyTabs = qobject_cast<QTabWidget *>(emptyWindow.centralWidget());
    if (!emptyTabs || emptyTabs->count() != 0) return 26;
    const QString path = settingsDirectory.filePath("welcome.cpp");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write("int value = 7;\n") < 0) return 27;
    file.close();
    if (!emptyWindow.openFile(path) || emptyTabs->count() != 1) return 28;
    auto *opened = dynamic_cast<Editor *>(emptyTabs->currentWidget());
    if (!opened || opened->toPlainText() != "int value = 7;\n") return 29;
    if (recentFiles().first() != QFileInfo(path).canonicalFilePath()) return 31;

#ifndef Q_OS_WIN
    const QString fakeGpgPath = settingsDirectory.filePath("fake-gpg");
    QFile fakeGpg(fakeGpgPath);
    const QByteArray fakeScript =
        "#!/bin/sh\n"
        "case \"$*\" in\n"
        "  *--list-keys*) printf 'pub:u:255:22:1234567890ABCDEF:1:::u:::scESC:::::\\n"
        "fpr:::::::::0123456789ABCDEF0123456789ABCDEF01234567:\\n"
        "uid:u::::::::Test User <test@example.com>::::::::::0:\\n' ;;\n"
        "  *--encrypt*) printf '%s\\n' '-----BEGIN PGP MESSAGE-----' '' 'TEST' '-----END PGP MESSAGE-----' ;;\n"
        "  *--pinentry-mode*--decrypt*) read secret; [ \"$secret\" = 'test-password' ] && printf 'messaggio riservato' || exit 2 ;;\n"
        "  *--decrypt*) echo 'gpg: decryption failed: Inappropriate ioctl for device' >&2; exit 2 ;;\n"
        "  *--version*) echo 'gpg (GnuPG) test' ;;\n"
        "esac\n";
    if (!fakeGpg.open(QIODevice::WriteOnly) || fakeGpg.write(fakeScript) != fakeScript.size()) return 45;
    fakeGpg.close();
    fakeGpg.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    QSettings().setValue("openpgp/gpgExecutable", fakeGpgPath);
    MainWindow pgpWindow;
    auto *pgpTabs = qobject_cast<QTabWidget *>(pgpWindow.centralWidget());
    auto *pgpEditor = pgpTabs ? dynamic_cast<Editor *>(pgpTabs->currentWidget()) : nullptr;
    auto *encryptDocument = pgpWindow.findChild<QAction *>("pgpEncryptAction");
    if (!pgpEditor || !encryptDocument) return 46;
    pgpEditor->setPlainText("messaggio riservato");
    bool recipientShown = false;
    QTimer dialogWatcher;
    dialogWatcher.setInterval(10);
    QObject::connect(&dialogWatcher, &QTimer::timeout, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (dialog && dialog->objectName() == "pgpRecipientDialog") {
            recipientShown = true;
            dialog->accept();
        }
    });
    dialogWatcher.start();
    encryptDocument->trigger();
    dialogWatcher.stop();
    if (!recipientShown || !pgpEditor->toPlainText().startsWith("-----BEGIN PGP MESSAGE-----")) return 47;
    auto *decryptDocument = pgpWindow.findChild<QAction *>("pgpDecryptAction");
    bool passwordShown = false;
    QObject::disconnect(&dialogWatcher, nullptr, nullptr, nullptr);
    QObject::connect(&dialogWatcher, &QTimer::timeout, [&] {
        auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
        if (dialog) {
            passwordShown = true;
            dialog->setTextValue("test-password");
            dialog->accept();
        }
    });
    dialogWatcher.start();
    decryptDocument->trigger();
    dialogWatcher.stop();
    if (!passwordShown || pgpEditor->toPlainText() != "messaggio riservato") return 48;
#endif
    return 0;
}
