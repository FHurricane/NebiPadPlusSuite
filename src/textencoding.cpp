#include "textencoding.h"

#include <QStringConverter>

namespace {
constexpr char16_t windows1252High[32] = {
    0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178
};
}

QString encodingName(TextEncoding encoding)
{
    switch (encoding) {
    case TextEncoding::Ansi: return "ANSI";
    case TextEncoding::Utf8: return "UTF-8";
    case TextEncoding::Utf8Bom: return "UTF-8-BOM";
    case TextEncoding::Utf16BeBom: return "UTF-16 BE BOM";
    case TextEncoding::Utf16LeBom: return "UTF-16 LE BOM";
    }
    return "UTF-8";
}

TextEncoding detectEncoding(const QByteArray &bytes)
{
    if (bytes.startsWith("\xef\xbb\xbf")) return TextEncoding::Utf8Bom;
    if (bytes.startsWith("\xfe\xff")) return TextEncoding::Utf16BeBom;
    if (bytes.startsWith("\xff\xfe")) return TextEncoding::Utf16LeBom;
    QString decoded;
    return decodeText(bytes, TextEncoding::Utf8, &decoded)
        ? TextEncoding::Utf8 : TextEncoding::Ansi;
}

bool decodeText(const QByteArray &bytes, TextEncoding encoding, QString *text)
{
    if (!text) return false;
    if (encoding == TextEncoding::Ansi) {
        text->clear();
        text->reserve(bytes.size());
        for (const unsigned char byte : bytes) {
            const char16_t code = byte >= 0x80 && byte <= 0x9f
                ? windows1252High[byte - 0x80] : byte;
            text->append(QChar(code));
        }
        return true;
    }

    QByteArrayView payload(bytes);
    QStringConverter::Encoding codec = QStringConverter::Utf8;
    if (encoding == TextEncoding::Utf8Bom && bytes.startsWith("\xef\xbb\xbf"))
        payload = payload.sliced(3);
    if (encoding == TextEncoding::Utf16BeBom) {
        if (bytes.startsWith("\xfe\xff")) payload = payload.sliced(2);
        codec = QStringConverter::Utf16BE;
    } else if (encoding == TextEncoding::Utf16LeBom) {
        if (bytes.startsWith("\xff\xfe")) payload = payload.sliced(2);
        codec = QStringConverter::Utf16LE;
    }
    QStringDecoder decoder(codec);
    *text = decoder(payload);
    const auto final = decoder.finalize();
    return !decoder.hasError() && final.error == QStringConverter::FinalizeResultError::NoError;
}

bool encodeText(const QString &text, TextEncoding encoding, QByteArray *bytes)
{
    if (!bytes) return false;
    bytes->clear();
    if (encoding == TextEncoding::Utf8 || encoding == TextEncoding::Utf8Bom) {
        if (encoding == TextEncoding::Utf8Bom) bytes->append("\xef\xbb\xbf", 3);
        bytes->append(text.toUtf8());
        return true;
    }
    if (encoding == TextEncoding::Ansi) {
        bytes->reserve(text.size());
        for (const QChar character : text) {
            const char16_t code = character.unicode();
            if (code < 0x80 || code >= 0xa0 && code <= 0xff) {
                bytes->append(char(code));
                continue;
            }
            bool found = false;
            for (int i = 0; i < 32; ++i) {
                if (windows1252High[i] == code) {
                    bytes->append(char(0x80 + i));
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }
    const bool bigEndian = encoding == TextEncoding::Utf16BeBom;
    bytes->reserve(2 + text.size() * 2);
    bytes->append(bigEndian ? "\xfe\xff" : "\xff\xfe", 2);
    for (const QChar character : text) {
        const char16_t code = character.unicode();
        bytes->append(char(bigEndian ? code >> 8 : code & 0xff));
        bytes->append(char(bigEndian ? code & 0xff : code >> 8));
    }
    return true;
}
