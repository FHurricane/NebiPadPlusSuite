#include "lspclient.h"

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    LspClient client;
    const QString code = "int localValue = 0;\nint main() {\n  loc\n}\n";
    client.setDocument("C++", QDir::tempPath() + "/nebipad-completion-test.cpp", code);
    if (!client.available()) return 0;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    bool received = false;
    bool found = false;
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    client.requestCompletion(code, 2, 5, [&](const QVector<LspCompletion> &items) {
        received = true;
        for (const auto &item : items)
            if (item.label.contains("localValue")) found = true;
        loop.quit();
    });
    timeout.start(15000);
    loop.exec();
    if (!received || !found) return 1;
    const QString updated = "int changedValue = 0;\nint main() {\n  chan\n}\n";
    received = false;
    found = false;
    client.requestCompletion(updated, 2, 6, [&](const QVector<LspCompletion> &items) {
        received = true;
        for (const auto &item : items)
            if (item.label.contains("changedValue")) found = true;
        loop.quit();
    });
    timeout.start(15000);
    loop.exec();
    return received && found ? 0 : 2;
}
