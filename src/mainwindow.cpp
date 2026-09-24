#include "mainwindow.h"
#include "editor.h"
#include "language.h"
#include "recentfiles.h"
#include "dictationcontroller.h"
#include "dictationoverlay.h"
#include "speechbackend.h"
#include "recordingcontroller.h"
#include "audiolinks.h"
#include "googlecloudmenu.h"
#include "savepaths.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDialog>
#include <QDialogButtonBox>
#include <QEvent>
#include <QColorDialog>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QCollator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFontDatabase>
#include <QIcon>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QTextEdit>
#include <QPixmap>
#include <QPrintDialog>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QPrinter>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QSvgRenderer>
#include <QTabWidget>
#include <QTabBar>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QProgressBar>
#include <QProgressDialog>
#include <QProcess>
#include <QEventLoop>
#include <QTimer>
#include <QTemporaryFile>
#include <QUuid>
#include <QUrl>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
QIcon toolbarIcon(const QString &name)
{
    QSvgRenderer renderer(QString(":/icons/%1.svg").arg(name));
    QIcon icon;
    for (const int size : {24, 48}) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        renderer.render(&painter);
        icon.addPixmap(pixmap);
    }
    return icon;
}

QString languageSaveFilter(const LanguageDefinition &language)
{
    QStringList patterns;
    for (const QString &extension : language.extensions)
        patterns << "*." + extension;
    return QString("%1 (%2)").arg(language.name, patterns.join(' '));
}

QStringList editorSaveFilters()
{
    QStringList filters{QObject::tr("File di testo (*.txt)")};
    for (const LanguageDefinition &language : languages())
        if (!language.extensions.isEmpty()) filters << languageSaveFilter(language);
    filters << QObject::tr("Tutti i file (*)");
    return filters;
}

QString firstFilterExtension(const QString &filter, const QString &fallback)
{
    const QRegularExpressionMatch match = QRegularExpression("\\*\\.([A-Za-z0-9_+-]+)").match(filter);
    return match.hasMatch() ? match.captured(1) : fallback;
}

QString gpgExecutablePath()
{
    const QString configured = QSettings().value("openpgp/gpgExecutable").toString();
    if (!configured.isEmpty() && QFileInfo(configured).isExecutable()) return configured;
    for (const QString &name : {QStringLiteral("gpg"), QStringLiteral("gpg2")}) {
        const QString path = QStandardPaths::findExecutable(name);
        if (!path.isEmpty()) return path;
    }
    QStringList candidates;
#ifdef Q_OS_MACOS
    candidates << "/opt/homebrew/bin/gpg" << "/usr/local/bin/gpg"
               << "/usr/local/MacGPG2/bin/gpg2";
#elif defined(Q_OS_WIN)
    candidates << "C:/Program Files/GnuPG/bin/gpg.exe"
               << "C:/Program Files (x86)/GnuPG/bin/gpg.exe"
               << "C:/Program Files (x86)/Gpg4win/bin/gpg.exe"
               << QCoreApplication::applicationDirPath() + "/gpg/bin/gpg.exe";
#else
    candidates << "/usr/bin/gpg" << "/usr/local/bin/gpg"
               << QCoreApplication::applicationDirPath() + "/gpg";
#endif
    for (const QString &path : candidates)
        if (QFileInfo(path).isExecutable()) return path;
    return {};
}

struct GpgResult {
    QByteArray output;
    QString error;
    int exitCode = -1;
    bool canceled = false;
};

GpgResult runGpg(QWidget *parent, const QString &executable, const QStringList &arguments,
                 const QByteArray &input, const QString &label)
{
    GpgResult result;
    QProcess process;
    process.setProgram(executable);
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    QProgressDialog progress(label, QObject::tr("Annulla"), 0, 0, parent);
    progress.setWindowTitle(QObject::tr("OpenPGP"));
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(400);
    progress.setAutoClose(false);
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop, &QEventLoop::quit);
    QObject::connect(&process, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
    const QMetaObject::Connection cancelConnection =
        QObject::connect(&progress, &QProgressDialog::canceled, &loop, [&] {
        result.canceled = true;
        process.kill();
    });
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&] {
        result.error = QObject::tr("L'operazione OpenPGP ha superato il tempo massimo.");
        process.kill();
    });
    process.start();
    if (!process.waitForStarted(4000)) {
        result.error = process.errorString();
        return result;
    }
    process.write(input);
    process.closeWriteChannel();
    timeout.start(120000);
    loop.exec();
    process.waitForFinished(1000);
    timeout.stop();
    QObject::disconnect(cancelConnection);
    progress.close();
    result.output = process.readAllStandardOutput();
    if (result.error.isEmpty()) result.error = QString::fromUtf8(process.readAllStandardError()).trimmed();
    result.exitCode = process.exitCode();
    return result;
}

GpgResult runGpgWithInputFile(QWidget *parent, const QString &executable,
                              QStringList arguments, const QByteArray &input,
                              const QString &label, const QByteArray &processInput = {})
{
    QTemporaryFile inputFile(QDir::tempPath() + "/nebipad-openpgp-XXXXXX");
    inputFile.setAutoRemove(true);
    if (!inputFile.open() || inputFile.write(input) != input.size() || !inputFile.flush()) {
        GpgResult result;
        result.error = QObject::tr("Impossibile preparare il documento temporaneo per GnuPG: %1")
                           .arg(inputFile.errorString());
        return result;
    }
    inputFile.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    arguments << inputFile.fileName();
    const GpgResult result = runGpg(parent, executable, arguments, processInput, label);
    inputFile.close();
    return result;
}

QString decodeGpgField(const QByteArray &value)
{
    QByteArray decoded;
    for (int i = 0; i < value.size(); ++i) {
        if (value.at(i) == '\\' && i + 3 < value.size() && value.at(i + 1) == 'x') {
            bool ok = false;
            const char byte = char(value.mid(i + 2, 2).toUInt(&ok, 16));
            if (ok) { decoded += byte; i += 3; continue; }
        }
        decoded += value.at(i);
    }
    return QString::fromUtf8(decoded);
}

}

MainWindow::MainWindow(QWidget *parent, bool startEmpty)
    : QMainWindow(parent), tabs_(new QTabWidget(this)),
      fileTypeLabel_(new QLabel(this)), lengthLabel_(new QLabel(this)),
      linesLabel_(new QLabel(this)), cursorLabel_(new QLabel(this)),
      systemLabel_(new QLabel(this)), encodingLabel_(new QLabel(this))
{
    setWindowTitle("NebiPad+");
    resize(1000, 700);
    menuBar()->setNativeMenuBar(false);
    tabs_->setDocumentMode(true);
    tabs_->setTabsClosable(false);
    setCentralWidget(tabs_);
    dictation_ = new DictationController(this);
    recording_ = new RecordingController(this);
    dictationOverlay_ = new DictationOverlay(tabs_);
    recording_->setStateChanged([this] {
        const bool active = recording_->isRecording();
        recordAction_->setChecked(active);
        if (active) {
            dictationOverlay_->setRecordingMode(true);
            dictationOverlay_->setState(DictationController::State::Listening);
            placeDictationOverlay();
            statusBar()->showMessage(tr("Registrazione audio in corso · premi Stop per terminare"));
        } else {
            dictationOverlay_->setState(DictationController::State::Idle);
            dictationOverlay_->setRecordingMode(false);
            statusBar()->showMessage(tr("Registrazione audio salvata."), 4000);
        }
    });
    recording_->setLevelsChanged([this](const QVector<float> &levels) {
        dictationOverlay_->setLevels(levels);
    });
    recording_->setError([this](const QString &message) {
        statusBar()->showMessage(message, 8000);
        QMessageBox::warning(this, tr("Registrazione audio"), message);
    });
    tabs_->installEventFilter(this);
    dictation_->setCallbacks(
        [this](DictationController::State state) {
            dictationOverlay_->setState(state);
            placeDictationOverlay();
            if (state == DictationController::State::Listening)
                statusBar()->showMessage(tr("Dettatura attiva · F12 per terminare"));
            else if (state == DictationController::State::Processing)
                statusBar()->showMessage(tr("Trascrizione in corso..."));
            else if (state == DictationController::State::Idle)
                statusBar()->clearMessage();
        },
        [this](const QVector<float> &levels) { dictationOverlay_->setLevels(levels); },
        [this](const QString &text) { dictationOverlay_->setTranscript(text); },
        [this](const QString &text) { insertDictatedText(text); },
        [this](const QString &message) {
            statusBar()->showMessage(message, 8000);
            QMessageBox::warning(this, tr("Dettatura vocale"), message);
        });

    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    auto *newAction = fileMenu->addAction(tr("&Nuovo"), QKeySequence::New);
    auto *openAction = fileMenu->addAction(tr("&Apri..."), QKeySequence::Open);
    fileMenu->addSeparator();
    auto *saveAction = fileMenu->addAction(tr("&Salva"), QKeySequence::Save);
    auto *saveAsAction = fileMenu->addAction(tr("Salva &con nome..."), QKeySequence::SaveAs);
    auto *saveAllAction = fileMenu->addAction(tr("Salva tutto"));
    auto *recentMenu = fileMenu->addMenu(tr("File recenti"));
    connect(recentMenu, &QMenu::aboutToShow, this, [this, recentMenu] {
        recentMenu->clear();
        const QStringList paths = recentFiles();
        if (paths.isEmpty()) {
            recentMenu->addAction(tr("Nessun file recente"))->setEnabled(false);
            return;
        }
        for (const QString &path : paths) {
            auto *action = recentMenu->addAction(QFileInfo(path).fileName());
            action->setToolTip(path);
            connect(action, &QAction::triggered, this, [this, path] { openFile(path); });
        }
    });
    fileMenu->addSeparator();
    auto *closeAction = fileMenu->addAction(tr("&Chiudi scheda"), QKeySequence::Close);
    auto *closeAllAction = fileMenu->addAction(tr("Chiudi tutte le schede"));
    fileMenu->addSeparator();
    auto *printAction = fileMenu->addAction(tr("&Stampa..."), QKeySequence::Print);
    auto *exportPdfAction = fileMenu->addAction(tr("Esporta in PDF..."));
    exportPdfAction->setObjectName("mainExportPdfAction");

    auto *editMenu = menuBar()->addMenu(tr("&Modifica"));
    auto *undoAction = editMenu->addAction(tr("&Annulla"), QKeySequence::Undo);
    auto *redoAction = editMenu->addAction(tr("&Ripeti"), QKeySequence::Redo);
    editMenu->addSeparator();
    auto *cutAction = editMenu->addAction(tr("&Taglia"), QKeySequence::Cut);
    auto *copyAction = editMenu->addAction(tr("&Copia"), QKeySequence::Copy);
    auto *pasteAction = editMenu->addAction(tr("&Incolla"), QKeySequence::Paste);
    auto *selectAllAction = editMenu->addAction(tr("Seleziona &tutto"), QKeySequence::SelectAll);

    auto *searchMenu = menuBar()->addMenu(tr("&Ricerca"));
    auto *findAction = searchMenu->addAction(tr("&Trova..."), QKeySequence::Find);
    auto *replaceAction = searchMenu->addAction(tr("&Sostituisci tutto..."), QKeySequence::Replace);

    auto *viewMenu = menuBar()->addMenu(tr("&Visualizza"));
    auto *zoomInAction = viewMenu->addAction(tr("Ingrandisci"));
    auto *zoomOutAction = viewMenu->addAction(tr("Riduci"));
    viewMenu->addSeparator();
    auto *wrapAction = viewMenu->addAction(tr("Ritorno a capo automatico"));
    wrapAction->setCheckable(true);
    wrapAction->setChecked(true);

    auto *encryptAction = new QAction(toolbarIcon("encrypt"), tr("Crittografa documento con OpenPGP"), this);
    encryptAction->setObjectName("pgpEncryptAction");
    auto *decryptAction = new QAction(toolbarIcon("decrypt"), tr("Decrittografa documento OpenPGP"), this);
    decryptAction->setObjectName("pgpDecryptAction");
    connect(encryptAction, &QAction::triggered, this, &MainWindow::encryptCurrentDocument);
    connect(decryptAction, &QAction::triggered, this, &MainWindow::decryptCurrentDocument);

    auto *toolbar = addToolBar(tr("Main Toolbar"));
    toolbar->setObjectName("mainToolbar");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(24, 24));
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->addAction(newAction);
    toolbar->addAction(openAction);
    toolbar->addAction(saveAction);
    toolbar->addAction(saveAllAction);
    toolbar->addAction(closeAction);
    toolbar->addAction(closeAllAction);
    toolbar->addAction(printAction);
    toolbar->addSeparator();
    toolbar->addAction(undoAction);
    toolbar->addAction(redoAction);
    toolbar->addSeparator();
    toolbar->addAction(cutAction);
    toolbar->addAction(copyAction);
    toolbar->addAction(pasteAction);
    toolbar->addSeparator();
    toolbar->addAction(findAction);
    toolbar->addAction(replaceAction);
    toolbar->addSeparator();
    toolbar->addAction(zoomInAction);
    toolbar->addAction(zoomOutAction);
    toolbar->addAction(wrapAction);
    toolbar->addSeparator();
    toolbar->addAction(encryptAction);
    toolbar->addAction(decryptAction);
    toolbar->addSeparator();
    recordAction_ = toolbar->addAction(toolbarIcon("record"), tr("Registra voce"));
    recordAction_->setObjectName("recordAudioAction");
    recordAction_->setCheckable(true);
    playPauseAction_ = toolbar->addAction(toolbarIcon("play-pause"), tr("Play / Pausa audio"));
    playPauseAction_->setObjectName("playPauseAudioAction");
    stopAudioAction_ = toolbar->addAction(toolbarIcon("stop-audio"), tr("Stop audio"));
    stopAudioAction_->setObjectName("stopAudioAction");
    auto *gainLabel = new QLabel(tr("Gain"), toolbar);
    gainLabel->setContentsMargins(8, 0, 3, 0);
    toolbar->addWidget(gainLabel);
    auto *gainControl = new QSpinBox(toolbar);
    gainControl->setObjectName("recordingGainControl");
    gainControl->setRange(25, 100);
    gainControl->setSuffix(" %");
    gainControl->setToolTip(tr("Livello registrazione audio"));
    gainControl->setAccessibleName(tr("Guadagno registrazione"));
    gainControl->setFixedWidth(76);
    const int savedGain = qBound(25, QSettings().value("audio/recordingGain", 75).toInt(), 100);
    gainControl->setValue(savedGain);
    recording_->setGainPercent(savedGain);
    toolbar->addWidget(gainControl);
    connect(gainControl, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int percent) {
        recording_->setGainPercent(percent);
        QSettings().setValue("audio/recordingGain", percent);
    });
    for (auto *action : {recordAction_, playPauseAction_, stopAudioAction_})
        action->setToolTip(action->text());
    connect(recordAction_, &QAction::triggered, this, &MainWindow::startRecording);
    connect(playPauseAction_, &QAction::triggered, this, &MainWindow::playPauseRecording);
    connect(stopAudioAction_, &QAction::triggered, this, &MainWindow::stopAudio);

    addToolBarBreak();
    auto *formatToolbar = addToolBar(tr("Formatting"));
    formatToolbar->setObjectName("formatToolbar");
    formatToolbar->setMovable(false);
    formatToolbar->setIconSize(QSize(24, 24));
    fontCombo_ = new QFontComboBox(formatToolbar);
    fontCombo_->setToolTip(tr("Font family"));
    fontCombo_->setMinimumWidth(170);
    formatToolbar->addWidget(fontCombo_);
    formatToolbar->addSeparator();

    auto *formatMenu = menuBar()->addMenu(tr("F&ormato"));
    boldAction_ = formatMenu->addAction(tr("Grassetto"), QKeySequence::Bold);
    italicAction_ = formatMenu->addAction(tr("Corsivo"), QKeySequence::Italic);
    underlineAction_ = formatMenu->addAction(tr("Sottolineato"), QKeySequence::Underline);
    strikeAction_ = formatMenu->addAction(tr("Barrato"));
    auto *colorAction = formatMenu->addAction(tr("Colore del testo..."));
    for (auto *action : {boldAction_, italicAction_, underlineAction_, strikeAction_})
        action->setCheckable(true);
    formatToolbar->addAction(boldAction_);
    formatToolbar->addAction(italicAction_);
    formatToolbar->addAction(underlineAction_);
    formatToolbar->addAction(strikeAction_);
    formatToolbar->addAction(colorAction);
    formatToolbar->addSeparator();

    alignLeftAction_ = formatMenu->addAction(tr("Allinea a sinistra"));
    alignCenterAction_ = formatMenu->addAction(tr("Centra"));
    alignRightAction_ = formatMenu->addAction(tr("Allinea a destra"));
    alignJustifyAction_ = formatMenu->addAction(tr("Giustifica"));
    auto *alignmentGroup = new QActionGroup(this);
    for (auto *action : {alignLeftAction_, alignCenterAction_, alignRightAction_, alignJustifyAction_}) {
        action->setCheckable(true);
        alignmentGroup->addAction(action);
        formatToolbar->addAction(action);
    }

    formatMenu->addSeparator();
    auto *encodingMenu = formatMenu->addMenu(tr("Codifica"));
    auto *encodingGroup = new QActionGroup(this);
    for (int i = 0; i < 5; ++i) {
        const auto encoding = static_cast<TextEncoding>(i);
        auto *action = encodingMenu->addAction(encodingName(encoding));
        action->setCheckable(true);
        encodingGroup->addAction(action);
        encodingActions_[i] = action;
        connect(action, &QAction::triggered, this, [this, encoding] { selectEncoding(encoding); });
    }
    formatMenu->addSeparator();
    for (int i = 0; i < 5; ++i) {
        const auto encoding = static_cast<TextEncoding>(i);
        auto *action = formatMenu->addAction(tr("Converti in %1").arg(encodingName(encoding)));
        connect(action, &QAction::triggered, this, [this, encoding] { convertEncoding(encoding); });
    }

    auto *languageMenu = menuBar()->addMenu(tr("&Linguaggio"));
    languageMenu->setObjectName("languageMenu");
    auto *languageGroup = new QActionGroup(this);
    plainLanguageAction_ = languageMenu->addAction(tr("Testo normale"));
    plainLanguageAction_->setCheckable(true);
    languageGroup->addAction(plainLanguageAction_);
    connect(plainLanguageAction_, &QAction::triggered, this, [this] { selectLanguage(-1); });
    languageMenu->addSeparator();
    QChar currentLetter;
    QMenu *letterMenu = nullptr;
    for (int i = 0; i < languages().size(); ++i) {
        const QChar letter = languages()[i].name.at(0).toUpper();
        if (letter != currentLetter) {
            currentLetter = letter;
            letterMenu = languageMenu->addMenu(QString(letter));
        }
        auto *action = letterMenu->addAction(languages()[i].name);
        action->setCheckable(true);
        languageGroup->addAction(action);
        languageActions_.append(action);
        connect(action, &QAction::triggered, this, [this, i] { selectLanguage(i); });
    }
    auto *settingsMenu = menuBar()->addMenu(tr("&Configurazione"));
    settingsMenu->setObjectName("settingsMenu");
    auto *toolsMenu = menuBar()->addMenu(tr("&Strumenti"));
    toolsMenu->setObjectName("toolsMenu");
    auto *dictationAction = toolsMenu->addAction(tr("Dettatura vocale"));
    dictationAction->setObjectName("dictationAction");
    dictationAction->setShortcut(QKeySequence(Qt::Key_F12));
    dictationAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(dictationAction, &QAction::triggered, this, &MainWindow::toggleDictation);
    toolsMenu->addSeparator();
    auto *hashAction = toolsMenu->addAction(tr("Generatore SHA-256 / SHA-512..."));
    hashAction->setObjectName("hashGeneratorAction");
    connect(hashAction, &QAction::triggered, this, &MainWindow::showHashGenerator);
    auto *passwordAction = toolsMenu->addAction(tr("Generatore password casuali..."));
    passwordAction->setObjectName("passwordGeneratorAction");
    connect(passwordAction, &QAction::triggered, this, &MainWindow::showPasswordGenerator);
    toolsMenu->addSeparator();
    auto *pgpMenu = toolsMenu->addMenu(tr("OpenPGP"));
    pgpMenu->setObjectName("openPgpMenu");
    pgpMenu->addAction(encryptAction);
    pgpMenu->addAction(decryptAction);
    pgpMenu->addSeparator();
    auto *selectGpgAction = pgpMenu->addAction(tr("Seleziona eseguibile GnuPG..."));
    selectGpgAction->setObjectName("selectGpgExecutableAction");
    connect(selectGpgAction, &QAction::triggered, this, &MainWindow::selectGpgExecutable);
    pgpMenu->addSeparator();
    auto *createPgpKeyAction = pgpMenu->addAction(tr("Crea coppia di chiavi..."));
    createPgpKeyAction->setObjectName("createPgpKeyAction");
    connect(createPgpKeyAction, &QAction::triggered, this, [this] { createPgpKeyPair(); });
    auto *importPgpKeyAction = pgpMenu->addAction(tr("Importa chiave pubblica..."));
    importPgpKeyAction->setObjectName("importPgpKeyAction");
    connect(importPgpKeyAction, &QAction::triggered, this, [this] { importPgpKey(); });
