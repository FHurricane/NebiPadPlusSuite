#include "mainwindow.h"
#include "welcomewindow.h"
#include "suitewindow.h"
#include "localization.h"
#include "nebimagewindow.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QSplashScreen>
#include <QSvgRenderer>
#include <QTimer>
#include <functional>

namespace {
class NebiSplashScreen final : public QSplashScreen
{
public:
    using QSplashScreen::QSplashScreen;

protected:
    void mousePressEvent(QMouseEvent *event) override { event->ignore(); }
};

QPixmap renderImage(const QString &path, const QSize &size)
{
    QSvgRenderer renderer(path);
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter, QRectF(QPointF(0, 0), QSizeF(size)));
    return pixmap;
}

QString applicationSplashPath(int selection)
{
    switch (selection) {
    case 1: return QStringLiteral(":/brand/splash-mindnet.svg");
    case 2: return QStringLiteral(":/brand/splash-impagination.svg");
    case 3: return QStringLiteral(":/brand/splash-math.svg");
    case 4: return QStringLiteral(":/brand/splash-nebimage.svg");
    default: return QStringLiteral(":/brand/splash.svg");
    }
}
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("NebiPad+");
    QApplication::setOrganizationName("NebiPad");
    NebiTranslator englishTranslator;
    if (suiteLanguage() == "en") app.installTranslator(&englishTranslator);
    app.setWindowIcon(QIcon(renderImage(":/brand/logo.svg", QSize(256, 256))));

    NebiSplashScreen splash(renderImage(":/brand/splash.svg", QSize(900, 500)),
                            Qt::WindowStaysOnTopHint);
    splash.show();
    app.processEvents();
    QElapsedTimer visibleTime;
    visibleTime.start();
    MainWindow window(nullptr, true);
    WelcomeWindow welcome;
    const auto launchWithSplash = [&](int selection, std::function<void()> launch) {
        welcome.setEnabled(false);
        auto *applicationSplash = new NebiSplashScreen(
            renderImage(applicationSplashPath(selection), QSize(900, 500)),
            Qt::WindowStaysOnTopHint);
        applicationSplash->setAttribute(Qt::WA_DeleteOnClose);
        applicationSplash->show();
        QTimer::singleShot(4000, applicationSplash,
            [&, applicationSplash, launch = std::move(launch)] {
                applicationSplash->close();
                welcome.setEnabled(true);
                launch();
            });
    };
    welcome.setHandlers(
        [&] {
            launchWithSplash(0, [&] { window.newDocument(); window.show(); });
        },
        [&] {
            launchWithSplash(0, [&] { window.newDocument(); window.show(); });
        },
        [&](const QString &path) {
            launchWithSplash(0, [&, path] {
                if (window.openFile(path)) window.show();
            });
            return true;
        });
    welcome.setSuiteHandler([&](int selection) {
        launchWithSplash(selection, [&, selection] {
            if (selection == 0) {
                window.newDocument();
                window.show();
            } else if (selection < 4) {
                SuiteMode mode = selection == 1 ? SuiteMode::MindNet
                    : selection == 2 ? SuiteMode::Impagination : SuiteMode::Math;
                auto *suite = new SuiteWindow(mode);
                suite->setAttribute(Qt::WA_DeleteOnClose);
                suite->show();
            } else {
                auto *images = new NebImageWindow;
                images->setAttribute(Qt::WA_DeleteOnClose);
                images->show();
            }
        });
    });
    welcome.setRecentFileHandler([&](const QString &path, const QString &application) {
        int selection = 0;
        if (application.contains("MindNet", Qt::CaseInsensitive)) selection = 1;
        else if (application.contains("Impagination", Qt::CaseInsensitive)) selection = 2;
        else if (application.contains("Math", Qt::CaseInsensitive)) selection = 3;
        else if (application.contains("NebImage", Qt::CaseInsensitive)) selection = 4;
        launchWithSplash(selection, [&, path, selection] {
            if (selection == 0) {
                if (window.openFile(path)) window.show();
                return;
            }
            if (selection == 4) {
                auto *images = new NebImageWindow;
                images->setAttribute(Qt::WA_DeleteOnClose);
                if (images->openFile(path)) images->show(); else images->deleteLater();
                return;
            }
            const SuiteMode mode = selection == 1 ? SuiteMode::MindNet
                : selection == 2 ? SuiteMode::Impagination : SuiteMode::Math;
            auto *suite = new SuiteWindow(mode);
            suite->setAttribute(Qt::WA_DeleteOnClose);
            if (suite->openFile(path)) suite->show();
            else suite->deleteLater();
        });
        return true;
    });
    QTimer::singleShot(qMax(0, 6000 - int(visibleTime.elapsed())), &app, [&] {
        welcome.refreshRecentFiles();
        welcome.show();
        splash.finish(&welcome);
    });
    return app.exec();
}
