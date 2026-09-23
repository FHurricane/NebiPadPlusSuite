#include "googlecloudmenu.h"
#include "googledriveclient.h"

#include <QAction>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStatusBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
QIcon accountIcon(const QByteArray &data)
{
    QPixmap source;
    source.loadFromData(data);
    if (source.isNull()) {
        source = QPixmap(64, 64); source.fill(Qt::transparent);
        QPainter painter(&source); painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor("#4285f4")); painter.setPen(Qt::NoPen);
        painter.drawEllipse(source.rect().adjusted(2, 2, -2, -2));
        QFont font = painter.font(); font.setBold(true); font.setPixelSize(34); painter.setFont(font);
        painter.setPen(Qt::white); painter.drawText(source.rect(), Qt::AlignCenter, "G");
    }
    QPixmap avatar(64, 64); avatar.fill(Qt::transparent);
    QPainter painter(&avatar); painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path; path.addEllipse(avatar.rect().adjusted(1, 1, -1, -1));
    painter.setClipPath(path);
    painter.drawPixmap(avatar.rect(), source.scaled(avatar.size(), Qt::KeepAspectRatioByExpanding,
                                                    Qt::SmoothTransformation));
    return QIcon(avatar);
}

void showDriveBrowser(QMainWindow *window, GoogleDriveClient *client,
                      const std::function<bool(const QString &)> &openLocalFile)
{
    window->statusBar()->showMessage(window->tr("Lettura dei documenti Google Drive…"));
    client->listFiles([window, client, openLocalFile](const QVector<GoogleDriveFile> &files,
                                                      const QString &error) {
        if (!error.isEmpty()) { QMessageBox::warning(window, window->tr("Google Drive"), error); return; }
        QDialog dialog(window); dialog.setObjectName("googleDriveBrowser");
        dialog.setWindowTitle(window->tr("Apri da Google Drive")); dialog.resize(650, 430);
        auto *layout = new QVBoxLayout(&dialog);
        auto *title = new QLabel(window->tr("Documenti disponibili per NebiPad+"), &dialog);
        title->setStyleSheet("font-size:17px;font-weight:650"); layout->addWidget(title);
        auto *list = new QListWidget(&dialog); list->setObjectName("googleDriveFileList");
        list->setAlternatingRowColors(true);
        for (int i = 0; i < files.size(); ++i) {
            const auto &file = files[i];
            const QString date = QDateTime::fromString(file.modifiedTime, Qt::ISODate)
                .toLocalTime().toString("dd/MM/yyyy HH:mm");
            auto *item = new QListWidgetItem(QString("%1\n%2   ·   %3 KB")
                .arg(file.name, date).arg(qMax<qint64>(1, file.size / 1024)), list);
            item->setData(Qt::UserRole, i); item->setSizeHint(QSize(0, 48));
        }
        if (files.isEmpty()) list->addItem(window->tr("Nessun documento disponibile"));
        layout->addWidget(list, 1);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Open | QDialogButtonBox::Cancel, &dialog);
        buttons->button(QDialogButtonBox::Open)->setEnabled(!files.isEmpty()); layout->addWidget(buttons);
        QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        QObject::connect(list, &QListWidget::itemDoubleClicked, &dialog,
                         [&dialog](QListWidgetItem *) { dialog.accept(); });
        if (dialog.exec() != QDialog::Accepted || !list->currentItem()) return;
        const int index = list->currentItem()->data(Qt::UserRole).toInt();
        if (index < 0 || index >= files.size()) return;
        client->downloadFile(files[index], [window, openLocalFile](const QString &path,
                                                                  const QString &downloadError) {
            if (!downloadError.isEmpty()) QMessageBox::warning(window, window->tr("Google Drive"), downloadError);
            else if (!openLocalFile(path)) QMessageBox::warning(window, window->tr("Google Drive"),
                window->tr("Il formato del documento non può essere aperto."));
        });
    });
}
}

