#pragma once

#include <QString>
#include <QStringList>

QString audioTargetAt(const QString &text, int position);
QString nearestAudioTarget(const QString &text, int position);
QString resolveAudioTarget(const QString &documentPath, const QString &target);
QStringList relativeAudioFiles(const QString &text);
int nextRecordingNumber(const QString &documentPath, const QString &text);
bool registerRecordingLink(const QString &documentPath, int number,
                           const QString &target, QString *error);
bool copyRecordingLinks(const QString &oldDocument, const QString &newDocument,
                        const QString &text, QString *error);
bool migrateLegacyAudioLinks(const QString &documentPath, QString *text, QString *error);
