#pragma once

#include <QDialog>
#include <QImage>
#include <QRect>

class QSpinBox;
class QCheckBox;
class QWidget;

class ImageEditDialog final : public QDialog
{
public:
    explicit ImageEditDialog(const QImage &image, QWidget *parent = nullptr);
    QImage resultImage() const;
    QSize imageSize() const;
    int angle() const;
    QRect cropRect() const;
    void setImageSize(const QSize &size);
    void resizeFromHandle(const QSize &size);
    void setAngle(int degrees);
    void setCropRect(const QRect &rect);

private:
    class Preview;
    QImage original_;
    QRect crop_;
    QSpinBox *width_;
    QSpinBox *height_;
    QSpinBox *angle_;
    QCheckBox *keepAspectRatio_;
    qreal aspectRatio_ = 1.0;
    Preview *preview_;
    bool cropping_ = false;
    QRect candidate_;
    void applyCrop();
};
