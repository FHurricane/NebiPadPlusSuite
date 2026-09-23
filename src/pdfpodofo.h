#pragma once

#include <QString>

struct PdfTextOverlay { int page = 1; double x = 50; double y = 750; double size = 12; QString text; };
struct PdfImageOverlay { int page = 1; double x = 50; double y = 500; double width = 200; double height = 150; QString imagePath; };
struct PdfMetadataValues { QString title; QString author; QString subject; QString keywords; };

bool podofoAvailable();
int podofoPageCount(const QString &path, QString *error = nullptr);
bool podofoAddText(const QString &path, const PdfTextOverlay &overlay, QString *error = nullptr);
bool podofoAddImage(const QString &path, const PdfImageOverlay &overlay, QString *error = nullptr);
bool podofoInsertPage(const QString &path, int page, QString *error = nullptr);
bool podofoDeletePage(const QString &path, int page, QString *error = nullptr);
bool podofoRotatePage(const QString &path, int page, int degrees, QString *error = nullptr);
bool podofoMovePage(const QString &path, int from, int to, QString *error = nullptr);
bool podofoMergePdf(const QString &path, const QString &other, QString *error = nullptr);
bool podofoSetMetadata(const QString &path, const PdfMetadataValues &values, QString *error = nullptr);