#ifndef Q_OS_MACOS
    auto *executableAction = toolsMenu->addAction(tr("Seleziona whisper-cli..."));
    connect(executableAction, &QAction::triggered, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, tr("Seleziona whisper-cli"),
            whisperExecutablePath());
        if (!path.isEmpty()) setWhisperExecutablePath(path);
    });
    auto *modelAction = toolsMenu->addAction(tr("Seleziona modello Whisper..."));
    connect(modelAction, &QAction::triggered, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, tr("Seleziona modello Whisper"),
            whisperModelPath(), tr("Modelli Whisper (*.bin);;Tutti i file (*)"));
        if (!path.isEmpty()) setWhisperModelPath(path);
    });
#endif
    auto *macroMenu = menuBar()->addMenu(tr("&Macro"));
    macroMenu->setObjectName("macroMenu");
    startMacroAction_ = macroMenu->addAction(tr("Avvia registrazione macro"));
    startMacroAction_->setObjectName("macroStartRecordingAction");
    startMacroAction_->setShortcut(QKeySequence("Ctrl+Shift+R"));
    stopMacroAction_ = macroMenu->addAction(tr("Ferma registrazione macro"));
    stopMacroAction_->setObjectName("macroStopRecordingAction");
    stopMacroAction_->setEnabled(false);
    playMacroAction_ = macroMenu->addAction(tr("Riproduci macro registrata"));
    playMacroAction_->setObjectName("macroPlayRecordedAction");
    playMacroAction_->setShortcut(QKeySequence("Ctrl+Shift+P"));
    playMacroAction_->setEnabled(false);
    auto *playMultiple = macroMenu->addAction(tr("Riproduci più volte..."));
    playMultiple->setObjectName("macroPlayMultipleAction");
    saveMacroAction_ = macroMenu->addAction(tr("Salva macro registrata..."));
    saveMacroAction_->setObjectName("macroSaveRecordedAction");
    saveMacroAction_->setEnabled(false);
    auto *manageMacrosAction = macroMenu->addAction(tr("Gestisci macro..."));
    manageMacrosAction->setObjectName("macroManageAction");
    macroMenu->addSeparator();
    savedMacrosMenu_ = macroMenu->addMenu(tr("Macro registrate"));
    savedMacrosMenu_->setObjectName("savedMacrosMenu");
    connect(startMacroAction_, &QAction::triggered, this, &MainWindow::startMacroRecording);
    connect(stopMacroAction_, &QAction::triggered, this, &MainWindow::stopMacroRecording);
    connect(playMacroAction_, &QAction::triggered, this, [this] { playMacro(recordedMacro_); });
    connect(saveMacroAction_, &QAction::triggered, this, &MainWindow::saveRecordedMacro);
    connect(playMultiple, &QAction::triggered, this, [this] {
        if (recordedMacro_.isEmpty()) {
            QMessageBox::information(this, tr("Macro"), tr("Registra o seleziona prima una macro."));
            return;
        }
        bool accepted = false;
        const int count = QInputDialog::getInt(this, tr("Riproduci macro"),
                                               tr("Numero di ripetizioni:"), 2, 1, 1000, 1, &accepted);
        if (accepted) playMacro(recordedMacro_, count);
    });
    connect(manageMacrosAction, &QAction::triggered, this, &MainWindow::manageMacros);
    refreshSavedMacros();
    auto *runMenu = menuBar()->addMenu(tr("&Esegui"));
    runMenu->setObjectName("runMenu");
    auto *runAction = runMenu->addAction(tr("Esegui documento"), QKeySequence(Qt::Key_F5));
    runAction->setObjectName("runDocumentAction");
    auto *runWithArgumentsAction = runMenu->addAction(
        tr("Esegui con argomenti..."), QKeySequence("Shift+F5"));
    runWithArgumentsAction->setObjectName("runWithArgumentsAction");
    auto *stopRunAction = runMenu->addAction(
        tr("Interrompi esecuzione"), QKeySequence("Shift+F6"));
    stopRunAction->setObjectName("stopRunAction");
    runMenu->addSeparator();
    auto *chooseRunnerAction = runMenu->addAction(tr("Seleziona compilatore/interprete..."));
    chooseRunnerAction->setObjectName("chooseRunToolAction");
    connect(runAction, &QAction::triggered, this, [this] { runCurrentDocument(false); });
    connect(runWithArgumentsAction, &QAction::triggered, this,
            [this] { runCurrentDocument(true); });
    connect(stopRunAction, &QAction::triggered, this, &MainWindow::stopRunningProgram);
    connect(chooseRunnerAction, &QAction::triggered, this, [this] {
        auto *editor = currentEditor();
        if (!editor || editor->languageIndex() < 0) {
            QMessageBox::information(this, tr("Esegui"),
                                     tr("Seleziona prima il linguaggio del documento."));
            return;
        }
        const QString language = languages()[editor->languageIndex()].name;
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Seleziona compilatore o interprete per %1").arg(language));
        if (!path.isEmpty()) QSettings().setValue("runner/languages/" + language, path);
    });
    auto *pluginMenu = menuBar()->addMenu(tr("&Plugin"));
    pluginMenu->setObjectName("pluginMenu");
    auto *windowMenu = menuBar()->addMenu(tr("&Finestra"));
    windowMenu->setObjectName("windowMenu");
    installGoogleDriveMenu(this, windowMenu->menuAction(),
        [this] { return documentPath(tabs_->currentIndex()); },
        [this] {
            const int index = tabs_->currentIndex();
            return index >= 0 && saveDocument(index);
        },
        [this](const QString &path) { return openFile(path); });
    auto *helpMenu = menuBar()->addMenu(tr("?"));
    helpMenu->setObjectName("helpMenu");

    settingsMenu->addAction(toolbar->toggleViewAction());
    settingsMenu->addAction(formatToolbar->toggleViewAction());
    toolbar->toggleViewAction()->setText(tr("Barra degli strumenti principale"));
    formatToolbar->toggleViewAction()->setText(tr("Barra di formattazione"));
    connect(windowMenu, &QMenu::aboutToShow, this, [this, windowMenu] {
        windowMenu->clear();
        auto *next = windowMenu->addAction(tr("Scheda successiva"));
        auto *previous = windowMenu->addAction(tr("Scheda precedente"));
        next->setEnabled(tabs_->count() > 1);
        previous->setEnabled(tabs_->count() > 1);
        connect(next, &QAction::triggered, this, [this] {
            tabs_->setCurrentIndex((tabs_->currentIndex() + 1) % tabs_->count());
        });
        connect(previous, &QAction::triggered, this, [this] {
            tabs_->setCurrentIndex((tabs_->currentIndex() + tabs_->count() - 1) % tabs_->count());
        });
        windowMenu->addSeparator();
        const auto addSortAction = [this, windowMenu](const QString &label,
                                                      TabSortField field, bool ascending) {
            auto *action = windowMenu->addAction(label);
            action->setEnabled(tabs_->count() > 1);
            connect(action, &QAction::triggered, this,
                    [this, field, ascending] { sortTabs(field, ascending); });
        };
        addSortAction(tr("Nome dalla A alla Z"), TabSortField::Name, true);
        addSortAction(tr("Nome dalla Z alla A"), TabSortField::Name, false);
        addSortAction(tr("Percorso dalla A alla Z"), TabSortField::Path, true);
        addSortAction(tr("Percorso dalla Z alla A"), TabSortField::Path, false);
        addSortAction(tr("Tipo dalla A alla Z"), TabSortField::Type, true);
        addSortAction(tr("Tipo dalla Z alla A"), TabSortField::Type, false);
        addSortAction(tr("Dimensione crescente"), TabSortField::Size, true);
        addSortAction(tr("Dimensione decrescente"), TabSortField::Size, false);
        addSortAction(tr("Data modifica crescente"), TabSortField::Modified, true);
        addSortAction(tr("Data modifica decrescente"), TabSortField::Modified, false);
        if (tabs_->count() > 0) windowMenu->addSeparator();
        for (int i = 0; i < tabs_->count(); ++i) {
            auto *tabAction = windowMenu->addAction(tabs_->tabText(i));
            tabAction->setCheckable(true);
            tabAction->setChecked(i == tabs_->currentIndex());
            connect(tabAction, &QAction::triggered, this, [this, i] { tabs_->setCurrentIndex(i); });
        }
    });
    auto *aboutAction = helpMenu->addAction(tr("Informazioni su NebiPad+"));
    aboutAction->setMenuRole(QAction::NoRole);
    connect(aboutAction, &QAction::triggered, this, [this] {
        QMessageBox::about(this, tr("Informazioni su NebiPad+"),
            tr("NebiPad+ 0.1.0\nEditor di testo per macOS, sviluppato con C++ e Qt 6."));
    });
    auto *aboutQtAction = helpMenu->addAction(tr("Informazioni su Qt"));
    aboutQtAction->setMenuRole(QAction::NoRole);
    connect(aboutQtAction, &QAction::triggered, this, [this] { QMessageBox::aboutQt(this); });

    boldAction_->setIcon(toolbarIcon("bold"));
    italicAction_->setIcon(toolbarIcon("italic"));
    underlineAction_->setIcon(toolbarIcon("underline"));
    strikeAction_->setIcon(toolbarIcon("strike"));
    colorAction->setIcon(toolbarIcon("color"));
    alignLeftAction_->setIcon(toolbarIcon("align-left"));
    alignCenterAction_->setIcon(toolbarIcon("align-center"));
    alignRightAction_->setIcon(toolbarIcon("align-right"));
    alignJustifyAction_->setIcon(toolbarIcon("align-justify"));

    newAction->setIcon(toolbarIcon("new"));
    openAction->setIcon(toolbarIcon("open"));
    saveAction->setIcon(toolbarIcon("save"));
    saveAllAction->setIcon(toolbarIcon("save-all"));
    closeAction->setIcon(toolbarIcon("close"));
    closeAllAction->setIcon(toolbarIcon("close-all"));
    printAction->setIcon(toolbarIcon("print"));
    undoAction->setIcon(toolbarIcon("undo"));
    redoAction->setIcon(toolbarIcon("redo"));
    cutAction->setIcon(toolbarIcon("cut"));
    copyAction->setIcon(toolbarIcon("copy"));
    pasteAction->setIcon(toolbarIcon("paste"));
    findAction->setIcon(toolbarIcon("find"));
    replaceAction->setIcon(toolbarIcon("replace"));
    zoomInAction->setIcon(toolbarIcon("zoom-in"));
    zoomOutAction->setIcon(toolbarIcon("zoom-out"));
    wrapAction->setIcon(toolbarIcon("wrap"));
    for (auto *action : {newAction, openAction, saveAction, saveAllAction,
                         closeAction, closeAllAction, printAction,
                         undoAction, redoAction, cutAction, copyAction,
                         pasteAction, findAction, replaceAction,
                         zoomInAction, zoomOutAction, wrapAction}) {
        action->setToolTip(action->text().remove('&'));
    }
    for (auto *action : {boldAction_, italicAction_, underlineAction_, strikeAction_, colorAction,
                         alignLeftAction_, alignCenterAction_, alignRightAction_, alignJustifyAction_})
        action->setToolTip(action->text().remove('&'));

    connect(newAction, &QAction::triggered, this, [this] { addDocument(); });
    connect(openAction, &QAction::triggered, this, &MainWindow::openDocument);
    connect(saveAction, &QAction::triggered, this, [this] { saveDocument(tabs_->currentIndex()); });
    connect(saveAsAction, &QAction::triggered, this, [this] { saveDocument(tabs_->currentIndex(), true); });
    connect(saveAllAction, &QAction::triggered, this, [this] {
        for (int i = 0; i < tabs_->count(); ++i) {
            auto *editor = qobject_cast<QTextEdit *>(tabs_->widget(i));
            if (editor && editor->document()->isModified() && !saveDocument(i)) return;
        }
    });
    connect(closeAction, &QAction::triggered, this, [this] { closeDocument(tabs_->currentIndex()); });
    connect(closeAllAction, &QAction::triggered, this, [this] { closeAllDocuments(); });
    connect(printAction, &QAction::triggered, this, &MainWindow::printDocument);
    connect(exportPdfAction, &QAction::triggered, this, &MainWindow::exportPdf);
    connect(tabs_, &QTabWidget::tabCloseRequested, this, [this](int index) { closeDocument(index); });
    connect(tabs_, &QTabWidget::currentChanged, this, [this] {
        updateStatusBar();
        updateFormattingControls();
        updateEncodingControls();
        updateLanguageControls();
        dictationOverlay_->raise();
    });

    connect(undoAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->undo(); });
    connect(redoAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->redo(); });
    connect(cutAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->cut(); });
    connect(copyAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->copy(); });
    connect(pasteAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->paste(); });
    connect(selectAllAction, &QAction::triggered, this, [this] { if (auto *e = currentEditor()) e->selectAll(); });
    connect(findAction, &QAction::triggered, this, &MainWindow::findText);
    connect(replaceAction, &QAction::triggered, this, &MainWindow::replaceText);
    connect(zoomInAction, &QAction::triggered, this, [this] {
        if (auto *editor = currentEditor()) editor->zoomIn();
    });
    connect(zoomOutAction, &QAction::triggered, this, [this] {
        if (auto *editor = currentEditor()) editor->zoomOut();
    });
    connect(wrapAction, &QAction::toggled, this, [this](bool enabled) {
        wordWrapEnabled_ = enabled;
        for (int i = 0; i < tabs_->count(); ++i) {
            if (auto *editor = dynamic_cast<Editor *>(tabs_->widget(i)))
                editor->setLineWrapMode(enabled ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
        }
    });

    connect(fontCombo_, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
        if (auto *editor = currentEditor()) {
            QTextCharFormat format;
            format.setFontFamilies({font.family()});
            editor->mergeCurrentCharFormat(format);
            editor->setRichFormattingUsed(true);
            editor->setFocus();
        }
    });
    connect(boldAction_, &QAction::triggered, this, [this](bool checked) {
        if (auto *editor = currentEditor()) {
            QTextCharFormat format;
            format.setFontWeight(checked ? QFont::Bold : QFont::Normal);
            editor->mergeCurrentCharFormat(format);
            editor->setRichFormattingUsed(true);
            editor->setFocus();
        }
    });
    connect(italicAction_, &QAction::triggered, this, [this](bool checked) {
        if (auto *editor = currentEditor()) {
            QTextCharFormat format;
            format.setFontItalic(checked);
            editor->mergeCurrentCharFormat(format);
            editor->setRichFormattingUsed(true);
            editor->setFocus();
        }
    });
    connect(underlineAction_, &QAction::triggered, this, [this](bool checked) {
        if (auto *editor = currentEditor()) {
            QTextCharFormat format;
            format.setFontUnderline(checked);
            editor->mergeCurrentCharFormat(format);
            editor->setRichFormattingUsed(true);
            editor->setFocus();
        }
    });
    connect(strikeAction_, &QAction::triggered, this, [this](bool checked) {
        if (auto *editor = currentEditor()) {
            QTextCharFormat format;
            format.setFontStrikeOut(checked);
            editor->mergeCurrentCharFormat(format);
            editor->setRichFormattingUsed(true);
            editor->setFocus();
        }
    });
    connect(colorAction, &QAction::triggered, this, [this] {
        auto *editor = currentEditor();
        if (!editor) return;
        const QColor color = QColorDialog::getColor(editor->textColor(), this, tr("Text Color"));
        if (!color.isValid()) return;
        QTextCharFormat format;
        format.setForeground(color);
        editor->mergeCurrentCharFormat(format);
        editor->setRichFormattingUsed(true);
        editor->setFocus();
    });
    const auto connectAlignment = [this](QAction *action, Qt::Alignment alignment) {
        connect(action, &QAction::triggered, this, [this, alignment] {
            if (auto *editor = currentEditor()) {
                editor->setAlignment(alignment);
                editor->setRichFormattingUsed(true);
                editor->setFocus();
            }
        });
    };
    connectAlignment(alignLeftAction_, Qt::AlignLeft);
    connectAlignment(alignCenterAction_, Qt::AlignHCenter);
    connectAlignment(alignRightAction_, Qt::AlignRight);
    connectAlignment(alignJustifyAction_, Qt::AlignJustify);

    fileTypeLabel_->setObjectName("languageStatusLabel");
    fileTypeLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    fileTypeLabel_->setContentsMargins(10, 0, 0, 0);
    statusBar()->addWidget(fileTypeLabel_, 1);
    for (auto *label : {lengthLabel_, linesLabel_, cursorLabel_, systemLabel_, encodingLabel_}) {
        label->setContentsMargins(10, 0, 10, 0);
        statusBar()->addPermanentWidget(label);
    }
