#include "languagehighlighter.h"
#include "language.h"

#include <QGuiApplication>
#include <QPalette>
#include <QTextDocument>
#include <QVector>

namespace {
enum Role { Plain, Variable, Keyword, Type, Function, Include, Number, String, Comment, Tag, Property };

void mark(QVector<Role> &roles, int start, int length, Role role)
{
    for (int i = qMax(0, start); i < qMin(start + length, roles.size()); ++i)
        roles[i] = role;
}

void matches(QVector<Role> &roles, const QString &text, const QRegularExpression &expression,
             Role role, int capture = 0)
{
    auto it = expression.globalMatch(text);
    while (it.hasNext()) {
        const auto match = it.next();
        if (match.capturedStart(capture) >= 0)
            mark(roles, match.capturedStart(capture), match.capturedLength(capture), role);
    }
}

bool isMarkup(const QString &name)
{
    return name == "HTML" || name == "XML" || name == "SVG" || name == "JSP" || name == "ASP";
}
}

LanguageHighlighter::LanguageHighlighter(QTextDocument *document) : QSyntaxHighlighter(document) {}

void LanguageHighlighter::setLanguage(int index)
{
    languageIndex_ = index;
    keywords_ = {};
    if (index >= 0 && index < languages().size() && !languages()[index].keywords.isEmpty()) {
        QStringList escaped;
        for (const QString &word : languages()[index].keywords)
            escaped << QRegularExpression::escape(word);
        keywords_ = QRegularExpression("\\b(?:" + escaped.join('|') + ")\\b",
                                       QRegularExpression::CaseInsensitiveOption);
    }
    rehighlight();
}

void LanguageHighlighter::highlightBlock(const QString &text)
{
    if (languageIndex_ < 0 || languageIndex_ >= languages().size()) return;
    const auto &language = languages()[languageIndex_];
    QVector<Role> roles(text.size(), Plain);
    const bool markup = isMarkup(language.name);
    const bool data = markup || language.name == "Markdown" || language.name == "Diff" ||
        language.name == "INI" || language.name == "Properties" || language.name == "Registry" ||
        language.name == "Intel HEX" || language.name == "S-Record" || language.name == "YAML";

    static const QRegularExpression identifiers(R"(\b[A-Za-z_$][\w$]*\b)");
    static const QRegularExpression types(R"(\b[A-Z][A-Za-z0-9_]*\b)");
    static const QRegularExpression numbers(R"(\b(?:0x[\da-fA-F]+|\d+(?:\.\d+)?)\b)");
    static const QRegularExpression calls(R"(\b([A-Za-z_$][\w$]*)\s*(?=\())");
    static const QRegularExpression declarations(R"(\b(?:def|defp|function|func|fn|fun|proc|procedure|sub|method|defun)\s+([A-Za-z_$][\w$]*))",
                                                 QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression directives(R"((?:^\s*#\s*(?:include|import|define|pragma)\b|\b(?:import|from|using|require|include|use|package|namespace|open)\b))",
                                               QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression strings(R"re("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|`(?:\\.|[^`\\])*`)re");
    static const QRegularExpression tags(R"(</?\s*([A-Za-z][\w:-]*))");
    static const QRegularExpression attributes(R"(\b([A-Za-z_:][\w:.-]*)\s*=)");
    static const QRegularExpression properties(R"((?:^|[;{]\s*)([-\w]+)\s*:)");

    if (!data) {
        matches(roles, text, identifiers, Variable);
        matches(roles, text, types, Type);
        matches(roles, text, calls, Function, 1);
        matches(roles, text, declarations, Function, 1);
    }
    matches(roles, text, numbers, Number);
    if (markup) {
        matches(roles, text, tags, Tag, 1);
        matches(roles, text, attributes, Property, 1);
    } else if (language.name == "CSS") {
        matches(roles, text, properties, Property, 1);
    }
    if (keywords_.isValid() && !keywords_.pattern().isEmpty())
        matches(roles, text, keywords_, Keyword);
    if (!data) matches(roles, text, directives, Include);
    matches(roles, text, strings, String);

    // Block comments keep their state across lines; line comments take precedence over code.
    if (!language.blockCommentStart.isEmpty() && !language.blockCommentEnd.isEmpty()) {
        int offset = previousBlockState() == 1 ? 0 : text.indexOf(language.blockCommentStart);
        while (offset >= 0) {
            const int content = offset + (previousBlockState() == 1 && offset == 0
                                          ? 0 : language.blockCommentStart.size());
            const int end = text.indexOf(language.blockCommentEnd, content);
            if (end < 0) {
                mark(roles, offset, text.size() - offset, Comment);
                setCurrentBlockState(1);
                break;
            }
            mark(roles, offset, end + language.blockCommentEnd.size() - offset, Comment);
            offset = text.indexOf(language.blockCommentStart, end + language.blockCommentEnd.size());
        }
    }
    if (!language.lineComment.isEmpty()) {
        const int comment = text.indexOf(language.lineComment);
        if (comment >= 0 && (comment == 0 || roles[comment] != String))
            mark(roles, comment, text.size() - comment, Comment);
    }
    const bool dark = QGuiApplication::palette().color(QPalette::Base).lightness() < 128;
    const QColor light[] = { {}, "#187c7a", "#7445aa", "#9b6200", "#1764a6", "#a03980",
                             "#9155a2", "#24805a", "#78858f", "#1764a6", "#9b6200" };
    const QColor darkColors[] = { {}, "#8bd5ca", "#c9a0ef", "#e6c383", "#82bbf0", "#e7a1ce",
                                  "#d9a3ee", "#9dd7ad", "#8f9ca8", "#82bbf0", "#e6c383" };
    for (int i = 0; i < roles.size();) {
        const Role role = roles[i];
        int end = i + 1;
        while (end < roles.size() && roles[end] == role) ++end;
        if (role != Plain) {
            QTextCharFormat format;
            format.setForeground(dark ? darkColors[role] : light[role]);
            if (role == Keyword || role == Include) format.setFontWeight(QFont::DemiBold);
            setFormat(i, end - i, format);
        }
        i = end;
    }
}