void installGoogleDriveMenu(QMainWindow *window, QAction *before,
                            std::function<QString()> currentPath,
                            std::function<bool()> saveCurrent,
                            std::function<bool(const QString &)> openLocalFile)
{
    auto *menu = new QMenu(window->tr("&Cloud"), window); menu->setObjectName("googleDriveMenu");
    window->menuBar()->insertMenu(before, menu);
    auto *connectAction = menu->addAction(window->tr("Collega Google Drive…"));
    connectAction->setObjectName("googleDriveConnectAction");
    auto *openAction = menu->addAction(window->tr("Apri da Google Drive…"));
    openAction->setObjectName("googleDriveOpenAction");
    auto *saveAction = menu->addAction(window->tr("Salva su Google Drive"));
    saveAction->setObjectName("googleDriveSaveAction");
    auto *syncAction = menu->addAction(window->tr("Sincronizza documento"));
    syncAction->setObjectName("googleDriveSyncAction");
    menu->addSeparator();
    auto *configureAction = menu->addAction(window->tr("Configura credenziali OAuth…"));
    configureAction->setObjectName("googleDriveConfigureAction");
    auto *disconnectAction = menu->addAction(window->tr("Disconnetti Google Drive"));
    disconnectAction->setObjectName("googleDriveDisconnectAction");

    QMenu *fileMenu = window->menuBar()->actions().isEmpty() ? nullptr
                                                             : window->menuBar()->actions().first()->menu();
    if (fileMenu) { fileMenu->addSeparator(); fileMenu->addAction(openAction); fileMenu->addAction(saveAction); }

    auto *accountWidget = new QWidget(window->menuBar()); accountWidget->setObjectName("googleDriveAccountWidget");
    auto *accountLayout = new QHBoxLayout(accountWidget);
    accountLayout->setContentsMargins(8, 2, 8, 2); accountLayout->setSpacing(7);
    auto *status = new QLabel(window->tr("Connesso a Google Drive"), accountWidget);
    status->setObjectName("googleDriveConnectedLabel");
    status->setStyleSheet("color:#335675;font-weight:600");
    auto *avatar = new QToolButton(accountWidget); avatar->setObjectName("googleDriveAvatar");
    avatar->setAutoRaise(true); avatar->setIconSize(QSize(28, 28)); avatar->setFixedSize(34, 34);
    avatar->setPopupMode(QToolButton::InstantPopup);
    auto *avatarMenu = new QMenu(avatar);
    auto *accountInfo = avatarMenu->addAction(QString()); accountInfo->setEnabled(false);
    avatarMenu->addSeparator(); avatarMenu->addAction(openAction); avatarMenu->addAction(saveAction);
    avatarMenu->addSeparator(); avatarMenu->addAction(disconnectAction); avatar->setMenu(avatarMenu);
    accountLayout->addWidget(status); accountLayout->addWidget(avatar);
    window->menuBar()->setCornerWidget(accountWidget, Qt::TopRightCorner);

    auto *client = GoogleDriveClient::instance();
    const auto update = [=] {
        const bool connected = client->isConnected(); const QString account = client->accountName();
        accountWidget->setVisible(connected);
        accountInfo->setText(account.isEmpty() ? window->tr("Account Google") : account);
        connectAction->setText(connected && !account.isEmpty() ? window->tr("Google Drive: %1").arg(account)
                                                               : window->tr("Collega Google Drive…"));
        openAction->setEnabled(connected); saveAction->setEnabled(connected);
        syncAction->setEnabled(connected); disconnectAction->setEnabled(connected);
        avatar->setIcon(accountIcon(client->avatarData()));
    };
    update();
    QObject::connect(connectAction, &QAction::triggered, window, [=] { client->connectAccount(window); });
    QObject::connect(openAction, &QAction::triggered, window,
                     [=] { showDriveBrowser(window, client, openLocalFile); });
    const auto upload = [=] {
        if (!saveCurrent()) return;
        const QString path = currentPath();
        if (path.isEmpty()) { QMessageBox::information(window, window->tr("Google Drive"),
            window->tr("Salva prima il documento sul computer.")); return; }
        client->uploadFile(path);
    };
    QObject::connect(saveAction, &QAction::triggered, window, upload);
    QObject::connect(syncAction, &QAction::triggered, window, upload);
    QObject::connect(configureAction, &QAction::triggered, window, [=] { client->configure(window); });
    QObject::connect(disconnectAction, &QAction::triggered, window, [=] { client->disconnectAccount(); });
    QObject::connect(client, &GoogleDriveClient::connectionChanged, window,
                     [=](bool, const QString &) { update(); });
    QObject::connect(client, &GoogleDriveClient::avatarChanged, window,
                     [=](const QByteArray &) { update(); });
    QObject::connect(client, &GoogleDriveClient::operationStarted, window,
                     [=](const QString &message) { window->statusBar()->showMessage(message); });
    QObject::connect(client, &GoogleDriveClient::operationFinished, window,
                     [=](bool success, const QString &message) {
        window->statusBar()->showMessage(message, 7000);
        if (!success) QMessageBox::warning(window, window->tr("Google Drive"), message);
    });
}
