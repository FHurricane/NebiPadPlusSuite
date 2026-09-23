#include "imageeditdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <cmath>

class ImageEditDialog::Preview final : public QWidget
{
public:
    explicit Preview(ImageEditDialog *dialog) : QWidget(dialog), dialog_(dialog)
    {
        setMinimumSize(640, 420);
        setMouseTracking(true);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor("#e9eef5"));
        painter.setPen(QPen(QColor("#cbd8e6"), 1));
        painter.setBrush(Qt::white);
        painter.drawRoundedRect(rect().adjusted(8, 8, -8, -8), 8, 8);

        const QImage image = dialog_->cropping_ ? dialog_->original_
                                                  : dialog_->original_.copy(dialog_->crop_);
        const QRectF target = imageRect();
        painter.save();
        if (!dialog_->cropping_) {
            painter.translate(target.center());
            painter.rotate(dialog_->angle());
            painter.translate(-target.center());
        }
        painter.drawImage(target, image);
        painter.setPen(QPen(QColor("#2380bf"), 1.5, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(target);
        painter.restore();

        if (dialog_->cropping_) {
            if (!dialog_->candidate_.isEmpty()) {
                const QRectF selection = imageToCanvas(dialog_->candidate_);
                painter.fillRect(selection, QColor(35, 128, 191, 55));
                painter.setPen(QPen(QColor("#1670a9"), 2, Qt::DashLine));
                painter.drawRect(selection);
            }
            return;
        }
        painter.setPen(QPen(QColor("#1670a9"), 1.5));
        painter.setBrush(Qt::white);
        const auto points = handles();
        painter.drawLine(points[4], mapRotated(QPointF(target.center().x(), target.top())));
        for (const QPointF &point : points)
            painter.drawRect(QRectF(point.x() - 6, point.y() - 6, 12, 12));
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton) return;
        if (dialog_->cropping_) {
            cropStart_ = canvasToImage(event->position());
            dialog_->candidate_ = QRect(cropStart_, cropStart_);
            draggingCrop_ = true;
            update();
            return;
        }
        const auto points = handles();
        for (int i = 0; i < points.size(); ++i)
            if (QLineF(points[i], event->position()).length() <= 14) {
                activeHandle_ = i;
                event->accept();
                return;
            }
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (draggingCrop_) {
            dialog_->candidate_ = QRect(cropStart_, canvasToImage(event->position())).normalized();
            update();
            return;
        }
        if (activeHandle_ < 0) return;
        const QRectF target = imageRect();
        const QPointF center = target.center();
        if (activeHandle_ == 4) {
            const QPointF delta = event->position() - center;
            const qreal degrees = std::atan2(delta.y(), delta.x()) * 180.0 /
                                  3.14159265358979323846 + 90.0;
            dialog_->setAngle(qRound(degrees));
        } else {
            const qreal radians = -dialog_->angle() * 3.14159265358979323846 / 180.0;
            const QPointF delta = event->position() - center;
            const qreal x = delta.x() * std::cos(radians) - delta.y() * std::sin(radians);
            const qreal y = delta.x() * std::sin(radians) + delta.y() * std::cos(radians);
            const qreal scale = previewScale();
            dialog_->resizeFromHandle(QSize(qBound(16, qRound(2 * std::abs(x) / scale), 4000),
                                            qBound(16, qRound(2 * std::abs(y) / scale), 4000)));
        }
        update();
    }

    void mouseReleaseEvent(QMouseEvent *) override
    {
        draggingCrop_ = false;
        activeHandle_ = -1;
    }

private:
    ImageEditDialog *dialog_;
    int activeHandle_ = -1;
    bool draggingCrop_ = false;
    QPoint cropStart_;

