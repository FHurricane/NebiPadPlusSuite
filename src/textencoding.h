#pragma once

#include <QByteArray>
#include <QString>

enum class TextEncoding { Ansi, Utf8, Utf8Bom, Utf16BeBom, Utf16LeBom };

QString encodingName(TextEncoding encoding);
TextEncoding detectEncoding(const QByteArray &bytes);
bool decodeText(const QByteArray &bytes, TextEncoding encoding, QString *text);
bool encodeText(const QString &text, TextEncoding encoding, QByteArray *bytes);
