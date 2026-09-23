#include "dictationoverlay.h"

#include <QColor>
#include <QFontMetrics>
#include <QPainter>
#include <QPaintEvent>

DictationOverlay::DictationOverlay(QWidget *parent) : QWidget(parent)
{
    setObjectName("dictationOverlay");
    setFixedSize(492, 126);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    animation_.setInterval(33);
    connect(&animation_, &QTimer::timeout, this, [this] {
        for (int i = 0; i < displayedLevels_.size(); ++i) {
            const float target = state_ == DictationController::State::Listening
                ? targetLevels_[i] : 0.0f;
            displayedLevels_[i] += (target - displayedLevels_[i]) * (target > displayedLevels_[i] ? 0.42f : 0.18f);
        }
        update();
    });
    hide();
}

void DictationOverlay::setState(DictationController::State state)
{
    state_ = state;
    if (state == DictationController::State::Idle) {
        animation_.stop();
        hide();
    } else {
        if (state == DictationController::State::Preparing) transcript_.clear();
        show();
        raise();
        animation_.start();
        update();
    }
}

void DictationOverlay::setTranscript(const QString &text)
{
    transcript_ = text;
    update();
}

void DictationOverlay::setLevels(const QVector<float> &levels)
{
    for (int i = 0; i < qMin(targetLevels_.size(), levels.size()); ++i)
        targetLevels_[i] = qBound(0.0f, levels[i], 1.0f);
}

void DictationOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#668fb9"), 1));
    painter.setBrush(QColor(14, 29, 53, 239));
    painter.drawRoundedRect(QRectF(1, 1, width() - 2, height() - 2), 18, 18);

    const bool listening = state_ == DictationController::State::Listening;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(listening ? "#77e4d1" : "#789abf"));
    painter.drawEllipse(QPointF(24, 25), 5.5, 5.5);
    painter.setPen(QColor("#dfeeff"));
    QFont title = font();
    title.setBold(true);
    title.setPixelSize(14);
    painter.setFont(title);
    QString status;
    switch (state_) {
    case DictationController::State::Preparing: status = tr("Attendo il microfono..."); break;
    case DictationController::State::Listening:
        status = recordingMode_ ? tr("Registrazione in corso  ·  Stop per terminare")
                                : tr("In ascolto  ·  F12 per terminare");
        break;
    case DictationController::State::Processing: status = tr("Trascrizione in corso..."); break;
    case DictationController::State::Idle: return;
    }
    painter.drawText(QRect(39, 11, width() - 54, 28), Qt::AlignVCenter, status);

    QFont subtitle = font();
    subtitle.setPixelSize(13);
    painter.setFont(subtitle);
    painter.setPen(QColor("#acc7e5"));
    const QString displayed = recordingMode_ ? tr("Audio salvato nel documento") :
        (transcript_.isEmpty() ? tr("Parla nel microfono") : transcript_);
    painter.drawText(QRect(22, 38, width() - 44, 26), Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(displayed, Qt::ElideLeft, width() - 48));

    const qreal left = 22.0;
    const qreal baseline = 111.0;
    const qreal widthPerBar = 11.0;
    const qreal spacing = 8.1;
    for (int i = 0; i < displayedLevels_.size(); ++i) {
        const qreal fraction = qreal(i) / qMax(1, displayedLevels_.size() - 1);
        const QColor color = QColor::fromHsvF(0.57 + 0.21 * fraction, 0.72, 0.98);
        painter.setBrush(color);
        const qreal height = 5.0 + displayedLevels_[i] * 38.0;
        painter.drawRoundedRect(QRectF(left + i * (widthPerBar + spacing), baseline - height,
                                       widthPerBar, height), 3.2, 3.2);
    }
}