#if defined(Q_OS_MACOS)
    systemLabel_->setText(tr("macOS"));
#elif defined(Q_OS_WIN)
    systemLabel_->setText(tr("Windows"));
#elif defined(Q_OS_LINUX)
    systemLabel_->setText(tr("Linux"));
#else
    systemLabel_->setText(tr("Unknown OS"));
#endif
    encodingLabel_->setText("UTF-8");
    if (startEmpty) {
        updateStatusBar();
        updateFormattingControls();
        updateEncodingControls();
        updateLanguageControls();
    } else {
        addDocument();
    }
}

void MainWindow::newDocument()
{
    addDocument();
}

Editor *MainWindow::currentEditor() const
{
    return dynamic_cast<Editor *>(tabs_->currentWidget());
}

int MainWindow::addDocument(const QString &path, const QString &text, bool html, TextEncoding encoding)
{
    auto *editor = new Editor(tabs_);
    editor->installEventFilter(this);
    editor->setLineWrapMode(wordWrapEnabled_ ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
    if (html) editor->setHtml(text);
    else editor->setPlainText(text);
    editor->setRichFormattingUsed(html);
    editor->setEncoding(encoding, false);
    editor->setDocumentPath(path);
    editor->setAudioLinkActivated([this, editor](const QString &target) {
        playAudioLink(editor, target);
    });
    editor->setLanguage(detectLanguage(path, text));
    editor->document()->setModified(false);
    editor->setProperty("filePath", path);
    editor->setProperty("tabPinned", false);
    editor->setProperty("tabReadOnly", false);
    const int index = tabs_->addTab(editor, QString());
    installTabControls(index);
    updateTabTitle(index);
    tabs_->setCurrentIndex(index);
    connect(editor->document(), &QTextDocument::modificationChanged, this,
            [this, editor] { updateTabTitle(tabs_->indexOf(editor)); });
    connect(editor, &QTextEdit::textChanged, this, [this, editor] {
        if (editor == currentEditor()) updateStatusBar();
    });
    connect(editor, &QTextEdit::cursorPositionChanged, this, [this, editor] {
        if (editor == currentEditor()) {
            updateStatusBar();
            updateFormattingControls();
        }
    });
    connect(editor, &QTextEdit::currentCharFormatChanged, this, [this, editor] {
        if (editor == currentEditor()) updateFormattingControls();
    });
    updateStatusBar();
    updateFormattingControls();
    updateEncodingControls();
    updateLanguageControls();
    editor->setFocus();
    return index;
}

QString MainWindow::documentPath(int index) const
{
    auto *editor = qobject_cast<QTextEdit *>(tabs_->widget(index));
    return editor ? editor->property("filePath").toString() : QString();
}

void MainWindow::setDocumentPath(int index, const QString &path)
{
    if (auto *editor = qobject_cast<QTextEdit *>(tabs_->widget(index))) {
        editor->setProperty("filePath", path);
        if (auto *typedEditor = dynamic_cast<Editor *>(editor)) typedEditor->setDocumentPath(path);
        updateTabTitle(index);
        if (index == tabs_->currentIndex()) updateStatusBar();
        rememberRecentFile(path);
    }
}

void MainWindow::updateTabTitle(int index)
{
    auto *editor = qobject_cast<QTextEdit *>(tabs_->widget(index));
    if (!editor) return;
    const QString path = documentPath(index);
    const QString name = path.isEmpty() ? tr("Untitled") : QFileInfo(path).fileName();
    const auto *typedEditor = dynamic_cast<Editor *>(editor);
    const bool modified = editor->document()->isModified() ||
        (typedEditor && typedEditor->encodingDirty());
    tabs_->setTabText(index, name + (modified ? " *" : ""));
    tabs_->setTabToolTip(index, path.isEmpty() ? name : path);
    updateTabControls(index);
}

void MainWindow::installTabControls(int index)
{
    QWidget *page = tabs_->widget(index);
    if (!page) return;

    auto makeButton = [this](const char *name, const QString &icon) {
        auto *button = new QToolButton(tabs_->tabBar());
        button->setObjectName(QString::fromLatin1(name));
        button->setAutoRaise(true);
        button->setIcon(toolbarIcon(icon));
        button->setIconSize(QSize(14, 14));
        button->setFixedSize(22, 22);
        button->setStyleSheet(
            "QToolButton { border: 0; border-radius: 4px; padding: 2px; }"
            "QToolButton:hover { background: rgba(49,68,91,35); }"
            "QToolButton:checked { background: rgba(22,134,167,48); }"
            "QToolButton:pressed { background: rgba(22,134,167,70); }");
        return button;
    };

    auto *readOnly = makeButton("tabReadOnlyButton", "save");
    readOnly->setCheckable(true);
    readOnly->setAccessibleName(tr("Sola lettura"));
    connect(readOnly, &QToolButton::clicked, this, [this, page] { toggleTabReadOnly(page); });
    tabs_->tabBar()->setTabButton(index, QTabBar::LeftSide, readOnly);

    auto *right = new QWidget(tabs_->tabBar());
    right->setObjectName("tabRightControls");
    auto *layout = new QHBoxLayout(right);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto *pin = makeButton("tabPinButton", "pin");
    pin->setCheckable(true);
    pin->setAccessibleName(tr("Fissa scheda"));
    auto *close = makeButton("tabCloseButton", "close");
    close->setAccessibleName(tr("Chiudi documento"));
    layout->addWidget(pin);
    layout->addWidget(close);
    connect(pin, &QToolButton::clicked, this, [this, page] { toggleTabPinned(page); });
    connect(close, &QToolButton::clicked, this, [this, page] {
        closeDocument(tabs_->indexOf(page));
    });
    tabs_->tabBar()->setTabButton(index, QTabBar::RightSide, right);
    updateTabControls(index);
}

void MainWindow::updateTabControls(int index)
{
    QWidget *page = tabs_->widget(index);
    if (!page) return;
    if (auto *button = qobject_cast<QToolButton *>(tabs_->tabBar()->tabButton(index, QTabBar::LeftSide))) {
        const bool readOnly = page->property("tabReadOnly").toBool();
        button->setChecked(readOnly);
        button->setToolTip(readOnly ? tr("Sola lettura attiva · clicca per modificare")
                                    : tr("Clicca per rendere il documento in sola lettura"));
    }
    if (auto *controls = tabs_->tabBar()->tabButton(index, QTabBar::RightSide)) {
        if (auto *pin = controls->findChild<QToolButton *>("tabPinButton")) {
            const bool pinned = page->property("tabPinned").toBool();
            pin->setChecked(pinned);
            pin->setToolTip(pinned ? tr("Rimuovi il pin dalla scheda") : tr("Fissa la scheda"));
        }
        if (auto *close = controls->findChild<QToolButton *>("tabCloseButton"))
            close->setToolTip(page->property("tabPinned").toBool()
                ? tr("Rimuovi il pin prima di chiudere") : tr("Chiudi documento"));
    }
}

void MainWindow::toggleTabReadOnly(QWidget *page)
{
    auto *editor = dynamic_cast<Editor *>(page);
    const int index = tabs_->indexOf(page);
    if (!editor || index < 0) return;
    const bool readOnly = !page->property("tabReadOnly").toBool();
    page->setProperty("tabReadOnly", readOnly);
    editor->setReadOnly(readOnly);
    updateTabControls(index);
    statusBar()->showMessage(readOnly ? tr("Documento impostato in sola lettura")
                                      : tr("Documento nuovamente modificabile"), 2500);
}

bool MainWindow::tabIsPinned(int index) const
{
    QWidget *page = tabs_->widget(index);
    return page && page->property("tabPinned").toBool();
}

void MainWindow::toggleTabPinned(QWidget *page)
{
    int index = tabs_->indexOf(page);
    if (index < 0) return;
    const bool pinned = !page->property("tabPinned").toBool();
    page->setProperty("tabPinned", pinned);
    int pinnedOthers = 0;
    for (int i = 0; i < tabs_->count(); ++i)
        if (tabs_->widget(i) != page && tabIsPinned(i)) ++pinnedOthers;
    tabs_->tabBar()->moveTab(index, pinned ? pinnedOthers : pinnedOthers);
    index = tabs_->indexOf(page);
    tabs_->setCurrentIndex(index);
    updateTabControls(index);
    statusBar()->showMessage(pinned ? tr("Scheda fissata") : tr("Pin rimosso dalla scheda"), 2500);
}

bool MainWindow::saveDocument(int index, bool saveAs)
{
    auto *editor = dynamic_cast<Editor *>(tabs_->widget(index));
    if (!editor) return false;
    if (saveAs && recordingTarget_ == editor && recording_->isRecording()) recording_->stop();
    const QString previousPath = documentPath(index);
    QString path = previousPath;
    if (path.isEmpty() || saveAs) {
        const QString suggestion = path.isEmpty()
            ? initialDocumentSavePath(editor->hasRichFormatting()) : path;
        const QStringList filters = editorSaveFilters();
        QString selectedFilter = filters.first();
        if (editor->languageIndex() >= 0)
            selectedFilter = languageSaveFilter(languages()[editor->languageIndex()]);
        path = QFileDialog::getSaveFileName(this, tr("Salva file"), suggestion,
                                            filters.join(";;"), &selectedFilter);
        if (path.isEmpty()) return false;
        if (QFileInfo(path).suffix().isEmpty())
            path += "." + firstFilterExtension(selectedFilter, "txt");
    }
    QString suffix = QFileInfo(path).suffix().toLower();
    bool html = suffix == "html" || suffix == "htm";
    if (editor->hasRichFormatting() && !html) {
        QMessageBox prompt(this);
        prompt.setWindowTitle(tr("Text formatting"));
        prompt.setText(tr("This file format cannot keep text formatting."));
        auto *htmlButton = prompt.addButton(tr("Save as HTML..."), QMessageBox::AcceptRole);
        auto *plainButton = prompt.addButton(tr("Save plain text"), QMessageBox::DestructiveRole);
        prompt.addButton(QMessageBox::Cancel);
        prompt.exec();
        if (prompt.clickedButton() == htmlButton) {
            path = QFileDialog::getSaveFileName(this, tr("Save as HTML"),
                QFileInfo(path).absolutePath() + "/" + QFileInfo(path).completeBaseName() + ".html",
                tr("HTML files (*.html *.htm)"));
            if (path.isEmpty()) return false;
            if (!path.endsWith(".html", Qt::CaseInsensitive) &&
                !path.endsWith(".htm", Qt::CaseInsensitive)) path += ".html";
            html = true;
        } else if (prompt.clickedButton() != plainButton) {
            return false;
        }
    }
    QByteArray bytes;
    const bool htmlSource = editor->languageIndex() >= 0 &&
        languages()[editor->languageIndex()].name == "HTML";
    const QString content = html && editor->hasRichFormatting() && !htmlSource
        ? editor->toHtml() : editor->toPlainText();
    if (!previousPath.isEmpty() && QFileInfo(previousPath).absoluteFilePath() !=
        QFileInfo(path).absoluteFilePath()) {
        QString audioError;
        if (!copyRecordingLinks(previousPath, path, editor->toPlainText(), &audioError)) {
            QMessageBox::warning(this, tr("Registrazione audio"), audioError);
            return false;
        }
    }
    if (!previousPath.isEmpty() && QFileInfo(previousPath).absolutePath() != QFileInfo(path).absolutePath()) {
        for (const QString &name : relativeAudioFiles(editor->toPlainText())) {
            const QString oldAudio = QFileInfo(previousPath).absolutePath() + "/recording/" + name;
            const QString newDirectory = QFileInfo(path).absolutePath() + "/recording";
            if (QFileInfo::exists(oldAudio) &&
                (!QDir().mkpath(newDirectory) ||
                 (!QFileInfo::exists(newDirectory + "/" + name) &&
                  !QFile::copy(oldAudio, newDirectory + "/" + name)))) {
                QMessageBox::warning(this, tr("Registrazione audio"),
                    tr("Impossibile copiare %1 nella cartella recording del nuovo documento.").arg(name));
                return false;
            }
        }
    }
    if (!encodeText(content, editor->encoding(), &bytes)) {
        QMessageBox::warning(this, tr("Codifica non compatibile"),
            tr("Il documento contiene caratteri non rappresentabili in %1.")
                .arg(encodingName(editor->encoding())));
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Save failed"),
            tr("Impossibile salvare %1: %2").arg(path, file.errorString()));
        return false;
    }
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        QMessageBox::critical(this, tr("Save failed"),
            tr("Impossibile salvare %1: %2").arg(path, file.errorString()));
        return false;
    }
    editor->document()->setModified(false);
    editor->setEncoding(editor->encoding(), false);
    setDocumentPath(index, path);
    if (!editor->languageChosenManually()) {
        editor->setLanguage(detectLanguage(path, editor->toPlainText()));
        if (index == tabs_->currentIndex()) {
            updateStatusBar();
            updateLanguageControls();
        }
    }
    statusBar()->showMessage(tr("Saved %1").arg(path), 3000);
    return true;
}

