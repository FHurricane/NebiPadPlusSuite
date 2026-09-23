#include "editor.h"
#include "language.h"
#include "languagehighlighter.h"
#include "audiolinks.h"

#include <QPainter>
#include <QApplication>
#include <QPalette>
#include <QCompleter>
#include <QPaintEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QAbstractTextDocumentLayout>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextDocument>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <QSet>
#include <QMap>
#include <QWheelEvent>
#include <QTimer>
#include <algorithm>

namespace {
QIcon completionIcon(QChar letter, const QColor &color)
{
    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(1, 1, 18, 18), 4, 4);
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(12);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, letter);
    return QIcon(pixmap);
}

class LineNumberArea final : public QWidget
{
public:
    explicit LineNumberArea(Editor *editor) : QWidget(editor), editor_(editor) {}

    QSize sizeHint() const override { return {editor_->lineNumberAreaWidth(), 0}; }

protected:
    void paintEvent(QPaintEvent *event) override { editor_->paintLineNumbers(event); }

private:
    Editor *editor_;
};
}

Editor::Editor(QWidget *parent)
    : QTextEdit(parent), lineNumberArea_(new LineNumberArea(this)),
      highlighter_(new LanguageHighlighter(document())),
      completer_(new QCompleter(this)), completionModel_(new QStandardItemModel(this)),
      lspClient_(new LspClient(this))
{
    lineNumberArea_->setObjectName("lineNumberArea");
    completer_->setModel(completionModel_);
    completer_->setCompletionRole(Qt::UserRole);
    completer_->setWidget(this);
    completer_->setCompletionMode(QCompleter::PopupCompletion);
    completer_->setCaseSensitivity(Qt::CaseInsensitive);
    completer_->setModelSorting(QCompleter::UnsortedModel);
    completer_->setMaxVisibleItems(10);
    connect(completer_, QOverload<const QString &>::of(&QCompleter::activated),
            this, &Editor::insertCompletion);
    connect(document(), &QTextDocument::blockCountChanged, this, [this] {
        updateLineNumberAreaWidth();
        lineNumberArea_->update();
    });
    connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged,
            lineNumberArea_, [this] { lineNumberArea_->update(); });
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            lineNumberArea_, [this] { lineNumberArea_->update(); });
    connect(this, &QTextEdit::cursorPositionChanged, this, [this] {
        highlightCurrentLine();
        lineNumberArea_->update();
    });
    updateLineNumberAreaWidth();
    highlightCurrentLine();
}

void Editor::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton &&
        (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier)) &&
        audioLinkActivated_) {
        const QTextCursor clicked = cursorForPosition(event->pos());
        const QString line = clicked.block().text();
        const QString target = audioTargetAt(line, clicked.positionInBlock());
        if (!target.isEmpty()) {
            audioLinkActivated_(target);
            event->accept();
            return;
        }
    }
    QTextEdit::mouseReleaseEvent(event);
}

void Editor::highlightCurrentLine()
{
    QTextEdit::ExtraSelection selection;
    const bool dark = palette().color(QPalette::Base).lightness() < 128;
    selection.format.setBackground(QColor(dark ? "#214032" : "#e3f5e7"));
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    setExtraSelections({selection});
}

void Editor::setLanguage(int index, bool manual)
{
    completer_->popup()->hide();
    languageIndex_ = index >= 0 && index < languages().size() ? index : -1;
    languageChosenManually_ = manual;
    highlighter_->setLanguage(languageIndex_);
    lspClient_->setDocument(languageIndex_ >= 0 ? languages()[languageIndex_].name : QString(),
                            documentPath_, toPlainText());
    const int width = languageIndex_ >= 0 ? languages()[languageIndex_].indentWidth : 4;
    setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * width);
}

void Editor::setDocumentPath(const QString &path)
{
    if (documentPath_ == path) return;
    documentPath_ = path;
    if (languageIndex_ >= 0)
        lspClient_->setDocument(languages()[languageIndex_].name, documentPath_, toPlainText());
}

