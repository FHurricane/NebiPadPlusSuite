#pragma once

#include "dictationcontroller.h"

#include <QTimer>
#include <QVector>
#include <QWidget>

class DictationOverlay final : public QWidget
{
public:
    explicit DictationOverlay(QWidget *parent = nullptr);
    void setState(DictationController::State state);
    void setTranscript(const QString &text);
    void setRecordingMode(bool value) { recordingMode_ = value; update(); }
    void setLevels(const QVector<float> &levels);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool recordingMode_ = false;
    DictationController::State state_ = DictationController::State::Idle;
    QString transcript_;
    QVector<float> targetLevels_ = QVector<float>(24, 0.0f);
    QVector<float> displayedLevels_ = QVector<float>(24, 0.0f);
    QTimer animation_;
};