bool MainWindow::maybeSave(int index)
{
    auto *editor = dynamic_cast<Editor *>(tabs_->widget(index));
    if (!editor || (!editor->document()->isModified() && !editor->encodingDirty())) return true;
    const auto choice = QMessageBox::question(this, tr("Unsaved changes"),
        tr("Save changes to %1?").arg(tabs_->tabText(index)),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (choice == QMessageBox::Cancel) return false;
    return choice == QMessageBox::Discard || saveDocument(index);
}

bool MainWindow::closeDocument(int index)
{
    if (index < 0) return false;
    if (tabIsPinned(index)) {
        statusBar()->showMessage(tr("Rimuovi il pin dalla scheda prima di chiuderla"), 3500);
        return false;
    }
    if (!maybeSave(index)) return false;
    QWidget *widget = tabs_->widget(index);
    if (recordingTarget_ == widget) recording_->stop();
    tabs_->removeTab(index);
    widget->deleteLater();
    updateStatusBar();
    return true;
}

bool MainWindow::closeAllDocuments()
{
    bool pinnedRemain = false;
    for (int i = tabs_->count() - 1; i >= 0; --i) {
        if (tabIsPinned(i)) {
            pinnedRemain = true;
            continue;
        }
        if (!maybeSave(i)) return false;
        QWidget *widget = tabs_->widget(i);
        if (recordingTarget_ == widget) recording_->stop();
        tabs_->removeTab(i);
        widget->deleteLater();
    }
    updateStatusBar();
    if (pinnedRemain)
        statusBar()->showMessage(tr("Le schede fissate sono rimaste aperte"), 3500);
    return true;
}

void MainWindow::updateStatusBar()
{
    auto *editor = currentEditor();
    if (!editor) {
        fileTypeLabel_->setText(tr("No document"));
        lengthLabel_->setText(tr("Length: 0"));
        linesLabel_->setText(tr("Lines: 0"));
        cursorLabel_->setText(tr("Ln: —  Col: —  Pos: —"));
        encodingLabel_->setText("—");
        return;
    }
    encodingLabel_->setText(encodingName(editor->encoding()));
    fileTypeLabel_->setText(editor->languageIndex() >= 0
        ? languages()[editor->languageIndex()].name : tr("Normal Text File"));
    lengthLabel_->setText(tr("Length: %1").arg(editor->document()->characterCount() - 1));
    linesLabel_->setText(tr("Lines: %1").arg(editor->document()->blockCount()));
    const QTextCursor cursor = editor->textCursor();
    cursorLabel_->setText(tr("Ln: %1  Col: %2  Pos: %3")
        .arg(cursor.blockNumber() + 1)
        .arg(cursor.positionInBlock() + 1)
        .arg(cursor.position()));
}

void MainWindow::printDocument()
{
    auto *editor = currentEditor();
    if (!editor) return;
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer, this);
    if (dialog.exec() == QDialog::Accepted) editor->print(&printer);
}

