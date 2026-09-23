#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>

class LanguageHighlighter final : public QSyntaxHighlighter
{
public:
    explicit LanguageHighlighter(QTextDocument *document);
    void setLanguage(int index);

protected:
    void highlightBlock(const QString &text) override;

private:
    int languageIndex_ = -1;
    QRegularExpression keywords_;
};
