#pragma once

#include <QObject>
#include <QPointer>
#include <QVector>
#include <functional>

struct GoogleDriveFile {
    QString id;
    QString name;
    QString modifiedTime;
    qint64 size = 0;
};

class QNetworkAccessManager;
class QNetworkReply;
class QTcpServer;
class QUrl;
class QWidget;

class GoogleDriveClient final : public QObject
{
    Q_OBJECT
public:
    static GoogleDriveClient *instance();
    bool isConfigured() const;
    bool isConnected() const;
    QString accountName() const;
    QByteArray avatarData() const;
    void configure(QWidget *parent, std::function<void(bool)> completed = {});
    void connectAccount(QWidget *parent);
    void disconnectAccount();
    void uploadFile(const QString &localPath);
    void listFiles(std::function<void(const QVector<GoogleDriveFile> &, const QString &)> completed);
    void downloadFile(const GoogleDriveFile &file,
                      std::function<void(const QString &, const QString &)> completed);

signals:
    void connectionChanged(bool connected, const QString &account);
    void avatarChanged(const QByteArray &data);
    void operationStarted(const QString &message);
    void operationFinished(bool success, const QString &message);

private:
    explicit GoogleDriveClient(QObject *parent = nullptr);
    void beginAuthorization(QWidget *parent);
    void exchangeCode(const QString &code, const QUrl &redirectUri);
    void refreshAccessToken(std::function<void(bool)> completed);
    void ensureAccessToken(std::function<void(bool)> completed);
    void fetchAccountName();
    void performUpload(const QString &localPath);
    void finishTokenReply(QNetworkReply *reply, bool includeRefresh,
                          std::function<void(bool)> completed = {});
    QString settingKeyForPath(const QString &path, const QString &field) const;

    QNetworkAccessManager *network_;
    QPointer<QTcpServer> callbackServer_;
    QString verifier_;
    QString state_;
    qint64 expiresAt_ = 0;
};