QString Editor::indentUnit() const
{
    if (languageIndex_ >= 0) {
        const auto &language = languages()[languageIndex_];
        if (language.tabs) return "\t";
        return QString(language.indentWidth, QLatin1Char(' '));
    }
    return "    ";
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && event->modifiers() == Qt::ControlModifier) {
        updateCompletions(true);
        return;
    }
    if (completer_->popup()->isVisible()) {
        if (event->key() == Qt::Key_Escape) {
            completer_->popup()->hide();
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter ||
            event->key() == Qt::Key_Tab) {
            insertCompletion(completer_->currentCompletion());
            return;
        }
        if (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down ||
            event->key() == Qt::Key_PageUp || event->key() == Qt::Key_PageDown) {
            QApplication::sendEvent(completer_->popup(), event);
            return;
        }
    }
    if (event->key() == Qt::Key_Tab && event->modifiers() == Qt::NoModifier) {
        QTextCursor cursor = textCursor();
        cursor.insertText(indentUnit());
        setTextCursor(cursor);
        return;
    }
    if (event->key() == Qt::Key_Backtab) {
        QTextCursor cursor = textCursor();
        const QString line = cursor.block().text();
        const QString unit = indentUnit();
        if (line.startsWith(unit)) {
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, unit.size());
            cursor.removeSelectedText();
            return;
        }
    }
    if (languageIndex_ >= 0 && (languages()[languageIndex_].indentStyle == IndentStyle::Braces ||
        languages()[languageIndex_].indentStyle == IndentStyle::Paren) &&
        (event->text() == "}" || event->text() == "]" || event->text() == ")")) {
        QTextCursor cursor = textCursor();
        const QString before = cursor.block().text().left(cursor.positionInBlock());
        if (!before.isEmpty() && before.trimmed().isEmpty()) {
            const QString unit = indentUnit();
            if (before.endsWith(unit)) {
                cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, unit.size());
                cursor.removeSelectedText();
                setTextCursor(cursor);
            }
        }
    }
    if (languageIndex_ >= 0 && languages()[languageIndex_].indentStyle == IndentStyle::Markup &&
        event->text() == "/") {
        QTextCursor cursor = textCursor();
        const QString before = cursor.block().text().left(cursor.positionInBlock());
        const QString unit = indentUnit();
        if (before.trimmed() == "<" && before.startsWith(unit)) {
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, unit.size());
            cursor.removeSelectedText();
            setTextCursor(cursor);
            cursor.movePosition(QTextCursor::EndOfBlock);
            setTextCursor(cursor);
        }
    }
    if (event->key() != Qt::Key_Return && event->key() != Qt::Key_Enter) {
        QTextEdit::keyPressEvent(event);
        const QString currentLine = textCursor().block().text().left(textCursor().positionInBlock());
        if (lspClient_->available() && (event->text() == "." ||
            currentLine.endsWith("::") || currentLine.endsWith("->")))
            updateCompletions(true);
        else if (event->key() == Qt::Key_Backspace ||
            (event->text().size() == 1 &&
             (event->text().at(0).isLetterOrNumber() || event->text() == "_" || event->text() == "$")))
            updateCompletions();
        else
            completer_->popup()->hide();
        return;
    }

    const QTextCursor beforeCursor = textCursor();
    const QString line = beforeCursor.block().text().left(beforeCursor.positionInBlock());
    int leading = 0;
    while (leading < line.size() && (line[leading] == ' ' || line[leading] == '\t')) ++leading;
    QString indentation = line.left(leading);
    if (languageIndex_ >= 0) {
        const auto style = languages()[languageIndex_].indentStyle;
        const QString trimmed = line.trimmed();
        bool increase = false;
        if (style == IndentStyle::Braces || style == IndentStyle::Paren)
            increase = trimmed.endsWith('{') || trimmed.endsWith('[') || trimmed.endsWith('(');
        else if (style == IndentStyle::Colon)
            increase = trimmed.endsWith(':');
        else if (style == IndentStyle::Markup) {
            static const QRegularExpression openingTag(R"(^<([A-Za-z][\w:-]*)\b[^>]*>$)");
            static const QStringList voidTags = {
                "area", "base", "br", "col", "embed", "hr", "img", "input",
                "link", "meta", "param", "source", "track", "wbr"
            };
            const auto match = openingTag.match(trimmed);
            increase = match.hasMatch() && !trimmed.endsWith("/>") &&
                !voidTags.contains(match.captured(1).toLower());
        }
        else if (style == IndentStyle::Keywords) {
            static const QRegularExpression startsBlock(
                R"(^(?:if|for|foreach|while|until|unless|case|try|def|defp|defmodule|class|module|function|fun|sub|subroutine|procedure|proc|program|begin|do|let|match|receive|cond|fn|loop|select|repeat|type|struct)\b)",
                QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression endsBlock(
                R"(\b(?:then|do|begin|of|else|where|loop)\s*(?:[#;].*)?$)",
                QRegularExpression::CaseInsensitiveOption);
            increase = startsBlock.match(trimmed).hasMatch() || endsBlock.match(trimmed).hasMatch();
        }
        if (increase) indentation += indentUnit();
    }
    QTextCursor editCursor = textCursor();
    editCursor.beginEditBlock();
    QTextEdit::keyPressEvent(event);
    if (!indentation.isEmpty()) {
        QTextCursor afterCursor = textCursor();
        afterCursor.insertText(indentation);
        setTextCursor(afterCursor);
    }
    editCursor.endEditBlock();
    completer_->popup()->hide();
}