    qreal previewScale() const
    {
        const QSize source = dialog_->original_.size();
        return qMin(1.0, qMin(500.0 / qMax(1, source.width()),
                              310.0 / qMax(1, source.height())));
    }
    QRectF imageRect() const
    {
        const QSize dimensions = dialog_->cropping_ ? dialog_->original_.size()
                                                       : dialog_->imageSize();
        const QSizeF size(dimensions.width() * previewScale(),
                          dimensions.height() * previewScale());
        return QRectF(QPointF(width() / 2.0 - size.width() / 2,
                              height() / 2.0 - size.height() / 2), size);
    }
    QPointF mapRotated(const QPointF &point) const
    {
        const QPointF center = imageRect().center();
        const qreal radians = dialog_->angle() * 3.14159265358979323846 / 180.0;
        const QPointF delta = point - center;
        return center + QPointF(delta.x() * std::cos(radians) - delta.y() * std::sin(radians),
                                delta.x() * std::sin(radians) + delta.y() * std::cos(radians));
    }
    QVector<QPointF> handles() const
    {
        const QRectF target = imageRect();
        return {mapRotated(target.topLeft()), mapRotated(target.topRight()),
                mapRotated(target.bottomRight()), mapRotated(target.bottomLeft()),
                mapRotated(QPointF(target.center().x(), target.top() - 28))};
    }
    QPoint canvasToImage(const QPointF &point) const
    {
        const QRectF target = imageRect();
        const qreal scale = previewScale();
        return QPoint(qBound(0, qRound((point.x() - target.left()) / scale),
                             dialog_->original_.width()),
                      qBound(0, qRound((point.y() - target.top()) / scale),
                             dialog_->original_.height()));
    }
    QRectF imageToCanvas(const QRect &selection) const
    {
        const QRectF target = imageRect();
        const qreal scale = previewScale();
        return QRectF(target.left() + selection.left() * scale,
                      target.top() + selection.top() * scale,
                      selection.width() * scale, selection.height() * scale);
    }
};

ImageEditDialog::ImageEditDialog(const QImage &image, QWidget *parent)
    : QDialog(parent), original_(image), crop_(image.rect())
{
    setWindowTitle(tr("Modifica immagine"));
    setObjectName("impImageEditDialog");
    auto *layout = new QVBoxLayout(this);
    auto *hint = new QLabel(tr("Trascina gli angoli per ridimensionare; usa il quadratino superiore per ruotare."), this);
    layout->addWidget(hint);
    preview_ = new Preview(this);
    preview_->setObjectName("impImagePreview");
    layout->addWidget(preview_);
    auto *controls = new QHBoxLayout;
    width_ = new QSpinBox(this);
    width_->setObjectName("impImageWidth");
    width_->setRange(16, 4000);
    width_->setValue(qBound(16, image.width(), 4000));
    width_->setSuffix(" px");
    height_ = new QSpinBox(this);
    height_->setObjectName("impImageHeight");
    height_->setRange(16, 4000);
    height_->setValue(qBound(16, image.height(), 4000));
    height_->setSuffix(" px");
    aspectRatio_ = qreal(width_->value()) / height_->value();
    keepAspectRatio_ = new QCheckBox(tr("Mantieni proporzioni"), this);
    keepAspectRatio_->setObjectName("impImageKeepAspectRatio");
    keepAspectRatio_->setChecked(true);
    angle_ = new QSpinBox(this);
    angle_->setObjectName("impImageAngle");
    angle_->setRange(-180, 180);
    angle_->setSuffix("°");
    controls->addWidget(new QLabel(tr("Larghezza"), this)); controls->addWidget(width_);
    controls->addWidget(new QLabel(tr("Altezza"), this)); controls->addWidget(height_);
    controls->addWidget(keepAspectRatio_);
    controls->addWidget(new QLabel(tr("Angolo"), this)); controls->addWidget(angle_);
    layout->addLayout(controls);
    connect(keepAspectRatio_, &QCheckBox::toggled, this, [this](bool enabled) {
        if (enabled) aspectRatio_ = qreal(width_->value()) / height_->value();
    });
    connect(width_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int width) {
        if (!keepAspectRatio_->isChecked()) return;
        const QSignalBlocker blocker(height_);
        height_->setValue(qBound(16, qRound(width / aspectRatio_), 4000));
    });
    connect(height_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int height) {
        if (!keepAspectRatio_->isChecked()) return;
        const QSignalBlocker blocker(width_);
        width_->setValue(qBound(16, qRound(height * aspectRatio_), 4000));
    });
    auto *cropControls = new QHBoxLayout;
    auto *cropButton = new QPushButton(tr("Ritaglia"), this);
    cropButton->setObjectName("impCropImageAction");
    auto *applyButton = new QPushButton(tr("Applica ritaglio"), this);
    applyButton->setObjectName("impApplyCropAction");
    applyButton->setEnabled(false);
    auto *resetButton = new QPushButton(tr("Ripristina ritaglio"), this);
    cropControls->addWidget(cropButton);
    cropControls->addWidget(applyButton);
    cropControls->addWidget(resetButton);
    cropControls->addStretch();
    layout->addLayout(cropControls);
    connect(cropButton, &QPushButton::clicked, this, [this, applyButton, hint] {
        cropping_ = !cropping_;
        candidate_ = {};
        applyButton->setEnabled(cropping_);
        hint->setText(cropping_ ? tr("Trascina sull'immagine per scegliere l'area da conservare.")
                                : tr("Trascina gli angoli per ridimensionare; usa il quadratino superiore per ruotare."));
        preview_->update();
    });
    connect(applyButton, &QPushButton::clicked, this, [this, applyButton] {
        applyCrop();
        applyButton->setEnabled(false);
    });
    connect(resetButton, &QPushButton::clicked, this, [this, applyButton] {
        crop_ = original_.rect();
        candidate_ = {};
        cropping_ = false;
        setImageSize(original_.size());
        applyButton->setEnabled(false);
        preview_->update();
    });
    for (QSpinBox *spin : {width_, height_, angle_})
        connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), preview_,
                qOverload<>(&QWidget::update));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QImage ImageEditDialog::resultImage() const
{
    QImage result = original_.copy(crop_).scaled(imageSize(), Qt::IgnoreAspectRatio,
                                                   Qt::SmoothTransformation);
    if (angle() != 0) result = result.transformed(QTransform().rotate(angle()),
                                                   Qt::SmoothTransformation);
    return result;
}

