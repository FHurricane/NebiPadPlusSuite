#include "googledriveclient.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QSettings>
#include <QSaveFile>
#include <QStandardPaths>
#include <QDir>
#include <QHostAddress>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace {
QString base64Url(const QByteArray &value)
{
    return QString::fromLatin1(value.toBase64(QByteArray::Base64UrlEncoding |
                                               QByteArray::OmitTrailingEquals));
}

QByteArray randomBytes(int count)
{
    QByteArray result(count, Qt::Uninitialized);
    for (int i = 0; i < count; ++i)
        result[i] = char(QRandomGenerator::system()->generate() & 0xff);
    return result;
}

QString accessToken() { return QSettings().value("cloud/google/accessToken").toString(); }
QString refreshToken() { return QSettings().value("cloud/google/refreshToken").toString(); }
QString clientId() { return QSettings().value("cloud/google/clientId").toString().trimmed(); }
QString clientSecret() { return QSettings().value("cloud/google/clientSecret").toString().trimmed(); }
}

GoogleDriveClient *GoogleDriveClient::instance()
{
    static GoogleDriveClient client;
    return &client;
}

GoogleDriveClient::GoogleDriveClient(QObject *parent)
    : QObject(parent), network_(new QNetworkAccessManager(this))
{
    expiresAt_ = QSettings().value("cloud/google/expiresAt", 0).toLongLong();
}

bool GoogleDriveClient::isConfigured() const { return !clientId().isEmpty(); }
bool GoogleDriveClient::isConnected() const { return !refreshToken().isEmpty(); }
QString GoogleDriveClient::accountName() const
{
    return QSettings().value("cloud/google/account").toString();
}
QByteArray GoogleDriveClient::avatarData() const
{
    return QByteArray::fromBase64(QSettings().value("cloud/google/avatar").toByteArray());
}

void GoogleDriveClient::configure(QWidget *parent, std::function<void(bool)> completed)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(tr("Configura Google Drive"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *explanation = new QLabel(tr("Inserisci le credenziali OAuth di tipo Applicazione desktop "
                                      "create per NebiPad+ in Google Cloud Console."), &dialog);
    explanation->setWordWrap(true);
    layout->addWidget(explanation);
    auto *form = new QFormLayout;
    auto *id = new QLineEdit(clientId(), &dialog);
    id->setObjectName("googleDriveClientId");
    id->setPlaceholderText("000000000000-….apps.googleusercontent.com");
    auto *secret = new QLineEdit(clientSecret(), &dialog);
    secret->setObjectName("googleDriveClientSecret");
    secret->setEchoMode(QLineEdit::Password);
    form->addRow(tr("Client ID:"), id);
    form->addRow(tr("Client secret:"), secret);
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    const bool accepted = dialog.exec() == QDialog::Accepted && !id->text().trimmed().isEmpty();
    if (accepted) {
        QSettings settings;
        settings.setValue("cloud/google/clientId", id->text().trimmed());
        settings.setValue("cloud/google/clientSecret", secret->text().trimmed());
    }
    if (completed) completed(accepted);
}

void GoogleDriveClient::connectAccount(QWidget *parent)
{
    if (!isConfigured()) {
        configure(parent, [this, parent](bool ok) { if (ok) beginAuthorization(parent); });
        return;
    }
    beginAuthorization(parent);
}

void GoogleDriveClient::beginAuthorization(QWidget *parent)
{
    if (callbackServer_) callbackServer_->deleteLater();
    auto *server = new QTcpServer(this);
    callbackServer_ = server;
    // The redirect URL below is explicitly IPv4. Binding LocalHost can select
    // ::1 on macOS, which makes the browser's 127.0.0.1 callback fail.
    if (!server->listen(QHostAddress(QStringLiteral("127.0.0.1")), 0)) {
        QMessageBox::warning(parent, tr("Google Drive"),
                             tr("Impossibile avviare il collegamento OAuth locale."));
        server->deleteLater();
        return;
    }
    verifier_ = base64Url(randomBytes(64));
    state_ = base64Url(randomBytes(24));
    const QString challenge = base64Url(QCryptographicHash::hash(verifier_.toUtf8(),
                                                                 QCryptographicHash::Sha256));
    const QUrl redirect(QString("http://127.0.0.1:%1/").arg(server->serverPort()));
    connect(server, &QTcpServer::newConnection, this, [this, server, redirect] {
        QTcpSocket *socket = server->nextPendingConnection();
        const auto processRequest = [this, socket, server, redirect] {
            if (socket->property("oauthHandled").toBool() || !socket->canReadLine()) return;
            socket->setProperty("oauthHandled", true);
            const QList<QByteArray> firstLine = socket->readLine().trimmed().split(' ');
            QUrl received(firstLine.size() > 1 ? QString("http://127.0.0.1") +
                          QString::fromUtf8(firstLine[1]) : QString());
            QUrlQuery query(received);
            const bool valid = query.queryItemValue("state") == state_ &&
                               !query.queryItemValue("code").isEmpty();
            const QByteArray body = valid
                ? "<html><body><h2>NebiPad+ collegato a Google Drive</h2>Puoi chiudere questa finestra.</body></html>"
                : "<html><body><h2>Collegamento non riuscito</h2>Torna a NebiPad+.</body></html>";
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nConnection: close\r\nContent-Length: " +
                          QByteArray::number(body.size()) + "\r\n\r\n" + body);
            socket->flush();
            socket->disconnectFromHost();
            server->close(); server->deleteLater();
            if (valid) exchangeCode(query.queryItemValue("code"), redirect);
            else emit operationFinished(false, query.queryItemValue("error").isEmpty()
                ? tr("Autorizzazione Google Drive non valida.")
                : tr("Autorizzazione Google Drive annullata: %1").arg(query.queryItemValue("error")));
        };
        connect(socket, &QTcpSocket::readyRead, this, processRequest);
        // readyRead may have fired before nextPendingConnection() returned.
        QTimer::singleShot(0, socket, processRequest);
    });
    QUrl url("https://accounts.google.com/o/oauth2/v2/auth");
    QUrlQuery query;
    query.addQueryItem("client_id", clientId());
    query.addQueryItem("redirect_uri", redirect.toString());
    query.addQueryItem("response_type", "code");
    query.addQueryItem("scope", "openid email profile https://www.googleapis.com/auth/drive.file");
    query.addQueryItem("code_challenge", challenge);
    query.addQueryItem("code_challenge_method", "S256");
    query.addQueryItem("access_type", "offline");
    query.addQueryItem("prompt", "consent");
    query.addQueryItem("state", state_);
    url.setQuery(query);
    emit operationStarted(tr("Autorizzazione Google Drive nel browser…"));
    QDesktopServices::openUrl(url);
}

