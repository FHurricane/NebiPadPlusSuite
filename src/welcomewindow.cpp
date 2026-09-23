#include "welcomewindow.h"
#include "recentfiles.h"
#include "localization.h"

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QProcess>
#include <QSignalBlocker>
#include <QSvgRenderer>
#include <QVBoxLayout>
#include <QTimer>

namespace {
QPixmap renderSvg(const QString &path, const QSize &size)
{
    QSvgRenderer renderer(path);
    QPixmap pixmap(size * 2);
    pixmap.setDevicePixelRatio(2);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
    return pixmap;
}
}

WelcomeWindow::WelcomeWindow(QWidget *parent) : QDialog(parent),
    recentList_(new QListWidget(this)), emptyRecentLabel_(new QLabel(this))
{
    setObjectName("welcomeWindow");
    setWindowTitle(tr("Benvenuto in NebiPad+"));
    setFixedSize(980, 690);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    auto *brand = new QLabel(this);
    brand->setObjectName("welcomeBrandImage");
    brand->setFixedSize(340, 690);
    brand->setPixmap(renderSvg(":/brand/welcome.svg", QSize(340, 690)));
    root->addWidget(brand);

    auto *right = new QWidget(this);
    right->setObjectName("welcomeContent");
    auto *layout = new QVBoxLayout(right);
    layout->setContentsMargins(38, 38, 38, 28);
    layout->setSpacing(0);

    auto *heading = new QLabel(tr("Benvenuto in NebiPad+"), right);
    heading->setObjectName("welcomeHeading");
    layout->addWidget(heading);
    auto *subtitle = new QLabel(tr("Scegli l'applicazione della suite."), right);
    subtitle->setObjectName("welcomeSubtitle");
    layout->addSpacing(6);
    layout->addWidget(subtitle);
    auto *languageRow = new QHBoxLayout;
    languageRow->addStretch();
    auto *languageLabel = new QLabel(tr("Lingua"), right);
    languageLabel->setObjectName("suiteLanguageLabel");
    auto *language = new QComboBox(right);
    language->setObjectName("suiteLanguageCombo");
    language->addItem(tr("Italiano"), "it");
    language->addItem(tr("Inglese"), "en");
    language->setCurrentIndex(qMax(0, language->findData(suiteLanguage())));
    language->setMinimumWidth(145);
    languageRow->addWidget(languageLabel);
    languageRow->addSpacing(8);
    languageRow->addWidget(language);
    layout->addSpacing(10);
    layout->addLayout(languageRow);
    layout->addSpacing(16);

    auto *suiteHeading = new QLabel(tr("LA SUITE NEBIPAD+"), right);
    suiteHeading->setObjectName("recentHeading");
    layout->addWidget(suiteHeading);
    layout->addSpacing(10);
    auto *suiteButtons = new QHBoxLayout;
    suiteButtons->setSpacing(8);
    const QStringList names{tr("NebiPad+"), tr("MindNet"), tr("Impagination"), tr("Math")};
    for (int i = 0; i < names.size(); ++i) {
        auto *button = new QPushButton(names[i], right);
        button->setObjectName(QString("suiteButton%1").arg(i));
        button->setIcon(QIcon(renderSvg(QString(":/brand/suite-%1.svg").arg(i), QSize(28, 28))));
        button->setIconSize(QSize(28, 28));
        button->setFixedHeight(56);
        button->setAutoDefault(false);
        button->setFocusPolicy(Qt::NoFocus);
        suiteButtons->addWidget(button);
        connect(button, &QPushButton::clicked, this, [this, i] {
            if (openSuite_) openSuite_(i);
            else if (i == 0 && start_) start_();
        });
    }
    layout->addLayout(suiteButtons);
    layout->addSpacing(28);

    auto *recentHeading = new QLabel(tr("FILE RECENTI"), right);
    recentHeading->setObjectName("recentHeading");
    layout->addWidget(recentHeading);
    layout->addSpacing(11);
    recentList_->setObjectName("recentFilesList");
    recentList_->setFrameShape(QFrame::NoFrame);
    recentList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    recentList_->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(recentList_, 1);
    emptyRecentLabel_->setObjectName("emptyRecentLabel");
    emptyRecentLabel_->setText(tr("I documenti aperti di recente compariranno qui."));
    layout->addWidget(emptyRecentLabel_, 1);
    layout->addSpacing(18);
    auto *footer = new QLabel(tr("I file recenti si aprono automaticamente nell'applicazione corretta."), right);
    footer->setObjectName("welcomeFooter");
    layout->addWidget(footer);
    auto *copyright = new QLabel(QStringLiteral("Developer: Costa Fabio  ·  Copyright © 2026"), right);
    copyright->setObjectName("welcomeCopyright");
    copyright->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addSpacing(5);
    layout->addWidget(copyright);
    root->addWidget(right, 1);

    setStyleSheet(R"(
        #welcomeContent { background: #f7faff; }
        #welcomeHeading { color: #172b47; font-size: 29px; font-weight: 700; }
        #welcomeSubtitle { color: #60748e; font-size: 15px; }
        #suiteLanguageLabel { color: #60748e; font-size: 13px; }
        QComboBox#suiteLanguageCombo { min-height: 28px; padding: 2px 10px;
                                      border: 1px solid #c7d7e8; border-radius: 7px;
                                      background: white; color: #263e59; }
        #recentHeading { color: #355574; font-size: 12px; font-weight: 700; letter-spacing: 2px; }
        #emptyRecentLabel { color: #8294aa; font-size: 14px; }
        #welcomeFooter { color: #91a0b1; font-size: 12px; }
        #welcomeCopyright { color: #6e8198; font-size: 11px; font-weight: 600; }
        QPushButton { text-align: left; padding-left: 20px; border: 1px solid #c7d7e8;
                      border-radius: 10px; background: #ffffff; color: #263e59;
                      font-size: 15px; font-weight: 600; }
        QPushButton:hover { background: #eaf4ff; border-color: #8bb8e6; }
        QPushButton:pressed { background: #d9ebff; }
        QListWidget#recentFilesList { background: #ffffff; border: 1px solid #dbe5ef;
                                      border-radius: 10px; color: #314d69; font-size: 13px;
                                      outline: none; }
        QListWidget#recentFilesList::item { padding: 6px 12px; border-bottom: 1px solid #edf2f7; }
        QListWidget#recentFilesList::item:hover { background: #eaf4ff; }
        QListWidget#recentFilesList::item:selected { background: #dceeff; color: #174675; }
    )");

    connect(recentList_, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        openPath(item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toString());
    });
    connect(recentList_, &QListWidget::itemActivated, this, [this](QListWidgetItem *item) {
        openPath(item->data(Qt::UserRole).toString(), item->data(Qt::UserRole + 1).toString());
    });
    connect(language, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, language](int index) {
        const QString selected = language->itemData(index).toString();
        const QString current = suiteLanguage();
        if (selected == current) return;
        const auto answer = QMessageBox::question(this, tr("Cambia lingua"),
            tr("Per applicare la nuova lingua a tutta la suite, NebiPad+ verrà riavviato. "
               "Salvare prima eventuali documenti aperti. Continuare?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes) {
            const QSignalBlocker blocker(language);
            language->setCurrentIndex(language->findData(current));
            return;
        }
        setSuiteLanguage(selected);
        const QString executable = QCoreApplication::applicationFilePath();
        const QStringList arguments = QCoreApplication::arguments().mid(1);
        if (!QProcess::startDetached(executable, arguments)) {
            QMessageBox::warning(this, tr("Cambia lingua"),
                                 tr("Impossibile riavviare NebiPad+ automaticamente."));
            return;
        }
        QApplication::quit();
    });
    refreshRecentFiles();
    auto *recentTimer = new QTimer(this);
    recentTimer->setInterval(750);
    connect(recentTimer, &QTimer::timeout, this, &WelcomeWindow::refreshRecentFiles);
    recentTimer->start();
}

