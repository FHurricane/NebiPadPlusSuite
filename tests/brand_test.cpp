#include <QApplication>
#include <QImage>
#include <QFile>
#include <QPainter>
#include <QSvgRenderer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QSvgRenderer splash(QStringLiteral(":/brand/splash.svg"));
    QSvgRenderer logo(QStringLiteral(":/brand/logo.svg"));
    QSvgRenderer welcome(QStringLiteral(":/brand/welcome.svg"));
    if (!splash.isValid() || !logo.isValid() || !welcome.isValid()) return 1;
    QFile splashSource(QStringLiteral(":/brand/splash.svg"));
    if (!splashSource.open(QIODevice::ReadOnly)) return 7;
    const QByteArray svg = splashSource.readAll();
    if (!svg.contains("NebiPad+ Suite") ||
        !svg.contains("Developer: Costa Fabio Copyright 2026")) return 8;
    const int logoStart = svg.indexOf("<path d=\"M139 147");
    const int logoEnd = svg.indexOf("/>", logoStart);
    if (logoStart < 0 || logoEnd < 0) return 9;
    const QByteArray originalLogo = svg.mid(logoStart, logoEnd - logoStart + 2);
    const QStringList variants{QStringLiteral("mindnet"), QStringLiteral("impagination"),
                               QStringLiteral("math")};
    const QStringList titles{QStringLiteral("MindNet"), QStringLiteral("Impagination"),
                             QStringLiteral("Math")};
    QList<QColor> backgrounds;
    for (int i = 0; i < variants.size(); ++i) {
        const QString &variant = variants[i];
        const QString path = QStringLiteral(":/brand/splash-%1.svg").arg(variant);
        QSvgRenderer renderer(path);
        QFile source(path);
        if (!renderer.isValid() || renderer.defaultSize() != QSize(900, 500) ||
            !source.open(QIODevice::ReadOnly)) return 10;
        const QByteArray content = source.readAll();
        if (!content.contains(originalLogo) || !content.contains("Developer: Costa Fabio Copyright 2026") ||
            !content.contains((QStringLiteral("NebiPad+ ") + titles[i]).toUtf8()))
            return 11;
        QImage variantImage(900, 500, QImage::Format_ARGB32_Premultiplied);
        variantImage.fill(Qt::transparent);
        QPainter variantPainter(&variantImage);
        renderer.render(&variantPainter);
        variantPainter.end();
        backgrounds.append(variantImage.pixelColor(850, 100));
        if (argc > 4 + i &&
            !variantImage.save(QString::fromLocal8Bit(argv[4 + i]))) return 13;
    }
    if (backgrounds[0] == backgrounds[1] || backgrounds[0] == backgrounds[2] ||
        backgrounds[1] == backgrounds[2]) return 12;
    if (splash.defaultSize() != QSize(900, 500) || logo.defaultSize() != QSize(256, 256) ||
        welcome.defaultSize() != QSize(340, 600))
        return 2;
    QImage image(900, 500, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    splash.render(&painter);
    painter.end();
    if (image.pixelColor(450, 250).alpha() == 0 || image.pixelColor(120, 120).alpha() == 0)
        return 3;
    if (argc > 1 && !image.save(QString::fromLocal8Bit(argv[1]))) return 4;
    if (argc > 2) {
        QImage icon(1024, 1024, QImage::Format_ARGB32_Premultiplied);
        icon.fill(Qt::transparent);
        QPainter iconPainter(&icon);
        logo.render(&iconPainter);
        iconPainter.end();
        if (!icon.save(QString::fromLocal8Bit(argv[2]))) return 5;
    }
    if (argc > 3) {
        QImage panel(340, 600, QImage::Format_ARGB32_Premultiplied);
        panel.fill(Qt::transparent);
        QPainter panelPainter(&panel);
        welcome.render(&panelPainter);
        panelPainter.end();
        if (!panel.save(QString::fromLocal8Bit(argv[3]))) return 6;
    }
    return 0;
}
