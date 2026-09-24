#pragma once

#include <QMainWindow>
#include <QPointer>
#include <QString>
#include <QVector>
#include "textencoding.h"

class Editor;
class QAction;
class QFontComboBox;
class QMenu;
class QTabWidget;
class QLabel;
class QPlainTextEdit;
class QProcess;
class QUrl;
class DictationController;
class DictationOverlay;
class RecordingController;

class MainWindow final : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr, bool startEmpty = false);
    void newDocument();
    bool openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class TabSortField { Name, Path, Type, Size, Modified };
    struct MacroStep { int key = 0; int modifiers = 0; QString text; };

    QTabWidget *tabs_;
    QLabel *fileTypeLabel_;
    QLabel *lengthLabel_;
    QLabel *linesLabel_;
    QLabel *cursorLabel_;
    QLabel *systemLabel_;
    QLabel *encodingLabel_;
    DictationController *dictation_ = nullptr;
    DictationOverlay *dictationOverlay_ = nullptr;
    QPointer<Editor> dictationTarget_;
    RecordingController *recording_ = nullptr;
    QPointer<Editor> recordingTarget_;
    QAction *recordAction_ = nullptr;
    QAction *playPauseAction_ = nullptr;
    QAction *stopAudioAction_ = nullptr;
    QString searchText_;
    bool wordWrapEnabled_ = true;
    QFontComboBox *fontCombo_ = nullptr;
    QAction *boldAction_ = nullptr;
    QAction *italicAction_ = nullptr;
    QAction *underlineAction_ = nullptr;
    QAction *strikeAction_ = nullptr;
    QAction *alignLeftAction_ = nullptr;
    QAction *alignCenterAction_ = nullptr;
    QAction *alignRightAction_ = nullptr;
    QAction *alignJustifyAction_ = nullptr;
    QAction *encodingActions_[5] = {};
    QAction *plainLanguageAction_ = nullptr;
    QVector<QAction *> languageActions_;
    QMenu *savedMacrosMenu_ = nullptr;
    QAction *startMacroAction_ = nullptr;
    QAction *stopMacroAction_ = nullptr;
    QAction *playMacroAction_ = nullptr;
    QAction *saveMacroAction_ = nullptr;
    QVector<MacroStep> recordedMacro_;
    bool macroRecording_ = false;
    bool macroPlaying_ = false;
    QPointer<QProcess> runProcess_;
    QPointer<QPlainTextEdit> runOutput_;
    QString runArguments_;

    Editor *currentEditor() const;
    int addDocument(const QString &path = {}, const QString &text = {},
                    bool html = false, TextEncoding encoding = TextEncoding::Utf8);
    QString documentPath(int index) const;
    void setDocumentPath(int index, const QString &path);
    void updateTabTitle(int index);
    void installTabControls(int index);
    void updateTabControls(int index);
    void toggleTabPinned(QWidget *page);
    void toggleTabReadOnly(QWidget *page);
    bool tabIsPinned(int index) const;
    bool saveDocument(int index, bool saveAs = false);
    bool maybeSave(int index);
    bool closeDocument(int index);
    bool closeAllDocuments();
    void openDocument();
    void printDocument();
    void exportPdf();
    void findText();
    void replaceText();
    void updateStatusBar();
    void updateFormattingControls();
    void updateEncodingControls();
    void selectEncoding(TextEncoding encoding);
    void convertEncoding(TextEncoding encoding);
    void sortTabs(TabSortField field, bool ascending);
    void selectLanguage(int index);
    void updateLanguageControls();
    void toggleDictation();
    void showHashGenerator();
    void showPasswordGenerator();
    void encryptCurrentDocument();
    void decryptCurrentDocument();
    void selectGpgExecutable();
    bool createPgpKeyPair();
    bool importPgpKey();
    void placeDictationOverlay();
    void insertDictatedText(const QString &text);
    void startRecording();
    void playPauseRecording();
    void stopAudio();
    void playAudioLink(Editor *editor, const QString &target);
    QString audioPathNearCursor(Editor *editor) const;
    void startMacroRecording();
    void stopMacroRecording();
    void playMacro(const QVector<MacroStep> &steps, int repetitions = 1);
    void saveRecordedMacro();
    void refreshSavedMacros();
    QVector<MacroStep> loadMacro(const QString &name) const;
    void storeMacro(const QString &name, const QVector<MacroStep> &steps);
    void manageMacros();
    void runCurrentDocument(bool askArguments = false);
    void stopRunningProgram();
    QString resolveRunTool(const QString &toolId, const QStringList &candidates) const;
    void showMissingRunTool(const QString &toolId, const QString &displayName,
                            const QUrl &downloadUrl);
};