void WelcomeWindow::setHandlers(std::function<void()> newDocument,
                                std::function<void()> start,
                                std::function<bool(const QString &)> openFile)
{
    newDocument_ = std::move(newDocument);
    start_ = std::move(start);
    openFile_ = std::move(openFile);
}

void WelcomeWindow::setSuiteHandler(std::function<void(int)> openSuite)
{
    openSuite_ = std::move(openSuite);
}

void WelcomeWindow::setRecentFileHandler(
    std::function<bool(const QString &, const QString &)> openRecentFile)
{
    openRecentFile_ = std::move(openRecentFile);
}

void WelcomeWindow::refreshRecentFiles()
{
    const QVector<RecentFileEntry> entries = recentFileEntries();
    QStringList snapshot;
    for (const RecentFileEntry &entry : entries)
        snapshot.append(entry.path + QChar(0x1f) + entry.application);
    if (snapshot == recentSnapshot_ && recentList_->count() == entries.size() &&
        recentList_->isVisible() == !entries.isEmpty()) return;
    recentSnapshot_ = snapshot;
    recentList_->clear();
    for (const RecentFileEntry &entry : entries) {
        const QFileInfo info(entry.path);
        auto *item = new QListWidgetItem(
            QStringLiteral("%1 (%2)\n%3").arg(info.fileName(), entry.application, entry.path), recentList_);
        item->setData(Qt::UserRole, entry.path);
        item->setData(Qt::UserRole + 1, entry.application);
        item->setToolTip(entry.path);
        item->setSizeHint(QSize(0, 56));
    }
    recentList_->setVisible(!entries.isEmpty());
    emptyRecentLabel_->setVisible(entries.isEmpty());
}

void WelcomeWindow::openPath(const QString &path, const QString &application)
{
    if (openRecentFile_) openRecentFile_(path, application);
    else if (openFile_) openFile_(path);
    refreshRecentFiles();
}