void MainWindow::exportPdf()
{
    auto *editor = currentEditor();
    if (!editor) return;
    QString suggestion = documentPath(tabs_->currentIndex());
    suggestion = suggestion.isEmpty() ? tr("Documento.pdf")
        : QFileInfo(suggestion).absolutePath() + "/" + QFileInfo(suggestion).completeBaseName() + ".pdf";
    QString path = QFileDialog::getSaveFileName(this, tr("Esporta in PDF"), suggestion,
                                                tr("PDF (*.pdf)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(".pdf", Qt::CaseInsensitive)) path += ".pdf";
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    editor->print(&printer);
    statusBar()->showMessage(tr("PDF esportato: %1").arg(path), 5000);
}

QString MainWindow::resolveRunTool(const QString &toolId, const QStringList &candidates) const
{
    const QString configured = QSettings().value("runner/tools/" + toolId).toString();
    if (!configured.isEmpty() && QFileInfo(configured).isExecutable()) return configured;
    for (const QString &candidate : candidates) {
        const QString executable = QStandardPaths::findExecutable(candidate);
        if (!executable.isEmpty()) return executable;
    }
    return {};
}

void MainWindow::showMissingRunTool(const QString &toolId, const QString &displayName,
                                    const QUrl &downloadUrl)
{
    QMessageBox dialog(this);
    dialog.setIcon(QMessageBox::Information);
    dialog.setWindowTitle(tr("Compilatore o interprete non trovato"));
    dialog.setText(tr("NebiPad+ non ha trovato %1 nel sistema.").arg(displayName));
    dialog.setInformativeText(tr("Puoi aprire la pagina ufficiale per scaricarlo oppure "
                                 "selezionare un eseguibile già installato."));
    auto *download = dialog.addButton(tr("Scarica %1").arg(displayName), QMessageBox::ActionRole);
    auto *select = dialog.addButton(tr("Seleziona eseguibile..."), QMessageBox::ActionRole);
    dialog.addButton(QMessageBox::Cancel);
    dialog.exec();
    if (dialog.clickedButton() == download) {
        QDesktopServices::openUrl(downloadUrl);
    } else if (dialog.clickedButton() == select) {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Seleziona %1").arg(displayName));
        if (!path.isEmpty()) QSettings().setValue("runner/tools/" + toolId, path);
    }
}

void MainWindow::stopRunningProgram()
{
    if (!runProcess_ || runProcess_->state() == QProcess::NotRunning) {
        statusBar()->showMessage(tr("Nessun programma in esecuzione"), 2500);
        return;
    }
    runProcess_->terminate();
    if (!runProcess_->waitForFinished(1200)) runProcess_->kill();
    if (runOutput_) runOutput_->appendPlainText(tr("\n[esecuzione interrotta]"));
}

void MainWindow::runCurrentDocument(bool askArguments)
{
    Editor *editor = currentEditor();
    const int tabIndex = tabs_->currentIndex();
    if (!editor || tabIndex < 0) return;
    if (editor->languageIndex() < 0) {
        QMessageBox::information(this, tr("Esegui"),
                                 tr("Seleziona un linguaggio di programmazione prima di eseguire."));
        return;
    }
    if (runProcess_ && runProcess_->state() != QProcess::NotRunning) {
        QMessageBox::information(this, tr("Esegui"),
                                 tr("Un programma è già in esecuzione. Interrompilo prima di avviarne un altro."));
        return;
    }
    if (documentPath(tabIndex).isEmpty() || editor->document()->isModified() || editor->encodingDirty()) {
        if (!saveDocument(tabIndex)) return;
    }

    const QString source = QFileInfo(documentPath(tabIndex)).absoluteFilePath();
    const QString workingDirectory = QFileInfo(source).absolutePath();
    const QString language = languages()[editor->languageIndex()].name;
    if (askArguments) {
        bool accepted = false;
        const QString entered = QInputDialog::getText(
            this, tr("Argomenti del programma"),
            tr("Argomenti da passare al programma:"), QLineEdit::Normal,
            runArguments_, &accepted);
        if (!accepted) return;
        runArguments_ = entered;
    }
    const QStringList userArguments = QProcess::splitCommand(runArguments_);

    if (language == "HTML" || language == "SVG" || language == "XML") {
        QDesktopServices::openUrl(QUrl::fromLocalFile(source));
        statusBar()->showMessage(tr("Documento aperto nel browser"), 3000);
        return;
    }

    struct Plan {
        QString toolId;
        QString displayName;
        QStringList candidates;
        QUrl download;
        QStringList compileArguments;
        QString runProgram;
        QStringList runArguments;
        bool compile = false;
    } plan;
    const QString base = QFileInfo(source).completeBaseName();
    const QString outputDirectory = QDir(QDir::tempPath()).filePath("NebiPadPlus/run");
    QDir().mkpath(outputDirectory);
    QString nativeOutput = QDir(outputDirectory).filePath(base);
#ifdef Q_OS_WIN
    nativeOutput += ".exe";
#endif
    const auto interpreted = [&](const QString &id, const QString &name,
                                 const QStringList &commands, const QString &url) {
        plan.toolId = id; plan.displayName = name; plan.candidates = commands;
        plan.download = QUrl(url); plan.runArguments = {source};
        plan.runArguments.append(userArguments);
    };
    const auto compiled = [&](const QString &id, const QString &name,
                              const QStringList &commands, const QString &url,
                              const QStringList &arguments) {
        plan.toolId = id; plan.displayName = name; plan.candidates = commands;
        plan.download = QUrl(url); plan.compile = true;
        plan.compileArguments = arguments; plan.runProgram = nativeOutput;
        plan.runArguments = userArguments;
    };

    if (language == "Python") interpreted("python", "Python", {"python3", "python"}, "https://www.python.org/downloads/");
    else if (language == "JavaScript") interpreted("node", "Node.js", {"node"}, "https://nodejs.org/en/download");
    else if (language == "TypeScript") interpreted("ts-node", "ts-node", {"ts-node", "tsx"}, "https://www.typescriptlang.org/download/");
    else if (language == "Ruby") interpreted("ruby", "Ruby", {"ruby"}, "https://www.ruby-lang.org/en/downloads/");
    else if (language == "PHP") interpreted("php", "PHP", {"php"}, "https://www.php.net/downloads.php");
    else if (language == "Perl") interpreted("perl", "Perl", {"perl"}, "https://www.perl.org/get.html");
    else if (language == "Lua") interpreted("lua", "Lua", {"lua", "lua5.4", "lua5.3"}, "https://www.lua.org/download.html");
    else if (language == "R") interpreted("rscript", "R", {"Rscript"}, "https://cran.r-project.org/");
    else if (language == "Julia") interpreted("julia", "Julia", {"julia"}, "https://julialang.org/downloads/");
    else if (language == "Dart") interpreted("dart", "Dart SDK", {"dart"}, "https://dart.dev/get-dart");
    else if (language == "Swift") interpreted("swift", "Swift", {"swift"}, "https://www.swift.org/install/");
    else if (language == "Bash") interpreted("bash", "Bash", {"bash"}, "https://www.gnu.org/software/bash/");
    else if (language == "Shell") interpreted("sh", "Shell POSIX", {"sh"}, "https://www.gnu.org/software/bash/");
    else if (language == "Zsh") interpreted("zsh", "Zsh", {"zsh"}, "https://www.zsh.org/");
    else if (language == "Fish") interpreted("fish", "Fish", {"fish"}, "https://fishshell.com/");
    else if (language == "PowerShell") interpreted("pwsh", "PowerShell", {"pwsh", "powershell"}, "https://learn.microsoft.com/powershell/scripting/install/installing-powershell");
    else if (language == "Tcl") interpreted("tclsh", "Tcl", {"tclsh"}, "https://www.tcl.tk/software/tcltk/download.html");
    else if (language == "Groovy") interpreted("groovy", "Groovy", {"groovy"}, "https://groovy.apache.org/download.html");
    else if (language == "Haskell") interpreted("runhaskell", "GHC", {"runhaskell", "runghc"}, "https://www.haskell.org/ghcup/");
    else if (language == "Elixir") interpreted("elixir", "Elixir", {"elixir"}, "https://elixir-lang.org/install.html");
    else if (language == "Clojure") interpreted("clojure", "Clojure", {"clojure"}, "https://clojure.org/guides/install_clojure");
    else if (language == "F#") interpreted("dotnet", ".NET SDK", {"dotnet"}, "https://dotnet.microsoft.com/download"), plan.runArguments = QStringList{"fsi", source} + userArguments;
    else if (language == "Scala") interpreted("scala", "Scala", {"scala"}, "https://www.scala-lang.org/download/");
    else if (language == "CoffeeScript") interpreted("coffee", "CoffeeScript", {"coffee"}, "https://coffeescript.org/#installation");
    else if (language == "Lisp") interpreted("sbcl", "SBCL", {"sbcl"}, "https://www.sbcl.org/platform-table.html"), plan.runArguments = QStringList{"--script", source} + userArguments;
    else if (language == "Scheme") interpreted("guile", "GNU Guile", {"guile"}, "https://www.gnu.org/software/guile/download/"), plan.runArguments = QStringList{"-s", source} + userArguments;
    else if (language == "Octave") interpreted("octave", "GNU Octave", {"octave"}, "https://octave.org/download"), plan.runArguments = QStringList{"-qf", source} + userArguments;
    else if (language == "Prolog") interpreted("swipl", "SWI-Prolog", {"swipl"}, "https://www.swi-prolog.org/download/stable"), plan.runArguments = QStringList{"-s", source} + userArguments;
    else if (language == "Forth") interpreted("gforth", "Gforth", {"gforth"}, "https://www.gnu.org/software/gforth/");
    else if (language == "Raku") interpreted("raku", "Raku", {"raku"}, "https://rakudo.org/downloads");
    else if (language == "Escript") interpreted("escript", "Erlang/OTP", {"escript"}, "https://www.erlang.org/downloads");
    else if (language == "AWK") interpreted("awk", "AWK", {"awk", "gawk"}, "https://www.gnu.org/software/gawk/");
    else if (language == "C") compiled("c", "Clang/GCC", {"clang", "gcc"}, "https://clang.llvm.org/get_started.html", {source, "-o", nativeOutput});
    else if (language == "C++") compiled("cpp", "Clang++/G++", {"clang++", "g++"}, "https://clang.llvm.org/get_started.html", {source, "-std=c++17", "-o", nativeOutput});
    else if (language == "Ada") compiled("ada", "GNAT", {"gnatmake"}, "https://www.adacore.com/download", {source, "-o", nativeOutput});
    else if (language == "Fortran" || language == "Fortran fixed form") compiled("fortran", "GFortran", {"gfortran"}, "https://gcc.gnu.org/fortran/", {source, "-o", nativeOutput});
    else if (language == "D") compiled("d", "LDC/DMD", {"ldc2", "dmd"}, "https://dlang.org/download.html", {source, "-of=" + nativeOutput});
    else if (language == "COBOL") compiled("cobol", "GnuCOBOL", {"cobc"}, "https://gnucobol.sourceforge.io/", {"-x", source, "-o", nativeOutput});
    else if (language == "Pascal") compiled("pascal", "Free Pascal", {"fpc"}, "https://www.freepascal.org/download.html", {"-o" + nativeOutput, source});
    else if (language == "Nim") compiled("nim", "Nim", {"nim"}, "https://nim-lang.org/install.html", {"c", "-o:" + nativeOutput, source});
    else if (language == "Crystal") compiled("crystal", "Crystal", {"crystal"}, "https://crystal-lang.org/install/", {"build", source, "-o", nativeOutput});
    else if (language == "Rust") compiled("rust", "Rust", {"rustc"}, "https://www.rust-lang.org/tools/install", {source, "-o", nativeOutput});
    else if (language == "Zig") compiled("zig", "Zig", {"zig"}, "https://ziglang.org/download/", {"build-exe", source, "-femit-bin=" + nativeOutput});
    else if (language == "Go") interpreted("go", "Go", {"go"}, "https://go.dev/dl/"), plan.runArguments = QStringList{"run", source} + userArguments;
    else if (language == "Java") {
        plan.toolId = "javac"; plan.displayName = "JDK"; plan.candidates = {"javac"};
        plan.download = QUrl("https://adoptium.net/"); plan.compile = true;
        plan.compileArguments = {"-d", outputDirectory, source};
        plan.runProgram = resolveRunTool("java", {"java"});
        plan.runArguments = {"-cp", outputDirectory, base}; plan.runArguments.append(userArguments);
    } else if (language == "Makefile") interpreted("make", "Make", {"make", "gmake"}, "https://www.gnu.org/software/make/"), plan.runArguments = QStringList{"-f", source} + userArguments;
    else if (language == "CMake") interpreted("cmake", "CMake", {"cmake"}, "https://cmake.org/download/"), plan.runArguments = QStringList{"-P", source} + userArguments;
    else if (language == "Arduino Files") interpreted("arduino-cli", "Arduino CLI", {"arduino-cli"}, "https://arduino.github.io/arduino-cli/latest/installation/"), plan.runArguments = QStringList{"compile", workingDirectory} + userArguments;
    else if (language == "Batch") {
#ifdef Q_OS_WIN
        interpreted("cmd", "Prompt dei comandi", {"cmd.exe"}, "https://learn.microsoft.com/windows-server/administration/windows-commands/cmd");
        plan.runArguments = QStringList{"/c", source} + userArguments;
#endif
    }

    if (plan.toolId.isEmpty()) {
        QMessageBox::information(this, tr("Esegui"),
            tr("L'esecuzione automatica non è ancora configurata per %1. "
               "Puoi selezionare manualmente un eseguibile dal menu Esegui.").arg(language));
        return;
    }
    QString tool = QSettings().value("runner/languages/" + language).toString();
    if (tool.isEmpty() || !QFileInfo(tool).isExecutable())
        tool = resolveRunTool(plan.toolId, plan.candidates);
    if (tool.isEmpty()) {
        showMissingRunTool(plan.toolId, plan.displayName, plan.download);
        return;
    }
    if (language == "Java" && plan.runProgram.isEmpty()) {
        showMissingRunTool("java", "Java Runtime", QUrl("https://adoptium.net/"));
        return;
    }

    auto *dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Output esecuzione · %1").arg(QFileInfo(source).fileName()));
    dialog->resize(820, 440);
    auto *layout = new QVBoxLayout(dialog);
    auto *output = new QPlainTextEdit(dialog);
    output->setObjectName("runOutputConsole");
    output->setReadOnly(true);
    output->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont consoleFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    output->setFont(consoleFont);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, dialog);
    auto *stop = buttons->addButton(tr("Interrompi"), QDialogButtonBox::ActionRole);
    layout->addWidget(output); layout->addWidget(buttons);
    auto *process = new QProcess(dialog);
    process->setWorkingDirectory(workingDirectory);
    process->setProcessChannelMode(QProcess::SeparateChannels);
    runProcess_ = process; runOutput_ = output;
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::close);
    connect(stop, &QPushButton::clicked, this, &MainWindow::stopRunningProgram);
    connect(dialog, &QDialog::finished, this, [this, process] {
        if (process->state() != QProcess::NotRunning) process->kill();
        if (runProcess_ == process) { runProcess_.clear(); runOutput_.clear(); }
    });
    connect(process, &QProcess::readyReadStandardOutput, dialog, [process, output] {
        output->moveCursor(QTextCursor::End);
        output->insertPlainText(QString::fromLocal8Bit(process->readAllStandardOutput()));
    });
    connect(process, &QProcess::readyReadStandardError, dialog, [process, output] {
        output->moveCursor(QTextCursor::End);
        output->insertPlainText(QString::fromLocal8Bit(process->readAllStandardError()));
    });
    connect(process, &QProcess::errorOccurred, dialog, [output](QProcess::ProcessError) {
        output->appendPlainText(QObject::tr("\n[errore di avvio del processo]"));
    });
    process->setProperty("compilePhase", plan.compile);
    process->setProperty("runProgram", plan.runProgram);
    process->setProperty("runArguments", plan.runArguments);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), dialog,
            [process, output, stop](int exitCode, QProcess::ExitStatus exitStatus) {
        if (process->property("compilePhase").toBool()) {
            process->setProperty("compilePhase", false);
            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                output->appendPlainText(QObject::tr("\n[compilazione completata · avvio programma]\n"));
                process->start(process->property("runProgram").toString(),
                               process->property("runArguments").toStringList());
                return;
            }
            output->appendPlainText(QObject::tr("\n[compilazione fallita · codice %1]").arg(exitCode));
        } else {
            output->appendPlainText(QObject::tr("\n[processo terminato · codice %1]").arg(exitCode));
        }
        stop->setEnabled(false);
    });
    output->appendPlainText(plan.compile
        ? tr("Compilazione di %1...\n").arg(source)
        : tr("Esecuzione di %1...\n").arg(source));
    dialog->show();
    process->start(tool, plan.compile ? plan.compileArguments : plan.runArguments);
    statusBar()->showMessage(tr("Programma avviato"), 2500);
}

void MainWindow::openDocument()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open File"));
    if (!path.isEmpty()) openFile(path);
}

bool MainWindow::openFile(const QString &path)
{
    if (path.isEmpty()) return false;
    for (int i = 0; i < tabs_->count(); ++i) {
        if (documentPath(i) == path) {
            tabs_->setCurrentIndex(i);
            rememberRecentFile(path);
            return true;
        }
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(isVisible() ? this : nullptr, tr("Open failed"), file.errorString());
        forgetRecentFile(path);
        return false;
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        QMessageBox::critical(isVisible() ? this : nullptr, tr("Open failed"), file.errorString());
        return false;
    }
    const TextEncoding encoding = detectEncoding(bytes);
    QString content;
    if (!decodeText(bytes, encoding, &content)) {
        QMessageBox::critical(isVisible() ? this : nullptr, tr("Errore di codifica"),
            tr("Impossibile leggere il file come %1.").arg(encodingName(encoding)));
        return false;
    }
    QString migratedContent = content;
    QString migrationError;
    const bool migrationSucceeded = migrateLegacyAudioLinks(path, &migratedContent, &migrationError);
    if (!migrationSucceeded && !migrationError.isEmpty())
        statusBar()->showMessage(migrationError, 8000);
    const bool migrated = migrationSucceeded && migratedContent != content;
    if (migrated) content = migratedContent;
    const int index = addDocument(path, content, false, encoding);
    if (migrated) {
        if (auto *editor = dynamic_cast<Editor *>(tabs_->widget(index)))
            editor->document()->setModified(true);
        updateTabTitle(index);
    }
    rememberRecentFile(path);
    return true;
}

void MainWindow::updateEncodingControls()
{
    auto *editor = currentEditor();
    for (int i = 0; i < 5; ++i) {
        encodingActions_[i]->setEnabled(editor != nullptr);
        if (editor && static_cast<int>(editor->encoding()) == i)
            encodingActions_[i]->setChecked(true);
    }
}

void MainWindow::updateLanguageControls()
{
    auto *editor = currentEditor();
    plainLanguageAction_->setEnabled(editor != nullptr);
    if (editor && editor->languageIndex() < 0) plainLanguageAction_->setChecked(true);
    for (int i = 0; i < languageActions_.size(); ++i) {
        languageActions_[i]->setEnabled(editor != nullptr);
        if (editor && editor->languageIndex() == i) languageActions_[i]->setChecked(true);
    }
}

void MainWindow::selectLanguage(int index)
{
    auto *editor = currentEditor();
    if (!editor) return;
    const QString newShebang = index >= 0 ? languages()[index].shebang : QString();
    const QTextBlock firstBlock = editor->document()->firstBlock();
    const bool hasShebang = firstBlock.text().startsWith("#!");
    if ((hasShebang && firstBlock.text() != newShebang) ||
        (!hasShebang && !newShebang.isEmpty())) {
        const QTextCursor originalCursor = editor->textCursor();
        const int oldPrefix = hasShebang
            ? firstBlock.text().size() + (firstBlock.next().isValid() ? 1 : 0) : 0;
        const int newPrefix = newShebang.isEmpty() ? 0 : newShebang.size() + 1;
        const auto shiftedPosition = [oldPrefix, newPrefix](int position) {
            return position < oldPrefix ? qMin(position, newPrefix)
                                        : position - oldPrefix + newPrefix;
        };
        QTextCursor editCursor(editor->document());
        editCursor.beginEditBlock();
        if (hasShebang) {
            editCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            if (firstBlock.next().isValid())
                editCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            editCursor.removeSelectedText();
        }
        if (!newShebang.isEmpty()) {
            editCursor.movePosition(QTextCursor::Start);
            editCursor.insertText(newShebang + '\n');
        }
        editCursor.endEditBlock();
        const int maximum = editor->document()->characterCount() - 1;
        QTextCursor restored(editor->document());
        restored.setPosition(qBound(0, shiftedPosition(originalCursor.anchor()), maximum));
        restored.setPosition(qBound(0, shiftedPosition(originalCursor.position()), maximum),
                             QTextCursor::KeepAnchor);
        editor->setTextCursor(restored);
    }
    editor->setLanguage(index, true);
    updateStatusBar();
    updateLanguageControls();
    editor->setFocus();
}