QString Editor::completionPrefix() const
{
    const QTextCursor cursor = textCursor();
    const QString line = cursor.block().text();
    int start = cursor.positionInBlock();
    while (start > 0) {
        const QChar character = line.at(start - 1);
        if (!character.isLetterOrNumber() && character != '_' && character != '$') break;
        --start;
    }
    return line.mid(start, cursor.positionInBlock() - start);
}

void Editor::updateCompletions(bool explicitRequest)
{
    if (languageIndex_ < 0 || languageIndex_ >= languages().size()) {
        completer_->popup()->hide();
        return;
    }
    const QString prefix = completionPrefix();
    if (!explicitRequest && prefix.size() < 2) {
        completer_->popup()->hide();
        return;
    }
    ++completionGeneration_;
    entries_.clear();
    const auto add = [this, &prefix](const QString &word, const QString &detail,
                                     int score, bool callable = false,
                                     const QString &insertText = {}, bool snippet = false) {
        if (word.size() <= prefix.size() || !word.startsWith(prefix, Qt::CaseInsensitive)) return;
        const QString key = word.toCaseFolded();
        if (entries_.contains(key) && entries_[key].score >= score) return;
        entries_.insert(key, {word, insertText.isEmpty() ? word : insertText, detail,
                              score, callable, snippet});
    };
    for (const QString &word : languages()[languageIndex_].keywords)
        add(word, tr("Parola chiave"), 30);
    const QString language = languages()[languageIndex_].name;
    static const QMap<QString, QString> builtins = {
        {"Python", "print input len range enumerate zip list dict set tuple str int float bool open sum min max sorted reversed any all isinstance getattr hasattr super self None True False"},
        {"JavaScript", "console log warn error document window Array Object Promise Math JSON parseInt parseFloat setTimeout fetch map filter reduce forEach push pop"},
        {"TypeScript", "console log warn error document window Array Object Promise Math JSON parseInt parseFloat setTimeout fetch map filter reduce forEach push pop"},
        {"C++", "std cout cin cerr endl vector string map unordered_map unique_ptr shared_ptr make_unique make_shared printf scanf size begin end push_back emplace_back"},
        {"C", "printf fprintf scanf snprintf malloc calloc realloc free strlen strcpy strcmp memcpy memset fopen fclose fread fwrite"},
        {"Rust", "println eprintln format vec Some None Ok Err Result Option unwrap expect clone iter map filter collect"},
        {"Go", "fmt Println Printf Sprintf Errorf make append len cap close panic recover new"},
        {"Swift", "print String Int Double Bool Optional Array Dictionary Set map filter reduce guard let var"},
        {"Java", "System out println print String Integer Boolean List Map Set ArrayList HashMap Objects Optional"},
        {"PHP", "echo print_r var_dump isset empty count array_map array_filter strlen strpos explode implode"},
        {"Ruby", "puts print p require attr_reader attr_accessor each map select reduce nil true false"},
        {"Bash", "echo printf read export local source declare test grep sed awk find sort uniq"},
        {"Shell", "echo printf read export local source test grep sed awk find sort uniq"},
        {"Zsh", "echo printf read export local source typeset test grep sed awk find sort uniq"},
        {"SQL", "COUNT SUM AVG MIN MAX COALESCE CAST CASE WHEN THEN ELSE END DISTINCT LIMIT OFFSET"},
        {"HTML", "div span section article header footer main nav button input form label ul li p a img script style"},
        {"CSS", "color background background-color display position margin padding border font-size font-weight width height flex grid justify-content align-items"}
    };
    static const QStringList callableBuiltins = QString(
        "print input len range enumerate zip open sum min max sorted reversed any all "
        "isinstance getattr hasattr super log warn error parseInt parseFloat setTimeout fetch "
        "map filter reduce forEach push pop printf scanf fprintf snprintf malloc calloc "
        "realloc free strlen strcpy strcmp memcpy memset fopen fclose fread fwrite "
        "println eprintln format unwrap expect clone iter collect Println Printf Sprintf "
        "Errorf make append cap close panic recover new COUNT SUM AVG MIN MAX "
        "COALESCE CAST echo read export grep sed awk find sort uniq print_r var_dump "
        "isset empty count array_map array_filter strpos explode implode puts require each "
        "select").split(' ', Qt::SkipEmptyParts);
    for (const QString &word : builtins.value(language).split(' ', Qt::SkipEmptyParts))
        add(word, tr("Funzione o simbolo standard"), 40, callableBuiltins.contains(word));
    static const QRegularExpression identifier(R"(\b[A-Za-z_$][A-Za-z0-9_$]*\b)");
    const QString content = toPlainText();
    const int shebangEnd = content.startsWith("#!") ? content.indexOf('\n') : -1;
    auto matches = identifier.globalMatch(content);
    int scanned = 0;
    while (matches.hasNext() && scanned < 20000) {
        const auto match = matches.next();
        if (shebangEnd >= 0 && match.capturedStart() < shebangEnd) continue;
        const QString word = match.captured();
        if (languages()[languageIndex_].keywords.contains(word, Qt::CaseInsensitive)) continue;
        const int after = match.capturedEnd();
        int next = after;
        while (next < content.size() && content[next].isSpace() && content[next] != '\n') ++next;
        const bool callable = next < content.size() && content[next] == '(';
        add(word, callable ? tr("Funzione nel documento") : tr("Simbolo nel documento"),
            callable ? 70 : 50, callable);
        ++scanned;
    }
    showCompletions(prefix);
    if (lspClient_->available()) {
        const int generation = completionGeneration_;
        const int position = textCursor().position();
        QTimer::singleShot(120, this, [this, generation, position, prefix] {
            if (generation != completionGeneration_ || textCursor().position() != position ||
                completionPrefix() != prefix) return;
            lspClient_->requestCompletion(toPlainText(), textCursor().blockNumber(),
                                          textCursor().positionInBlock(),
                                          [this, generation, position, prefix](QVector<LspCompletion> suggestions) {
            if (generation != completionGeneration_ || textCursor().position() != position ||
                completionPrefix() != prefix) return;
            for (const auto &item : suggestions) {
                const QString key = item.label.toCaseFolded();
                if (item.label.size() <= prefix.size() ||
                    !item.label.startsWith(prefix, Qt::CaseInsensitive)) continue;
                entries_.insert(key, {item.label, item.insertText, item.detail, 100,
                                      item.kind == 2 || item.kind == 3, item.snippet});
            }
            showCompletions(prefix);
            });
        });
    }
}

