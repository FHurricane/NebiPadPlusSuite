#pragma once

#include <QMainWindow>
#include <QColor>
#include <QPointer>

class QTextEdit;
class QGraphicsView;
class QGraphicsScene;
class QTabWidget;
class QComboBox;
class QLabel;
class QToolButton;
class QFontComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QAction;
class QGraphicsTextItem;
class QTextCharFormat;
class QTextCursor;
class QPoint;
class QPrinter;
class QMenu;
class QToolBar;
class DictationController;
class DictationOverlay;
class RecordingController;

enum class SuiteMode { MindNet, Impagination, Math };

class SuiteWindow final : public QMainWindow
{
public:
    explicit SuiteWindow(SuiteMode mode, QWidget *parent = nullptr);
    ~SuiteWindow() override;
    void newDocument();
    bool openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    SuiteMode mode_;
    QTabWidget *tabs_;
    QComboBox *paperSize_;
    QLabel *status_;
    DictationController *dictation_ = nullptr;
    DictationOverlay *dictationOverlay_ = nullptr;
    QPointer<QTextEdit> dictationTarget_;
    QAction *dictationAction_ = nullptr;
    RecordingController *recording_ = nullptr;
    QPointer<QTextEdit> recordingTarget_;
    QAction *recordAction_ = nullptr;
    QAction *playPauseAction_ = nullptr;
    QAction *stopAudioAction_ = nullptr;
    QColor shapeColor_ = QColor("#e7f4ff");
    QColor lineColor_ = QColor("#4a74a0");
    int lineWidth_ = 2;
    int lineType_ = 0;
    QFontComboBox *mindFontCombo_ = nullptr;
    QSpinBox *mindFontSize_ = nullptr;
    QAction *mindGrowAction_ = nullptr;
    QAction *mindShrinkAction_ = nullptr;
    QAction *mindBoldAction_ = nullptr;
    QAction *mindItalicAction_ = nullptr;
    QAction *mindUnderlineAction_ = nullptr;
    QAction *mindStrikeAction_ = nullptr;
    QAction *mindColorAction_ = nullptr;
    QAction *orientationAction_ = nullptr;
    QAction *rulersAction_ = nullptr;
    QAction *gridAction_ = nullptr;
    QComboBox *rulerUnit_ = nullptr;
    QComboBox *gridUnit_ = nullptr;
    QDoubleSpinBox *rulerStep_ = nullptr;
    QDoubleSpinBox *gridStep_ = nullptr;
    QString pathFor(int index) const;
    void updateTitle(int index);
    bool save(int index, bool saveAs = false);
    bool maybeSave(int index);
    QTextEdit *textEditor() const;
    QGraphicsView *diagramView() const;
    QGraphicsScene *diagramScene() const;
    void addNode(const QString &shape);
    void addConnection(int type);
    void addTextBox();
    QGraphicsTextItem *currentMindText() const;
    void applyMindTextFormat(const QTextCharFormat &format);
    void updateMindTextControls();
    void syncMindViewControls();
    void addDiagramTemplate(const QString &type);
    void setShapeColor();
    void setLineColor();
    void editPageText(const QString &role);
    void zoom(qreal factor);
    void updateStatus();
    void setupImpaginationTools(QMenu *file, QMenu *edit, QMenu *format, QMenu *tools,
                                QToolBar *toolbar);
    void setupPdfTools();
    void toggleDictation();
    void placeDictationOverlay();
    void insertDictatedText(const QString &text);
    void startRecording();
    void playPauseRecording();
    void stopAudio();
    QString audioPathNearCursor(QTextEdit *editor) const;
    void installImpaginationTabControls(int index);
    void updateImpaginationTabControls(int index);
    bool tabIsPinned(int index) const;
    bool closeTab(int index);
    void editImpaginationImage(const QTextCursor &cursor, bool cropImmediately = false);
    void setImpaginationImageWrap(const QTextCursor &cursor, int mode);
    void moveImpaginationImage(const QTextCursor &cursor, const QPoint &viewportPosition);
    void insertImpaginationVideo(const QString &source, bool youtube);
    void playImpaginationVideo(const QString &reference);
    void insertImpaginationChart();
    void printImpagination(QPrinter *printer) const;
    void exportCurrentPdf();
};
