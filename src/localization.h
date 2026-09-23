#pragma once

#include <QTranslator>

class NebiTranslator final : public QTranslator
{
public:
    explicit NebiTranslator(QObject *parent = nullptr);
    QString translate(const char *context, const char *sourceText,
                      const char *disambiguation = nullptr, int n = -1) const override;
};

QString suiteLanguage();
void setSuiteLanguage(const QString &language);
