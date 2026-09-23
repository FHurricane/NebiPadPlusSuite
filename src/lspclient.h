#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QVector>

#include <functional>

struct LspCompletion {
    QString label;
    QString insertText;
    QString detail;
    int kind = 1;
    bool snippet = false;
};

class LspClient final : public QObject
{
public:
    explicit LspClient(QObject *parent = nullptr);
    ~LspClient() override;
    void setDocument(const QString &language, const QString &path, const QString &text);
    void requestCompletion(const QString &text, int line, int character,
                           std::function<void(QVector<LspCompletion>)> callback);
    bool available() const { return !program_.isEmpty(); }

private:
    QProcess process_;
    QByteArray buffer_;
    QString program_;
    QStringList arguments_;
    QString languageId_;
    QString uri_;
    QString rootUri_;
    QString text_;
    int version_ = 1;
    int nextId_ = 1;
    int initializeId_ = 0;
    int completionId_ = 0;
    int syncKind_ = 1;
    bool initialized_ = false;
    bool pendingCompletion_ = false;
    int pendingLine_ = 0;
    int pendingCharacter_ = 0;
    std::function<void(QVector<LspCompletion>)> callback_;

    void chooseServer(const QString &language);
    void send(const QJsonObject &message);
    void notify(const QString &method, const QJsonObject &params);
    void readMessages();
    void handleMessage(const QJsonObject &message);
    void sync(const QString &text);
};