void Editor::showCompletions(const QString &prefix)
{
    QVector<CompletionEntry> choices = entries_.values().toVector();
    std::sort(choices.begin(), choices.end(), [](const CompletionEntry &a, const CompletionEntry &b) {
        if (a.score != b.score) return a.score > b.score;
        return a.label.compare(b.label, Qt::CaseInsensitive) < 0;
    });
    completionModel_->clear();
    for (int i = 0; i < qMin(choices.size(), 80); ++i) {
        const auto &entry = choices[i];
        auto *item = new QStandardItem(entry.detail.isEmpty() ? entry.label :
                                       entry.label + "    ·    " + entry.detail.left(70));
        static const QIcon functionIcon = completionIcon('F', QColor("#327ac7"));
        static const QIcon keywordIcon = completionIcon('K', QColor("#8252b5"));
        static const QIcon symbolIcon = completionIcon('V', QColor("#278d79"));
        item->setIcon(entry.callable ? functionIcon :
                      entry.detail == tr("Parola chiave") ? keywordIcon : symbolIcon);
        item->setData(entry.label, Qt::UserRole);
        completionModel_->appendRow(item);
    }
    if (completionModel_->rowCount() == 0) {
        completer_->popup()->hide();
        return;
    }
    completer_->setCompletionPrefix(prefix);
    completer_->popup()->setCurrentIndex(completer_->completionModel()->index(0, 0));
    QRect bounds = cursorRect();
    bounds.setWidth(qBound(260, completer_->popup()->sizeHintForColumn(0) + 36, 600));
    completer_->complete(bounds);
}