void MainWindow::selectEncoding(TextEncoding encoding)
{
    auto *editor = currentEditor();
    if (!editor || editor->encoding() == encoding) return;
    int index = tabs_->currentIndex();
    QString path = documentPath(index);
    if (path.isEmpty()) {
        editor->setEncoding(encoding, !editor->toPlainText().isEmpty());
        updateTabTitle(index);
        updateStatusBar();
        return;
    }
    if (!maybeSave(index)) {
        updateEncodingControls();
        return;
    }
    path = documentPath(index);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Open failed"), file.errorString());
        updateEncodingControls();
        return;
    }
    const QByteArray bytes = file.readAll();
    const bool missingBom =
        (encoding == TextEncoding::Utf8Bom && !bytes.startsWith("\xef\xbb\xbf")) ||
        (encoding == TextEncoding::Utf16BeBom && !bytes.startsWith("\xfe\xff")) ||
        (encoding == TextEncoding::Utf16LeBom && !bytes.startsWith("\xff\xfe"));
    if (missingBom) {
        QMessageBox::warning(this, tr("BOM assente"),
            tr("Il file non contiene il BOM richiesto. Usa «Converti in %1» per aggiungerlo.")
                .arg(encodingName(encoding)));
        updateEncodingControls();
        return;
    }
    QString content;
    if (file.error() != QFileDevice::NoError || !decodeText(bytes, encoding, &content)) {
        QMessageBox::warning(this, tr("Errore di codifica"),
            tr("Impossibile interpretare il file come %1.").arg(encodingName(encoding)));
        updateEncodingControls();
        return;
    }
    editor->setPlainText(content);
    editor->setRichFormattingUsed(false);
    editor->document()->setModified(false);
    editor->setEncoding(encoding, false);
    updateTabTitle(index);
    updateStatusBar();
    updateFormattingControls();
    updateEncodingControls();
}

void MainWindow::convertEncoding(TextEncoding encoding)
{
    auto *editor = currentEditor();
    if (!editor || editor->encoding() == encoding) return;
    QByteArray bytes;
    if (!encodeText(editor->toPlainText(), encoding, &bytes)) {
        QMessageBox::warning(this, tr("Codifica non compatibile"),
            tr("Il documento contiene caratteri non rappresentabili in %1.")
                .arg(encodingName(encoding)));
        return;
    }
    editor->setEncoding(encoding, true);
    updateTabTitle(tabs_->currentIndex());
    updateStatusBar();
    updateEncodingControls();
}

void MainWindow::sortTabs(TabSortField field, bool ascending)
{
    if (tabs_->count() < 2) return;
    struct TabInfo {
        QWidget *widget;
        QString name;
        QString path;
        QString type;
        qint64 size;
        qint64 modified;
    };
    std::vector<TabInfo> documents;
    documents.reserve(tabs_->count());
    QWidget *active = tabs_->currentWidget();
    for (int i = 0; i < tabs_->count(); ++i) {
        auto *editor = dynamic_cast<Editor *>(tabs_->widget(i));
        const QString path = documentPath(i);
        const QFileInfo file(path);
        const bool onDisk = !path.isEmpty() && file.exists();
        qint64 size = onDisk ? file.size() : 0;
        if (editor && (!onDisk || editor->document()->isModified() || editor->encodingDirty())) {
            QByteArray bytes;
            const bool html = file.suffix().compare("html", Qt::CaseInsensitive) == 0 ||
                              file.suffix().compare("htm", Qt::CaseInsensitive) == 0;
            const bool htmlSource = editor->languageIndex() >= 0 &&
                languages()[editor->languageIndex()].name == "HTML";
            const QString content = html && editor->hasRichFormatting() && !htmlSource
                ? editor->toHtml() : editor->toPlainText();
            size = encodeText(content, editor->encoding(), &bytes)
                ? bytes.size() : content.toUtf8().size();
        }
        documents.push_back({tabs_->widget(i),
                             path.isEmpty() ? tr("Senza titolo") : file.fileName(),
                             path.isEmpty() ? QString() : file.absoluteFilePath(),
                             file.suffix(), size,
                             onDisk ? file.lastModified().toMSecsSinceEpoch() : 0});
    }

    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setNumericMode(true);
    std::stable_sort(documents.begin(), documents.end(),
        [field, ascending, &collator](const TabInfo &a, const TabInfo &b) {
            int comparison = 0;
            switch (field) {
            case TabSortField::Name: comparison = collator.compare(a.name, b.name); break;
            case TabSortField::Path: comparison = collator.compare(a.path, b.path); break;
            case TabSortField::Type: comparison = collator.compare(a.type, b.type); break;
            case TabSortField::Size: comparison = (a.size > b.size) - (a.size < b.size); break;
            case TabSortField::Modified:
                comparison = (a.modified > b.modified) - (a.modified < b.modified);
                break;
            }
            return ascending ? comparison < 0 : comparison > 0;
        });
    for (int target = 0; target < int(documents.size()); ++target) {
        const int current = tabs_->indexOf(documents[target].widget);
        if (current != target) tabs_->tabBar()->moveTab(current, target);
    }
    tabs_->setCurrentWidget(active);
    updateStatusBar();
    updateFormattingControls();
    updateEncodingControls();
}

void MainWindow::updateFormattingControls()
{
    if (!fontCombo_) return;
    auto *editor = currentEditor();
    const bool hasEditor = editor != nullptr;
    fontCombo_->setEnabled(hasEditor);
    for (auto *action : {boldAction_, italicAction_, underlineAction_, strikeAction_,
                         alignLeftAction_, alignCenterAction_, alignRightAction_, alignJustifyAction_})
        action->setEnabled(hasEditor);
    if (!editor) return;

    const QTextCharFormat format = editor->currentCharFormat();
    {
        const QSignalBlocker blocker(fontCombo_);
        fontCombo_->setCurrentFont(editor->currentFont());
    }
    boldAction_->setChecked(format.fontWeight() >= QFont::Bold);
    italicAction_->setChecked(format.fontItalic());
    underlineAction_->setChecked(format.fontUnderline());
    strikeAction_->setChecked(format.fontStrikeOut());
    const Qt::Alignment alignment = editor->alignment();
    alignLeftAction_->setChecked(alignment.testFlag(Qt::AlignLeft));
    alignCenterAction_->setChecked(alignment.testFlag(Qt::AlignHCenter));
    alignRightAction_->setChecked(alignment.testFlag(Qt::AlignRight));
    alignJustifyAction_->setChecked(alignment.testFlag(Qt::AlignJustify));
}

void MainWindow::findText()
{
    auto *editor = currentEditor();
    if (!editor) return;
    const QString selected = editor->textCursor().selectedText();
    bool accepted = false;
    const QString query = QInputDialog::getText(this, tr("Find"), tr("Text to find:"),
                                                QLineEdit::Normal,
                                                selected.isEmpty() ? searchText_ : selected,
                                                &accepted);
    if (!accepted || query.isEmpty()) return;
    searchText_ = query;
    if (!editor->find(query)) {
        QTextCursor cursor = editor->textCursor();
        cursor.movePosition(QTextCursor::Start);
        editor->setTextCursor(cursor);
        if (!editor->find(query)) statusBar()->showMessage(tr("Text not found"), 3000);
    }
}