void GoogleDriveClient::exchangeCode(const QString &code, const QUrl &redirectUri)
{
    QUrlQuery form;
    form.addQueryItem("client_id", clientId());
    if (!clientSecret().isEmpty()) form.addQueryItem("client_secret", clientSecret());
    form.addQueryItem("code", code);
    form.addQueryItem("code_verifier", verifier_);
    form.addQueryItem("grant_type", "authorization_code");
    form.addQueryItem("redirect_uri", redirectUri.toString());
    QNetworkRequest request(QUrl("https://oauth2.googleapis.com/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    auto *reply = network_->post(request, form.query(QUrl::FullyEncoded).toUtf8());
    finishTokenReply(reply, true, [this](bool ok) { if (ok) fetchAccountName(); });
}

void GoogleDriveClient::finishTokenReply(QNetworkReply *reply, bool includeRefresh,
                                         std::function<void(bool)> completed)
{
    connect(reply, &QNetworkReply::finished, this, [this, reply, includeRefresh, completed] {
        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        const QString token = json.value("access_token").toString();
        if (reply->error() != QNetworkReply::NoError || token.isEmpty()) {
            emit operationFinished(false, tr("Google Drive: %1").arg(
                json.value("error_description").toString(reply->errorString())));
            if (completed) completed(false);
            reply->deleteLater(); return;
        }
        QSettings settings;
        settings.setValue("cloud/google/accessToken", token);
        if (includeRefresh && !json.value("refresh_token").toString().isEmpty())
            settings.setValue("cloud/google/refreshToken", json.value("refresh_token").toString());
        expiresAt_ = QDateTime::currentSecsSinceEpoch() + json.value("expires_in").toInt(3600) - 60;
        settings.setValue("cloud/google/expiresAt", expiresAt_);
        if (completed) completed(true);
        reply->deleteLater();
    });
}

void GoogleDriveClient::refreshAccessToken(std::function<void(bool)> completed)
{
    QUrlQuery form;
    form.addQueryItem("client_id", clientId());
    if (!clientSecret().isEmpty()) form.addQueryItem("client_secret", clientSecret());
    form.addQueryItem("refresh_token", refreshToken());
    form.addQueryItem("grant_type", "refresh_token");
    QNetworkRequest request(QUrl("https://oauth2.googleapis.com/token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    finishTokenReply(network_->post(request, form.query(QUrl::FullyEncoded).toUtf8()), false,
                     std::move(completed));
}

void GoogleDriveClient::ensureAccessToken(std::function<void(bool)> completed)
{
    if (!accessToken().isEmpty() && QDateTime::currentSecsSinceEpoch() < expiresAt_) completed(true);
    else if (!refreshToken().isEmpty()) refreshAccessToken(std::move(completed));
    else completed(false);
}

void GoogleDriveClient::fetchAccountName()
{
    QNetworkRequest request(QUrl("https://openidconnect.googleapis.com/v1/userinfo"));
    request.setRawHeader("Authorization", "Bearer " + accessToken().toUtf8());
    auto *reply = network_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        const QString account = json.value("email").toString();
        if (!account.isEmpty()) QSettings().setValue("cloud/google/account", account);
        const QUrl avatarUrl(json.value("picture").toString());
        if (avatarUrl.isValid()) {
            auto *avatarReply = network_->get(QNetworkRequest(avatarUrl));
            connect(avatarReply, &QNetworkReply::finished, this, [this, avatarReply] {
                const QByteArray data = avatarReply->error() == QNetworkReply::NoError
                    ? avatarReply->readAll() : QByteArray();
                if (!data.isEmpty()) {
                    QSettings().setValue("cloud/google/avatar", data.toBase64());
                    emit avatarChanged(data);
                }
                avatarReply->deleteLater();
            });
        }
        emit connectionChanged(true, account);
        emit operationFinished(true, account.isEmpty() ? tr("Google Drive collegato.")
                                                        : tr("Google Drive collegato a %1.").arg(account));
        reply->deleteLater();
    });
}

void GoogleDriveClient::listFiles(
    std::function<void(const QVector<GoogleDriveFile> &, const QString &)> completed)
{
    ensureAccessToken([this, completed](bool ok) {
        if (!ok) { completed({}, tr("Collega prima un account Google Drive.")); return; }
        QUrl url("https://www.googleapis.com/drive/v3/files");
        QUrlQuery query;
        query.addQueryItem("q", "trashed = false");
        query.addQueryItem("orderBy", "modifiedTime desc");
        query.addQueryItem("pageSize", "100");
        query.addQueryItem("fields", "files(id,name,modifiedTime,size,mimeType)");
        url.setQuery(query);
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", "Bearer " + accessToken().toUtf8());
        auto *reply = network_->get(request);
        connect(reply, &QNetworkReply::finished, this, [reply, completed] {
            const QByteArray bytes = reply->readAll();
            if (reply->error() != QNetworkReply::NoError) {
                completed({}, reply->errorString()); reply->deleteLater(); return;
            }
            QVector<GoogleDriveFile> files;
            const QJsonArray array = QJsonDocument::fromJson(bytes).object().value("files").toArray();
            for (const QJsonValue &value : array) {
                const QJsonObject object = value.toObject();
                if (object.value("mimeType").toString().startsWith("application/vnd.google-apps."))
                    continue;
                files.append({object.value("id").toString(), object.value("name").toString(),
                              object.value("modifiedTime").toString(),
                              object.value("size").toString().toLongLong()});
            }
            completed(files, {}); reply->deleteLater();
        });
    });
}

void GoogleDriveClient::downloadFile(
    const GoogleDriveFile &remote,
    std::function<void(const QString &, const QString &)> completed)
{
    ensureAccessToken([this, remote, completed](bool ok) {
        if (!ok) { completed({}, tr("Collega prima un account Google Drive.")); return; }
        QUrl url("https://www.googleapis.com/drive/v3/files/" + remote.id);
        QUrlQuery query; query.addQueryItem("alt", "media"); url.setQuery(query);
        QNetworkRequest request(url);
        request.setRawHeader("Authorization", "Bearer " + accessToken().toUtf8());
        emit operationStarted(tr("Download di %1 da Google Drive…").arg(remote.name));
        auto *reply = network_->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply, remote, completed] {
            const QByteArray data = reply->readAll();
            if (reply->error() != QNetworkReply::NoError) {
                completed({}, reply->errorString()); reply->deleteLater(); return;
            }
            const QString directory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                + "/GoogleDriveCache";
            QDir().mkpath(directory);
            const QString safeName = QFileInfo(remote.name).fileName().isEmpty()
                ? QStringLiteral("Documento") : QFileInfo(remote.name).fileName();
            const QString path = QDir(directory).filePath(remote.id.left(12) + "_" + safeName);
            QSaveFile output(path);
            if (!output.open(QIODevice::WriteOnly) || output.write(data) != data.size() || !output.commit()) {
                completed({}, output.errorString()); reply->deleteLater(); return;
            }
            QSettings().setValue(settingKeyForPath(path, "id"), remote.id);
            emit operationFinished(true, tr("%1 aperto da Google Drive.").arg(remote.name));
            completed(path, {}); reply->deleteLater();
        });
    });
}

