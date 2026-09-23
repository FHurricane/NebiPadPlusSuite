#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

enum class IndentStyle { Preserve, Braces, Paren, Colon, Keywords, Markup };

struct LanguageDefinition {
    QString name;
    QStringList extensions;
    QString shebang;
    QString lineComment;
    QString blockCommentStart;
    QString blockCommentEnd;
    QStringList keywords;
    IndentStyle indentStyle = IndentStyle::Preserve;
    int indentWidth = 4;
    bool tabs = false;
};

const QVector<LanguageDefinition> &languages();
int detectLanguage(const QString &path, const QString &text);
