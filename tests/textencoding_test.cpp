#include "textencoding.h"

#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    const QString sample = QString::fromUtf8("Caffè € 😀\nSeconda riga");
    for (const auto encoding : {TextEncoding::Utf8, TextEncoding::Utf8Bom,
                                TextEncoding::Utf16BeBom, TextEncoding::Utf16LeBom}) {
        QByteArray bytes;
        QString decoded;
        if (!encodeText(sample, encoding, &bytes) || detectEncoding(bytes) != encoding ||
            !decodeText(bytes, encoding, &decoded) || decoded != sample) {
            qCritical() << "Round trip failed:" << encodingName(encoding);
            return 1;
        }
    }
    QByteArray ansi;
    QString decoded;
    const QString ansiSample = QString::fromUtf8("Caffè € “test”");
    if (!encodeText(ansiSample, TextEncoding::Ansi, &ansi) ||
        detectEncoding(ansi) != TextEncoding::Ansi ||
        !decodeText(ansi, TextEncoding::Ansi, &decoded) || decoded != ansiSample ||
        encodeText(sample, TextEncoding::Ansi, &ansi)) {
        qCritical() << "ANSI conversion failed";
        return 1;
    }
    if (decodeText(QByteArray("\xc3\x28", 2), TextEncoding::Utf8, &decoded)) {
        qCritical() << "Invalid UTF-8 was accepted";
        return 1;
    }
    return 0;
}