void MainWindow::replaceText()
{
    auto *editor = currentEditor();
    if (!editor) return;
    bool accepted = false;
    const QString query = QInputDialog::getText(this, tr("Replace All"), tr("Text to find:"),
                                                QLineEdit::Normal, searchText_, &accepted);
    if (!accepted || query.isEmpty()) return;
    const QString replacement = QInputDialog::getText(this, tr("Replace All"),
                                                       tr("Replace with:"), QLineEdit::Normal,
                                                       {}, &accepted);
    if (!accepted) return;
    searchText_ = query;
    QTextCursor editCursor(editor->document());
    QTextCursor cursor(editor->document());
    int count = 0;
    editCursor.beginEditBlock();
    while (!(cursor = editor->document()->find(query, cursor)).isNull()) {
        cursor.insertText(replacement);
        ++count;
    }
    editCursor.endEditBlock();
    statusBar()->showMessage(tr("Replaced %1 occurrence(s)").arg(count), 3000);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (recording_->isRecording()) recording_->stop();
    for (int i = tabs_->count() - 1; i >= 0; --i) {
        if (!maybeSave(i)) {
            event->ignore();
            return;
        }
    }
    recording_->stop();
    event->accept();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == tabs_ && (event->type() == QEvent::Resize || event->type() == QEvent::Show))
        placeDictationOverlay();
    if (macroRecording_ && !macroPlaying_ && watched == currentEditor() &&
        event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        const bool macroShortcut = key->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) &&
                                   (key->key() == Qt::Key_R || key->key() == Qt::Key_P);
        if (!key->isAutoRepeat() && key->key() != Qt::Key_unknown && !macroShortcut &&
            key->key() != Qt::Key_Shift && key->key() != Qt::Key_Control &&
            key->key() != Qt::Key_Alt && key->key() != Qt::Key_Meta) {
            recordedMacro_.append({key->key(), int(key->modifiers()), key->text()});
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::startMacroRecording()
{
    if (!currentEditor()) return;
    recordedMacro_.clear();
    macroRecording_ = true;
    startMacroAction_->setEnabled(false);
    stopMacroAction_->setEnabled(true);
    playMacroAction_->setEnabled(false);
    saveMacroAction_->setEnabled(false);
    currentEditor()->setFocus();
    statusBar()->showMessage(tr("Registrazione macro in corso..."));
}

void MainWindow::stopMacroRecording()
{
    if (!macroRecording_) return;
    macroRecording_ = false;
    startMacroAction_->setEnabled(true);
    stopMacroAction_->setEnabled(false);
    const bool available = !recordedMacro_.isEmpty();
    playMacroAction_->setEnabled(available);
    saveMacroAction_->setEnabled(available);
    statusBar()->showMessage(available
        ? tr("Macro registrata: %1 operazioni").arg(recordedMacro_.size())
        : tr("Registrazione macro vuota"), 4000);
}

void MainWindow::playMacro(const QVector<MacroStep> &steps, int repetitions)
{
    Editor *editor = currentEditor();
    if (!editor || steps.isEmpty() || editor->isReadOnly()) return;
    if (macroRecording_) stopMacroRecording();
    macroPlaying_ = true;
    editor->setFocus();
    QTextCursor transaction = editor->textCursor();
    transaction.beginEditBlock();
    for (int iteration = 0; iteration < repetitions; ++iteration) {
        for (const MacroStep &step : steps) {
            QKeyEvent press(QEvent::KeyPress, step.key,
                            Qt::KeyboardModifiers(step.modifiers), step.text);
            QApplication::sendEvent(editor, &press);
            QKeyEvent release(QEvent::KeyRelease, step.key,
                              Qt::KeyboardModifiers(step.modifiers), step.text);
            QApplication::sendEvent(editor, &release);
        }
    }
    transaction.endEditBlock();
    macroPlaying_ = false;
    statusBar()->showMessage(tr("Macro eseguita %1 volta/e").arg(repetitions), 3000);
}

QVector<MainWindow::MacroStep> MainWindow::loadMacro(const QString &name) const
{
    QVector<MacroStep> result;
    const QJsonDocument document = QJsonDocument::fromJson(
        QSettings().value("macros/entries").toByteArray());
    const QJsonArray steps = document.object().value(name).toArray();
    for (const QJsonValue &value : steps) {
        const QJsonObject step = value.toObject();
        result.append({step.value("key").toInt(), step.value("modifiers").toInt(),
                       step.value("text").toString()});
    }
    return result;
}

void MainWindow::storeMacro(const QString &name, const QVector<MacroStep> &steps)
{
    QSettings settings;
    QJsonObject macros = QJsonDocument::fromJson(settings.value("macros/entries").toByteArray()).object();
    QJsonArray serialized;
    for (const MacroStep &step : steps)
        serialized.append(QJsonObject{{"key", step.key}, {"modifiers", step.modifiers}, {"text", step.text}});
    macros.insert(name, serialized);
    settings.setValue("macros/entries", QJsonDocument(macros).toJson(QJsonDocument::Compact));
    refreshSavedMacros();
}

void MainWindow::saveRecordedMacro()
{
    if (recordedMacro_.isEmpty()) return;
    bool accepted = false;
    const QString name = QInputDialog::getText(this, tr("Salva macro"), tr("Nome della macro:"),
                                                QLineEdit::Normal, {}, &accepted).trimmed();
    if (!accepted || name.isEmpty()) return;
    const QJsonObject existing = QJsonDocument::fromJson(
        QSettings().value("macros/entries").toByteArray()).object();
    if (existing.contains(name) && QMessageBox::question(this, tr("Sostituisci macro"),
        tr("Esiste già una macro chiamata “%1”. Sostituirla?").arg(name)) != QMessageBox::Yes) return;
    storeMacro(name, recordedMacro_);
    statusBar()->showMessage(tr("Macro “%1” salvata").arg(name), 3000);
}

void MainWindow::refreshSavedMacros()
{
    if (!savedMacrosMenu_) return;
    savedMacrosMenu_->clear();
    const QJsonObject macros = QJsonDocument::fromJson(
        QSettings().value("macros/entries").toByteArray()).object();
    QStringList names = macros.keys();
    names.sort(Qt::CaseInsensitive);
    if (names.isEmpty()) {
        QAction *empty = savedMacrosMenu_->addAction(tr("Nessuna macro registrata"));
        empty->setEnabled(false);
        return;
    }
    for (const QString &name : names) {
        QAction *action = savedMacrosMenu_->addAction(name);
        action->setData(name);
        connect(action, &QAction::triggered, this, [this, name] {
            recordedMacro_ = loadMacro(name);
            playMacroAction_->setEnabled(!recordedMacro_.isEmpty());
            saveMacroAction_->setEnabled(!recordedMacro_.isEmpty());
            playMacro(recordedMacro_);
        });
    }
}

void MainWindow::manageMacros()
{
    QSettings settings;
    QJsonObject macros = QJsonDocument::fromJson(settings.value("macros/entries").toByteArray()).object();
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Gestisci macro"));
    dialog.resize(430, 330);
    auto *layout = new QVBoxLayout(&dialog);
    auto *list = new QListWidget(&dialog);
    list->setObjectName("macroManagementList");
    QStringList names = macros.keys(); names.sort(Qt::CaseInsensitive); list->addItems(names);
    layout->addWidget(list);
    auto *buttons = new QHBoxLayout;
    auto *rename = new QPushButton(tr("Rinomina..."), &dialog);
    auto *remove = new QPushButton(tr("Elimina"), &dialog);
    buttons->addWidget(rename); buttons->addWidget(remove); buttons->addStretch();
    layout->addLayout(buttons);
    auto *close = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    layout->addWidget(close);
    connect(close, &QDialogButtonBox::rejected, &dialog, &QDialog::accept);
    connect(remove, &QPushButton::clicked, &dialog, [&] {
        QListWidgetItem *item = list->currentItem(); if (!item) return;
        macros.remove(item->text()); delete item;
    });
    connect(rename, &QPushButton::clicked, &dialog, [&] {
        QListWidgetItem *item = list->currentItem(); if (!item) return;
        bool accepted = false;
        const QString replacement = QInputDialog::getText(&dialog, tr("Rinomina macro"),
            tr("Nuovo nome:"), QLineEdit::Normal, item->text(), &accepted).trimmed();
        if (!accepted || replacement.isEmpty() || replacement == item->text()) return;
        macros.insert(replacement, macros.take(item->text())); item->setText(replacement);
    });
    dialog.exec();
    settings.setValue("macros/entries", QJsonDocument(macros).toJson(QJsonDocument::Compact));
    refreshSavedMacros();
}

void MainWindow::placeDictationOverlay()
{
    if (!dictationOverlay_) return;
    dictationOverlay_->move(qMax(0, (tabs_->width() - dictationOverlay_->width()) / 2),
                            qMax(0, tabs_->height() - dictationOverlay_->height() - 18));
}

void MainWindow::toggleDictation()
{
    if (recording_->isRecording()) {
        statusBar()->showMessage(tr("Termina la registrazione audio prima di avviare la dettatura."), 5000);
        return;
    }
    if (dictation_->state() == DictationController::State::Idle) {
        dictationTarget_ = currentEditor();
        if (!dictationTarget_) {
            statusBar()->showMessage(tr("Apri un documento prima di iniziare la dettatura."), 5000);
            return;
        }
        dictationOverlay_->setTranscript({});
    }
    dictation_->toggle();
}

void MainWindow::showHashGenerator()
{
    QDialog dialog(this);
    dialog.setObjectName("hashGeneratorDialog");
    dialog.setWindowTitle(tr("Generatore SHA-256 / SHA-512"));
    dialog.resize(680, 470);
    auto *layout = new QVBoxLayout(&dialog);
    auto *algorithmRow = new QHBoxLayout;
    algorithmRow->addWidget(new QLabel(tr("Algoritmo:"), &dialog));
    auto *algorithm = new QComboBox(&dialog);
    algorithm->setObjectName("hashAlgorithm");
    algorithm->addItems({"SHA-256", "SHA-512"});
    algorithmRow->addWidget(algorithm);
    algorithmRow->addStretch();
    layout->addLayout(algorithmRow);

    auto *inputTabs = new QTabWidget(&dialog);
    inputTabs->setObjectName("hashInputTabs");
    auto *textInput = new QPlainTextEdit(inputTabs);
    textInput->setObjectName("hashTextInput");
    textInput->setPlaceholderText(tr("Scrivi o incolla il testo da elaborare"));
    inputTabs->addTab(textInput, tr("Testo"));
    auto *filePage = new QWidget(inputTabs);
    auto *fileLayout = new QVBoxLayout(filePage);
    auto *fileRow = new QHBoxLayout;
    auto *filePath = new QLineEdit(filePage);
    filePath->setObjectName("hashFilePath");
    filePath->setPlaceholderText(tr("Seleziona un file"));
    auto *browse = new QPushButton(tr("Sfoglia..."), filePage);
    browse->setObjectName("hashBrowseAction");
    fileRow->addWidget(filePath, 1); fileRow->addWidget(browse);
    fileLayout->addLayout(fileRow); fileLayout->addStretch();
    inputTabs->addTab(filePage, tr("File"));
    layout->addWidget(inputTabs, 1);

    auto *resultLabel = new QLabel(tr("Hash esadecimale:"), &dialog);
    layout->addWidget(resultLabel);
    auto *result = new QLineEdit(&dialog);
    result->setObjectName("hashResult");
    result->setReadOnly(true);
    result->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(result);
    auto *status = new QLabel(&dialog);
    status->setObjectName("hashStatus");
    status->setStyleSheet("color:#526b85;");
    layout->addWidget(status);

    auto *actions = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto *calculate = actions->addButton(tr("Calcola"), QDialogButtonBox::ActionRole);
    calculate->setObjectName("hashCalculateAction");
    auto *copy = actions->addButton(tr("Copia"), QDialogButtonBox::ActionRole);
    copy->setObjectName("hashCopyAction");
    copy->setEnabled(false);
    layout->addWidget(actions);
    connect(actions, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(browse, &QPushButton::clicked, &dialog, [this, filePath] {
        const QString path = QFileDialog::getOpenFileName(this, tr("Seleziona file"));
        if (!path.isEmpty()) filePath->setText(path);
    });
    connect(calculate, &QPushButton::clicked, &dialog,
            [algorithm, inputTabs, textInput, filePath, result, status, copy] {
        const auto method = algorithm->currentIndex() == 0
            ? QCryptographicHash::Sha256 : QCryptographicHash::Sha512;
        QCryptographicHash hash(method);
        if (inputTabs->currentIndex() == 0) {
            const QByteArray bytes = textInput->toPlainText().toUtf8();
            hash.addData(bytes);
            status->setText(QObject::tr("Elaborati %1 byte di testo").arg(bytes.size()));
        } else {
            QFile file(filePath->text());
            if (!file.open(QIODevice::ReadOnly)) {
                result->clear(); copy->setEnabled(false);
                status->setText(QObject::tr("Impossibile leggere il file: %1").arg(file.errorString()));
                return;
            }
            qint64 processed = 0;
            while (!file.atEnd()) {
                const QByteArray chunk = file.read(1024 * 1024);
                if (chunk.isEmpty() && file.error() != QFile::NoError) {
                    result->clear(); copy->setEnabled(false);
                    status->setText(QObject::tr("Errore durante la lettura: %1").arg(file.errorString()));
                    return;
                }
                hash.addData(chunk);
                processed += chunk.size();
            }
            status->setText(QObject::tr("Elaborati %1 byte dal file").arg(processed));
        }
        result->setText(QString::fromLatin1(hash.result().toHex()));
        copy->setEnabled(true);
    });
    connect(copy, &QPushButton::clicked, &dialog, [result, status] {
        QApplication::clipboard()->setText(result->text());
        status->setText(QObject::tr("Hash copiato negli appunti"));
    });
    dialog.exec();
}

void MainWindow::showPasswordGenerator()
{
    QDialog dialog(this);
    dialog.setObjectName("passwordGeneratorDialog");
    dialog.setWindowTitle(tr("Generatore password casuali"));
    dialog.resize(680, 520);
    auto *layout = new QVBoxLayout(&dialog);
    auto *options = new QGridLayout;
    auto *length = new QSpinBox(&dialog);
    length->setObjectName("passwordLength");
    length->setRange(8, 128);
    length->setValue(20);
    auto *count = new QSpinBox(&dialog);
    count->setObjectName("passwordCount");
    count->setRange(1, 20);
    count->setValue(1);
    options->addWidget(new QLabel(tr("Lunghezza:"), &dialog), 0, 0);
    options->addWidget(length, 0, 1);
    options->addWidget(new QLabel(tr("Numero di password:"), &dialog), 0, 2);
    options->addWidget(count, 0, 3);
    layout->addLayout(options);

    auto *lowercase = new QCheckBox(tr("Lettere minuscole (a-z)"), &dialog);
    auto *uppercase = new QCheckBox(tr("Lettere maiuscole (A-Z)"), &dialog);
    auto *numbers = new QCheckBox(tr("Numeri (0-9)"), &dialog);
    auto *symbols = new QCheckBox(tr("Simboli (!@#$...)"), &dialog);
    auto *ambiguous = new QCheckBox(tr("Escludi caratteri ambigui (I, l, 1, O, 0, o, |)"), &dialog);
    lowercase->setObjectName("passwordLowercase");
    uppercase->setObjectName("passwordUppercase");
    numbers->setObjectName("passwordNumbers");
    symbols->setObjectName("passwordSymbols");
    ambiguous->setObjectName("passwordExcludeAmbiguous");
    for (QCheckBox *option : {lowercase, uppercase, numbers, symbols, ambiguous})
        option->setChecked(true);
    layout->addWidget(lowercase); layout->addWidget(uppercase);
    layout->addWidget(numbers); layout->addWidget(symbols); layout->addWidget(ambiguous);

    auto *strength = new QProgressBar(&dialog);
    strength->setObjectName("passwordStrength");
    strength->setRange(0, 100);
    strength->setTextVisible(true);
    auto *entropy = new QLabel(&dialog);
    entropy->setObjectName("passwordEntropy");
    layout->addWidget(strength);
    layout->addWidget(entropy);
    auto *result = new QPlainTextEdit(&dialog);
    result->setObjectName("passwordResult");
    result->setReadOnly(true);
    result->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    result->setPlaceholderText(tr("Le password generate compariranno qui"));
    layout->addWidget(result, 1);
    auto *status = new QLabel(&dialog);
    status->setObjectName("passwordStatus");
    status->setStyleSheet("color:#526b85;");
    layout->addWidget(status);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    auto *generate = buttons->addButton(tr("Genera"), QDialogButtonBox::ActionRole);
    generate->setObjectName("passwordGenerateAction");
    auto *copy = buttons->addButton(tr("Copia"), QDialogButtonBox::ActionRole);
    copy->setObjectName("passwordCopyAction");
    copy->setEnabled(false);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    const auto updateStrength = [=] {
        QString alphabet;
        if (lowercase->isChecked()) alphabet += "abcdefghijklmnopqrstuvwxyz";
        if (uppercase->isChecked()) alphabet += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        if (numbers->isChecked()) alphabet += "0123456789";
        if (symbols->isChecked()) alphabet += "!@#$%^&*()-_=+[]{};:,.?/\\~`";
        if (ambiguous->isChecked())
            for (QChar character : QString("Il1O0o|")) alphabet.remove(character);
        const int alphabetSize = alphabet.size();
        const double bits = alphabetSize > 1
            ? length->value() * std::log2(double(alphabetSize)) : 0;
        const int score = qBound(0, qRound(bits / 1.28), 100);
        strength->setValue(score);
        const QString rating = bits < 50 ? QObject::tr("Debole")
            : bits < 70 ? QObject::tr("Discreta")
            : bits < 100 ? QObject::tr("Forte") : QObject::tr("Molto forte");
        strength->setFormat(rating + " · %p%");
        entropy->setText(QObject::tr("Entropia stimata: %1 bit").arg(bits, 0, 'f', 1));
        generate->setEnabled(alphabetSize > 0);
    };
    for (QCheckBox *option : {lowercase, uppercase, numbers, symbols, ambiguous})
        connect(option, &QCheckBox::toggled, &dialog, updateStrength);
    connect(length, QOverload<int>::of(&QSpinBox::valueChanged), &dialog,
            [updateStrength](int) { updateStrength(); });
    updateStrength();

    connect(generate, &QPushButton::clicked, &dialog, [=] {
        const QString ambiguousCharacters = "Il1O0o|";
        QVector<QString> groups;
        if (lowercase->isChecked()) groups << "abcdefghijklmnopqrstuvwxyz";
        if (uppercase->isChecked()) groups << "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        if (numbers->isChecked()) groups << "0123456789";
        if (symbols->isChecked()) groups << "!@#$%^&*()-_=+[]{};:,.?/\\~`";
        if (ambiguous->isChecked())
            for (QString &group : groups)
                for (QChar character : ambiguousCharacters) group.remove(character);
        groups.erase(std::remove_if(groups.begin(), groups.end(),
            [](const QString &group) { return group.isEmpty(); }), groups.end());
        if (groups.isEmpty()) {
            result->clear(); copy->setEnabled(false);
            status->setText(QObject::tr("Seleziona almeno un gruppo di caratteri"));
            return;
        }
        QString alphabet;
        for (const QString &group : groups) alphabet += group;
        QStringList generated;
        auto *random = QRandomGenerator::system();
        for (int passwordIndex = 0; passwordIndex < count->value(); ++passwordIndex) {
            QString password;
            for (const QString &group : groups)
                password += group.at(random->bounded(group.size()));
            while (password.size() < length->value())
                password += alphabet.at(random->bounded(alphabet.size()));
            for (int index = password.size() - 1; index > 0; --index) {
                const int other = random->bounded(index + 1);
                const QChar character = password.at(index);
                password[index] = password.at(other);
                password[other] = character;
            }
            generated << password;
        }
        result->setPlainText(generated.join('\n'));
        copy->setEnabled(true);
        status->setText(QObject::tr("Generate %1 password con casualità del sistema")
                        .arg(generated.size()));
    });
    connect(copy, &QPushButton::clicked, &dialog, [result, status] {
        QApplication::clipboard()->setText(result->toPlainText());
        status->setText(QObject::tr("Password copiate negli appunti"));
    });
    generate->click();
    dialog.exec();
}

void MainWindow::selectGpgExecutable()
{
    const QString current = gpgExecutablePath();
    const QString path = QFileDialog::getOpenFileName(this, tr("Seleziona GnuPG"),
        current.isEmpty() ? QDir::homePath() : current,
#ifdef Q_OS_WIN
        tr("Eseguibili (*.exe);;Tutti i file (*)"));
#else
        tr("Tutti i file (*)"));
#endif
    if (path.isEmpty()) return;
    QProcess check;
    check.start(path, {"--version"});
    if (!check.waitForStarted(3000) || !check.waitForFinished(5000) ||
        check.exitCode() != 0 || !check.readAllStandardOutput().startsWith("gpg")) {
        QMessageBox::warning(this, tr("OpenPGP"),
            tr("Il file selezionato non è un eseguibile GnuPG valido."));
        return;
    }
    QSettings().setValue("openpgp/gpgExecutable", path);
    statusBar()->showMessage(tr("GnuPG configurato: %1").arg(path), 5000);
}

bool MainWindow::createPgpKeyPair()
{
    const QString executable = gpgExecutablePath();
    if (executable.isEmpty()) {
        QMessageBox::warning(this, tr("GnuPG non trovato"),
            tr("Installa o configura GnuPG prima di creare una chiave."));
        return false;
    }
    QDialog dialog(this);
    dialog.setObjectName("pgpCreateKeyDialog");
    dialog.setWindowTitle(tr("Crea coppia di chiavi OpenPGP"));
    dialog.resize(540, 280);
    auto *layout = new QGridLayout(&dialog);
    auto *name = new QLineEdit(&dialog);
    auto *email = new QLineEdit(&dialog);
    auto *password = new QLineEdit(&dialog);
    auto *confirmation = new QLineEdit(&dialog);
    name->setObjectName("pgpKeyName");
    email->setObjectName("pgpKeyEmail");
    password->setObjectName("pgpKeyPassword");
    confirmation->setObjectName("pgpKeyPasswordConfirmation");
    password->setEchoMode(QLineEdit::Password);
    confirmation->setEchoMode(QLineEdit::Password);
    layout->addWidget(new QLabel(tr("Nome:"), &dialog), 0, 0); layout->addWidget(name, 0, 1);
    layout->addWidget(new QLabel(tr("E-mail:"), &dialog), 1, 0); layout->addWidget(email, 1, 1);
    layout->addWidget(new QLabel(tr("Password della chiave:"), &dialog), 2, 0); layout->addWidget(password, 2, 1);
    layout->addWidget(new QLabel(tr("Ripeti password:"), &dialog), 3, 0); layout->addWidget(confirmation, 3, 1);
    auto *note = new QLabel(tr("La chiave privata rimane nel portachiavi GnuPG locale. Conserva la password in un luogo sicuro."), &dialog);
    note->setWordWrap(true); note->setStyleSheet("color:#526b85;");
    layout->addWidget(note, 4, 0, 1, 2);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Crea chiave"));
    layout->addWidget(buttons, 5, 0, 1, 2);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (name->text().trimmed().size() < 2 || !email->text().contains('@')) {
            QMessageBox::information(&dialog, tr("Dati incompleti"), tr("Inserisci un nome e un indirizzo e-mail valido."));
            return;
        }
        if (password->text().size() < 8 || password->text() != confirmation->text()) {
            QMessageBox::information(&dialog, tr("Password non valida"),
                tr("Usa almeno 8 caratteri e verifica che le due password coincidano."));
            return;
        }
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return false;
    const QString identity = QStringLiteral("%1 <%2>").arg(name->text().trimmed(), email->text().trimmed());
    QByteArray secret = password->text().toUtf8();
    secret.append('\n');
    password->clear(); confirmation->clear();
    const GpgResult generated = runGpg(this, executable,
        {"--batch", "--yes", "--pinentry-mode", "loopback", "--passphrase-fd", "0",
         "--quick-generate-key", identity, "default", "default", "0"},
        secret, tr("Creazione della coppia di chiavi OpenPGP..."));
    secret.fill('\0');
    if (generated.canceled) return false;
    if (generated.exitCode != 0) {
        QMessageBox::critical(this, tr("Creazione chiave non riuscita"), generated.error);
        return false;
    }
    QMessageBox::information(this, tr("Chiave OpenPGP creata"),
        tr("La coppia di chiavi per %1 è stata creata correttamente.").arg(identity));
    return true;
}

bool MainWindow::importPgpKey()
{
    const QString executable = gpgExecutablePath();
    if (executable.isEmpty()) {
        QMessageBox::warning(this, tr("GnuPG non trovato"), tr("Installa o configura GnuPG prima di importare una chiave."));
        return false;
    }
    const QString path = QFileDialog::getOpenFileName(this, tr("Importa chiave pubblica OpenPGP"),
        QDir::homePath(), tr("Chiavi OpenPGP (*.asc *.pgp *.gpg);;Tutti i file (*)"));
    if (path.isEmpty()) return false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Importazione non riuscita"), file.errorString());
        return false;
    }
    const GpgResult imported = runGpg(this, executable, {"--batch", "--yes", "--import"},
                                      file.readAll(), tr("Importazione della chiave OpenPGP..."));
    if (imported.canceled) return false;
    if (imported.exitCode != 0) {
        QMessageBox::critical(this, tr("Importazione non riuscita"), imported.error);
        return false;
    }
    QMessageBox::information(this, tr("Chiave importata"), tr("La chiave OpenPGP è stata importata correttamente."));
    return true;
}