QString GoogleDriveClient::settingKeyForPath(const QString &path, const QString &field) const
{
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(
        QFileInfo(path).canonicalFilePath().toUtf8(), QCryptographicHash::Sha256).toHex());
    return "cloud/google/files/" + hash + '/' + field;
}

void GoogleDriveClient::uploadFile(const QString &localPath)
{
    if (localPath.isEmpty() || !QFileInfo::exists(localPath)) {
        emit operationFinished(false, tr("Salva prima il documento sul computer.")); return;
    }
    ensureAccessToken([this, localPath](bool ok) {
        if (!ok) { emit operationFinished(false, tr("Collega prima un account Google Drive.")); return; }
        performUpload(localPath);
    });
}

void GoogleDriveClient::performUpload(const QString &localPath)
{
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit operationFinished(false, tr("Impossibile leggere %1.").arg(localPath)); return;
    }
    const QByteArray content = file.readAll();
    const QString id = QSettings().value(settingKeyForPath(localPath, "id")).toString();
    QUrl url(id.isEmpty() ? "https://www.googleapis.com/upload/drive/v3/files"
                          : "https://www.googleapis.com/upload/drive/v3/files/" + id);
    QUrlQuery query; query.addQueryItem("uploadType", id.isEmpty() ? "multipart" : "media");
    query.addQueryItem("fields", "id,name,version,modifiedTime"); url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken().toUtf8());
    QByteArray body = content;
    if (id.isEmpty()) {
        const QByteArray boundary = "nebipad_cloud_boundary";
        const QByteArray metadata = QJsonDocument(QJsonObject{{"name", QFileInfo(localPath).fileName()}}).toJson(QJsonDocument::Compact);
        body = "--" + boundary + "\r\nContent-Type: application/json; charset=UTF-8\r\n\r\n" +
               metadata + "\r\n--" + boundary + "\r\nContent-Type: application/octet-stream\r\n\r\n" +
               content + "\r\n--" + boundary + "--\r\n";
        request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/related; boundary=" + boundary);
    } else request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    emit operationStarted(tr("Caricamento di %1 su Google Drive…").arg(QFileInfo(localPath).fileName()));
    QNetworkReply *reply = id.isEmpty() ? network_->post(request, body)
                                        : network_->sendCustomRequest(request, "PATCH", body);
    connect(reply, &QNetworkReply::finished, this, [this, reply, localPath] {
        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        if (reply->error() != QNetworkReply::NoError || json.value("id").toString().isEmpty()) {
            const QString apiMessage = json.value("error").toObject().value("message").toString();
            emit operationFinished(false, tr("Google Drive: %1").arg(
                apiMessage.isEmpty() ? reply->errorString() : apiMessage));
        } else {
            QSettings settings;
            settings.setValue(settingKeyForPath(localPath, "id"), json.value("id").toString());
            settings.setValue(settingKeyForPath(localPath, "version"), json.value("version").toVariant());
            emit operationFinished(true, tr("%1 salvato su Google Drive.").arg(QFileInfo(localPath).fileName()));
        }
        reply->deleteLater();
    });
}

void GoogleDriveClient::disconnectAccount()
{
    QSettings settings;
    settings.remove("cloud/google/accessToken"); settings.remove("cloud/google/refreshToken");
    settings.remove("cloud/google/expiresAt"); settings.remove("cloud/google/account");
    settings.remove("cloud/google/avatar");
    expiresAt_ = 0;
    emit connectionChanged(false, {});
    emit operationFinished(true, tr("Account Google Drive disconnesso."));
}