void Editor::insertCompletion(const QString &completion)
{
    if (completion.isEmpty()) return;
    const CompletionEntry entry = entries_.value(completion.toCaseFolded());
    QString insertion = entry.insertText.isEmpty() ? completion : entry.insertText;
    int cursorOffset = -1;
    if (entry.snippet) {
        static const QRegularExpression placeholder(R"(\$\{(\d+)(?::([^}]*))?\}|\$(\d+))");
        QString expanded;
        int source = 0;
        auto matches = placeholder.globalMatch(insertion);
        while (matches.hasNext()) {
            const auto match = matches.next();
            expanded += insertion.mid(source, match.capturedStart() - source);
            const int number = match.captured(1).isEmpty() ? match.captured(3).toInt() : match.captured(1).toInt();
            if (number == 1 && cursorOffset < 0) cursorOffset = expanded.size();
            expanded += match.captured(2);
            source = match.capturedEnd();
        }
        insertion = expanded + insertion.mid(source);
    } else if (entry.callable && !insertion.endsWith('(')) {
        insertion += "()";
        cursorOffset = insertion.size() - 1;
    }
    QTextCursor cursor = textCursor();
    const QString prefix = completionPrefix();
    const QString line = cursor.block().text();
    int suffixLength = 0;
    while (cursor.positionInBlock() + suffixLength < line.size()) {
        const QChar character = line.at(cursor.positionInBlock() + suffixLength);
        if (!character.isLetterOrNumber() && character != '_' && character != '$') break;
        ++suffixLength;
    }
    cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, prefix.size());
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, prefix.size() + suffixLength);
    cursor.insertText(insertion);
    if (cursorOffset >= 0)
        cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor,
                            insertion.size() - cursorOffset);
    setTextCursor(cursor);
    completer_->popup()->hide();
}

int Editor::lineNumberAreaWidth() const
{
    int digits = 1;
    for (int count = qMax(1, document()->blockCount()); count >= 10; count /= 10) ++digits;
    return 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void Editor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void Editor::resizeEvent(QResizeEvent *event)
{
    QTextEdit::resizeEvent(event);
    const QRect contents = contentsRect();
    lineNumberArea_->setGeometry(contents.left(), contents.top(),
                                 lineNumberAreaWidth(), contents.height());
}

void Editor::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier) ||
        event->modifiers().testFlag(Qt::MetaModifier)) {
        const int delta = event->angleDelta().y() != 0
                            ? event->angleDelta().y() : event->pixelDelta().y() * 8;
        zoomWheelDelta_ += delta;
        const int steps = zoomWheelDelta_ / 120;
        zoomWheelDelta_ %= 120;
        if (steps > 0) zoomIn(steps);
        else if (steps < 0) zoomOut(-steps);
        event->accept();
        return;
    }
    QTextEdit::wheelEvent(event);
}

void Editor::changeEvent(QEvent *event)
{
    QTextEdit::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        highlightCurrentLine();
        lineNumberArea_->update();
    }
    if (event->type() == QEvent::FontChange) {
        updateLineNumberAreaWidth();
        lineNumberArea_->update();
        const int width = languageIndex_ >= 0 ? languages()[languageIndex_].indentWidth : 4;
        setTabStopDistance(fontMetrics().horizontalAdvance(QLatin1Char(' ')) * width);
    }
}

void Editor::paintLineNumbers(QPaintEvent *event)
{
    QPainter painter(lineNumberArea_);
    const bool dark = palette().color(QPalette::Base).lightness() < 128;
    painter.fillRect(event->rect(), QColor(dark ? "#263e56" : "#dceeff"));
    painter.setPen(QColor(dark ? "#9cc5ed" : "#315574"));

    const int scrollY = verticalScrollBar()->value();
    const int firstPosition = document()->documentLayout()->hitTest(QPointF(0, scrollY), Qt::FuzzyHit);
    QTextBlock block = document()->findBlock(qMax(0, firstPosition));

    while (block.isValid()) {
        const QRectF rect = document()->documentLayout()->blockBoundingRect(block);
        const int top = qRound(rect.top()) - scrollY;
        const int bottom = qRound(rect.bottom()) - scrollY;
        if (top > event->rect().bottom()) break;
        if (block.isVisible() && bottom >= event->rect().top()) {
            if (block.blockNumber() == textCursor().blockNumber()) {
                painter.fillRect(QRect(0, top, lineNumberArea_->width(), bottom - top),
                                 QColor(dark ? "#355878" : "#bfdaf7"));
                painter.setPen(QColor(dark ? "#ffffff" : "#174675"));
            }
            painter.drawText(0, top, lineNumberArea_->width() - 6,
                             fontMetrics().height(), Qt::AlignRight,
                             QString::number(block.blockNumber() + 1));
            painter.setPen(QColor(dark ? "#9cc5ed" : "#315574"));
        }
        block = block.next();
    }
    painter.setPen(QColor(dark ? "#4d7091" : "#a8cbe9"));
    painter.drawLine(lineNumberArea_->width() - 1, event->rect().top(),
                     lineNumberArea_->width() - 1, event->rect().bottom());
}
