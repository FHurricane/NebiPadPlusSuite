#pragma once

#include <QTextEdit>
#include <QHash>
#include "textencoding.h"
#include "lspclient.h"
#include <functional>

class QCompleter;
class QStandardItemModel;

class Editor final : public QTextEdit
{
public:
    explicit Editor(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;
    void paintLineNumbers(QPaintEvent *event);
    bool hasRichFormatting() const { return richFormattingUsed_; }
    void setRichFormattingUsed(bool value) { richFormattingUsed_ = value; }
    TextEncoding encoding() const { return encoding_; }
    bool encodingDirty() const { return encodingDirty_; }
    void setEncoding(TextEncoding encoding, bool dirty = true) {
        encoding_ = encoding;
        encodingDirty_ = dirty;
    }
    int languageIndex() const { return languageIndex_; }
    bool languageChosenManually() const { return languageChosenManually_; }
    void setLanguage(int index, bool manual = false);
    void setDocumentPath(const QString &path);
    void setAudioLinkActivated(std::function<void(const QString &)> callback) {
        audioLinkActivated_ = std::move(callback);
    }

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void changeEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QCompleter *completer_;
    QStandardItemModel *completionModel_;
    LspClient *lspClient_;
    struct CompletionEntry {
        QString label;
        QString insertText;
        QString detail;
        int score = 0;
        bool callable = false;
        bool snippet = false;
    };
    QHash<QString, CompletionEntry> entries_;
    QString documentPath_;
    std::function<void(const QString &)> audioLinkActivated_;
    int completionGeneration_ = 0;
    QWidget *lineNumberArea_;
    int zoomWheelDelta_ = 0;
    bool richFormattingUsed_ = false;
    TextEncoding encoding_ = TextEncoding::Utf8;
    bool encodingDirty_ = false;
    int languageIndex_ = -1;
    bool languageChosenManually_ = false;
    class LanguageHighlighter *highlighter_;

    void updateLineNumberAreaWidth();
    QString indentUnit() const;
    void updateCompletions(bool explicitRequest = false);
    void insertCompletion(const QString &completion);
    QString completionPrefix() const;
    void showCompletions(const QString &prefix);
    void highlightCurrentLine();
};