QSize ImageEditDialog::imageSize() const { return QSize(width_->value(), height_->value()); }
int ImageEditDialog::angle() const { return angle_->value(); }
QRect ImageEditDialog::cropRect() const { return crop_; }
void ImageEditDialog::setImageSize(const QSize &size)
{
    const QSignalBlocker widthBlocker(width_);
    const QSignalBlocker heightBlocker(height_);
    width_->setValue(size.width());
    height_->setValue(size.height());
    aspectRatio_ = qreal(width_->value()) / height_->value();
    preview_->update();
}
void ImageEditDialog::resizeFromHandle(const QSize &size)
{
    if (!keepAspectRatio_->isChecked()) { setImageSize(size); return; }
    const QSize current = imageSize();
    const qreal widthChange = std::abs(qreal(size.width()) / current.width() - 1.0);
    const qreal heightChange = std::abs(qreal(size.height()) / current.height() - 1.0);
    if (widthChange >= heightChange)
        setImageSize(QSize(size.width(), qBound(16, qRound(size.width() / aspectRatio_), 4000)));
    else
        setImageSize(QSize(qBound(16, qRound(size.height() * aspectRatio_), 4000), size.height()));
}
void ImageEditDialog::setAngle(int degrees)
{
    if (degrees > 180) degrees -= 360;
    if (degrees < -180) degrees += 360;
    angle_->setValue(degrees);
}
void ImageEditDialog::setCropRect(const QRect &rect)
{
    crop_ = rect.normalized().intersected(original_.rect());
    if (crop_.isEmpty()) crop_ = original_.rect();
    candidate_ = {};
    cropping_ = false;
    preview_->update();
}
void ImageEditDialog::applyCrop()
{
    if (candidate_.width() < 2 || candidate_.height() < 2) return;
    setCropRect(candidate_);
    setImageSize(crop_.size());
}