void MainWindow::encryptCurrentDocument()
{
    auto *editor = currentEditor();
    if (!editor) {
        statusBar()->showMessage(tr("Apri un documento da crittografare."), 4000);
        return;
    }
    if (editor->isReadOnly()) {
        QMessageBox::information(this, tr("OpenPGP"),
            tr("Disattiva la sola lettura della scheda prima di crittografare il documento."));
        return;
    }
    const QString executable = gpgExecutablePath();
    if (executable.isEmpty()) {
        QMessageBox::warning(this, tr("GnuPG non trovato"),
            tr("Installa GnuPG (Gpg4win su Windows, GnuPG su Linux o GPG Suite/Homebrew su macOS), "
               "oppure scegli l'eseguibile da Strumenti → OpenPGP."));
        return;
    }

    const GpgResult listing = runGpg(this, executable,
        {"--batch", "--with-colons", "--list-keys"}, {}, tr("Caricamento chiavi pubbliche..."));
    if (listing.canceled) return;
    if (listing.exitCode != 0) {
        QMessageBox::critical(this, tr("OpenPGP"),
            tr("Impossibile leggere le chiavi pubbliche.\n\n%1").arg(listing.error));
        return;
    }

    struct PublicKey { QString fingerprint; QString keyId; QString uid; };
    QVector<PublicKey> keys;
    PublicKey current;
    bool usable = false;
    const auto finishKey = [&] {
        if (usable && !current.fingerprint.isEmpty()) keys.append(current);
    };
    for (const QByteArray &line : listing.output.split('\n')) {
        const QList<QByteArray> fields = line.split(':');
        if (fields.isEmpty()) continue;
        if (fields.at(0) == "pub") {
            finishKey();
            current = {};
            current.keyId = fields.value(4);
            const QByteArray validity = fields.value(1);
            const QByteArray capabilities = fields.value(11).toLower();
            usable = !QByteArray("red").contains(validity.isEmpty() ? '\0' : validity.at(0)) &&
                     (capabilities.isEmpty() || capabilities.contains('e'));
        } else if (fields.at(0) == "fpr" && current.fingerprint.isEmpty()) {
            current.fingerprint = QString::fromLatin1(fields.value(9));
        } else if (fields.at(0) == "uid" && current.uid.isEmpty()) {
            current.uid = decodeGpgField(fields.value(9));
        }
    }
    finishKey();
    if (keys.isEmpty()) {
        QMessageBox prompt(this);
        prompt.setWindowTitle(tr("Nessuna chiave pubblica"));
        prompt.setText(tr("Il portachiavi OpenPGP è vuoto. Crea una coppia di chiavi oppure importa la chiave pubblica del destinatario."));
        auto *create = prompt.addButton(tr("Crea chiave"), QMessageBox::AcceptRole);
        auto *import = prompt.addButton(tr("Importa chiave"), QMessageBox::ActionRole);
        prompt.addButton(QMessageBox::Cancel);
        prompt.exec();
        const bool ready = prompt.clickedButton() == create ? createPgpKeyPair()
                         : prompt.clickedButton() == import ? importPgpKey() : false;
        if (ready) encryptCurrentDocument();
        return;
    }

    QDialog dialog(this);
    dialog.setObjectName("pgpRecipientDialog");
    dialog.setWindowTitle(tr("Crittografa documento con OpenPGP"));
    dialog.resize(620, 190);
    auto *layout = new QVBoxLayout(&dialog);
    auto *description = new QLabel(tr("Scegli il destinatario. Solo chi possiede la chiave privata "
                                      "corrispondente potrà leggere il documento."), &dialog);
    description->setWordWrap(true);
    layout->addWidget(description);
    auto *recipient = new QComboBox(&dialog);
    recipient->setObjectName("pgpRecipient");
    for (const PublicKey &key : keys) {
        const QString identity = key.uid.isEmpty() ? tr("Chiave senza identità") : key.uid;
        recipient->addItem(QStringLiteral("%1  ·  %2").arg(identity, key.keyId), key.fingerprint);
    }
    layout->addWidget(recipient);
    auto *armor = new QLabel(tr("Il risultato sarà salvato nel documento come messaggio OpenPGP ASCII Armor."),
                             &dialog);
    armor->setStyleSheet("color:#526b85;");
    layout->addWidget(armor);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, &dialog);
    auto *encrypt = buttons->addButton(tr("Crittografa"), QDialogButtonBox::AcceptRole);
    encrypt->setObjectName("pgpConfirmEncrypt");
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(encrypt, &QPushButton::clicked, &dialog, &QDialog::accept);
    if (dialog.exec() != QDialog::Accepted) return;

    const QString fingerprint = recipient->currentData().toString();
    const QByteArray plaintext = editor->toPlainText().toUtf8();
    const GpgResult encrypted = runGpgWithInputFile(this, executable,
        {"--batch", "--yes", "--armor", "--output", "-", "--trust-model", "always", "--encrypt",
         "--recipient", fingerprint}, plaintext, tr("Crittografia OpenPGP in corso..."));
    if (encrypted.canceled) return;
    if (encrypted.exitCode != 0 || !encrypted.output.startsWith("-----BEGIN PGP MESSAGE-----")) {
        QMessageBox::critical(this, tr("Crittografia non riuscita"),
            encrypted.error.isEmpty() ? tr("GnuPG non ha prodotto un messaggio valido.") : encrypted.error);
        return;
    }
    editor->setPlainText(QString::fromUtf8(encrypted.output));
    editor->document()->setModified(true);
    statusBar()->showMessage(tr("Documento crittografato per %1").arg(recipient->currentText()), 6000);
}

void MainWindow::decryptCurrentDocument()
{
    auto *editor = currentEditor();
    if (!editor) {
        statusBar()->showMessage(tr("Apri un documento OpenPGP da decrittografare."), 4000);
        return;
    }
    if (editor->isReadOnly()) {
        QMessageBox::information(this, tr("OpenPGP"),
            tr("Disattiva la sola lettura della scheda prima di decrittografare il documento."));
        return;
    }
    const QByteArray ciphertext = editor->toPlainText().toUtf8();
    if (!ciphertext.trimmed().startsWith("-----BEGIN PGP MESSAGE-----")) {
        QMessageBox::information(this, tr("OpenPGP"),
            tr("Il documento attivo non contiene un messaggio OpenPGP ASCII Armor."));
        return;
    }
    const QString executable = gpgExecutablePath();
    if (executable.isEmpty()) {
        QMessageBox::warning(this, tr("GnuPG non trovato"),
            tr("Configura GnuPG da Strumenti → OpenPGP → Seleziona eseguibile GnuPG."));
        return;
    }
    GpgResult decrypted = runGpgWithInputFile(this, executable,
        {"--batch", "--yes", "--output", "-", "--decrypt"}, ciphertext,
        tr("Decrittografia OpenPGP in corso..."));
    if (decrypted.canceled) return;
    const bool needsPassword = decrypted.exitCode != 0 &&
        (decrypted.error.contains("ioctl", Qt::CaseInsensitive) ||
         decrypted.error.contains("pinentry", Qt::CaseInsensitive) ||
         decrypted.error.contains("passphrase", Qt::CaseInsensitive) ||
         decrypted.error.contains("decifratura della chiave pubblica fallita", Qt::CaseInsensitive));
    if (needsPassword) {
        bool accepted = false;
        QString password = QInputDialog::getText(this, tr("Password chiave OpenPGP"),
            tr("Inserisci la password della chiave privata:"), QLineEdit::Password, {}, &accepted);
        if (!accepted) return;
        QByteArray secret = password.toUtf8();
        secret.append('\n');
        password.fill(QChar('\0'));
        decrypted = runGpgWithInputFile(this, executable,
            {"--batch", "--yes", "--pinentry-mode", "loopback", "--passphrase-fd", "0",
             "--output", "-", "--decrypt"}, ciphertext,
            tr("Decrittografia OpenPGP in corso..."), secret);
        secret.fill('\0');
        if (decrypted.canceled) return;
    }
    if (decrypted.exitCode != 0) {
        QMessageBox::critical(this, tr("Decrittografia non riuscita"),
            decrypted.error.isEmpty()
                ? tr("La chiave privata necessaria non è disponibile.") : decrypted.error);
        return;
    }
    editor->setPlainText(QString::fromUtf8(decrypted.output));
    editor->document()->setModified(true);
    statusBar()->showMessage(tr("Documento decrittografato con la chiave privata disponibile"), 6000);
}

void MainWindow::insertDictatedText(const QString &text)
{
    const QString spoken = text.trimmed();
    if (!dictationTarget_ || spoken.isEmpty()) {
        statusBar()->showMessage(tr("Nessuna parola riconosciuta."), 5000);
        return;
    }
    QTextCursor cursor = dictationTarget_->textCursor();
    const QString line = cursor.block().text();
    QString insertion = spoken;
    if (cursor.positionInBlock() > 0 && !line.at(cursor.positionInBlock() - 1).isSpace())
        insertion.prepend(' ');
    if (cursor.positionInBlock() < line.size() &&
        !line.at(cursor.positionInBlock()).isSpace())
        insertion.append(' ');
    cursor.insertText(insertion);
    dictationTarget_->setTextCursor(cursor);
    dictationTarget_->setFocus();
    statusBar()->showMessage(tr("Dettatura inserita nel documento."), 4000);
}

void MainWindow::startRecording()
{
    if (recording_->isRecording()) {
        recording_->stop();
        return;
    }
    recordAction_->setChecked(false);
    if (dictation_->state() != DictationController::State::Idle) {
        statusBar()->showMessage(tr("Termina la dettatura prima di registrare l'audio."), 5000);
        return;
    }
    auto *editor = currentEditor();
    if (!editor) return;
    int index = tabs_->currentIndex();
    if (documentPath(index).isEmpty() && !saveDocument(index)) return;
    const QString document = documentPath(index);
    const QString directory = QFileInfo(document).absolutePath() + "/recording";
    const QString name = QStringLiteral("audio-%1-%2.wav")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"),
             QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
    QString error;
    bool storageError = false;
    QString target = QStringLiteral("recording/%1").arg(name);
    const bool directoryReady = QDir().mkpath(directory);
    bool started = directoryReady &&
        recording_->start(directory + "/" + name, &error, &storageError);
    if (!started && (!directoryReady || storageError)) {
        const QString fallback = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
            + "/recording";
        if (QDir().mkpath(fallback)) {
            error.clear();
            started = recording_->start(fallback + "/" + name, &error, &storageError);
            if (started) {
                target = QUrl::fromLocalFile(fallback + "/" + name).toString(QUrl::FullyEncoded);
                statusBar()->showMessage(tr("Cartella del documento non scrivibile: audio salvato in %1")
                    .arg(fallback), 8000);
            }
        } else {
            error = tr("Impossibile creare %1").arg(fallback);
        }
    }
    if (!started) {
        QMessageBox::warning(this, tr("Registrazione audio"),
            tr("Impossibile registrare in %1. %2").arg(directory, error));
        return;
    }
    const int number = nextRecordingNumber(document, editor->toPlainText());
    QString indexError;
    if (!registerRecordingLink(document, number, target, &indexError)) {
        recording_->stop();
        QFile::remove(resolveAudioTarget(document, target));
        QMessageBox::warning(this, tr("Registrazione audio"), indexError);
        return;
    }
    recordingTarget_ = editor;
    QTextCursor cursor = editor->textCursor();
    QString marker = QStringLiteral("[Registrazione_%1]").arg(number);
    const QString line = cursor.block().text();
    if (cursor.positionInBlock() > 0 && !line.at(cursor.positionInBlock() - 1).isSpace())
        marker.prepend(' ');
    if (cursor.positionInBlock() < line.size() && !line.at(cursor.positionInBlock()).isSpace())
        marker.append(' ');
    cursor.insertText(marker);
    editor->setTextCursor(cursor);
    editor->setFocus();
}

QString MainWindow::audioPathNearCursor(Editor *editor) const
{
    if (!editor) return {};
    const int index = tabs_->indexOf(editor);
    const QString document = documentPath(index);
    if (document.isEmpty()) return {};
    return resolveAudioTarget(document,
        nearestAudioTarget(editor->toPlainText(), editor->textCursor().position()));
}

void MainWindow::playAudioLink(Editor *editor, const QString &target)
{
    const QString document = documentPath(tabs_->indexOf(editor));
    const QString path = resolveAudioTarget(document, target);
    QString error;
    recording_->playPause(path, &error);
    if (!error.isEmpty()) statusBar()->showMessage(error, 6000);
}

void MainWindow::playPauseRecording()
{
    QString error;
    recording_->playPause(audioPathNearCursor(currentEditor()), &error);
    if (!error.isEmpty()) statusBar()->showMessage(error, 6000);
}

void MainWindow::stopAudio()
{
    recording_->stop();
}
