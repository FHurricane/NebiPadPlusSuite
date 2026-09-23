#include "suitewindow.h"
#include "recentfiles.h"
#include "pdfpodofo.h"
#include "dictationcontroller.h"
#include "dictationoverlay.h"
#include "recordingcontroller.h"
#include "audiolinks.h"
#include "imageeditdialog.h"
#include "googlecloudmenu.h"

#include <QAction>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QBuffer>
#include <QAudioOutput>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontComboBox>
#include <QGraphicsEllipseItem>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsPathItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QInputDialog>
#include <QMediaPlayer>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPrintDialog>
#ifdef NEBIPAD_HAS_QTPDF
#include <QPdfDocument>
#endif
#include <QPrinter>
#include <QPageSize>
#include <QPageLayout>
#include <QPushButton>
#include <QRegularExpression>
#include <QPen>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QSlider>
#include <QSvgRenderer>
#include <QTimer>
#include <QScrollArea>
#include <QStatusBar>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTabBar>
#include <QTableWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextTable>
#include <QVideoWidget>
#include <QUrlQuery>
#ifdef NEBIPAD_HAS_WEBENGINE
#include <QWebEngineLoadingInfo>
#include <QWebEnginePage>
#include <QWebEngineView>
#endif
#include <QTextCharFormat>
#include <QTextBlockFormat>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextFragment>
#include <QTextList>
#include <QTextTable>
#include <QSignalBlocker>
#include <QToolBar>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QHorizontalBarSeries>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QValueAxis>
#include <QToolButton>
#include <QUuid>
#include <QWidgetAction>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QPainterPathStroker>
#include <cmath>
#include <functional>

namespace {
QIcon toolbarIcon(const QString &name)
{
    QSvgRenderer renderer(QString(":/icons/%1.svg").arg(name));
    QIcon icon;
    if (!renderer.isValid()) return icon;
    for (const int size : {24, 48}) {
        QPixmap pixmap(size * 2, size * 2);
        pixmap.setDevicePixelRatio(2);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        renderer.render(&painter, QRectF(0, 0, size, size));
        icon.addPixmap(pixmap);
    }
    return icon;
}

QString modeName(SuiteMode mode)
{
    switch (mode) {
    case SuiteMode::MindNet: return QStringLiteral("NebiPad+ MindNet");
    case SuiteMode::Impagination: return QStringLiteral("NebiPad+ Impagination");
    case SuiteMode::Math: return QStringLiteral("NebiPad+ Math");
    }
    return {};
}

QString suiteSaveFilters(SuiteMode mode)
{
    if (mode == SuiteMode::MindNet)
        return QObject::tr("NebiPad+ MindNet (*.nmn);;JSON (*.json)");
    if (mode == SuiteMode::Impagination)
        return QObject::tr("NebiPad+ Impagination (*.npi);;Microsoft Word (*.doc);;LibreOffice ODS (*.ods);;HTML (*.html *.htm);;Testo (*.txt)");
    return QObject::tr("NebiPad+ Math (*.nmt);;HTML (*.html *.htm);;Testo (*.txt)");
}

QString suiteDefaultExtension(SuiteMode mode)
{
    return mode == SuiteMode::MindNet ? QStringLiteral("nmn")
         : mode == SuiteMode::Impagination ? QStringLiteral("npi") : QStringLiteral("nmt");
}

QString extensionFromFilter(const QString &filter, const QString &fallback)
{
    const QRegularExpressionMatch match = QRegularExpression("\\*\\.([A-Za-z0-9]+)").match(filter);
    return match.hasMatch() ? match.captured(1) : fallback;
}

bool suiteRichDocument(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == "html" || suffix == "htm" || suffix == "npi" || suffix == "doc" ||
           suffix == "ods" || suffix == "nmt";
}

QPainterPath shapePath(const QString &shape)
{
    QPainterPath path;
    if (shape.startsWith("net_")) {
        if (shape == "net_cloud") { path.addEllipse(25, 18, 52, 38); path.addEllipse(52, 5, 66, 58); path.addEllipse(95, 22, 43, 35); path.addRoundedRect(30, 35, 105, 28, 12, 12); }
        else if (shape == "net_wifi" || shape == "net_access_point") {
            path.addRoundedRect(32, 45, 96, 20, 7, 7);
            path.addEllipse(76, 32, 8, 8);
            for (int radius : {20, 33}) path.arcMoveTo(QRectF(80-radius, 34-radius, radius*2, radius*2), 40), path.arcTo(QRectF(80-radius, 34-radius, radius*2, radius*2), 40, 100);
        } else if (shape == "net_pc" || shape == "net_server" || shape == "net_firewall" || shape == "net_nas") {
            if (shape == "net_pc") { path.addRoundedRect(25, 4, 110, 49, 4, 4); path.addRect(75, 53, 10, 9); path.addRoundedRect(55, 62, 50, 5, 2, 2); }
            else { path.addRoundedRect(42, 3, 76, 64, 5, 5); for (int y : {16, 32, 48}) { path.addRoundedRect(50, y, 60, 11, 2, 2); path.addEllipse(55, y+3, 4, 4); } }
        } else if (shape == "net_laptop") {
            path.addRoundedRect(35, 5, 90, 49, 4, 4); path.moveTo(30, 58); path.lineTo(130, 58); path.lineTo(141, 66); path.lineTo(19, 66); path.closeSubpath();
        } else if (shape == "net_phone" || shape == "net_tablet") {
            const QRectF body = shape == "net_phone" ? QRectF(63, 2, 34, 66) : QRectF(46, 2, 68, 66);
            path.addRoundedRect(body, 7, 7); path.addEllipse(78, 60, 4, 4);
        } else if (shape == "net_printer") {
            path.addRect(46, 3, 68, 20); path.addRoundedRect(30, 22, 100, 35, 5, 5); path.addRect(47, 46, 66, 21);
        } else if (shape == "net_database") {
            path.addEllipse(38, 3, 84, 21); path.addRect(38, 13, 84, 46); path.addEllipse(38, 48, 84, 20);
        } else if (shape == "net_camera") {
            path.addRoundedRect(28, 19, 82, 35, 6, 6); path.moveTo(110, 25); path.lineTo(137, 12); path.lineTo(137, 61); path.lineTo(110, 48); path.closeSubpath();
        } else if (shape == "net_antenna") {
            path.moveTo(80, 17); path.lineTo(66, 66); path.lineTo(94, 66); path.closeSubpath(); path.addEllipse(76, 9, 8, 8);
            for (int radius : {18, 30}) { path.arcMoveTo(QRectF(80-radius, 13-radius, radius*2, radius*2), 125); path.arcTo(QRectF(80-radius, 13-radius, radius*2, radius*2), 125, 110); path.arcMoveTo(QRectF(80-radius, 13-radius, radius*2, radius*2), -55); path.arcTo(QRectF(80-radius, 13-radius, radius*2, radius*2), -55, 110); }
        } else if (shape == "net_cable") {
            path.addRoundedRect(15, 25, 40, 24, 3, 3); path.addRoundedRect(105, 25, 40, 24, 3, 3); path.moveTo(55, 37); path.cubicTo(70, 8, 90, 66, 105, 37);
        } else if (shape == "net_vpn") {
            path.addRoundedRect(43, 28, 74, 38, 6, 6); path.arcMoveTo(QRectF(62, 6, 36, 42), 0); path.arcTo(QRectF(62, 6, 36, 42), 0, 180); path.addEllipse(77, 41, 6, 6);
        } else if (shape == "net_load_balancer") {
            path.addRoundedRect(18, 24, 124, 32, 7, 7); for (int x : {43, 80, 117}) path.addEllipse(x-8, 32, 16, 16);
        } else if (shape == "net_gateway" || shape == "net_modem") {
            path.addRoundedRect(26, 22, 108, 42, 7, 7); for (int x : {46, 61, 76}) path.addEllipse(x, 51, 5, 5); path.moveTo(80, 22); path.lineTo(80, 5);
        } else {
            path.addRoundedRect(22, 17, 116, 42, 12, 12);
            if (shape == "net_router") { path.moveTo(47, 39); path.lineTo(113, 39); path.moveTo(54, 32); path.lineTo(45, 39); path.lineTo(54, 46); path.moveTo(106, 32); path.lineTo(115, 39); path.lineTo(106, 46); }
            else { for (int x : {46, 68, 90, 112}) path.addRoundedRect(x-6, 32, 12, 11, 2, 2); if (shape == "net_switch_l3") { path.moveTo(68, 24); path.lineTo(90, 24); } }
        }
        return path;
    }
    if (shape == "terminal") path.addRoundedRect(0, 0, 160, 70, 35, 35);
    else if (shape == "decision") {
        path.moveTo(80, 0); path.lineTo(160, 35); path.lineTo(80, 70);
        path.lineTo(0, 35); path.closeSubpath();
    } else if (shape == "input") {
        path.moveTo(25, 0); path.lineTo(160, 0); path.lineTo(135, 70);
        path.lineTo(0, 70); path.closeSubpath();
    } else if (shape == "document") {
        path.moveTo(0, 0); path.lineTo(160, 0); path.lineTo(160, 58);
        path.cubicTo(115, 80, 55, 45, 0, 70); path.closeSubpath();
    } else if (shape == "database") {
        path.addEllipse(0, 0, 160, 24);
        path.addRect(0, 12, 160, 48);
        path.addEllipse(0, 48, 160, 24);
    } else if (shape == "connector") path.addEllipse(45, 0, 70, 70);
    else if (shape == "delay") {
        path.moveTo(0, 0); path.lineTo(115, 0);
        path.arcTo(80, 0, 80, 70, 90, -180);
        path.lineTo(0, 70); path.closeSubpath();
    } else path.addRect(0, 0, 160, 70);
    return path;
}

QIcon shapeIcon(const QString &shape)
{
    QPixmap pixmap(48, 36);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(4, 5);
    painter.scale(0.24, 0.36);
    painter.setPen(QPen(QColor("#24669d"), 7));
    painter.setBrush(QColor("#e7f4ff"));
    painter.drawPath(shapePath(shape));
    if (shape == "predefined") {
        painter.drawLine(18, 2, 18, 68);
        painter.drawLine(142, 2, 142, 68);
    }
    return QIcon(pixmap);
}

QGraphicsTextItem *editableTextIn(QGraphicsItem *item)
{
    if (auto *text = dynamic_cast<QGraphicsTextItem *>(item)) return text;
    for (QGraphicsItem *child : item->childItems())
        if (auto *text = editableTextIn(child)) return text;
    return nullptr;
}

void showObjectMenu(QGraphicsItem *item, const QPoint &screenPos)
{
    if (!item || !item->scene()) return;
    item->scene()->clearSelection();
    item->setSelected(true);
    QMenu menu;
    QGraphicsTextItem *text = editableTextIn(item);
    QAction *edit = text ? menu.addAction(QObject::tr("Modifica testo")) : nullptr;
    if (edit) menu.addSeparator();
    QAction *front = menu.addAction(QObject::tr("Porta avanti"));
    QAction *back = menu.addAction(QObject::tr("Porta indietro"));
    menu.addSeparator();
    QAction *remove = menu.addAction(QObject::tr("Elimina oggetto"));
    QAction *selected = menu.exec(screenPos);
    if (selected == edit) text->setFocus(Qt::MouseFocusReason);
    else if (selected == front) item->setZValue(item->zValue() + 1);
    else if (selected == back) item->setZValue(qMax(-10.0, item->zValue() - 1));
    else if (selected == remove) {
        item->scene()->removeItem(item);
        QTimer::singleShot(0, [item] { delete item; });
    }
}

class ShapeItem final : public QGraphicsPathItem
{
public:
    explicit ShapeItem(const QString &shape, const QColor &color,
                       qreal width = 160, qreal height = 70)
        : shape_(shape), width_(qMax(45.0, width)), height_(qMax(35.0, height))
    {
        setData(0, shape);
        setPen(QPen(QColor("#24669d"), 2));
        setBrush(color);
        setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
        rebuild();
    }
    qreal shapeWidth() const { return width_; }
    qreal shapeHeight() const { return height_; }
    QRectF boundingRect() const override
    {
        return QRectF(0, 0, width_, height_).adjusted(-20, -36, 20, 20);
    }
    QPainterPath shape() const override
    {
        QPainterPath result = QGraphicsPathItem::shape();
        if (isSelected()) {
            for (int i = 0; i < 5; ++i) result.addRect(handleRect(i));
        }
        return result;
    }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        QGraphicsPathItem::paint(painter, option, widget);
        if (!isSelected()) return;
        painter->save();
        painter->setPen(QPen(QColor("#2380bf"), 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(QRectF(0, 0, width_, height_));
        painter->drawLine(QPointF(width_ / 2, 0), QPointF(width_ / 2, -18));
        painter->setPen(QPen(QColor("#1670a9"), 1.5));
        painter->setBrush(Qt::white);
        for (int i = 0; i < 5; ++i) painter->drawRect(visibleHandleRect(i));
        painter->restore();
    }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (isSelected()) for (int i = 0; i < 5; ++i) {
            if (!handleRect(i).contains(event->pos())) continue;
            handle_ = i;
            startScene_ = event->scenePos();
            startPos_ = pos();
            startWidth_ = width_;
            startHeight_ = height_;
            startRotation_ = rotation();
            event->accept();
            return;
        }
        QGraphicsPathItem::mousePressEvent(event);
    }
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (handle_ < 0) { QGraphicsPathItem::mouseMoveEvent(event); return; }
        if (handle_ == 4) {
            const QPointF center = mapToScene(QPointF(width_ / 2, height_ / 2));
            const auto angle = [center](QPointF point) {
                return std::atan2(point.y() - center.y(), point.x() - center.x()) * 180.0 /
                    3.14159265358979323846;
            };
            setRotation(startRotation_ + angle(event->scenePos()) - angle(startScene_));
        } else {
            const QPointF movement = parentItem()
                ? parentItem()->mapFromScene(event->scenePos()) - parentItem()->mapFromScene(startScene_)
                : event->scenePos() - startScene_;
            const qreal radians = startRotation_ * 3.14159265358979323846 / 180.0;
            const qreal dx = movement.x() * std::cos(radians) + movement.y() * std::sin(radians);
            const qreal dy = -movement.x() * std::sin(radians) + movement.y() * std::cos(radians);
            const bool left = handle_ == 0 || handle_ == 3;
            const bool top = handle_ == 0 || handle_ == 1;
            const qreal newWidth = qMax(45.0, startWidth_ + (left ? -dx : dx));
            const qreal newHeight = qMax(35.0, startHeight_ + (top ? -dy : dy));
            const qreal shiftX = left ? startWidth_ - newWidth : 0;
            const qreal shiftY = top ? startHeight_ - newHeight : 0;
            prepareGeometryChange();
            width_ = newWidth;
            height_ = newHeight;
            setPos(startPos_ + QPointF(shiftX * std::cos(radians) - shiftY * std::sin(radians),
                                       shiftX * std::sin(radians) + shiftY * std::cos(radians)));
            rebuild();
        }
        event->accept();
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (handle_ >= 0) { handle_ = -1; event->accept(); return; }
        QGraphicsPathItem::mouseReleaseEvent(event);
    }
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override
    {
        showObjectMenu(this, event->screenPos());
        event->accept();
    }
private:
    QString shape_;
    qreal width_, height_;
    int handle_ = -1;
    QPointF startScene_, startPos_;
    qreal startWidth_ = 0, startHeight_ = 0, startRotation_ = 0;
    QRectF handleRect(int index) const
    {
        const QPointF point = index == 0 ? QPointF(0, 0)
            : index == 1 ? QPointF(width_, 0) : index == 2 ? QPointF(width_, height_)
            : index == 3 ? QPointF(0, height_) : QPointF(width_ / 2, -18);
        return QRectF(point.x() - 12, point.y() - 12, 24, 24);
    }
    QRectF visibleHandleRect(int index) const
    {
        const QRectF target = handleRect(index);
        return QRectF(target.center().x() - 6, target.center().y() - 6, 12, 12);
    }
    void rebuild()
    {
        setPath(QTransform::fromScale(width_ / 160.0, height_ / 70.0).map(shapePath(shape_)));
        setTransformOriginPoint(width_ / 2, height_ / 2);
        update();
    }
};

class EditableBoxText final : public QGraphicsTextItem
{
public:
    using QGraphicsTextItem::QGraphicsTextItem;
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (parentItem() && parentItem()->data(0).toString() == "nodeLabel" &&
            event->button() == Qt::LeftButton && (!editing_ || event->modifiers() & Qt::AltModifier)) {
            dragTarget_ = (event->modifiers() & Qt::AltModifier)
                ? parentItem() : parentItem()->parentItem();
            if (dragTarget_) {
                dragStartScene_ = event->scenePos();
                dragStartPos_ = dragTarget_->pos();
                if (scene()) scene()->clearSelection();
                dragTarget_->setSelected(true);
                event->accept();
                return;
            }
        }
        if (parentItem()) parentItem()->setSelected(true);
        QGraphicsTextItem::mousePressEvent(event);
    }
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (!dragTarget_) { QGraphicsTextItem::mouseMoveEvent(event); return; }
        const QPointF delta = dragTarget_->parentItem()
            ? dragTarget_->parentItem()->mapFromScene(event->scenePos()) -
              dragTarget_->parentItem()->mapFromScene(dragStartScene_)
            : event->scenePos() - dragStartScene_;
        dragTarget_->setPos(dragStartPos_ + delta);
        event->accept();
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (dragTarget_) { dragTarget_ = nullptr; event->accept(); return; }
        QGraphicsTextItem::mouseReleaseEvent(event);
    }
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (parentItem() && parentItem()->data(0).toString() == "nodeLabel") {
            dragTarget_ = nullptr;
            editing_ = true;
        }
        QGraphicsTextItem::mouseDoubleClickEvent(event);
    }
    void focusInEvent(QFocusEvent *event) override
    {
        if (parentItem()) parentItem()->setSelected(true);
        QGraphicsTextItem::focusInEvent(event);
    }
    void focusOutEvent(QFocusEvent *event) override
    {
        editing_ = false;
        QGraphicsTextItem::focusOutEvent(event);
    }
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override
    {
        showObjectMenu(parentItem() ? parentItem() : this, event->screenPos());
        event->accept();
    }
private:
    QGraphicsItem *dragTarget_ = nullptr;
    QPointF dragStartScene_, dragStartPos_;
    bool editing_ = false;
};

class TextBoxItem final : public QGraphicsRectItem
{
public:
    explicit TextBoxItem(qreal width = 250, qreal height = 105, bool nodeLabel = false)
        : QGraphicsRectItem(0, 0, width, height), text_(new EditableBoxText(this)), nodeLabel_(nodeLabel)
    {
        setData(0, nodeLabel_ ? "nodeLabel" : "textBox");
        setPen(nodeLabel_ ? QPen(Qt::NoPen) : QPen(QColor("#8bb2d3"), 1.5));
        setBrush(nodeLabel_ ? QBrush(Qt::NoBrush) : QBrush(QColor("#fbfdff")));
        setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
        setZValue(1);
        text_->setPos(8, 6);
        text_->setTextWidth(width - 16);
        text_->setDefaultTextColor(QColor("#172b47"));
        text_->setFont(QFont("Arial", 12));
        text_->setTextInteractionFlags(Qt::TextEditorInteraction);
        setTransformOriginPoint(rect().center());
        contentsConnection_ = QObject::connect(text_->document(), &QTextDocument::contentsChanged,
                                                text_, [this] {
            const qreal needed = text_->boundingRect().height() + 12;
            if (!sizing_ && needed > rect().height()) setBoxSize(rect().width(), needed);
        });
    }
    ~TextBoxItem() override { QObject::disconnect(contentsConnection_); }
    EditableBoxText *text() const { return text_; }
    QRectF boundingRect() const override
    {
        return QGraphicsRectItem::boundingRect().adjusted(-14, -30, 14, 14);
    }
    QPainterPath shape() const override
    {
        QPainterPath result = QGraphicsRectItem::shape();
        if (isSelected()) for (int i = 0; i < 9; ++i) result.addRect(handleRect(i));
        return result;
    }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        QGraphicsRectItem::paint(painter, option, widget);
        if (!isSelected()) return;
        painter->save();
        painter->setPen(QPen(QColor("#2380bf"), 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect());
        painter->drawLine(QPointF(rect().width() / 2, 0), QPointF(rect().width() / 2, -18));
        painter->setPen(QPen(QColor("#1670a9"), 1.5));
        painter->setBrush(Qt::white);
        for (int i = 0; i < 9; ++i) painter->drawRect(handleRect(i));
        painter->restore();
    }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (isSelected()) for (int i = 0; i < 9; ++i) {
            if (!handleRect(i).contains(event->pos())) continue;
            handle_ = i;
            startScene_ = event->scenePos();
            startPos_ = pos();
            startWidth_ = rect().width();
            startHeight_ = rect().height();
            startRotation_ = rotation();
            event->accept();
            return;
        }
        QGraphicsRectItem::mousePressEvent(event);
    }
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (handle_ < 0) { QGraphicsRectItem::mouseMoveEvent(event); return; }
        if (handle_ == 8) {
            const QPointF center = mapToScene(rect().center());
            const auto angle = [center](QPointF point) {
                return std::atan2(point.y() - center.y(), point.x() - center.x()) * 180.0 /
                    3.14159265358979323846;
            };
            setRotation(startRotation_ + angle(event->scenePos()) - angle(startScene_));
        } else {
            const QPointF movement = parentItem()
                ? parentItem()->mapFromScene(event->scenePos()) - parentItem()->mapFromScene(startScene_)
                : event->scenePos() - startScene_;
            const qreal radians = startRotation_ * 3.14159265358979323846 / 180.0;
            const qreal dx = movement.x() * std::cos(radians) + movement.y() * std::sin(radians);
            const qreal dy = -movement.x() * std::sin(radians) + movement.y() * std::cos(radians);
            const bool left = handle_ == 0 || handle_ == 6 || handle_ == 7;
            const bool right = handle_ == 2 || handle_ == 3 || handle_ == 4;
            const bool top = handle_ == 0 || handle_ == 1 || handle_ == 2;
            const bool bottom = handle_ == 4 || handle_ == 5 || handle_ == 6;
            const qreal width = qMax(nodeLabel_ ? 40.0 : 120.0, startWidth_ + (left ? -dx : right ? dx : 0));
            const qreal height = qMax(nodeLabel_ ? 28.0 : 60.0, startHeight_ + (top ? -dy : bottom ? dy : 0));
            setBoxSize(width, height);
            const qreal shiftX = left ? startWidth_ - rect().width() : 0;
            const qreal shiftY = top ? startHeight_ - rect().height() : 0;
            setPos(startPos_ + QPointF(shiftX * std::cos(radians) - shiftY * std::sin(radians),
                                       shiftX * std::sin(radians) + shiftY * std::cos(radians)));
        }
        event->accept();
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (handle_ >= 0) { handle_ = -1; event->accept(); return; }
        QGraphicsRectItem::mouseReleaseEvent(event);
    }
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override
    {
        showObjectMenu(this, event->screenPos());
        event->accept();
    }
private:
    EditableBoxText *text_;
    bool nodeLabel_ = false;
    QMetaObject::Connection contentsConnection_;
    bool sizing_ = false;
    int handle_ = -1;
    QPointF startScene_, startPos_;
    qreal startWidth_ = 0, startHeight_ = 0, startRotation_ = 0;
    QRectF handleRect(int index) const
    {
        const qreal width = rect().width(), height = rect().height();
        const QPointF point = index == 0 ? QPointF(0, 0)
            : index == 1 ? QPointF(width / 2, 0)
            : index == 2 ? QPointF(width, 0)
            : index == 3 ? QPointF(width, height / 2)
            : index == 4 ? QPointF(width, height)
            : index == 5 ? QPointF(width / 2, height)
            : index == 6 ? QPointF(0, height)
            : index == 7 ? QPointF(0, height / 2) : QPointF(width / 2, -18);
        return QRectF(point.x() - 5, point.y() - 5, 10, 10);
    }
    void setBoxSize(qreal width, qreal height)
    {
        sizing_ = true;
        text_->setTextWidth(width - 16);
        setRect(0, 0, width, qMax(height, text_->boundingRect().height() + 12));
        setTransformOriginPoint(rect().center());
        sizing_ = false;
    }
};

TextBoxItem *addNodeLabel(ShapeItem *parent, const QString &text, const QJsonObject &saved = {})
{
    const QJsonObject geometry = saved.value("labelBox").toObject();
    auto *label = new TextBoxItem(geometry.value("width").toDouble(140),
                                  geometry.value("height").toDouble(44), true);
    label->setParentItem(parent);
    label->setPos(geometry.value("x").toDouble(10), geometry.value("y").toDouble(12));
    QFont font = label->text()->font();
    font.setPointSize(saved.value("labelFontSize").toInt(12));
    label->text()->setFont(font);
    label->text()->setDefaultTextColor(QColor(saved.value("labelColor").toString("#173652")));
    if (saved.contains("labelHtml")) label->text()->setHtml(saved.value("labelHtml").toString());
    else label->text()->setPlainText(text);
    label->setRotation(geometry.value("rotation").toDouble());
    return label;
}

class ConnectorItem final : public QGraphicsPathItem
{
public:
    ConnectorItem(QVector<QPointF> points, int type, QColor color, int width)
        : points_(std::move(points)), type_(type)
    {
        setPen(QPen(color, width, type == 1 || type == 3 ? Qt::DashLine : Qt::SolidLine,
                    Qt::RoundCap, Qt::RoundJoin));
        setZValue(-1);
        setFlag(ItemIsSelectable);
        rebuild();
    }
    int lineType() const { return type_; }
    QVector<QPointF> points() const { return points_; }
    void setPoints(const QVector<QPointF> &points)
    {
        points_ = points;
        rebuild();
    }
    void setLineType(int type)
    {
        type_ = type;
        QPen current = pen();
        current.setStyle(type == 1 || type == 3 ? Qt::DashLine : Qt::SolidLine);
        setPen(current);
        update();
    }
    QRectF boundingRect() const override
    {
        return QGraphicsPathItem::boundingRect().adjusted(-16, -16, 16, 16);
    }
    QPainterPath shape() const override
    {
        QPainterPathStroker stroker;
        stroker.setWidth(qMax(14, pen().width() + 8));
        QPainterPath result = stroker.createStroke(path());
        if (isSelected()) for (const QPointF &point : points_)
            result.addRect(QRectF(point.x() - 6, point.y() - 6, 12, 12));
        return result;
    }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        QGraphicsPathItem::paint(painter, option, widget);
        if (type_ >= 2 && points_.size() >= 2) {
            const QLineF segment(points_[points_.size() - 2], points_.last());
            if (segment.length() > 0) {
                const QPointF direction = (segment.p2() - segment.p1()) / segment.length();
                const QPointF normal(-direction.y(), direction.x());
                const qreal size = qMax(12, pen().width() * 4);
                const QPointF tip = segment.p2();
                const QPointF base = tip - direction * size;
                painter->save();
                painter->setPen(Qt::NoPen);
                painter->setBrush(pen().color());
                painter->drawPolygon(QPolygonF{tip, base + normal * (size * 0.42),
                                              base - normal * (size * 0.42)});
                painter->restore();
            }
        }
        if (!isSelected()) return;
        painter->save();
        painter->setPen(QPen(QColor("#1670a9"), 1.5));
        painter->setBrush(Qt::white);
        for (const QPointF &point : points_)
            painter->drawRect(QRectF(point.x() - 5, point.y() - 5, 10, 10));
        painter->restore();
    }
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        setSelected(true);
        pressed_ = event->scenePos();
        initialPoints_ = points_;
        vertex_ = -1;
        for (int i = 0; i < points_.size(); ++i)
            if (QLineF(event->pos(), points_[i]).length() <= 9) { vertex_ = i; break; }
        event->accept();
    }
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (vertex_ >= 0) {
            points_[vertex_] = event->pos();
            rebuild();
        } else if (QLineF(pressed_, event->scenePos()).length() >= 4) {
            const QPointF delta = event->scenePos() - pressed_;
            for (int i = 0; i < points_.size(); ++i) points_[i] = initialPoints_[i] + delta;
            rebuild();
        }
        event->accept();
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override
    {
        if (vertex_ < 0 && QLineF(pressed_, event->scenePos()).length() < 4)
            showEditMenu(event->screenPos());
        vertex_ = -1;
        event->accept();
    }
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override
    {
        setSelected(true);
        showEditMenu(event->screenPos());
        event->accept();
    }
private:
    QVector<QPointF> points_;
    QVector<QPointF> initialPoints_;
    int type_;
    int vertex_ = -1;
    QPointF pressed_;
    void rebuild()
    {
        QPainterPath path;
        if (!points_.isEmpty()) {
            path.moveTo(points_.first());
            for (int i = 1; i < points_.size(); ++i) path.lineTo(points_[i]);
        }
        setPath(path);
        update();
    }
    void showEditMenu(const QPoint &screenPos)
    {
        QMenu menu;
        auto *addVertex = menu.addAction(QObject::tr("Aggiungi vertice"));
        auto *removeVertex = menu.addAction(QObject::tr("Rimuovi ultimo vertice"));
        removeVertex->setEnabled(points_.size() > 2);
        auto *styles = menu.addMenu(QObject::tr("Tipo di linea"));
        const QStringList names{QObject::tr("Continua"), QObject::tr("Tratteggiata"),
                                QObject::tr("Freccia"), QObject::tr("Freccia tratteggiata")};
        QList<QAction *> styleActions;
        for (int i = 0; i < names.size(); ++i) {
            QAction *action = styles->addAction(names[i]);
            action->setCheckable(true);
            action->setChecked(i == type_);
            styleActions.append(action);
        }
        auto *width = menu.addAction(QObject::tr("Spessore..."));
        auto *color = menu.addAction(QObject::tr("Colore..."));
        menu.addSeparator();
        auto *remove = menu.addAction(QObject::tr("Elimina oggetto"));
        QAction *selected = menu.exec(screenPos);
        if (selected == remove) {
            if (scene()) scene()->removeItem(this);
            QTimer::singleShot(0, [this] { delete this; });
            return;
        }
        if (selected == addVertex && points_.size() >= 2) {
            const int index = points_.size() - 1;
            points_.insert(index, (points_[index - 1] + points_[index]) / 2);
            rebuild();
        } else if (selected == removeVertex && points_.size() > 2) {
            points_.remove(points_.size() - 2);
            rebuild();
        } else if (selected == width) {
            bool accepted = false;
            const int value = QInputDialog::getInt(nullptr, QObject::tr("Spessore linea"),
                QObject::tr("Pixel:"), pen().width(), 1, 16, 1, &accepted);
            if (accepted) { QPen current = pen(); current.setWidth(value); setPen(current); }
        } else if (selected == color) {
            QColor chosen = QColorDialog::getColor(pen().color());
            if (chosen.isValid()) { QPen current = pen(); current.setColor(chosen); setPen(current); }
        } else {
            for (int i = 0; i < styleActions.size(); ++i)
                if (selected == styleActions[i]) setLineType(i);
        }
    }
};

constexpr qreal kMindPageMargin = 90;

qreal pixelsForValue(qreal value, const QString &unit)
{
    if (unit == "mm") return value * 96.0 / 25.4;
    if (unit == "cm") return value * 96.0 / 2.54;
    return value;
}

class PageItem final : public QGraphicsRectItem
{
public:
    explicit PageItem(const QRectF &rect) : QGraphicsRectItem(rect)
    {
        setData(0, "pageBoundary");
        setPen(QPen(QColor("#9dacbf"), 1));
        setBrush(Qt::white);
        setZValue(-100);
    }
    void setGrid(bool visible, qreal step)
    {
        gridVisible_ = visible;
        gridStep_ = qMax(1.0, step);
        update();
    }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override
    {
        QGraphicsRectItem::paint(painter, option, widget);
        if (!gridVisible_) return;
        painter->save();
        painter->setPen(QPen(QColor(54, 113, 178, 60), 0));
        for (qreal x = gridStep_; x < rect().width(); x += gridStep_)
            painter->drawLine(QPointF(x, 0), QPointF(x, rect().height()));
        for (qreal y = gridStep_; y < rect().height(); y += gridStep_)
            painter->drawLine(QPointF(0, y), QPointF(rect().width(), y));
        painter->restore();
    }
private:
    bool gridVisible_ = false;
    qreal gridStep_ = 38;
};

QSizeF paperDimensions(const QString &format)
{
    return format == "A3" ? QSizeF(1123, 1587) : QSizeF(794, 1123);
}

QSizeF orientedPaperDimensions(const QString &format, bool landscape)
{
    QSizeF size = paperDimensions(format);
    if (landscape) size.transpose();
    return size;
}

void configureDiagramPage(QGraphicsView *view, const QString &format, bool landscape = false)
{
    if (!view) return;
    auto *scene = view->scene();
    const QSizeF paper = orientedPaperDimensions(format, landscape);
    view->setProperty("paperFormat", format);
    view->setProperty("landscape", landscape);
    scene->setSceneRect(-kMindPageMargin, -kMindPageMargin,
                        paper.width() + 2 * kMindPageMargin,
                        paper.height() + 2 * kMindPageMargin);
    for (QGraphicsItem *item : scene->items()) {
        if (item->data(0).toString() == "pageBoundary") {
            if (auto *rect = dynamic_cast<PageItem *>(item))
                rect->setRect(QRectF(QPointF(0, 0), paper));
        } else if (item->data(0).toString() == "footer") {
            item->setPos(60, paper.height() - 54);
        } else if (item->data(0).toString() == "header") {
            item->setPos(60, 28);
        }
        if (item->data(0).toString() == "header" || item->data(0).toString() == "footer") {
            if (auto *text = dynamic_cast<QGraphicsTextItem *>(item))
                text->setTextWidth(paper.width() - 120);
        }
    }
    view->fitInView(QRectF(QPointF(-80, -80), paper + QSizeF(160, 160)), Qt::KeepAspectRatio);
}

class DiagramView final : public QGraphicsView
{
public:
    using QGraphicsView::QGraphicsView;
    void setRulersVisible(bool visible) { rulersVisible_ = visible; viewport()->update(); }
    void setGridVisible(bool visible) { gridVisible_ = visible; refreshGrid(); }
    void setRulerSettings(const QString &unit, qreal value)
    {
        rulerUnit_ = unit;
        rulerValue_ = value;
        viewport()->update();
    }
    void setGridSettings(const QString &unit, qreal value)
    {
        gridUnit_ = unit;
        gridValue_ = value;
        refreshGrid();
    }
    bool rulersVisible() const { return rulersVisible_; }
    bool gridVisible() const { return gridVisible_; }
    QString rulerUnit() const { return rulerUnit_; }
    QString gridUnit() const { return gridUnit_; }
    qreal rulerValue() const { return rulerValue_; }
    qreal gridValue() const { return gridValue_; }
    void setLineTool(int type, const QColor &color, int width)
    {
        drawingType_ = type;
        drawingColor_ = color;
        drawingWidth_ = width;
        viewport()->setCursor(Qt::CrossCursor);
        viewport()->update();
        setFocus();
    }
    void updateLineSettings(const QColor &color, int width)
    {
        drawingColor_ = color;
        drawingWidth_ = width;
    }
    void stopLineTool()
    {
        if (draft_) { scene()->removeItem(draft_); delete draft_; draft_ = nullptr; }
        drawingType_ = -1;
        viewport()->unsetCursor();
        viewport()->update();
    }
protected:
    void drawForeground(QPainter *painter, const QRectF &rect) override
    {
        QGraphicsView::drawForeground(painter, rect);
        if (drawingType_ >= 0) {
            painter->save();
            painter->resetTransform();
            painter->setPen(QPen(QColor("#1781c3"), 1.5));
            painter->setBrush(QColor(236, 248, 255, 225));
            for (const QPointF &anchor : anchorPoints()) {
                const QPoint pixel = mapFromScene(anchor);
                if (viewport()->rect().contains(pixel)) painter->drawEllipse(QPointF(pixel), 4, 4);
            }
            painter->restore();
        }
        if (!rulersVisible_) return;
        const QRectF paper = sceneRect().adjusted(kMindPageMargin, kMindPageMargin,
                                                  -kMindPageMargin, -kMindPageMargin);
        const QPoint topLeft = mapFromScene(paper.topLeft());
        const QPoint bottomRight = mapFromScene(paper.bottomRight());
        const qreal step = pixelsForValue(rulerValue_, rulerUnit_);
        if (step <= 0) return;
        painter->save();
        painter->resetTransform();
        painter->setPen(QPen(QColor("#9db3ca"), 1));
        painter->setBrush(QColor("#e9f2fc"));
        painter->drawRect(QRect(topLeft.x(), topLeft.y() - 25,
                                bottomRight.x() - topLeft.x(), 24));
        painter->drawRect(QRect(topLeft.x() - 35, topLeft.y(), 34,
                                bottomRight.y() - topLeft.y()));
        painter->drawRect(QRect(topLeft.x() - 35, topLeft.y() - 25, 34, 24));
        painter->setPen(QColor("#426480"));
        QFont font = painter->font();
        font.setPixelSize(10);
        painter->setFont(font);
        painter->drawText(topLeft.x() - 31, topLeft.y() - 9, rulerUnit_);
        for (int i = 0; i < 5000 && i * step <= paper.width(); ++i) {
            const int x = mapFromScene(QPointF(i * step, 0)).x();
            if (x < topLeft.x() - 1 || x > bottomRight.x() + 1) continue;
            const bool major = i % 5 == 0;
            painter->drawLine(x, topLeft.y() - 1, x, topLeft.y() - (major ? 13 : 7));
            if (major) painter->drawText(x + 2, topLeft.y() - 14,
                QString::number(i * rulerValue_, 'g', 5));
        }
        for (int i = 0; i < 5000 && i * step <= paper.height(); ++i) {
            const int y = mapFromScene(QPointF(0, i * step)).y();
            if (y < topLeft.y() - 1 || y > bottomRight.y() + 1) continue;
            const bool major = i % 5 == 0;
            painter->drawLine(topLeft.x() - 1, y, topLeft.x() - (major ? 13 : 7), y);
            if (major) painter->drawText(topLeft.x() - 33, y + 11,
                QString::number(i * rulerValue_, 'g', 5));
        }
        painter->restore();
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (drawingType_ >= 0 && event->button() == Qt::LeftButton) {
            const QRectF paper = sceneRect().adjusted(kMindPageMargin, kMindPageMargin,
                                                      -kMindPageMargin, -kMindPageMargin);
            const QPointF point = mapToScene(event->pos());
            if (!paper.contains(point)) { event->accept(); return; }
            start_ = snappedPoint(point);
            draft_ = new ConnectorItem({start_, start_}, drawingType_, drawingColor_, drawingWidth_);
            draft_->setFlag(QGraphicsItem::ItemIsSelectable, false);
            scene()->addItem(draft_);
            event->accept();
            return;
        }
        QGraphicsView::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (draft_) {
            const QRectF paper = sceneRect().adjusted(kMindPageMargin, kMindPageMargin,
                                                      -kMindPageMargin, -kMindPageMargin);
            const QPointF point = mapToScene(event->pos());
            const QPointF bounded(qBound(paper.left(), point.x(), paper.right()),
                                  qBound(paper.top(), point.y(), paper.bottom()));
            draft_->setPoints({start_, snappedPoint(bounded)});
            event->accept();
            return;
        }
        QGraphicsView::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (draft_ && event->button() == Qt::LeftButton) {
            mouseMoveEvent(event);
            if (QLineF(draft_->points().first(), draft_->points().last()).length() < 4) {
                scene()->removeItem(draft_);
                delete draft_;
            } else {
                draft_->setFlag(QGraphicsItem::ItemIsSelectable);
                scene()->clearSelection();
                draft_->setSelected(true);
            }
            draft_ = nullptr;
            viewport()->update();
            event->accept();
            return;
        }
        QGraphicsView::mouseReleaseEvent(event);
    }
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Escape && drawingType_ >= 0) {
            stopLineTool();
            event->accept();
            return;
        }
        QGraphicsView::keyPressEvent(event);
    }
    void wheelEvent(QWheelEvent *event) override
    {
        if (event->modifiers() & Qt::ControlModifier) {
            const qreal factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
            scale(factor, factor);
            event->accept();
        } else QGraphicsView::wheelEvent(event);
    }
private:
    QVector<QPointF> anchorPoints() const
    {
        QVector<QPointF> result;
        if (!scene()) return result;
        for (QGraphicsItem *item : scene()->items()) {
            if (item == draft_) continue;
            if (auto *line = dynamic_cast<ConnectorItem *>(item)) {
                for (const QPointF &point : line->points()) result.append(line->mapToScene(point));
            } else if (auto *shape = dynamic_cast<ShapeItem *>(item)) {
                const QRectF bounds = shape->path().boundingRect();
                for (const QPointF &point : {bounds.topLeft(), bounds.topRight(),
                     bounds.bottomRight(), bounds.bottomLeft(),
                     QPointF(bounds.center().x(), bounds.top()),
                     QPointF(bounds.right(), bounds.center().y()),
                     QPointF(bounds.center().x(), bounds.bottom()),
                     QPointF(bounds.left(), bounds.center().y())})
                    result.append(shape->mapToScene(point));
            }
        }
        return result;
    }
    QPointF snappedPoint(const QPointF &point) const
    {
        QPointF nearest = point;
        qreal distance = 14.0;
        const QPoint pixel = mapFromScene(point);
        for (const QPointF &anchor : anchorPoints()) {
            const qreal candidate = QLineF(QPointF(pixel), QPointF(mapFromScene(anchor))).length();
            if (candidate < distance) { distance = candidate; nearest = anchor; }
        }
        return nearest;
    }
    void refreshGrid()
    {
        if (!scene()) return;
        for (QGraphicsItem *item : scene()->items())
            if (auto *page = dynamic_cast<PageItem *>(item))
                page->setGrid(gridVisible_, pixelsForValue(gridValue_, gridUnit_));
    }
    bool rulersVisible_ = false;
    bool gridVisible_ = false;
    QString rulerUnit_ = "mm";
    QString gridUnit_ = "mm";
    qreal rulerValue_ = 10;
    qreal gridValue_ = 10;
    int drawingType_ = -1;
    QColor drawingColor_ = QColor("#4a74a0");
    int drawingWidth_ = 2;
    ConnectorItem *draft_ = nullptr;
    QPointF start_;
};

QVector<QPair<QString, int>> documentBookmarks(QTextDocument *document);

QTextCursor selectedImageAt(QTextCursor cursor)
{
    if (cursor.hasSelection() && cursor.charFormat().isImageFormat()) return cursor;
    cursor.clearSelection();
    for (int offset : {0, -1}) {
        QTextCursor candidate = cursor;
        if (offset < 0 && !candidate.movePosition(QTextCursor::PreviousCharacter)) continue;
        if (!candidate.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor)) continue;
        if (candidate.charFormat().isImageFormat() &&
            candidate.selectedText().contains(QChar::ObjectReplacementCharacter)) return candidate;
    }
    return QTextCursor();
}

class PageEdit final : public QTextEdit
{
public:
    using QTextEdit::QTextEdit;
    std::function<void(const QTextCursor &, bool)> editImage;
    std::function<void(const QTextCursor &, int)> wrapImage;
    std::function<void(const QTextCursor &, const QPoint &)> moveImage;
    std::function<void(const QString &)> playVideo;
    std::function<void(const QString &)> playAudio;
protected:
    void contextMenuEvent(QContextMenuEvent *event) override
    {
        if (!editImage) { QTextEdit::contextMenuEvent(event); return; }
        const QTextCursor image = selectedImageAt(cursorForPosition(event->pos()));
        if (image.isNull()) { QTextEdit::contextMenuEvent(event); return; }
        QMenu *menu = createStandardContextMenu();
        menu->insertSeparator(menu->actions().isEmpty() ? nullptr : menu->actions().first());
        QAction *edit = new QAction(tr("Modifica immagine..."), menu);
        QAction *crop = new QAction(tr("Ritaglia immagine..."), menu);
        QAction *move = new QAction(tr("Sposta"), menu);
        QAction *play = nullptr;
        QString videoReference = anchorAt(event->pos());
        if (!videoReference.startsWith("nebi-video:"))
            videoReference = image.charFormat().anchorHref();
        if (playVideo && videoReference.startsWith("nebi-video:")) {
            play = new QAction(tr("Riproduci video"), menu);
            menu->insertAction(menu->actions().first(), play);
        }
        move->setObjectName("impMoveImageAction");
        connect(move, &QAction::triggered, this, [this, image] {
            movingImage_ = image;
            setCursor(Qt::OpenHandCursor);
        });
        menu->insertAction(menu->actions().first(), crop);
        menu->insertAction(crop, edit);
        menu->insertAction(edit, move);
        QMenu *wrap = new QMenu(tr("Contorno testo"), menu);
        menu->insertMenu(menu->actions().first(), wrap);
        QAction *inlineImage = wrap->addAction(tr("In linea con il testo"));
        QAction *floatLeft = wrap->addAction(tr("Immagine a sinistra, testo a destra"));
        QAction *floatRight = wrap->addAction(tr("Immagine a destra, testo a sinistra"));
        QAction *chosen = menu->exec(event->globalPos());
        if (chosen == edit || chosen == crop) editImage(image, chosen == crop);
        else if (play && chosen == play) playVideo(videoReference);
        else if (wrapImage && (chosen == inlineImage || chosen == floatLeft || chosen == floatRight))
            wrapImage(image, chosen == floatLeft ? 1 : chosen == floatRight ? 2 : 0);
        delete menu;
    }
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && !chartSelection_.isNull()) {
            const QVector<QPoint> points = chartHandles();
            for (int index = 0; index < points.size(); ++index)
                if (QLineF(points[index], event->pos()).length() <= 12) {
                    resizingChart_ = true;
                    chartHandle_ = index;
                    const QRect rect = chartRect();
                    chartOpposite_ = points[(index + 2) % 4];
                    chartRatio_ = qreal(qMax(1, rect.width())) / qMax(1, rect.height());
                    const qreal storedWidth = chartSelection_.charFormat().toImageFormat().width();
                    chartDisplayScale_ = storedWidth > 0
                        ? qreal(rect.width()) / storedWidth : 1.0;
                    if (!std::isfinite(chartDisplayScale_) || chartDisplayScale_ <= 0.01)
                        chartDisplayScale_ = 1.0;
                    event->accept();
                    return;
                }
        }
        if (!movingImage_.isNull() && event->button() == Qt::LeftButton) {
            if (!selectedImageAt(cursorForPosition(event->pos())).isNull()) {
                draggingImage_ = true;
                dragPosition_ = event->pos();
                setCursor(Qt::ClosedHandCursor);
                event->accept();
                return;
            }
        }
        if (event->button() == Qt::LeftButton) {
            const QTextCursor image = selectedImageAt(cursorForPosition(event->pos()));
            if (!image.isNull() && image.charFormat().toImageFormat().name().startsWith("nebi-chart-")) {
                chartSelection_ = image;
                viewport()->update();
                event->accept();
                return;
            }
            chartSelection_ = QTextCursor();
            viewport()->update();
        }
        QTextEdit::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (draggingImage_) {
            dragPosition_ = event->pos();
            viewport()->update();
            event->accept();
            return;
        }
        if (resizingChart_) {
            const qreal visualWidth = qBound(80.0,
                qAbs(event->position().x() - chartOpposite_.x()), 2000.0);
            const qreal visualHeight = qMax(48.0, visualWidth / qMax(0.01, chartRatio_));
            const qreal width = visualWidth / chartDisplayScale_;
            const qreal height = visualHeight / chartDisplayScale_;
            QTextImageFormat format = chartSelection_.charFormat().toImageFormat();
            format.setWidth(width);
            format.setHeight(height);
            const int start = chartSelection_.selectionStart();
            QTextCursor cursor = chartSelection_;
            cursor.insertImage(format);
            chartSelection_ = QTextCursor(document());
            chartSelection_.setPosition(start);
            chartSelection_.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            document()->setModified(true);
            viewport()->update();
            event->accept();
            return;
        }
        QTextEdit::mouseMoveEvent(event);
    }
    void paintEvent(QPaintEvent *event) override
    {
        QTextEdit::paintEvent(event);
        QPainter painter(viewport());
        if (!chartSelection_.isNull()) {
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(QPen(QColor("#1768b2"), 1.5));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(chartRect());
            painter.setBrush(Qt::white);
            for (const QPoint &point : chartHandles())
                painter.drawRect(QRect(point - QPoint(6, 6), QSize(12, 12)));
        }
        if (!draggingImage_) return;
        const QTextImageFormat image = movingImage_.charFormat().toImageFormat();
        const QSize size(qMax(20, qRound(image.width())), qMax(20, qRound(image.height())));
        QRect preview(dragPosition_ - QPoint(size.width() / 2, size.height() / 2), size);
        painter.setPen(QPen(QColor("#1768b2"), 2, Qt::DashLine));
        painter.setBrush(QColor(23, 104, 178, 40));
        painter.drawRect(preview);
    }
    void keyPressEvent(QKeyEvent *event) override
    {
        if (!movingImage_.isNull() && event->key() == Qt::Key_Escape) {
            movingImage_ = QTextCursor();
            draggingImage_ = false;
            unsetCursor();
            viewport()->update();
            event->accept();
            return;
        }
        QTextEdit::keyPressEvent(event);
    }
    void mouseDoubleClickEvent(QMouseEvent *event) override
    {
        QString video = anchorAt(event->pos());
        if (!video.startsWith("nebi-video:"))
            video = selectedImageAt(cursorForPosition(event->pos())).charFormat().anchorHref();
        if (playVideo && video.startsWith("nebi-video:")) {
            playVideo(video);
            event->accept();
            return;
        }
        if (editImage) {
            const QTextCursor image = selectedImageAt(cursorForPosition(event->pos()));
            if (!image.isNull()) { editImage(image, false); event->accept(); return; }
        }
        QTextEdit::mouseDoubleClickEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (resizingChart_ && event->button() == Qt::LeftButton) {
            resizingChart_ = false;
            chartHandle_ = -1;
            event->accept();
            return;
        }
        if (draggingImage_ && event->button() == Qt::LeftButton) {
            draggingImage_ = false;
            unsetCursor();
            viewport()->update();
            if (moveImage) moveImage(movingImage_, event->pos());
            movingImage_ = QTextCursor();
            event->accept();
            return;
        }
        if (event->button() == Qt::LeftButton &&
            (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier))) {
            const QTextCursor clicked = cursorForPosition(event->pos());
            const QString audio = audioTargetAt(clicked.block().text(), clicked.positionInBlock());
            if (playAudio && !audio.isEmpty()) {
                playAudio(audio);
                event->accept();
                return;
            }
            const QString target = anchorAt(event->pos());
            if (playVideo && target.startsWith("nebi-video:")) {
                playVideo(target);
                event->accept();
                return;
            } else if (target.startsWith('#')) {
                for (const auto &bookmark : documentBookmarks(document()))
                    if (bookmark.first == target.mid(1)) {
                        QTextCursor cursor(document());
                        cursor.setPosition(bookmark.second);
                        setTextCursor(cursor);
                        ensureCursorVisible();
                        event->accept();
                        return;
                    }
            } else if (!target.isEmpty()) {
                QDesktopServices::openUrl(QUrl(target));
                event->accept();
                return;
            }
        }
        QTextEdit::mouseReleaseEvent(event);
    }
    void wheelEvent(QWheelEvent *event) override
    {
        if (event->modifiers() & Qt::ControlModifier) {
            if (event->angleDelta().y() > 0) zoomIn(); else zoomOut();
            event->accept();
        } else QTextEdit::wheelEvent(event);
    }
private:
    QRect chartRect() const
    {
        if (chartSelection_.isNull()) return {};
        QTextCursor start = chartSelection_;
        start.setPosition(chartSelection_.selectionStart());
        QTextCursor end = chartSelection_;
        end.setPosition(chartSelection_.selectionEnd());
        const QRect before = cursorRect(start);
        const QRect after = cursorRect(end);
        // Cursor geometry already contains document zoom, device scaling and
        // line layout. It therefore matches the pixels actually painted.
        if (before.top() == after.top() && after.left() > before.left())
            return QRect(before.left(), before.top(), after.left() - before.left(),
                         qMax(before.height(), after.height()));
        const QTextImageFormat format = chartSelection_.charFormat().toImageFormat();
        const qreal scale = format.height() > 0 && before.height() > 0
            ? qreal(before.height()) / format.height() : 1.0;
        return QRect(before.left(), before.top(), qRound(format.width() * scale),
                     qRound(format.height() * scale));
    }
    QVector<QPoint> chartHandles() const
    {
        const QRect rect = chartRect();
        return {rect.topLeft(), rect.topRight(), rect.bottomRight(), rect.bottomLeft()};
    }
    QTextCursor movingImage_;
    QTextCursor chartSelection_;
    bool draggingImage_ = false;
    bool resizingChart_ = false;
    int chartHandle_ = -1;
    QPoint chartOpposite_;
    qreal chartRatio_ = 1.0;
    qreal chartDisplayScale_ = 1.0;
    QPoint dragPosition_;
};

QString portableHtml(QTextDocument *document)
{
    QString html = document->toHtml();
    const QRegularExpression images("<img[^>]*\\bsrc=\"([^\"]+)\"");
    QRegularExpressionMatchIterator matches = images.globalMatch(html);
    while (matches.hasNext()) {
        const QString source = matches.next().captured(1);
        if (source.startsWith("data:")) continue;
        const QUrl url(source);
        const QVariant resource = document->resource(QTextDocument::ImageResource, url);
        QImage image = resource.value<QImage>();
        if (image.isNull() && resource.canConvert<QPixmap>())
            image = resource.value<QPixmap>().toImage();
        if (image.isNull()) {
            QImageReader reader(url.isLocalFile() ? url.toLocalFile() : source);
            image = reader.read();
        }
        if (image.isNull()) continue;
        QByteArray png;
        QBuffer buffer(&png);
        if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) continue;
        html.replace("src=\"" + source + "\"",
            "src=\"data:image/png;base64," + QString::fromLatin1(png.toBase64()) + "\"");
    }
    return html;
}

QVector<QPair<QString, int>> documentBookmarks(QTextDocument *document)
{
    QVector<QPair<QString, int>> result;
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next())
        for (auto fragment = block.begin(); !fragment.atEnd(); ++fragment)
            if (fragment.fragment().isValid())
                for (const QString &name : fragment.fragment().charFormat().anchorNames())
                    result.append({name, fragment.fragment().position()});
    return result;
}

void refreshImpaginationPageElements(QWidget *paper)
{
    if (!paper) return;
    const bool headerVisible = paper->property("headerVisible").toBool();
    const bool footerVisible = paper->property("footerVisible").toBool();
    const bool numbersVisible = paper->property("pageNumbersVisible").toBool();
    if (auto *header = paper->findChild<QLineEdit *>("impHeaderEdit"))
        header->setVisible(headerVisible);
    if (auto *footer = paper->findChild<QLineEdit *>("impFooterEdit"))
        footer->setVisible(footerVisible);
    if (auto *number = paper->findChild<QLabel *>("impPageNumberLabel"))
        number->setVisible(numbersVisible);
    if (auto *row = paper->findChild<QWidget *>("impFooterRow"))
        row->setVisible(footerVisible || numbersVisible);
}

QTextEdit *editorAt(QTabWidget *tabs, int index)
{
    QWidget *page = tabs->widget(index);
    return page ? page->findChild<QTextEdit *>("pageEditor") : nullptr;
}

QGraphicsView *viewAt(QTabWidget *tabs, int index)
{
    QWidget *page = tabs->widget(index);
    return page ? page->findChild<QGraphicsView *>("diagramView") : nullptr;
}
}

SuiteWindow::SuiteWindow(SuiteMode mode, QWidget *parent)
    : QMainWindow(parent), mode_(mode), tabs_(new QTabWidget(this)),
      paperSize_(new QComboBox(this)), status_(new QLabel(this))
{
    setWindowTitle(modeName(mode_));
    resize(1100, 780);
    menuBar()->setNativeMenuBar(false);
    tabs_->setDocumentMode(true);
    tabs_->setTabsClosable(mode_ != SuiteMode::Impagination);
    setCentralWidget(tabs_);
    auto *file = menuBar()->addMenu(tr("&File"));
    auto *create = file->addAction(tr("Nuovo"), QKeySequence::New);
    auto *open = file->addAction(tr("Apri..."), QKeySequence::Open);
    auto *saveAction = file->addAction(tr("Salva"), QKeySequence::Save);
    auto *saveAs = file->addAction(tr("Salva con nome..."), QKeySequence::SaveAs);
    file->addSeparator();
    auto *print = file->addAction(tr("Stampa..."), QKeySequence::Print);
    QAction *exportPdf = nullptr;
    if (mode_ != SuiteMode::Impagination) {
        exportPdf = file->addAction(tr("Esporta in PDF..."));
        exportPdf->setObjectName("suiteExportPdfAction");
        connect(exportPdf, &QAction::triggered, this, &SuiteWindow::exportCurrentPdf);
    }
    auto *close = file->addAction(tr("Chiudi scheda"), QKeySequence::Close);
    auto *edit = menuBar()->addMenu(tr("&Modifica"));
    auto *undo = edit->addAction(tr("Annulla"), QKeySequence::Undo);
    auto *redo = edit->addAction(tr("Ripeti"), QKeySequence::Redo);
    edit->addSeparator();
    auto *cut = edit->addAction(tr("Taglia"), QKeySequence::Cut);
    auto *copy = edit->addAction(tr("Copia"), QKeySequence::Copy);
    auto *paste = edit->addAction(tr("Incolla"), QKeySequence::Paste);
    auto *view = menuBar()->addMenu(tr("&Visualizza"));
    auto *zoomIn = view->addAction(tr("Ingrandisci"), QKeySequence::ZoomIn);
    auto *zoomOut = view->addAction(tr("Riduci"), QKeySequence::ZoomOut);
    auto *format = menuBar()->addMenu(tr("F&ormato"));
    auto *bold = format->addAction(tr("Grassetto"), QKeySequence::Bold);
    auto *italic = format->addAction(tr("Corsivo"), QKeySequence::Italic);
    auto *underline = format->addAction(tr("Sottolineato"), QKeySequence::Underline);
    auto *color = format->addAction(tr("Colore testo..."));
    auto *left = format->addAction(tr("Allinea a sinistra"));
    auto *center = format->addAction(tr("Centra"));
    auto *right = format->addAction(tr("Allinea a destra"));
    auto *justify = format->addAction(tr("Giustifica"));
    auto *tools = menuBar()->addMenu(mode_ == SuiteMode::MindNet ? tr("Diagramma") : tr("Strumenti"));
    if (mode_ == SuiteMode::Impagination) {
        dictation_ = new DictationController(this);
        recording_ = new RecordingController(this);
        dictationOverlay_ = new DictationOverlay(tabs_);
        tabs_->installEventFilter(this);
        dictationAction_ = tools->addAction(tr("Dettatura vocale"));
        dictationAction_->setObjectName("impDictationAction");
        dictationAction_->setIcon(toolbarIcon("record"));
        dictationAction_->setShortcut(QKeySequence(Qt::Key_F12));
        dictationAction_->setShortcutContext(Qt::ApplicationShortcut);
        connect(dictationAction_, &QAction::triggered, this, &SuiteWindow::toggleDictation);
        tools->addSeparator();
        dictation_->setCallbacks(
            [this](DictationController::State state) {
                dictationOverlay_->setState(state);
                placeDictationOverlay();
                if (state == DictationController::State::Listening)
                    statusBar()->showMessage(tr("Dettatura attiva · F12 per terminare"));
                else if (state == DictationController::State::Processing)
                    statusBar()->showMessage(tr("Trascrizione in corso..."));
                else if (state == DictationController::State::Idle)
                    statusBar()->clearMessage();
            },
            [this](const QVector<float> &levels) { dictationOverlay_->setLevels(levels); },
            [this](const QString &text) { dictationOverlay_->setTranscript(text); },
            [this](const QString &text) { insertDictatedText(text); },
            [this](const QString &message) {
                statusBar()->showMessage(message, 8000);
                QMessageBox::warning(this, tr("Dettatura vocale"), message);
            });
        recording_->setStateChanged([this] {
            const bool active = recording_->isRecording();
            recordAction_->setChecked(active);
            if (active) {
                dictationOverlay_->setRecordingMode(true);
                dictationOverlay_->setState(DictationController::State::Listening);
                placeDictationOverlay();
                statusBar()->showMessage(tr("Registrazione audio in corso · premi Stop per terminare"));
            } else {
                dictationOverlay_->setState(DictationController::State::Idle);
                dictationOverlay_->setRecordingMode(false);
                statusBar()->showMessage(tr("Registrazione audio salvata."), 4000);
            }
        });
        recording_->setLevelsChanged([this](const QVector<float> &levels) {
            dictationOverlay_->setLevels(levels);
        });
        recording_->setError([this](const QString &message) {
            statusBar()->showMessage(message, 8000);
            QMessageBox::warning(this, tr("Registrazione audio"), message);
        });
    }
    QMenu *linesMenu = nullptr;
    QAction *connection = nullptr;
    if (mode_ == SuiteMode::MindNet) {
        auto *templates = tools->addMenu(tr("Inserisci diagramma"));
        templates->setObjectName("diagramTemplatesMenu");
        const QVector<QPair<QString, QString>> diagramTypes{
            {"flow", tr("Diagramma di flusso")},
            {"mindmap", tr("Mappa mentale")},
            {"org", tr("Organigramma")},
            {"decision", tr("Albero decisionale")},
            {"process", tr("Diagramma di processo")},
            {"network", tr("Diagramma di rete")},
            {"uml", tr("Diagramma UML")},
            {"blocks", tr("Diagramma a blocchi")}};
        for (const auto &entry : diagramTypes) {
            auto *action = templates->addAction(entry.second);
            action->setObjectName("template_" + entry.first);
            connect(action, &QAction::triggered, this,
                    [this, type = entry.first] { addDiagramTemplate(type); });
        }
        tools->addSeparator();
        linesMenu = menuBar()->addMenu(tr("&Linee"));
        linesMenu->setObjectName("linesMenu");
        connection = linesMenu->addAction(toolbarIcon("line-solid"), tr("Linea"));
        connection->setObjectName("addLineAction");
        format->menuAction()->setVisible(false);
    } else if (mode_ == SuiteMode::Math) {
        auto *formula = tools->addAction(tr("Inserisci formula..."));
        connect(formula, &QAction::triggered, this, [this] {
            bool accepted = false;
            const QString expression = QInputDialog::getText(this, tr("Formula"),
                tr("Espressione matematica (Unicode):"), QLineEdit::Normal, {}, &accepted);
            if (accepted && !expression.isEmpty() && textEditor())
                textEditor()->insertPlainText(expression);
        });
        auto *symbols = tools->addMenu(tr("Simboli"));
        for (const QString &symbol : {QStringLiteral("∑"), QStringLiteral("∫"),
             QStringLiteral("√"), QStringLiteral("π"), QStringLiteral("∞"),
             QStringLiteral("≤"), QStringLiteral("≥"), QStringLiteral("±")}) {
            auto *action = symbols->addAction(symbol);
            connect(action, &QAction::triggered, this, [this, symbol] {
                if (textEditor()) textEditor()->insertPlainText(symbol);
            });
        }
    }
    auto *window = menuBar()->addMenu(tr("&Finestra"));
    installGoogleDriveMenu(this, window->menuAction(),
        [this] { return pathFor(tabs_->currentIndex()); },
        [this] {
            const int index = tabs_->currentIndex();
            return index >= 0 && save(index);
        },
        [this](const QString &path) { return openFile(path); });
    connect(window, &QMenu::aboutToShow, this, [this, window] {
        window->clear();
        for (int i = 0; i < tabs_->count(); ++i) {
            auto *action = window->addAction(tabs_->tabText(i));
            action->setCheckable(true);
            action->setChecked(i == tabs_->currentIndex());
            connect(action, &QAction::triggered, this, [this, i] { tabs_->setCurrentIndex(i); });
        }
    });
    if (mode_ == SuiteMode::Impagination) setupPdfTools();
    auto *helpMenu = menuBar()->addMenu(tr("?"));
    helpMenu->setObjectName("suiteHelpMenu");
    helpMenu->addAction(tr("Informazioni su %1").arg(modeName(mode_)), this,
        [this] { QMessageBox::about(this, modeName(mode_), tr("Suite NebiPad+ · C++ e Qt 6")); });

    auto *toolbar = addToolBar(tr("Strumenti principali"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setIconSize(QSize(24, 24));
    for (auto pair : {qMakePair(create, "new"), qMakePair(open, "open"),
                      qMakePair(saveAction, "save"), qMakePair(print, "print"),
                      qMakePair(undo, "undo"), qMakePair(redo, "redo"),
                      qMakePair(cut, "cut"), qMakePair(copy, "copy"),
                      qMakePair(paste, "paste"), qMakePair(zoomIn, "zoom-in"),
                      qMakePair(zoomOut, "zoom-out")})
        pair.first->setIcon(toolbarIcon(pair.second));
    for (auto *action : {create, open, saveAction, print, undo, redo, cut, copy, paste,
                         zoomIn, zoomOut}) toolbar->addAction(action);
    QToolBar *audioToolbar = toolbar;
    QToolBar *pageToolbar = toolbar;
    if (mode_ == SuiteMode::Impagination) {
        toolbar->setObjectName("impDocumentToolbar");
        audioToolbar = addToolBar(tr("Audio"));
        audioToolbar->setObjectName("impAudioToolbar");
        audioToolbar->setMovable(false);
        audioToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        audioToolbar->setIconSize(QSize(24, 24));
        pageToolbar = addToolBar(tr("Pagina"));
        pageToolbar->setObjectName("impPageToolbar");
        pageToolbar->setMovable(false);
        pageToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        pageToolbar->setIconSize(QSize(24, 24));
    }
    if (dictationAction_) {
        audioToolbar->addAction(dictationAction_);
        recordAction_ = audioToolbar->addAction(toolbarIcon("record"), tr("Registra nota audio"));
        recordAction_->setObjectName("impRecordAudioAction"); recordAction_->setCheckable(true);
        playPauseAction_ = audioToolbar->addAction(toolbarIcon("play-pause"), tr("Play / Pausa nota audio"));
        playPauseAction_->setObjectName("impPlayPauseAudioAction");
        stopAudioAction_ = audioToolbar->addAction(toolbarIcon("stop-audio"), tr("Stop nota audio"));
        stopAudioAction_->setObjectName("impStopAudioAction");
        connect(recordAction_, &QAction::triggered, this, &SuiteWindow::startRecording);
        connect(playPauseAction_, &QAction::triggered, this, &SuiteWindow::playPauseRecording);
        connect(stopAudioAction_, &QAction::triggered, this, &SuiteWindow::stopAudio);
        auto *gain = new QSpinBox(audioToolbar); gain->setObjectName("impRecordingGainControl");
        gain->setRange(25, 100); gain->setSuffix(" %"); gain->setFixedWidth(76);
        gain->setToolTip(tr("Guadagno registrazione audio"));
        const int savedGain = qBound(25, QSettings().value("audio/recordingGain", 75).toInt(), 100);
        gain->setValue(savedGain); recording_->setGainPercent(savedGain); audioToolbar->addWidget(gain);
        connect(gain, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
            recording_->setGainPercent(value); QSettings().setValue("audio/recordingGain", value);
        });
    }
    pageToolbar->addSeparator();
    paperSize_->addItems({"A4", "A3"});
    paperSize_->setObjectName("paperSizeCombo");
    paperSize_->setToolTip(tr("Formato pagina"));
    if (mode_ == SuiteMode::Math) paperSize_->setEnabled(false);
    pageToolbar->addWidget(paperSize_);
    if (mode_ == SuiteMode::MindNet) {
        orientationAction_ = toolbar->addAction(toolbarIcon("orientation"), tr("Pagina orizzontale"));
        orientationAction_->setObjectName("mindOrientationAction");
        orientationAction_->setCheckable(true);
        orientationAction_->setToolTip(tr("Alterna pagina verticale e orizzontale"));
        rulersAction_ = toolbar->addAction(toolbarIcon("ruler"), tr("Mostra righelli"));
        rulersAction_->setObjectName("mindRulersAction");
        rulersAction_->setCheckable(true);
        gridAction_ = toolbar->addAction(toolbarIcon("grid"), tr("Mostra griglia"));
        gridAction_->setObjectName("mindGridAction");
        gridAction_->setCheckable(true);
        toolbar->addSeparator();
        auto *selectAction = toolbar->addAction(toolbarIcon("select"), tr("Seleziona e sposta"));
        selectAction->setObjectName("selectDiagramTool");
        connect(selectAction, &QAction::triggered, this, [this] {
            if (auto *view = dynamic_cast<DiagramView *>(diagramView())) view->stopLineTool();
            statusBar()->showMessage(tr("Selezione attiva"), 2000);
        });
        auto *lineButton = new QToolButton(toolbar);
        lineButton->setObjectName("connectionButton");
        lineButton->setIcon(toolbarIcon("line-solid"));
        lineButton->setIconSize(QSize(24, 24));
        lineButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        lineButton->setToolTip(tr("Disegna una linea trascinando sul foglio"));
        lineButton->setPopupMode(QToolButton::InstantPopup);
        auto *lineMenu = new QMenu(lineButton);
        const QStringList lineNames{tr("Linea continua"), tr("Linea tratteggiata"),
                                    tr("Freccia continua"), tr("Freccia tratteggiata")};
        const QStringList lineIcons{"line-solid", "line-dashed", "line-arrow", "line-dashed-arrow"};
        for (int type = 0; type < lineNames.size(); ++type) {
            auto *choice = lineMenu->addAction(toolbarIcon(lineIcons[type]),
                                               lineNames[type]);
            choice->setData(type);
            connect(choice, &QAction::triggered, this, [this, type] { addConnection(type); });
            if (type > 0) {
                auto *menuChoice = linesMenu->addAction(choice->icon(), lineNames[type]);
                connect(menuChoice, &QAction::triggered, this, [this, type] { addConnection(type); });
            }
        }
        lineMenu->addSeparator();
        auto *widthAction = new QWidgetAction(lineMenu);
        auto *widthWidget = new QWidget(lineMenu);
        auto *widthLayout = new QHBoxLayout(widthWidget);
        widthLayout->setContentsMargins(10, 2, 10, 2);
        widthLayout->addWidget(new QLabel(tr("Spessore"), widthWidget));
        auto *lineWidth = new QSpinBox(widthWidget);
        lineWidth->setObjectName("connectionWidth");
        lineWidth->setRange(1, 16);
        lineWidth->setValue(lineWidth_);
        lineWidth->setSuffix(" px");
        widthLayout->addWidget(lineWidth);
        widthAction->setDefaultWidget(widthWidget);
        lineMenu->addAction(widthAction);
        connect(lineWidth, QOverload<int>::of(&QSpinBox::valueChanged), this,
                [this](int value) {
                    lineWidth_ = value;
                    if (auto *view = dynamic_cast<DiagramView *>(diagramView()))
                        view->updateLineSettings(lineColor_, lineWidth_);
                    if (auto *scene = diagramScene())
                        for (QGraphicsItem *item : scene->selectedItems())
                            if (auto *line = dynamic_cast<ConnectorItem *>(item)) {
                                QPen pen = line->pen();
                                pen.setWidth(value);
                                line->setPen(pen);
                            }
                });
        auto *lineColor = lineMenu->addAction(tr("Colore linea..."));
        lineColor->setIcon(toolbarIcon("color"));
        connect(lineColor, &QAction::triggered, this, &SuiteWindow::setLineColor);
        lineButton->setMenu(lineMenu);
        toolbar->addWidget(lineButton);
        auto *textBoxAction = tools->addAction(toolbarIcon("text-box"), tr("Inserisci casella di testo"));
        textBoxAction->setObjectName("addMindTextBoxAction");
        toolbar->addAction(textBoxAction);
        connect(textBoxAction, &QAction::triggered, this, &SuiteWindow::addTextBox);

        addToolBarBreak();
        auto *mindTextBar = addToolBar(tr("Testo MindNet"));
        mindTextBar->setObjectName("mindTextToolbar");
        mindTextBar->setMovable(false);
        mindTextBar->setIconSize(QSize(24, 24));
        mindTextBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        mindFontCombo_ = new QFontComboBox(mindTextBar);
        mindFontCombo_->setObjectName("mindFontCombo");
        mindFontCombo_->setToolTip(tr("Tipo di carattere"));
        mindFontCombo_->setMinimumWidth(185);
        mindTextBar->addWidget(mindFontCombo_);
        mindFontSize_ = new QSpinBox(mindTextBar);
        mindFontSize_->setObjectName("mindFontSize");
        mindFontSize_->setToolTip(tr("Dimensione carattere"));
        mindFontSize_->setRange(8, 96);
        mindFontSize_->setValue(12);
        mindFontSize_->setFixedWidth(65);
        mindTextBar->addWidget(mindFontSize_);
        mindGrowAction_ = mindTextBar->addAction(toolbarIcon("text-grow"), tr("Aumenta dimensione testo"));
        mindGrowAction_->setObjectName("mindGrowTextAction");
        mindShrinkAction_ = mindTextBar->addAction(toolbarIcon("text-shrink"), tr("Diminuisci dimensione testo"));
        mindShrinkAction_->setObjectName("mindShrinkTextAction");
        connect(mindGrowAction_, &QAction::triggered, this, [this] {
            if (currentMindText()) mindFontSize_->setValue(qMin(96, mindFontSize_->value() + 1));
        });
        connect(mindShrinkAction_, &QAction::triggered, this, [this] {
            if (currentMindText()) mindFontSize_->setValue(qMax(8, mindFontSize_->value() - 1));
        });
        mindTextBar->addSeparator();
        mindBoldAction_ = mindTextBar->addAction(toolbarIcon("bold"), tr("Grassetto"));
        mindItalicAction_ = mindTextBar->addAction(toolbarIcon("italic"), tr("Corsivo"));
        mindUnderlineAction_ = mindTextBar->addAction(toolbarIcon("underline"), tr("Sottolineato"));
        mindStrikeAction_ = mindTextBar->addAction(toolbarIcon("strike"), tr("Barrato"));
        mindBoldAction_->setObjectName("mindBoldAction");
        mindItalicAction_->setObjectName("mindItalicAction");
        mindUnderlineAction_->setObjectName("mindUnderlineAction");
        mindStrikeAction_->setObjectName("mindStrikeAction");
        for (auto *action : {mindBoldAction_, mindItalicAction_, mindUnderlineAction_, mindStrikeAction_})
            action->setCheckable(true);
        mindColorAction_ = mindTextBar->addAction(toolbarIcon("color"), tr("Colore del testo"));
        mindColorAction_->setObjectName("mindTextColorAction");
        connect(mindFontCombo_, &QFontComboBox::currentFontChanged, this, [this](const QFont &font) {
            auto *text = currentMindText();
            if (!text) return;
            QFont current = text->font(); current.setFamily(font.family()); text->setFont(current);
            QTextCharFormat format; format.setFontFamilies({font.family()});
            applyMindTextFormat(format);
        });
        connect(mindFontSize_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int size) {
            auto *text = currentMindText();
            if (!text) return;
            QFont current = text->font(); current.setPointSize(size); text->setFont(current);
            QTextCharFormat format; format.setFontPointSize(size);
            applyMindTextFormat(format);
        });
        connect(mindBoldAction_, &QAction::triggered, this, [this](bool checked) {
            auto *text = currentMindText(); if (!text) return;
            QFont font = text->font(); font.setBold(checked); text->setFont(font);
            QTextCharFormat format; format.setFontWeight(checked ? QFont::Bold : QFont::Normal);
            applyMindTextFormat(format);
        });
        connect(mindItalicAction_, &QAction::triggered, this, [this](bool checked) {
            auto *text = currentMindText(); if (!text) return;
            QFont font = text->font(); font.setItalic(checked); text->setFont(font);
            QTextCharFormat format; format.setFontItalic(checked);
            applyMindTextFormat(format);
        });
        connect(mindUnderlineAction_, &QAction::triggered, this, [this](bool checked) {
            auto *text = currentMindText(); if (!text) return;
            QFont font = text->font(); font.setUnderline(checked); text->setFont(font);
            QTextCharFormat format; format.setFontUnderline(checked);
            applyMindTextFormat(format);
        });
        connect(mindStrikeAction_, &QAction::triggered, this, [this](bool checked) {
            auto *text = currentMindText(); if (!text) return;
            QFont font = text->font(); font.setStrikeOut(checked); text->setFont(font);
            QTextCharFormat format; format.setFontStrikeOut(checked);
            applyMindTextFormat(format);
        });
        connect(mindColorAction_, &QAction::triggered, this, [this] {
            auto *text = currentMindText(); if (!text) return;
            const QColor chosen = QColorDialog::getColor(text->defaultTextColor(), this,
                                                         tr("Colore del testo"));
            if (!chosen.isValid()) return;
            text->setDefaultTextColor(chosen);
            QTextCharFormat format; format.setForeground(chosen);
            applyMindTextFormat(format);
        });

        addToolBarBreak();
        auto *measureBar = addToolBar(tr("Misure MindNet"));
        measureBar->setObjectName("mindMeasureToolbar");
        measureBar->setMovable(false);
        measureBar->addWidget(new QLabel(tr("Righello  passo"), measureBar));
        rulerStep_ = new QDoubleSpinBox(measureBar);
        rulerStep_->setObjectName("rulerStep");
        rulerStep_->setRange(0.1, 1000);
        rulerStep_->setDecimals(1);
        rulerStep_->setValue(10);
        rulerStep_->setFixedWidth(80);
        measureBar->addWidget(rulerStep_);
        rulerUnit_ = new QComboBox(measureBar);
        rulerUnit_->setObjectName("rulerUnit");
        rulerUnit_->addItems({"mm", "cm", "px"});
        measureBar->addWidget(rulerUnit_);
        measureBar->addSeparator();
        measureBar->addWidget(new QLabel(tr("Griglia  passo"), measureBar));
        gridStep_ = new QDoubleSpinBox(measureBar);
        gridStep_->setObjectName("gridStep");
        gridStep_->setRange(0.1, 1000);
        gridStep_->setDecimals(1);
        gridStep_->setValue(10);
        gridStep_->setFixedWidth(80);
        measureBar->addWidget(gridStep_);
        gridUnit_ = new QComboBox(measureBar);
        gridUnit_->setObjectName("gridUnit");
        gridUnit_->addItems({"mm", "cm", "px"});
        measureBar->addWidget(gridUnit_);
        const auto markPageChanged = [this] {
            if (!tabs_->currentWidget()) return;
            tabs_->currentWidget()->setProperty("modified", true);
            updateTitle(tabs_->currentIndex());
            updateStatus();
        };
        connect(orientationAction_, &QAction::toggled, this, [this, markPageChanged](bool landscape) {
            if (auto *view = diagramView()) {
                configureDiagramPage(view, paperSize_->currentText(), landscape);
                markPageChanged();
            }
        });
        connect(rulersAction_, &QAction::toggled, this, [this, markPageChanged](bool visible) {
            if (auto *view = dynamic_cast<DiagramView *>(diagramView())) {
                view->setRulersVisible(visible);
                markPageChanged();
            }
        });
        connect(gridAction_, &QAction::toggled, this, [this, markPageChanged](bool visible) {
            if (auto *view = dynamic_cast<DiagramView *>(diagramView())) {
                view->setGridVisible(visible);
                markPageChanged();
            }
        });
        connect(rulerStep_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [this, markPageChanged](double value) {
            if (auto *view = dynamic_cast<DiagramView *>(diagramView())) {
                view->setRulerSettings(rulerUnit_->currentText(), value);
                markPageChanged();
            }
        });
        connect(gridStep_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                [this, markPageChanged](double value) {
            if (auto *view = dynamic_cast<DiagramView *>(diagramView())) {
                view->setGridSettings(gridUnit_->currentText(), value);
                markPageChanged();
            }
        });
        connect(rulerUnit_, &QComboBox::currentTextChanged, this,
                [this, markPageChanged](const QString &unit) {
            auto *view = dynamic_cast<DiagramView *>(diagramView());
            if (!view) return;
            const qreal pixels = pixelsForValue(view->rulerValue(), view->rulerUnit());
            const qreal converted = pixels / pixelsForValue(1, unit);
            const QSignalBlocker blocker(rulerStep_);
            rulerStep_->setValue(qBound(0.1, converted, 1000.0));
            view->setRulerSettings(unit, rulerStep_->value());
            markPageChanged();
        });
        connect(gridUnit_, &QComboBox::currentTextChanged, this,
                [this, markPageChanged](const QString &unit) {
            auto *view = dynamic_cast<DiagramView *>(diagramView());
            if (!view) return;
            const qreal pixels = pixelsForValue(view->gridValue(), view->gridUnit());
            const qreal converted = pixels / pixelsForValue(1, unit);
            const QSignalBlocker blocker(gridStep_);
            gridStep_->setValue(qBound(0.1, converted, 1000.0));
            view->setGridSettings(unit, gridStep_->value());
            markPageChanged();
        });

        auto *dock = new QDockWidget(tr("Forme"), this);
        dock->setObjectName("mindNetShapesDock");
        dock->setAllowedAreas(Qt::RightDockWidgetArea);
        dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
        auto *palette = new QWidget(dock);
        auto *paletteLayout = new QVBoxLayout(palette);
        paletteLayout->setContentsMargins(12, 16, 12, 12);
        auto *intro = new QLabel(tr("Aggiungi un simbolo al foglio"), palette);
        intro->setStyleSheet("color: #5b7189; font-size: 12px;");
        paletteLayout->addWidget(intro);
        auto *category = new QComboBox(palette);
        category->setObjectName("mindShapeCategory");
        category->addItem(tr("Diagrammi di flusso"));
        category->addItem(tr("Networking"));
        paletteLayout->addWidget(category);
        auto *groups = new QStackedWidget(palette);
        paletteLayout->addWidget(groups, 1);
        const QVector<QPair<QString, QString>> shapes{
            {"process", tr("Processo")}, {"terminal", tr("Inizio / fine")},
            {"decision", tr("Decisione")}, {"input", tr("Input / output")},
            {"document", tr("Documento")}, {"database", tr("Database")},
            {"connector", tr("Connettore")}, {"predefined", tr("Sottoprocesso")},
            {"delay", tr("Attesa")}};
        const QVector<QPair<QString, QString>> networkShapes{
            {"net_router", tr("Router")}, {"net_switch_l2", tr("Switch Layer 2")},
            {"net_switch_l3", tr("Switch Layer 3")}, {"net_pc", tr("PC")},
            {"net_laptop", tr("Laptop")}, {"net_wifi", tr("Wi-Fi")},
            {"net_access_point", tr("Access point")}, {"net_server", tr("Server")},
            {"net_firewall", tr("Firewall")}, {"net_cloud", tr("Cloud / Internet")},
            {"net_nas", tr("NAS")}, {"net_database", tr("Database")},
            {"net_phone", tr("Smartphone")}, {"net_tablet", tr("Tablet")},
            {"net_printer", tr("Stampante")}, {"net_camera", tr("Telecamera IP")},
            {"net_antenna", tr("Antenna")}, {"net_modem", tr("Modem")},
            {"net_gateway", tr("Gateway")}, {"net_vpn", tr("VPN")},
            {"net_load_balancer", tr("Load balancer")}, {"net_cable", tr("Cavo / link")}};
        for (const auto &entries : {shapes, networkShapes}) {
            auto *scroll = new QScrollArea(groups);
            scroll->setWidgetResizable(true);
            scroll->setFrameShape(QFrame::NoFrame);
            auto *list = new QWidget(scroll);
            auto *grid = new QGridLayout(list);
            grid->setContentsMargins(0, 8, 0, 8);
            for (int i = 0; i < entries.size(); ++i) {
                auto *button = new QToolButton(list);
                button->setObjectName("shape_" + entries[i].first);
                button->setIcon(shapeIcon(entries[i].first));
                button->setIconSize(QSize(48, 36));
                button->setText(entries[i].second);
                button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
                button->setFixedSize(102, 78);
                button->setStyleSheet("QToolButton { background: white; border: 1px solid #cfdaea; border-radius: 7px; color: #284666; } QToolButton:hover { background: #eaf4ff; border-color: #75a9d8; }");
                grid->addWidget(button, i / 2, i % 2);
                connect(button, &QToolButton::clicked, this,
                        [this, shape = entries[i].first] { addNode(shape); });
            }
            grid->setRowStretch((entries.size() + 1) / 2, 1);
            scroll->setWidget(list);
            groups->addWidget(scroll);
        }
        connect(category, qOverload<int>(&QComboBox::currentIndexChanged),
                groups, &QStackedWidget::setCurrentIndex);
        auto *shapeColor = new QPushButton(tr("Colore forma..."), palette);
        shapeColor->setObjectName("shapeColorButton");
        shapeColor->setIcon(toolbarIcon("color"));
        paletteLayout->addWidget(shapeColor);
        connect(shapeColor, &QPushButton::clicked, this, &SuiteWindow::setShapeColor);
        paletteLayout->addSpacing(14);
        auto *pageLabel = new QLabel(tr("TESTI DELLA PAGINA"), palette);
        pageLabel->setStyleSheet("color: #3f6283; font-size: 11px; font-weight: 700;");
        paletteLayout->addWidget(pageLabel);
        for (const auto &entry : {qMakePair(QString("header"), tr("Intestazione...")),
                                  qMakePair(QString("footer"), tr("Piè di pagina..."))}) {
            auto *button = new QPushButton(entry.second, palette);
            button->setObjectName(entry.first + "Button");
            paletteLayout->addWidget(button);
            connect(button, &QPushButton::clicked, this,
                    [this, role = entry.first] { editPageText(role); });
        }
        paletteLayout->addStretch();
        dock->setWidget(palette);
        addDockWidget(Qt::RightDockWidgetArea, dock);
    } else {
        if (mode_ == SuiteMode::Impagination) addToolBarBreak();
        auto *formatBar = addToolBar(tr("Formattazione"));
        if (mode_ == SuiteMode::Impagination) formatBar->setObjectName("impTextToolbar");
        formatBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        formatBar->setIconSize(QSize(24, 24));
        auto *font = new QFontComboBox(formatBar);
        formatBar->addWidget(font);
        for (auto pair : {qMakePair(bold, "bold"), qMakePair(italic, "italic"),
                          qMakePair(underline, "underline"), qMakePair(color, "color"),
                          qMakePair(left, "align-left"), qMakePair(center, "align-center"),
                          qMakePair(right, "align-right"), qMakePair(justify, "align-justify")})
            pair.first->setIcon(toolbarIcon(pair.second));
        for (auto *action : {bold, italic, underline, color, left, center, right, justify})
            formatBar->addAction(action);
        connect(font, &QFontComboBox::currentFontChanged, this, [this](const QFont &selected) {
            if (auto *editor = textEditor()) editor->setCurrentFont(selected);
        });
        if (mode_ == SuiteMode::Impagination)
            setupImpaginationTools(file, edit, format, tools, formatBar);
    }
    status_->setContentsMargins(10, 0, 10, 0);
    statusBar()->addWidget(status_, 1);
    connect(create, &QAction::triggered, this, &SuiteWindow::newDocument);
    connect(open, &QAction::triggered, this, [this] {
        const QString filter = mode_ == SuiteMode::MindNet
            ? tr("Diagrammi MindNet (*.nmn *.json);;NebiPad+ MindNet (*.nmn);;JSON (*.json);;Tutti i file (*)")
            : mode_ == SuiteMode::Impagination
                ? tr("Documenti Impagination (*.npi *.doc *.ods *.html *.htm *.txt *.pdf);;NebiPad+ Impagination (*.npi);;Microsoft Word (*.doc);;LibreOffice ODS (*.ods);;PDF (*.pdf);;HTML (*.html *.htm);;Testo (*.txt);;Tutti i file (*)")
                : tr("Documenti Math (*.nmt *.html *.htm *.txt);;NebiPad+ Math (*.nmt);;HTML (*.html *.htm);;Testo (*.txt);;Tutti i file (*)");
        const QString path = QFileDialog::getOpenFileName(this, tr("Apri documento"), {}, filter);
        if (!path.isEmpty()) openFile(path);
    });
    connect(saveAction, &QAction::triggered, this, [this] { save(tabs_->currentIndex()); });
    connect(saveAs, &QAction::triggered, this, [this] { save(tabs_->currentIndex(), true); });
    connect(close, &QAction::triggered, this, [this] {
        closeTab(tabs_->currentIndex());
    });
    connect(tabs_, &QTabWidget::tabCloseRequested, this, [this](int index) {
        closeTab(index);
    });
    connect(tabs_, &QTabWidget::currentChanged, this, &SuiteWindow::updateStatus);
    if (mode_ == SuiteMode::Impagination)
        connect(tabs_, &QTabWidget::currentChanged, this, [this] {
            auto *editor = textEditor(); if (!editor) return;
            QWidget *paper = editor->parentWidget();
            const QSignalBlocker sizeBlock(paperSize_);
            paperSize_->setCurrentText(paper->property("paperFormat").toString().isEmpty()
                ? "A4" : paper->property("paperFormat").toString());
            if (auto *action = findChild<QAction *>("impLandscapeAction")) {
                const QSignalBlocker orientationBlock(action);
                action->setChecked(paper->property("landscape").toBool());
            }
            for (const auto &entry : {qMakePair("impHeaderToggle", "headerVisible"),
                                      qMakePair("impFooterToggle", "footerVisible"),
                                      qMakePair("impPageNumberToggle", "pageNumbersVisible")})
                if (auto *action = findChild<QAction *>(entry.first)) {
                    const QSignalBlocker blocker(action);
                    action->setChecked(paper->property(entry.second).toBool());
                }
        });
    connect(tabs_, &QTabWidget::currentChanged, this, &SuiteWindow::updateMindTextControls);
    connect(tabs_, &QTabWidget::currentChanged, this, &SuiteWindow::syncMindViewControls);
    connect(print, &QAction::triggered, this, [this] {
        QPrinter printer(QPrinter::HighResolution);
        QPrintDialog dialog(&printer, this);
        if (dialog.exec() != QDialog::Accepted) return;
        if (auto *editor = textEditor()) {
            if (mode_ == SuiteMode::Impagination) {
                printImpagination(&printer);
            } else {
                editor->print(&printer);
            }
        }
        else if (auto *scene = diagramScene()) {
            QPainter painter(&printer);
            scene->render(&painter, QRectF(),
                QRectF(QPointF(0, 0), orientedPaperDimensions(
                    diagramView()->property("paperFormat").toString(),
                    diagramView()->property("landscape").toBool())));
        }
    });
    connect(undo, &QAction::triggered, this, [this] { if (textEditor()) textEditor()->undo(); });
    connect(redo, &QAction::triggered, this, [this] { if (textEditor()) textEditor()->redo(); });
    connect(cut, &QAction::triggered, this, [this] { if (textEditor()) textEditor()->cut(); });
    connect(copy, &QAction::triggered, this, [this] { if (textEditor()) textEditor()->copy(); });
    connect(paste, &QAction::triggered, this, [this] { if (textEditor()) textEditor()->paste(); });
    connect(zoomIn, &QAction::triggered, this, [this] { zoom(1.15); });
    connect(zoomOut, &QAction::triggered, this, [this] { zoom(1.0 / 1.15); });
    connect(paperSize_, &QComboBox::currentTextChanged, this, [this](const QString &format) {
        if (auto *editor = textEditor()) {
            QWidget *paper = editor->parentWidget();
            QSize dimensions = format == "A3" ? QSize(1123, 1587) : QSize(794, 1123);
            if (paper->property("landscape").toBool()) dimensions.transpose();
            paper->setFixedSize(dimensions);
            paper->setProperty("paperFormat", format);
            editor->document()->setModified(true);
        } else if (auto *graphics = diagramView()) {
            configureDiagramPage(graphics, format,
                                 orientationAction_ && orientationAction_->isChecked());
            tabs_->currentWidget()->setProperty("modified", true);
            updateTitle(tabs_->currentIndex());
            updateStatus();
        }
    });
    if (mode_ == SuiteMode::MindNet) {
        connect(connection, &QAction::triggered, this, [this] { addConnection(lineType_); });
    } else {
        connect(bold, &QAction::triggered, this, [this] {
            if (textEditor()) textEditor()->setFontWeight(textEditor()->fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
        });
        connect(italic, &QAction::triggered, this, [this] { if (textEditor()) textEditor()->setFontItalic(!textEditor()->fontItalic()); });
        connect(underline, &QAction::triggered, this, [this] { if (textEditor()) textEditor()->setFontUnderline(!textEditor()->fontUnderline()); });
        connect(color, &QAction::triggered, this, [this] {
            if (auto *editor = textEditor()) {
                QColor selected = QColorDialog::getColor(editor->textColor(), this);
                if (selected.isValid()) editor->setTextColor(selected);
            }
        });
        for (auto pair : {qMakePair(left, Qt::AlignLeft), qMakePair(center, Qt::AlignHCenter),
                          qMakePair(right, Qt::AlignRight), qMakePair(justify, Qt::AlignJustify)})
            connect(pair.first, &QAction::triggered, this, [this, alignment = pair.second] {
                if (textEditor()) textEditor()->setAlignment(alignment);
            });
    }
    menuBar()->removeAction(helpMenu->menuAction());
    menuBar()->addMenu(helpMenu);
    newDocument();
}

SuiteWindow::~SuiteWindow()
{
    if (dictation_) dictation_->stop();
    for (auto *scene : findChildren<QGraphicsScene *>())
        QObject::disconnect(scene, nullptr, this, nullptr);
}

bool SuiteWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == tabs_ && dictationOverlay_ &&
        (event->type() == QEvent::Resize || event->type() == QEvent::Show))
        placeDictationOverlay();
    return QMainWindow::eventFilter(watched, event);
}

void SuiteWindow::placeDictationOverlay()
{
    if (!dictationOverlay_) return;
    dictationOverlay_->move(qMax(0, (tabs_->width() - dictationOverlay_->width()) / 2),
                            qMax(0, tabs_->height() - dictationOverlay_->height() - 18));
    dictationOverlay_->raise();
}

void SuiteWindow::toggleDictation()
{
    if (!dictation_) return;
    if (dictation_->state() == DictationController::State::Idle) {
        dictationTarget_ = textEditor();
        if (!dictationTarget_) {
            statusBar()->showMessage(tr("Apri un documento Impagination prima di iniziare la dettatura."), 5000);
            return;
        }
        dictationOverlay_->setTranscript({});
    }
    dictation_->toggle();
}

void SuiteWindow::insertDictatedText(const QString &text)
{
    const QString spoken = text.trimmed();
    if (!dictationTarget_ || spoken.isEmpty()) {
        statusBar()->showMessage(tr("Nessuna parola riconosciuta."), 5000);
        return;
    }
    QTextCursor cursor = dictationTarget_->textCursor();
    const QString line = cursor.block().text();
    QString insertion = spoken;
    if (cursor.positionInBlock() > 0 && !line.at(cursor.positionInBlock() - 1).isSpace())
        insertion.prepend(' ');
    if (cursor.positionInBlock() < line.size() && !line.at(cursor.positionInBlock()).isSpace())
        insertion.append(' ');
    cursor.insertText(insertion);
    dictationTarget_->setTextCursor(cursor);
    dictationTarget_->setFocus();
    statusBar()->showMessage(tr("Dettatura inserita nel documento Impagination."), 4000);
}

void SuiteWindow::startRecording()
{
    if (!recording_) return;
    if (recording_->isRecording()) { recording_->stop(); return; }
    recordAction_->setChecked(false);
    if (dictation_ && dictation_->state() != DictationController::State::Idle) {
        statusBar()->showMessage(tr("Termina la dettatura prima di registrare una nota audio."), 5000); return;
    }
    QTextEdit *editor = textEditor(); const int index = tabs_->currentIndex();
    if (!editor || (pathFor(index).isEmpty() && !save(index))) return;
    const QString document = pathFor(index);
    const QString directory = QFileInfo(document).absolutePath() + "/recording";
    const QString name = QStringLiteral("audio-%1-%2.wav").arg(
        QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"),
        QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
    QString error; bool storageError = false; QString target = "recording/" + name;
    const bool directoryReady = QDir().mkpath(directory);
    bool started = directoryReady && recording_->start(directory + "/" + name, &error, &storageError);
    if (!started && (!directoryReady || storageError)) {
        const QString fallback = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/recording";
        if (QDir().mkpath(fallback)) {
            error.clear(); started = recording_->start(fallback + "/" + name, &error, &storageError);
            if (started) target = QUrl::fromLocalFile(fallback + "/" + name).toString(QUrl::FullyEncoded);
        }
    }
    if (!started) { QMessageBox::warning(this,tr("Registrazione audio"),error); return; }
    const int number = nextRecordingNumber(document, editor->toPlainText());
    if (!registerRecordingLink(document, number, target, &error)) {
        recording_->stop(); QFile::remove(resolveAudioTarget(document,target));
        QMessageBox::warning(this,tr("Registrazione audio"),error); return;
    }
    recordingTarget_ = editor;
    QTextCursor cursor = editor->textCursor(); QString marker = QString("[Registrazione_%1]").arg(number);
    const QString line = cursor.block().text();
    if (cursor.positionInBlock() > 0 && !line.at(cursor.positionInBlock()-1).isSpace()) marker.prepend(' ');
    if (cursor.positionInBlock() < line.size() && !line.at(cursor.positionInBlock()).isSpace()) marker.append(' ');
    cursor.insertText(marker); editor->setTextCursor(cursor); editor->setFocus();
}

QString SuiteWindow::audioPathNearCursor(QTextEdit *editor) const
{
    if (!editor) return {};
    int index=-1; for(int i=0;i<tabs_->count();++i) if(editorAt(tabs_,i)==editor){index=i;break;}
    return resolveAudioTarget(pathFor(index), nearestAudioTarget(editor->toPlainText(), editor->textCursor().position()));
}

void SuiteWindow::playPauseRecording()
{
    if (!recording_) return; QString error; recording_->playPause(audioPathNearCursor(textEditor()), &error);
    if (!error.isEmpty()) statusBar()->showMessage(error,6000);
}

void SuiteWindow::stopAudio() { if (recording_) recording_->stop(); }

bool SuiteWindow::tabIsPinned(int index) const
{ QWidget *page=tabs_->widget(index); return page && page->property("tabPinned").toBool(); }

void SuiteWindow::installImpaginationTabControls(int index)
{
    QWidget *page=tabs_->widget(index); if(!page)return;
    auto button=[this](const char *name,const QString &icon){auto*b=new QToolButton(tabs_->tabBar());b->setObjectName(name);b->setAutoRaise(true);b->setIcon(toolbarIcon(icon));b->setIconSize(QSize(14,14));b->setFixedSize(22,22);b->setStyleSheet("QToolButton{border:0;border-radius:4px;padding:2px}QToolButton:hover{background:rgba(49,68,91,35)}QToolButton:checked{background:rgba(22,134,167,48)}");return b;};
    auto *readOnly=button("impTabReadOnlyButton","save");readOnly->setCheckable(true);
    connect(readOnly,&QToolButton::clicked,this,[this,page]{auto*editor=editorAt(tabs_,tabs_->indexOf(page));if(!editor)return;const bool value=!page->property("tabReadOnly").toBool();page->setProperty("tabReadOnly",value);editor->setReadOnly(value);updateImpaginationTabControls(tabs_->indexOf(page));});
    tabs_->tabBar()->setTabButton(index,QTabBar::LeftSide,readOnly);
    auto *right=new QWidget(tabs_->tabBar());right->setObjectName("impTabRightControls");auto*layout=new QHBoxLayout(right);layout->setContentsMargins(0,0,0,0);layout->setSpacing(0);
    auto *pin=button("impTabPinButton","pin");pin->setCheckable(true);auto*close=button("impTabCloseButton","close");layout->addWidget(pin);layout->addWidget(close);
    connect(pin,&QToolButton::clicked,this,[this,page]{int index=tabs_->indexOf(page);const bool value=!page->property("tabPinned").toBool();page->setProperty("tabPinned",value);int others=0;for(int i=0;i<tabs_->count();++i)if(tabs_->widget(i)!=page&&tabIsPinned(i))++others;tabs_->tabBar()->moveTab(index,others);tabs_->setCurrentWidget(page);updateImpaginationTabControls(tabs_->indexOf(page));});
    connect(close,&QToolButton::clicked,this,[this,page]{closeTab(tabs_->indexOf(page));});
    tabs_->tabBar()->setTabButton(index,QTabBar::RightSide,right);updateImpaginationTabControls(index);
}

void SuiteWindow::updateImpaginationTabControls(int index)
{
    QWidget *page=tabs_->widget(index);if(!page)return;
    if(auto*b=qobject_cast<QToolButton*>(tabs_->tabBar()->tabButton(index,QTabBar::LeftSide))){const bool ro=page->property("tabReadOnly").toBool();b->setChecked(ro);b->setToolTip(ro?tr("Sola lettura attiva · clicca per modificare"):tr("Rendi il documento in sola lettura"));}
    if(auto*w=tabs_->tabBar()->tabButton(index,QTabBar::RightSide)){if(auto*b=w->findChild<QToolButton*>("impTabPinButton")){b->setChecked(tabIsPinned(index));b->setToolTip(tabIsPinned(index)?tr("Rimuovi il pin"):tr("Fissa la scheda"));}if(auto*b=w->findChild<QToolButton*>("impTabCloseButton"))b->setToolTip(tabIsPinned(index)?tr("Rimuovi il pin prima di chiudere"):tr("Chiudi documento"));}
}

bool SuiteWindow::closeTab(int index)
{
    if(index<0)return false;if(tabIsPinned(index)){statusBar()->showMessage(tr("Rimuovi il pin dalla scheda prima di chiuderla"),3500);return false;}if(!maybeSave(index))return false;
    QWidget*page=tabs_->widget(index);if(auto*scene=page->findChild<QGraphicsScene*>())QObject::disconnect(scene,nullptr,this,nullptr);if(recordingTarget_&&page->isAncestorOf(recordingTarget_)&&recording_)recording_->stop();tabs_->removeTab(index);page->deleteLater();updateMindTextControls();return true;
}

void SuiteWindow::setupPdfTools()
{
    auto *menu = menuBar()->addMenu(tr("PDF"));
    menu->setObjectName("pdfMenu");
    auto activePdf = [this]() -> QString {
        const QString path = pathFor(tabs_->currentIndex());
        if (!path.endsWith(".pdf", Qt::CaseInsensitive))
            QMessageBox::information(this, tr("Strumenti PDF"), tr("Apri prima un documento PDF in Impagination."));
        return path.endsWith(".pdf", Qt::CaseInsensitive) ? path : QString();
    };
    auto refresh = [this](const QString &path) {
        QWidget *old = tabs_->currentWidget();
        if (!openFile(path)) return;
        const int oldIndex = tabs_->indexOf(old);
        if (oldIndex >= 0) { tabs_->removeTab(oldIndex); old->deleteLater(); }
        statusBar()->showMessage(tr("PDF modificato e ricaricato"), 4000);
    };
    auto failed = [this](const QString &error) {
        QMessageBox::critical(this, tr("Operazione PDF non riuscita"), error);
    };

    auto *open = menu->addAction(tr("Apri PDF..."));
    open->setObjectName("pdfOpenAction");
    connect(open, &QAction::triggered, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, tr("Apri PDF"), {}, tr("PDF (*.pdf)"));
        if (!path.isEmpty()) openFile(path);
    });
    auto *exportAction = menu->addAction(tr("Crea PDF dal documento corrente..."));
    exportAction->setObjectName("pdfCreateAction");
    connect(exportAction, &QAction::triggered, this, [this] {
        if (!textEditor()) return;
        const QString path = QFileDialog::getSaveFileName(this, tr("Crea PDF"), tr("Documento.pdf"), tr("PDF (*.pdf)"));
        if (path.isEmpty()) return;
        QPrinter printer(QPrinter::HighResolution); printer.setOutputFormat(QPrinter::PdfFormat); printer.setOutputFileName(path);
        printImpagination(&printer); rememberRecentFile(path, QStringLiteral("Impagination"));
    });
    menu->addSeparator();

    auto *text = menu->addAction(tr("Aggiungi testo al PDF..."));
    text->setObjectName("pdfAddTextAction");
    connect(text, &QAction::triggered, this, [=] {
        const QString path = activePdf(); if (path.isEmpty()) return;
        const int count = podofoPageCount(path); QDialog dialog(this); dialog.setWindowTitle(tr("Aggiungi testo al PDF"));
        auto *grid = new QGridLayout(&dialog); auto *page = new QSpinBox(&dialog); page->setRange(1, qMax(1, count));
        auto *x = new QDoubleSpinBox(&dialog); auto *y = new QDoubleSpinBox(&dialog); auto *size = new QDoubleSpinBox(&dialog);
        x->setRange(0, 2000); y->setRange(0, 2000); y->setValue(750); size->setRange(4, 144); size->setValue(12);
        auto *content = new QLineEdit(&dialog); auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        grid->addWidget(new QLabel(tr("Pagina:")),0,0); grid->addWidget(page,0,1); grid->addWidget(new QLabel("X:"),1,0); grid->addWidget(x,1,1);
        grid->addWidget(new QLabel("Y:"),2,0); grid->addWidget(y,2,1); grid->addWidget(new QLabel(tr("Dimensione:")),3,0); grid->addWidget(size,3,1);
        grid->addWidget(new QLabel(tr("Testo:")),4,0); grid->addWidget(content,4,1); grid->addWidget(buttons,5,0,1,2);
        connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept); connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if (dialog.exec()!=QDialog::Accepted || content->text().isEmpty()) return;
        QString error; if (!podofoAddText(path,{page->value(),x->value(),y->value(),size->value(),content->text()},&error)) failed(error); else refresh(path);
    });

    auto *image = menu->addAction(tr("Aggiungi immagine al PDF..."));
    image->setObjectName("pdfAddImageAction");
    connect(image, &QAction::triggered, this, [=] {
        const QString path = activePdf(); if (path.isEmpty()) return;
        const QString imagePath = QFileDialog::getOpenFileName(this,tr("Immagine"),{},tr("Immagini (*.png *.jpg *.jpeg *.tif *.tiff)"));
        if (imagePath.isEmpty()) return; bool ok=false; const int page=QInputDialog::getInt(this,tr("Pagina"),tr("Numero pagina:"),1,1,qMax(1,podofoPageCount(path)),1,&ok); if(!ok)return;
        QString error; PdfImageOverlay overlay; overlay.page=page; overlay.imagePath=imagePath;
        if (!podofoAddImage(path,overlay,&error)) failed(error); else refresh(path);
    });

    auto *pages = menu->addMenu(tr("Pagine")); pages->setObjectName("pdfPagesMenu");
    auto pageNumber = [this](const QString &path, const QString &title) {
        bool ok=false; return QInputDialog::getInt(this,title,tr("Numero pagina:"),1,1,qMax(1,podofoPageCount(path)),1,&ok) * (ok ? 1 : -1);
    };
    auto *insert = pages->addAction(tr("Inserisci pagina A4...")); insert->setObjectName("pdfInsertPageAction");
    connect(insert,&QAction::triggered,this,[=]{const QString p=activePdf();if(p.isEmpty())return;int n=pageNumber(p,tr("Inserisci pagina"));if(n<1)return;QString e;if(!podofoInsertPage(p,n,&e))failed(e);else refresh(p);});
    auto *remove = pages->addAction(tr("Elimina pagina...")); remove->setObjectName("pdfDeletePageAction");
    connect(remove,&QAction::triggered,this,[=]{const QString p=activePdf();if(p.isEmpty())return;int n=pageNumber(p,tr("Elimina pagina"));if(n<1)return;QString e;if(!podofoDeletePage(p,n,&e))failed(e);else refresh(p);});
    auto *rotate = pages->addAction(tr("Ruota pagina di 90°...")); rotate->setObjectName("pdfRotatePageAction");
    connect(rotate,&QAction::triggered,this,[=]{const QString p=activePdf();if(p.isEmpty())return;int n=pageNumber(p,tr("Ruota pagina"));if(n<1)return;QString e;if(!podofoRotatePage(p,n,90,&e))failed(e);else refresh(p);});
    auto *move = pages->addAction(tr("Sposta pagina...")); move->setObjectName("pdfMovePageAction");
    connect(move,&QAction::triggered,this,[=]{const QString p=activePdf();if(p.isEmpty())return;int from=pageNumber(p,tr("Pagina da spostare"));if(from<1)return;int to=pageNumber(p,tr("Nuova posizione"));if(to<1)return;QString e;if(!podofoMovePage(p,from,to,&e))failed(e);else refresh(p);});

    auto *merge = menu->addAction(tr("Unisci un altro PDF...")); merge->setObjectName("pdfMergeAction");
    connect(merge,&QAction::triggered,this,[=]{const QString p=activePdf();if(p.isEmpty())return;const QString other=QFileDialog::getOpenFileName(this,tr("PDF da unire"),{},tr("PDF (*.pdf)"));if(other.isEmpty())return;QString e;if(!podofoMergePdf(p,other,&e))failed(e);else refresh(p);});
    auto *metadata = menu->addAction(tr("Proprietà e metadati...")); metadata->setObjectName("pdfMetadataAction");
    connect(metadata,&QAction::triggered,this,[=]{const QString p=activePdf();if(p.isEmpty())return;QDialog d(this);d.setWindowTitle(tr("Metadati PDF"));auto*g=new QGridLayout(&d);QVector<QLineEdit*>f;for(int i=0;i<4;++i)f<<new QLineEdit(&d);const QStringList labels{tr("Titolo"),tr("Autore"),tr("Oggetto"),tr("Parole chiave")};for(int i=0;i<4;++i){g->addWidget(new QLabel(labels[i]),i,0);g->addWidget(f[i],i,1);}auto*b=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&d);g->addWidget(b,4,0,1,2);connect(b,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(b,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;QString e;if(!podofoSetMetadata(p,{f[0]->text(),f[1]->text(),f[2]->text(),f[3]->text()},&e))failed(e);else refresh(p);});

    if (!podofoAvailable()) {
        for (QAction *action : menu->actions()) if (action != open && action != exportAction) action->setEnabled(false);
        menu->setToolTip(tr("PoDoFo non è disponibile in questa compilazione"));
    }
}

void SuiteWindow::setupImpaginationTools(QMenu *file, QMenu *edit, QMenu *format,
                                         QMenu *tools, QToolBar *toolbar)
{
    auto *insert = menuBar()->addMenu(tr("&Inserisci"));
    insert->setObjectName("impInsertMenu");
    auto *layout = menuBar()->addMenu(tr("&Layout"));
    layout->setObjectName("impLayoutMenu");
    auto *paragraphToolbar = addToolBar(tr("Paragrafo"));
    paragraphToolbar->setObjectName("impParagraphToolbar");
    paragraphToolbar->setMovable(false);
    paragraphToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    paragraphToolbar->setIconSize(QSize(24, 24));
    auto *styles = new QComboBox(toolbar);
    styles->setObjectName("impStyleCombo");
    styles->addItems({tr("Normale"), tr("Titolo"), tr("Sottotitolo"),
                      tr("Titolo 1"), tr("Titolo 2"), tr("Titolo 3")});
    styles->setToolTip(tr("Stile del paragrafo"));
    styles->setMinimumWidth(115);
    toolbar->addWidget(styles);
    auto *size = new QSpinBox(toolbar);
    size->setObjectName("impFontSize");
    size->setRange(6, 96);
    size->setValue(12);
    size->setSuffix(" pt");
    size->setFixedWidth(77);
    toolbar->addWidget(size);
    auto mergeCharacter = [this](const QTextCharFormat &format) {
        if (auto *editor = textEditor()) {
            QTextCursor cursor = editor->textCursor();
            cursor.mergeCharFormat(format);
            editor->mergeCurrentCharFormat(format);
            editor->setFocus();
        }
    };
    connect(size, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [mergeCharacter](int points) {
        QTextCharFormat format; format.setFontPointSize(points); mergeCharacter(format);
    });
    connect(styles, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, mergeCharacter](int index) {
        auto *editor = textEditor();
        if (!editor) return;
        QTextCursor cursor = editor->textCursor();
        cursor.select(QTextCursor::BlockUnderCursor);
        QTextBlockFormat block;
        block.setHeadingLevel(index >= 3 ? index - 2 : 0);
        block.setTopMargin(index == 0 ? 0 : 10);
        block.setBottomMargin(index == 0 ? 0 : 6);
        cursor.mergeBlockFormat(block);
        editor->setTextCursor(cursor);
        QTextCharFormat character;
        character.setFontWeight(index == 0 || index == 2 ? QFont::Normal : QFont::Bold);
        character.setFontPointSize(index == 1 ? 24 : index == 2 ? 16 :
                                   index == 3 ? 20 : index == 4 ? 16 : index == 5 ? 14 : 12);
        mergeCharacter(character);
    });
    auto *bullets = paragraphToolbar->addAction(tr("•"));
    bullets->setObjectName("impBulletAction");
    bullets->setToolTip(tr("Elenco puntato"));
    auto *numbers = paragraphToolbar->addAction(tr("1."));
    numbers->setObjectName("impNumberAction");
    numbers->setToolTip(tr("Elenco numerato"));
    auto makeList = [this](QTextListFormat::Style style) {
        if (auto *editor = textEditor()) {
            QTextCursor cursor = editor->textCursor();
            cursor.beginEditBlock();
            cursor.createList(style);
            cursor.endEditBlock();
            editor->setFocus();
        }
    };
    connect(bullets, &QAction::triggered, this,
            [makeList] { makeList(QTextListFormat::ListDisc); });
    connect(numbers, &QAction::triggered, this,
            [makeList] { makeList(QTextListFormat::ListDecimal); });
    paragraphToolbar->addSeparator();
    auto *indentLess = paragraphToolbar->addAction(tr("⇤"));
    indentLess->setObjectName("impIndentLessAction");
    indentLess->setToolTip(tr("Riduci rientro"));
    auto *indentMore = paragraphToolbar->addAction(tr("⇥"));
    indentMore->setObjectName("impIndentMoreAction");
    indentMore->setToolTip(tr("Aumenta rientro"));
    auto indent = [this](int delta) {
        if (auto *editor = textEditor()) {
            QTextCursor cursor = editor->textCursor();
            QTextBlockFormat block = cursor.blockFormat();
            block.setIndent(qMax(0, block.indent() + delta));
            cursor.mergeBlockFormat(block);
        }
    };
    connect(indentLess, &QAction::triggered, this, [indent] { indent(-1); });
    connect(indentMore, &QAction::triggered, this, [indent] { indent(1); });
    auto *strike = format->addAction(tr("Barrato"));
    strike->setObjectName("impStrikeAction");
    connect(strike, &QAction::triggered, this, [this, mergeCharacter] {
        if (auto *editor = textEditor()) {
            QTextCharFormat f; f.setFontStrikeOut(!editor->currentCharFormat().fontStrikeOut());
            mergeCharacter(f);
        }
    });
    auto *highlight = format->addAction(tr("Evidenzia testo..."));
    highlight->setObjectName("impHighlightAction");
    connect(highlight, &QAction::triggered, this, [this, mergeCharacter] {
        const QColor color = QColorDialog::getColor(QColor("#fff2a8"), this, tr("Evidenziazione"));
        if (color.isValid()) { QTextCharFormat f; f.setBackground(color); mergeCharacter(f); }
    });
    for (const auto &entry : {qMakePair(tr("Apice"), QTextCharFormat::AlignSuperScript),
                              qMakePair(tr("Pedice"), QTextCharFormat::AlignSubScript),
                              qMakePair(tr("Posizione normale"), QTextCharFormat::AlignNormal)}) {
        auto *action = format->addAction(entry.first);
        connect(action, &QAction::triggered, this, [mergeCharacter, alignment = entry.second] {
            QTextCharFormat f; f.setVerticalAlignment(alignment); mergeCharacter(f);
        });
    }
    auto *clear = format->addAction(tr("Cancella formattazione"));
    clear->setObjectName("impClearFormatAction");
    connect(clear, &QAction::triggered, this, [this] {
        if (auto *editor = textEditor()) {
            QTextCursor cursor = editor->textCursor();
            cursor.setCharFormat(QTextCharFormat());
            editor->setCurrentCharFormat(QTextCharFormat());
        }
    });
    auto *listsMenu = format->addMenu(tr("Elenchi"));
    listsMenu->addAction(tr("Puntato"), this,
                         [makeList] { makeList(QTextListFormat::ListDisc); });
    listsMenu->addAction(tr("Numerato"), this,
                         [makeList] { makeList(QTextListFormat::ListDecimal); });
    listsMenu->addAction(tr("Numeri romani"), this,
                         [makeList] { makeList(QTextListFormat::ListUpperRoman); });
    listsMenu->addAction(tr("Lettere"), this,
                         [makeList] { makeList(QTextListFormat::ListLowerAlpha); });
    auto *spacing = format->addMenu(tr("Interlinea"));
    for (int percent : {100, 115, 150, 200}) {
        auto *action = spacing->addAction(QString::number(percent / 100.0, 'f', 2));
        connect(action, &QAction::triggered, this, [this, percent] {
            if (auto *editor = textEditor()) {
                QTextCursor cursor = editor->textCursor();
                QTextBlockFormat block = cursor.blockFormat();
                block.setLineHeight(percent, QTextBlockFormat::ProportionalHeight);
                cursor.mergeBlockFormat(block);
            }
        });
    }
    auto *paragraphSpace = format->addMenu(tr("Spazio dopo paragrafo"));
    for (int pixels : {0, 6, 12, 18}) {
        auto *action = paragraphSpace->addAction(tr("%1 px").arg(pixels));
        connect(action, &QAction::triggered, this, [this, pixels] {
            if (auto *editor = textEditor()) {
                QTextCursor cursor = editor->textCursor();
                QTextBlockFormat block = cursor.blockFormat();
                block.setBottomMargin(pixels);
                cursor.mergeBlockFormat(block);
            }
        });
    }
    auto *table = insert->addAction(tr("Tabella..."));
    table->setObjectName("impInsertTableAction");
    connect(table, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        bool accepted = false;
        const int rows = QInputDialog::getInt(this, tr("Inserisci tabella"), tr("Righe:"),
                                              3, 1, 100, 1, &accepted);
        if (!accepted) return;
        const int columns = QInputDialog::getInt(this, tr("Inserisci tabella"), tr("Colonne:"),
                                                 3, 1, 30, 1, &accepted);
        if (!accepted) return;
        QTextTableFormat f; f.setBorder(1); f.setCellPadding(5); f.setCellSpacing(0);
        f.setWidth(QTextLength(QTextLength::PercentageLength, 100));
        editor->textCursor().insertTable(rows, columns, f);
    });
    auto *chart = insert->addAction(tr("Grafico..."));
    chart->setObjectName("impInsertChartAction");
    connect(chart, &QAction::triggered, this, &SuiteWindow::insertImpaginationChart);
    auto *tableMenu = tools->addMenu(tr("Tabella"));
    tableMenu->setObjectName("impTableMenu");
    tableMenu->addAction(table);
    tableMenu->addSeparator();
    for (const auto &entry : {qMakePair(tr("Aggiungi riga"), 0),
                              qMakePair(tr("Aggiungi colonna"), 1),
                              qMakePair(tr("Elimina riga"), 2),
                              qMakePair(tr("Elimina colonna"), 3),
                              qMakePair(tr("Unisci celle selezionate"), 4),
                              qMakePair(tr("Dividi cella"), 5)}) {
        auto *action = tableMenu->addAction(entry.first);
        connect(action, &QAction::triggered, this, [this, operation = entry.second] {
            auto *editor = textEditor(); if (!editor) return;
            QTextCursor cursor = editor->textCursor();
            QTextTable *table = cursor.currentTable(); if (!table) return;
            const QTextTableCell cell = table->cellAt(cursor);
            if (operation == 0) table->insertRows(cell.row() + 1, 1);
            else if (operation == 1) table->insertColumns(cell.column() + 1, 1);
            else if (operation == 2 && table->rows() > 1) table->removeRows(cell.row(), 1);
            else if (operation == 3 && table->columns() > 1) table->removeColumns(cell.column(), 1);
            else if (operation == 4) table->mergeCells(cursor);
            else if (operation == 5) table->splitCell(cell.row(), cell.column(), 1, 1);
        });
    }
    auto *cellColor = tableMenu->addAction(tr("Colore cella..."));
    cellColor->setObjectName("impCellColorAction");
    connect(cellColor, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        QTextCursor cursor = editor->textCursor();
        QTextTable *table = cursor.currentTable(); if (!table) return;
        const QColor chosen = QColorDialog::getColor(QColor("#eaf4ff"), this, tr("Colore cella"));
        if (!chosen.isValid()) return;
        QTextTableCell cell = table->cellAt(cursor);
        QTextTableCellFormat cellFormat = cell.format().toTableCellFormat();
        cellFormat.setBackground(chosen);
        cell.setFormat(cellFormat);
    });
    auto *tableBorders = tableMenu->addAction(tr("Bordi tabella..."));
    tableBorders->setObjectName("impTableBorderAction");
    connect(tableBorders, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        QTextTable *table = editor->textCursor().currentTable(); if (!table) return;
        bool accepted = false;
        const int width = QInputDialog::getInt(this, tr("Bordi tabella"), tr("Spessore in pixel:"),
            qRound(table->format().border()), 0, 10, 1, &accepted);
        if (!accepted) return;
        QTextTableFormat format = table->format();
        format.setBorder(width);
        table->setFormat(format);
    });
    auto *caseMenu = format->addMenu(tr("Cambia maiuscole/minuscole"));
    for (const auto &entry : {qMakePair(tr("TUTTO MAIUSCOLO"), 0),
                              qMakePair(tr("tutto minuscolo"), 1),
                              qMakePair(tr("Iniziali Maiuscole"), 2)}) {
        auto *action = caseMenu->addAction(entry.first);
        connect(action, &QAction::triggered, this, [this, operation = entry.second] {
            auto *editor = textEditor(); if (!editor) return;
            QTextCursor cursor = editor->textCursor();
            if (!cursor.hasSelection()) return;
            QString text = cursor.selectedText();
            if (operation == 0) text = text.toUpper();
            else if (operation == 1) text = text.toLower();
            else {
                bool startWord = true;
                for (int index = 0; index < text.size(); ++index) {
                    QChar character = text[index];
                    if (character.isLetter()) {
                        text[index] = startWord ? character.toUpper() : character.toLower();
                        startWord = false;
                    } else startWord = !character.isNumber();
                }
            }
            cursor.insertText(text);
        });
    }
    auto *direction = format->addMenu(tr("Direzione del paragrafo"));
    for (const auto &entry : {qMakePair(tr("Da sinistra a destra"), Qt::LeftToRight),
                              qMakePair(tr("Da destra a sinistra"), Qt::RightToLeft)}) {
        auto *action = direction->addAction(entry.first);
        connect(action, &QAction::triggered, this, [this, layoutDirection = entry.second] {
            if (auto *editor = textEditor()) {
                QTextCursor cursor = editor->textCursor();
                QTextBlockFormat block = cursor.blockFormat();
                block.setLayoutDirection(layoutDirection);
                cursor.mergeBlockFormat(block);
            }
        });
    }
    auto *picture = insert->addAction(tr("Immagine..."));
    picture->setObjectName("impInsertImageAction");
    connect(picture, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        const QString path = QFileDialog::getOpenFileName(this, tr("Inserisci immagine"), {},
            tr("Immagini (*.png *.jpg *.jpeg *.bmp *.gif *.webp)"));
        if (path.isEmpty()) return;
        QImageReader reader(path);
        const QImage image = reader.read();
        if (image.isNull()) { QMessageBox::warning(this, tr("Immagine"), reader.errorString()); return; }
        editor->textCursor().insertImage(image, QFileInfo(path).fileName());
    });
    auto *videoMenu = insert->addMenu(tr("Video"));
    videoMenu->setObjectName("impInsertVideoMenu");
    auto *videoFile = videoMenu->addAction(tr("Dal PC..."));
    videoFile->setObjectName("impInsertLocalVideoAction");
    connect(videoFile, &QAction::triggered, this, [this] {
        const QString path = QFileDialog::getOpenFileName(this, tr("Inserisci video"), {},
            tr("Video (*.mp4 *.mov *.m4v *.webm *.avi *.mkv);;Tutti i file (*)"));
        if (!path.isEmpty()) insertImpaginationVideo(path, false);
    });
    auto *videoYoutube = videoMenu->addAction(tr("Da YouTube..."));
    videoYoutube->setObjectName("impInsertYouTubeVideoAction");
    connect(videoYoutube, &QAction::triggered, this, [this] {
        bool accepted = false;
        const QString url = QInputDialog::getText(this, tr("Video YouTube"),
            tr("Indirizzo del video:"), QLineEdit::Normal, {}, &accepted);
        if (accepted && !url.trimmed().isEmpty()) insertImpaginationVideo(url, true);
    });
    auto *imageTools = tools->addMenu(tr("Immagine"));
    auto *editImage = imageTools->addAction(tr("Ridimensiona e ruota..."));
    editImage->setObjectName("impEditImageAction");
    connect(editImage, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        editImpaginationImage(selectedImageAt(editor->textCursor()));
    });
    auto *cropImage = imageTools->addAction(tr("Ritaglia..."));
    cropImage->setObjectName("impCropImageMenuAction");
    connect(cropImage, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        editImpaginationImage(selectedImageAt(editor->textCursor()), true);
    });
    auto *wrapMenu = imageTools->addMenu(tr("Contorno testo"));
    for (const auto &entry : {qMakePair(tr("In linea con il testo"), 0),
                              qMakePair(tr("Immagine a sinistra, testo a destra"), 1),
                              qMakePair(tr("Immagine a destra, testo a sinistra"), 2)}) {
        auto *action = wrapMenu->addAction(entry.first);
        action->setObjectName(entry.second == 1 ? "impWrapLeftAction"
            : entry.second == 2 ? "impWrapRightAction" : "impWrapInlineAction");
        connect(action, &QAction::triggered, this, [this, mode = entry.second] {
            auto *editor = textEditor(); if (!editor) return;
            setImpaginationImageWrap(selectedImageAt(editor->textCursor()), mode);
        });
    }
    auto *link = insert->addAction(tr("Collegamento..."));
    link->setObjectName("impInsertLinkAction");
    connect(link, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        bool accepted = false;
        const QString url = QInputDialog::getText(this, tr("Collegamento"), tr("Indirizzo URL:"),
            QLineEdit::Normal, QStringLiteral("https://"), &accepted);
        if (!accepted || url.isEmpty()) return;
        QTextCursor cursor = editor->textCursor();
        QTextCharFormat f; f.setAnchor(true); f.setAnchorHref(url);
        f.setForeground(QColor("#1768b2")); f.setFontUnderline(true);
        if (cursor.hasSelection()) cursor.mergeCharFormat(f);
        else cursor.insertText(url, f);
    });
    insert->addAction(tr("Linea orizzontale"), this, [this] {
        if (auto *editor = textEditor()) editor->textCursor().insertHtml("<hr/>");
    });
    insert->addAction(tr("Data e ora"), this, [this] {
        if (auto *editor = textEditor())
            editor->textCursor().insertText(QDateTime::currentDateTime().toString("dd/MM/yyyy HH:mm"));
    });
    auto *symbols = insert->addMenu(tr("Simboli"));
    for (const QString &symbol : {QStringLiteral("©"), QStringLiteral("®"), QStringLiteral("™"),
                                   QStringLiteral("€"), QStringLiteral("£"), QStringLiteral("°"),
                                   QStringLiteral("±"), QStringLiteral("×"), QStringLiteral("÷"),
                                   QStringLiteral("→"), QStringLiteral("✓")})
        symbols->addAction(symbol, this, [this, symbol] {
            if (auto *editor = textEditor()) editor->textCursor().insertText(symbol);
        });
    auto *pageBreak = insert->addAction(tr("Interruzione di pagina"));
    pageBreak->setObjectName("impPageBreakAction");
    connect(pageBreak, &QAction::triggered, this, [this] {
        if (auto *editor = textEditor()) {
            QTextCursor cursor = editor->textCursor();
            cursor.insertBlock();
            QTextBlockFormat block = cursor.blockFormat();
            block.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
            cursor.setBlockFormat(block);
        }
    });
    auto *references = menuBar()->addMenu(tr("&Riferimenti"));
    references->setObjectName("impReferencesMenu");
    auto *toc = references->addAction(tr("Inserisci indice dei titoli"));
    toc->setObjectName("impTableOfContentsAction");
    connect(toc, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        struct Entry { int level; QString title; QString anchor; };
        QVector<Entry> entries;
        QTextDocument *document = editor->document();
        for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
            const int level = block.blockFormat().headingLevel();
            if (level < 1 || level > 3 || block.text().trimmed().isEmpty()) continue;
            QString anchor;
            for (auto fragment = block.begin(); !fragment.atEnd() && anchor.isEmpty(); ++fragment)
                for (const QString &name : fragment.fragment().charFormat().anchorNames())
                    if (name.startsWith("nebi-heading-")) { anchor = name; break; }
            if (anchor.isEmpty()) {
                anchor = "nebi-heading-" + QUuid::createUuid().toString(QUuid::WithoutBraces);
                QTextCursor heading(block);
                heading.select(QTextCursor::BlockUnderCursor);
                QTextCharFormat mark; mark.setAnchor(true); mark.setAnchorNames({anchor});
                heading.mergeCharFormat(mark);
            }
            entries.append({level, block.text().trimmed(), anchor});
        }
        if (entries.isEmpty()) {
            statusBar()->showMessage(tr("Applica Titolo 1, 2 o 3 prima di creare l'indice"), 4000);
            return;
        }
        QTextCursor cursor = editor->textCursor();
        cursor.beginEditBlock();
        cursor.insertBlock();
        QTextCharFormat title; title.setFontWeight(QFont::Bold); title.setFontPointSize(16);
        cursor.insertText(tr("Indice"), title);
        for (const Entry &entry : entries) {
            cursor.insertBlock();
            QTextBlockFormat block; block.setIndent(entry.level - 1);
            cursor.setBlockFormat(block);
            QTextCharFormat link; link.setAnchor(true); link.setAnchorHref("#" + entry.anchor);
            link.setForeground(QColor("#1768b2")); link.setFontUnderline(true);
            cursor.insertText(entry.title, link);
        }
        cursor.insertBlock();
        cursor.endEditBlock();
        editor->setTextCursor(cursor);
    });
    auto *refreshToc = references->addAction(tr("Aggiorna indice dei titoli"));
    refreshToc->setObjectName("impRefreshTableOfContentsAction");
    connect(refreshToc, &QAction::triggered, this, [this, toc] {
        auto *editor = textEditor(); if (!editor) return;
        QTextDocument *document = editor->document();
        for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
            if (block.text().trimmed() != tr("Indice")) continue;
            QTextBlock last = block;
            for (QTextBlock entry = block.next(); entry.isValid(); entry = entry.next()) {
                bool tocLink = false;
                for (auto fragment = entry.begin(); !fragment.atEnd(); ++fragment)
                    if (fragment.fragment().charFormat().anchorHref().startsWith("#nebi-heading-"))
                        tocLink = true;
                if (!tocLink) break;
                last = entry;
            }
            if (last == block) continue;
            QTextCursor cursor(document);
            cursor.setPosition(block.position());
            cursor.setPosition(last.position() + last.length(), QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            editor->setTextCursor(cursor);
            toc->trigger();
            return;
        }
        toc->trigger();
    });
    auto *bookmark = references->addAction(tr("Inserisci segnalibro..."));
    bookmark->setObjectName("impInsertBookmarkAction");
    connect(bookmark, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        bool accepted = false;
        QString name = QInputDialog::getText(this, tr("Segnalibro"), tr("Nome:"),
            QLineEdit::Normal, {}, &accepted).trimmed();
        if (!accepted || name.isEmpty()) return;
        name.replace(QRegularExpression("[^A-Za-z0-9_-]"), "_");
        if (name.isEmpty()) return;
        for (const auto &entry : documentBookmarks(editor->document()))
            if (entry.first == name) {
                QMessageBox::information(this, tr("Segnalibro"), tr("Esiste già un segnalibro con questo nome."));
                return;
            }
        QTextCursor cursor = editor->textCursor();
        QTextCharFormat mark; mark.setAnchor(true); mark.setAnchorNames({name});
        if (cursor.hasSelection()) cursor.mergeCharFormat(mark);
        else cursor.insertText(QString(QChar(0x200B)), mark);
    });
    auto *goToBookmark = references->addMenu(tr("Vai al segnalibro"));
    goToBookmark->setObjectName("impGoToBookmarkMenu");
    connect(goToBookmark, &QMenu::aboutToShow, this, [this, goToBookmark] {
        goToBookmark->clear();
        auto *editor = textEditor(); if (!editor) return;
        for (const auto &entry : documentBookmarks(editor->document())) {
            if (entry.first.startsWith("nebi-heading-") || entry.first.startsWith("nebi-note-")) continue;
            goToBookmark->addAction(entry.first, this, [editor, position = entry.second] {
                QTextCursor cursor(editor->document());
                cursor.setPosition(position);
                editor->setTextCursor(cursor);
                editor->ensureCursorVisible();
                editor->setFocus();
            });
        }
        if (goToBookmark->actions().isEmpty()) {
            QAction *empty = goToBookmark->addAction(tr("Nessun segnalibro"));
            empty->setEnabled(false);
        }
    });
    auto *endnote = references->addAction(tr("Inserisci nota finale..."));
    endnote->setObjectName("impInsertEndnoteAction");
    connect(endnote, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        bool accepted = false;
        const QString note = QInputDialog::getMultiLineText(this, tr("Nota finale"), tr("Testo della nota:"),
            {}, &accepted).trimmed();
        if (!accepted || note.isEmpty()) return;
        QTextDocument *document = editor->document();
        const int number = document->property("endnoteCount").toInt() + 1;
        document->setProperty("endnoteCount", number);
        const QString anchor = "nebi-note-" + QString::number(number);
        QTextCursor body = editor->textCursor();
        body.beginEditBlock();
        QTextCharFormat reference; reference.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
        reference.setAnchor(true); reference.setAnchorHref("#" + anchor);
        reference.setForeground(QColor("#1768b2"));
        body.insertText(QString::number(number), reference);
        QTextCursor tail(document);
        tail.movePosition(QTextCursor::End);
        tail.insertBlock();
        if (number == 1) {
            QTextCharFormat heading; heading.setFontWeight(QFont::Bold);
            tail.insertText(tr("Note finali"), heading);
            tail.insertBlock();
        }
        QTextCharFormat target; target.setAnchor(true); target.setAnchorNames({anchor});
        tail.insertText(QString("[%1] ").arg(number), target);
        tail.insertText(note);
        body.endEditBlock();
        editor->setTextCursor(body);
    });
    auto *find = edit->addAction(tr("Trova..."), QKeySequence::Find);
    find->setObjectName("impFindAction");
    connect(find, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        bool accepted = false;
        const QString query = QInputDialog::getText(this, tr("Trova"), tr("Testo da cercare:"),
            QLineEdit::Normal, editor->textCursor().selectedText(), &accepted);
        if (!accepted || query.isEmpty()) return;
        editor->setProperty("findQuery", query);
        QTextCursor hit = editor->document()->find(query, editor->textCursor());
        if (hit.isNull()) hit = editor->document()->find(query, 0);
        if (!hit.isNull()) { editor->setTextCursor(hit); editor->ensureCursorVisible(); }
        else statusBar()->showMessage(tr("Testo non trovato"), 3000);
    });
    auto *findNext = edit->addAction(tr("Trova successivo"), QKeySequence(Qt::Key_F3));
    findNext->setObjectName("impFindNextAction");
    auto *findPrevious = edit->addAction(tr("Trova precedente"), QKeySequence(Qt::SHIFT | Qt::Key_F3));
    findPrevious->setObjectName("impFindPreviousAction");
    for (const auto &entry : {qMakePair(findNext, false), qMakePair(findPrevious, true)})
        connect(entry.first, &QAction::triggered, this, [this, backward = entry.second] {
            auto *editor = textEditor(); if (!editor) return;
            const QString query = editor->property("findQuery").toString();
            if (query.isEmpty()) return;
            const auto flags = backward ? QTextDocument::FindBackward : QTextDocument::FindFlags();
            QTextCursor hit = editor->document()->find(query, editor->textCursor(), flags);
            if (hit.isNull()) hit = editor->document()->find(query,
                backward ? editor->document()->characterCount() - 1 : 0, flags);
            if (!hit.isNull()) { editor->setTextCursor(hit); editor->ensureCursorVisible(); }
        });
    auto *replace = edit->addAction(tr("Sostituisci tutto..."), QKeySequence::Replace);
    replace->setObjectName("impReplaceAction");
    connect(replace, &QAction::triggered, this, [this] {
        auto *editor = textEditor(); if (!editor) return;
        bool accepted = false;
        const QString query = QInputDialog::getText(this, tr("Sostituisci"), tr("Trova:"),
            QLineEdit::Normal, {}, &accepted);
        if (!accepted || query.isEmpty()) return;
        const QString replacement = QInputDialog::getText(this, tr("Sostituisci"),
            tr("Sostituisci con:"), QLineEdit::Normal, {}, &accepted);
        if (!accepted) return;
        int count = 0, position = 0;
        QTextCursor transaction(editor->document());
        transaction.beginEditBlock();
        while (true) {
            QTextCursor hit = editor->document()->find(query, position);
            if (hit.isNull()) break;
            position = hit.selectionStart() + replacement.size();
            hit.insertText(replacement);
            ++count;
        }
        transaction.endEditBlock();
        statusBar()->showMessage(tr("Sostituzioni: %1").arg(count), 4000);
    });
    auto *orientation = layout->addAction(tr("Pagina orizzontale"));
    orientation->setObjectName("impLandscapeAction");
    orientation->setCheckable(true);
    connect(orientation, &QAction::toggled, this, [this](bool horizontal) {
        auto *editor = textEditor(); if (!editor) return;
        QWidget *paper = editor->parentWidget();
        paper->setProperty("landscape", horizontal);
        QSize dimensions = paperSize_->currentText() == "A3" ? QSize(1123, 1587) : QSize(794, 1123);
        if (horizontal) dimensions.transpose();
        paper->setFixedSize(dimensions);
        editor->document()->setModified(true);
    });
    auto *margins = layout->addMenu(tr("Margini"));
    for (const auto &entry : {qMakePair(tr("Stretti"), 40), qMakePair(tr("Normali"), 78),
                              qMakePair(tr("Largi"), 110)}) {
        auto *action = margins->addAction(entry.first);
        connect(action, &QAction::triggered, this, [this, pixels = entry.second] {
            auto *editor = textEditor(); if (!editor) return;
            QWidget *paper = editor->parentWidget();
            paper->layout()->setContentsMargins(pixels, pixels, pixels, pixels);
            paper->setProperty("margin", pixels);
            editor->document()->setModified(true);
        });
    }
    auto *pageElements = layout->addMenu(tr("Intestazione e piè di pagina"));
    for (const auto &entry : {qMakePair(tr("Intestazione"), qMakePair("impHeaderToggle", "headerVisible")),
                              qMakePair(tr("Piè di pagina"), qMakePair("impFooterToggle", "footerVisible")),
                              qMakePair(tr("Numeri di pagina"), qMakePair("impPageNumberToggle", "pageNumbersVisible"))}) {
        auto *action = pageElements->addAction(entry.first);
        action->setObjectName(entry.second.first);
        action->setCheckable(true);
        connect(action, &QAction::toggled, this, [this, property = entry.second.second](bool visible) {
            auto *editor = textEditor(); if (!editor) return;
            QWidget *paper = editor->parentWidget();
            paper->setProperty(property, visible);
            refreshImpaginationPageElements(paper);
            editor->document()->setModified(true);
        });
    }
    auto *exportPdf = file->addAction(tr("Esporta PDF..."));
    exportPdf->setObjectName("impExportPdfAction");
    connect(exportPdf, &QAction::triggered, this, &SuiteWindow::exportCurrentPdf);

    addToolBarBreak();
    auto *insertToolbar = addToolBar(tr("Inserimento"));
    insertToolbar->setObjectName("impInsertToolbar");
    insertToolbar->setMovable(false);
    insertToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    insertToolbar->setIconSize(QSize(24, 24));
    table->setIcon(toolbarIcon("table"));
    chart->setIcon(toolbarIcon("chart"));
    picture->setIcon(toolbarIcon("image"));
    insertToolbar->addAction(table);
    insertToolbar->addAction(chart);
    insertToolbar->addAction(picture);

    auto *layoutToolbar = addToolBar(tr("Layout pagina"));
    layoutToolbar->setObjectName("impLayoutToolbar");
    layoutToolbar->setMovable(false);
    layoutToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    layoutToolbar->setIconSize(QSize(24, 24));
    orientation->setIcon(toolbarIcon("orientation"));
    exportPdf->setIcon(toolbarIcon("pdf"));
    layoutToolbar->addAction(orientation);
    layoutToolbar->addAction(exportPdf);
}

void SuiteWindow::exportCurrentPdf()
{
    const QString source = pathFor(tabs_->currentIndex());
    const QString suggestion = source.isEmpty() ? tr("Documento.pdf")
        : QFileInfo(source).absolutePath() + "/" + QFileInfo(source).completeBaseName() + ".pdf";
    QString path = QFileDialog::getSaveFileName(this, tr("Esporta in PDF"), suggestion,
                                                tr("PDF (*.pdf)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(".pdf", Qt::CaseInsensitive)) path += ".pdf";
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    if (mode_ == SuiteMode::Impagination) {
        printImpagination(&printer);
    } else if (mode_ == SuiteMode::Math) {
        if (auto *editor = textEditor()) editor->print(&printer);
    } else if (auto *scene = diagramScene()) {
        const bool landscape = diagramView()->property("landscape").toBool();
        printer.setPageSize(QPageSize(diagramView()->property("paperFormat").toString() == "A3"
            ? QPageSize::A3 : QPageSize::A4));
        printer.setPageOrientation(landscape ? QPageLayout::Landscape : QPageLayout::Portrait);
        QPainter painter(&printer);
        const QRectF target(printer.pageLayout().paintRectPixels(printer.resolution()));
        const QRectF sourceRect(QPointF(0, 0), orientedPaperDimensions(
            diagramView()->property("paperFormat").toString(), landscape));
        scene->render(&painter, target, sourceRect, Qt::KeepAspectRatio);
    }
    statusBar()->showMessage(tr("PDF esportato: %1").arg(path), 5000);
}

void SuiteWindow::printImpagination(QPrinter *printer) const
{
    auto *editor = textEditor();
    if (!editor || !printer) return;
    QWidget *paper = editor->parentWidget();
    printer->setPageSize(QPageSize(paper->property("paperFormat").toString() == "A3"
        ? QPageSize::A3 : QPageSize::A4));
    printer->setPageOrientation(paper->property("landscape").toBool()
        ? QPageLayout::Landscape : QPageLayout::Portrait);
    const qreal marginMm = paper->property("margin").toDouble() * 25.4 / 96.0;
    printer->setPageMargins(QMarginsF(marginMm, marginMm, marginMm, marginMm),
                            QPageLayout::Millimeter);
    const bool headerVisible = paper->property("headerVisible").toBool();
    const bool footerVisible = paper->property("footerVisible").toBool();
    const bool numbersVisible = paper->property("pageNumbersVisible").toBool();
    const QString headerText = paper->findChild<QLineEdit *>("impHeaderEdit")->text();
    const QString footerText = paper->findChild<QLineEdit *>("impFooterEdit")->text();
    const qreal scale = printer->resolution() / 96.0;
    const QRectF printable = QRectF(printer->pageLayout().paintRectPixels(printer->resolution()));
    const QRectF area(printable.x() / scale, printable.y() / scale,
                      printable.width() / scale, printable.height() / scale);
    const qreal headerHeight = headerVisible ? 28 : 0;
    const qreal footerHeight = footerVisible || numbersVisible ? 28 : 0;
    const QRectF body(area.left(), area.top() + headerHeight,
                      area.width(), qMax(50.0, area.height() - headerHeight - footerHeight));
    QTextDocument document;
    document.setDefaultFont(editor->document()->defaultFont());
    document.setHtml(portableHtml(editor->document()));
    document.setPageSize(body.size());
    const int pages = qMax(1, document.pageCount());
    QPainter painter(printer);
    if (!painter.isActive()) return;
    painter.scale(scale, scale);
    QFont printFont("Arial");
    printFont.setPixelSize(13);
    painter.setFont(printFont);
    painter.setPen(QColor("#526b85"));
    for (int page = 0; page < pages; ++page) {
        if (page > 0) printer->newPage();
        if (headerVisible)
            painter.drawText(QRectF(area.left(), area.top(), area.width(), headerHeight),
                             Qt::AlignLeft | Qt::AlignVCenter, headerText);
        painter.save();
        painter.setClipRect(body);
        painter.translate(body.left(), body.top() - page * body.height());
        document.drawContents(&painter,
            QRectF(0, page * body.height(), body.width(), body.height()));
        painter.restore();
        if (footerVisible || numbersVisible) {
            const QRectF footerArea(area.left(), body.bottom(), area.width(), footerHeight);
            if (footerVisible)
                painter.drawText(QRectF(footerArea.left(), footerArea.top(),
                    footerArea.width() * (numbersVisible ? 0.75 : 1.0), footerArea.height()),
                    Qt::AlignLeft | Qt::AlignVCenter, footerText);
            if (numbersVisible)
                painter.drawText(footerArea, Qt::AlignRight | Qt::AlignVCenter,
                                 tr("Pagina %1").arg(page + 1));
        }
    }
}

void SuiteWindow::editImpaginationImage(const QTextCursor &selection, bool cropImmediately)
{
    auto *editor = textEditor();
    if (!editor || selection.isNull() || !selection.charFormat().isImageFormat()) {
        statusBar()->showMessage(tr("Seleziona un'immagine nel documento"), 3000);
        return;
    }
    const QTextImageFormat format = selection.charFormat().toImageFormat();
    const QString name = format.name();
    const QVariant resource = editor->document()->resource(QTextDocument::ImageResource, QUrl(name));
    QImage image = resource.value<QImage>();
    if (image.isNull() && resource.canConvert<QPixmap>()) image = resource.value<QPixmap>().toImage();
    if (image.isNull() && name.startsWith("data:image/")) {
        const int comma = name.indexOf(',');
        if (comma >= 0) image.loadFromData(QByteArray::fromBase64(name.mid(comma + 1).toLatin1()));
    }
    if (image.isNull()) {
        QImageReader reader(QUrl(name).isLocalFile() ? QUrl(name).toLocalFile() : name);
        image = reader.read();
    }
    if (image.isNull()) {
        QMessageBox::warning(this, tr("Immagine"), tr("Impossibile leggere l'immagine selezionata."));
        return;
    }
    ImageEditDialog dialog(image, this);
    if (format.width() > 0 && format.height() > 0)
        dialog.setImageSize(QSize(qRound(format.width()), qRound(format.height())));
    if (cropImmediately)
        if (auto *button = dialog.findChild<QPushButton *>("impCropImageAction")) button->click();
    if (dialog.exec() != QDialog::Accepted) return;
    QImage edited = dialog.resultImage();
    if (edited.isNull()) return;
    if (QTextFrame *frame = selection.currentFrame();
        frame && frame != editor->document()->rootFrame() &&
        !dynamic_cast<QTextTable *>(frame) &&
        frame->frameFormat().position() != QTextFrameFormat::InFlow) {
        QWidget *paper = editor->parentWidget();
        const int maxWidth = qMax(100, qRound((paper->width() -
            2 * paper->property("margin").toDouble()) * 0.6));
        if (edited.width() > maxWidth)
            edited = edited.scaledToWidth(maxWidth, Qt::SmoothTransformation);
    }
    QTextCursor cursor = selection;
    cursor.beginEditBlock();
    QTextImageFormat replacement = format;
    replacement.setName("nebi-image-" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".png");
    replacement.setWidth(edited.width());
    replacement.setHeight(edited.height());
    editor->document()->addResource(QTextDocument::ImageResource, QUrl(replacement.name()), edited);
    cursor.insertImage(replacement);
    if (QTextFrame *frame = cursor.currentFrame();
        frame && frame != editor->document()->rootFrame() && !dynamic_cast<QTextTable *>(frame)) {
        QTextFrameFormat frameFormat = frame->frameFormat();
        if (frameFormat.position() != QTextFrameFormat::InFlow) {
            frameFormat.setWidth(edited.width());
            frame->setFrameFormat(frameFormat);
        }
    }
    cursor.endEditBlock();
    editor->setTextCursor(cursor);
    editor->setFocus();
}

void SuiteWindow::setImpaginationImageWrap(const QTextCursor &selection, int mode)
{
    auto *editor = textEditor();
    if (!editor || selection.isNull() || !selection.charFormat().isImageFormat()) return;
    const auto placement = mode == 1 ? QTextFrameFormat::FloatLeft
                         : mode == 2 ? QTextFrameFormat::FloatRight
                                     : QTextFrameFormat::InFlow;
    QTextCursor cursor = selection;
    QTextFrame *frame = cursor.currentFrame();
    if (frame == editor->document()->rootFrame() || dynamic_cast<QTextTable *>(frame)) frame = nullptr;
    if (frame) {
        QTextFrameFormat frameFormat = frame->frameFormat();
        frameFormat.setPosition(placement);
        frameFormat.setLeftMargin(mode == 2 ? 12 : 0);
        frameFormat.setRightMargin(mode == 1 ? 12 : 0);
        frame->setFrameFormat(frameFormat);
    } else if (mode != 0) {
        QTextImageFormat image = selection.charFormat().toImageFormat();
        qreal width = image.width();
        qreal height = image.height();
        if (width <= 0 || height <= 0) {
            const QVariant resource = editor->document()->resource(QTextDocument::ImageResource,
                                                                    QUrl(image.name()));
            QImage source = resource.value<QImage>();
            if (source.isNull() && resource.canConvert<QPixmap>())
                source = resource.value<QPixmap>().toImage();
            if (source.isNull()) source = QImage(image.name());
            width = source.isNull() ? 150 : source.width();
            height = source.isNull() ? 100 : source.height();
        }
        QWidget *paper = editor->parentWidget();
        const qreal available = paper->width() - 2 * paper->property("margin").toDouble();
        const qreal maxWidth = qMax(100.0, available * 0.6);
        if (width > maxWidth) { height *= maxWidth / width; width = maxWidth; }
        image.setWidth(width);
        image.setHeight(height);
        image.setProperty(QTextFormat::CssFloat, int(QTextFrameFormat::InFlow));
        cursor.beginEditBlock();
        cursor.insertImage(image);
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        QTextFrameFormat frameFormat;
        frameFormat.setPosition(placement);
        frameFormat.setWidth(width);
        frameFormat.setLeftMargin(mode == 2 ? 12 : 0);
        frameFormat.setRightMargin(mode == 1 ? 12 : 0);
        frame = cursor.insertFrame(frameFormat);
        cursor.endEditBlock();
    }
    if (frame) {
        cursor = QTextCursor(editor->document());
        cursor.setPosition(qMin(frame->lastPosition() + 1,
                                editor->document()->characterCount() - 1));
    }
    editor->setTextCursor(cursor);
    editor->document()->setModified(true);
}

void SuiteWindow::moveImpaginationImage(const QTextCursor &selection,
                                       const QPoint &viewportPosition)
{
    auto *editor = textEditor();
    if (!editor || selection.isNull() || !selection.charFormat().isImageFormat()) return;
    QTextDocument *document = editor->document();
    const QTextImageFormat image = selection.charFormat().toImageFormat();
    const qreal width = image.width() > 0 ? image.width() : 150;
    QTextCursor destination = editor->cursorForPosition(viewportPosition);
    QTextFrame *oldFrame = selection.currentFrame();
    if (oldFrame == document->rootFrame() || dynamic_cast<QTextTable *>(oldFrame)) oldFrame = nullptr;
    if (oldFrame && destination.currentFrame() == oldFrame) {
        destination = QTextCursor(document);
        destination.setPosition(qMin(oldFrame->lastPosition() + 1,
                                     document->characterCount() - 1));
    }
    const qreal lineOffset = qBound(0.0,
        qreal(viewportPosition.y() - editor->cursorRect(destination).top()), 24.0);
    const qreal freeWidth = qMax(0.0, qreal(editor->viewport()->width()) - width - 16);
    const qreal x = qBound(0.0, qreal(viewportPosition.x()) - width / 2, freeWidth);
    const bool onLeft = x + width / 2 < editor->viewport()->width() / 2;
    const auto placement = onLeft ? QTextFrameFormat::FloatLeft : QTextFrameFormat::FloatRight;
    QTextFrameFormat frameFormat;
    frameFormat.setPosition(placement);
    frameFormat.setWidth(width);
    frameFormat.setTopMargin(lineOffset);
    if (onLeft) frameFormat.setLeftMargin(x);
    else frameFormat.setRightMargin(qMax(0.0, qreal(editor->viewport()->width()) - x - width));
    QTextCursor edit = oldFrame ? QTextCursor(document) : selection;
    edit.beginEditBlock();
    if (oldFrame) {
        edit.setPosition(oldFrame->firstPosition() - 1);
        edit.setPosition(oldFrame->lastPosition() + 1, QTextCursor::KeepAnchor);
    }
    edit.removeSelectedText();
    destination.insertImage(image);
    destination.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    QTextFrame *newFrame = destination.insertFrame(frameFormat);
    edit.endEditBlock();
    QTextCursor outside(document);
    outside.setPosition(qMin(newFrame->lastPosition() + 1, document->characterCount() - 1));
    editor->setTextCursor(outside);
    document->setModified(true);
}

void SuiteWindow::insertImpaginationVideo(const QString &source, bool youtube)
{
    auto *editor = textEditor();
    if (!editor || source.isEmpty()) return;
    QString reference;
    QString label;
    if (youtube) {
        const QUrl url = QUrl::fromUserInput(source.trimmed());
        const QString host = url.host().toLower();
        QString id;
        if (host == "youtu.be" || host == "www.youtu.be") id = url.path().mid(1).section('/', 0, 0);
        else if (host == "youtube.com" || host == "www.youtube.com" || host == "m.youtube.com") {
            if (url.path() == "/watch") id = QUrlQuery(url).queryItemValue("v");
            else if (url.path().startsWith("/shorts/") || url.path().startsWith("/embed/"))
                id = url.path().section('/', 2, 2);
        }
        if (!QRegularExpression("^[A-Za-z0-9_-]{11}$").match(id).hasMatch()) {
            QMessageBox::warning(this, tr("Video YouTube"), tr("Inserisci un indirizzo YouTube valido."));
            return;
        }
        reference = "nebi-video:youtube:" + id;
        label = tr("Video YouTube");
    } else {
        if (!QFileInfo::exists(source)) return;
        reference = "nebi-video:local:" + QString::fromLatin1(
            source.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
        label = QFileInfo(source).fileName();
    }
    QImage poster(480, 270, QImage::Format_ARGB32_Premultiplied);
    poster.fill(Qt::transparent);
    QPainter painter(&poster);
    painter.setRenderHint(QPainter::Antialiasing);
    QLinearGradient background(0, 0, 480, 270);
    background.setColorAt(0, QColor("#183752"));
    background.setColorAt(1, QColor("#33427d"));
    painter.setBrush(background);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(poster.rect().adjusted(2, 2, -2, -2), 12, 12);
    painter.setBrush(QColor(255, 255, 255, 225));
    painter.drawEllipse(QPointF(240, 116), 39, 39);
    QPolygonF triangle{QPointF(230, 94), QPointF(230, 138), QPointF(264, 116)};
    painter.setBrush(QColor("#234b7c"));
    painter.drawPolygon(triangle);
    painter.setPen(Qt::white);
    QFont font = painter.font(); font.setPixelSize(20); font.setBold(true); painter.setFont(font);
    painter.drawText(QRect(20, 185, 440, 35), Qt::AlignCenter, painter.fontMetrics().elidedText(label, Qt::ElideMiddle, 430));
    font.setPixelSize(13); font.setBold(false); painter.setFont(font);
    painter.drawText(QRect(20, 225, 440, 24), Qt::AlignCenter, tr("Doppio clic per riprodurre"));
    painter.end();
    QTextImageFormat format;
    format.setName("nebi-video-poster-" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".png");
    format.setWidth(400);
    format.setHeight(225);
    format.setAnchor(true);
    format.setAnchorHref(reference);
    editor->document()->addResource(QTextDocument::ImageResource, QUrl(format.name()), poster);
    QTextCursor cursor = editor->textCursor();
    cursor.insertImage(format);
    editor->setTextCursor(cursor);
}

void SuiteWindow::insertImpaginationChart()
{
    auto *editor = textEditor();
    if (!editor) return;
    QDialog dialog(this);
    dialog.setObjectName("impChartDialog");
    dialog.setWindowTitle(tr("Inserisci grafico"));
    dialog.resize(760, 520);
    auto *dialogLayout = new QVBoxLayout(&dialog);
    auto *settings = new QGridLayout;
    auto *type = new QComboBox(&dialog);
    type->setObjectName("impChartType");
    type->addItems({tr("Colonne"), tr("Barre"), tr("Linee"), tr("Area"), tr("Torta")});
    auto *title = new QLineEdit(tr("Andamento"), &dialog);
    title->setObjectName("impChartTitle");
    auto *theme = new QComboBox(&dialog);
    theme->setObjectName("impChartTheme");
    theme->addItems({tr("Colori NebiPad+"), tr("Blu"), tr("Azzurro"),
                     tr("Contrasto elevato"), tr("Marrone"), tr("Chiaro")});
    auto *legend = new QCheckBox(tr("Mostra legenda"), &dialog);
    legend->setObjectName("impChartLegend");
    legend->setChecked(true);
    settings->addWidget(new QLabel(tr("Tipo"), &dialog), 0, 0);
    settings->addWidget(type, 0, 1);
    settings->addWidget(new QLabel(tr("Titolo"), &dialog), 0, 2);
    settings->addWidget(title, 0, 3);
    settings->addWidget(new QLabel(tr("Tema colori"), &dialog), 1, 0);
    settings->addWidget(theme, 1, 1);
    settings->addWidget(legend, 1, 3);
    dialogLayout->addLayout(settings);
    auto *data = new QTableWidget(4, 4, &dialog);
    data->setObjectName("impChartData");
    data->setHorizontalHeaderLabels({tr("Categoria"), tr("Serie 1"), tr("Serie 2"), tr("Serie 3")});
    const QStringList categories{tr("Gennaio"), tr("Febbraio"), tr("Marzo"), tr("Aprile")};
    const QVector<QVector<double>> values{{12, 18, 15, 24}, {8, 14, 20, 17}, {16, 11, 22, 28}};
    for (int row = 0; row < data->rowCount(); ++row) {
        data->setItem(row, 0, new QTableWidgetItem(categories[row]));
        for (int column = 1; column < data->columnCount(); ++column)
            data->setItem(row, column, new QTableWidgetItem(
                QString::number(values[column - 1][row])));
    }
    data->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    dialogLayout->addWidget(data, 1);
    auto *dataButtons = new QHBoxLayout;
    auto *addCategory = new QPushButton(tr("Aggiungi categoria"), &dialog);
    auto *removeCategory = new QPushButton(tr("Rimuovi categoria"), &dialog);
    auto *addSeries = new QPushButton(tr("Aggiungi serie"), &dialog);
    auto *removeSeries = new QPushButton(tr("Rimuovi serie"), &dialog);
    dataButtons->addWidget(addCategory); dataButtons->addWidget(removeCategory);
    dataButtons->addWidget(addSeries); dataButtons->addWidget(removeSeries);
    dataButtons->addStretch();
    dialogLayout->addLayout(dataButtons);
    connect(addCategory, &QPushButton::clicked, data, [data] {
        const int row = data->rowCount(); data->insertRow(row);
        data->setItem(row, 0, new QTableWidgetItem(QObject::tr("Categoria %1").arg(row + 1)));
        for (int column = 1; column < data->columnCount(); ++column)
            data->setItem(row, column, new QTableWidgetItem("0"));
    });
    connect(removeCategory, &QPushButton::clicked, data, [data] {
        if (data->rowCount() > 1) data->removeRow(data->rowCount() - 1);
    });
    connect(addSeries, &QPushButton::clicked, data, [data] {
        const int column = data->columnCount(); data->insertColumn(column);
        data->setHorizontalHeaderItem(column,
            new QTableWidgetItem(QObject::tr("Serie %1").arg(column)));
        for (int row = 0; row < data->rowCount(); ++row)
            data->setItem(row, column, new QTableWidgetItem("0"));
    });
    connect(removeSeries, &QPushButton::clicked, data, [data] {
        if (data->columnCount() > 2) data->removeColumn(data->columnCount() - 1);
    });
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    dialogLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;

    QStringList categoryNames;
    QVector<QVector<qreal>> seriesValues(data->columnCount() - 1);
    qreal maximum = 0;
    for (int row = 0; row < data->rowCount(); ++row) {
        categoryNames << (data->item(row, 0) ? data->item(row, 0)->text() : tr("Categoria %1").arg(row + 1));
        for (int column = 1; column < data->columnCount(); ++column) {
            bool valid = false;
            const qreal value = data->item(row, column)
                ? data->item(row, column)->text().replace(',', '.').toDouble(&valid) : 0;
            seriesValues[column - 1].append(valid ? value : 0);
            maximum = qMax(maximum, valid ? value : 0);
        }
    }
    auto *chart = new QChart;
    chart->setTitle(title->text().trimmed());
    const QVector<QChart::ChartTheme> themes{QChart::ChartThemeDark,
        QChart::ChartThemeBlueCerulean, QChart::ChartThemeBlueIcy,
        QChart::ChartThemeHighContrast, QChart::ChartThemeBrownSand,
        QChart::ChartThemeLight};
    chart->setTheme(themes.value(theme->currentIndex(), QChart::ChartThemeDark));
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->setBackgroundRoundness(10);
    chart->setMargins(QMargins(18, 14, 18, 14));
    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(18);
    titleFont.setWeight(QFont::DemiBold);
    chart->setTitleFont(titleFont);
    const int chartType = type->currentIndex();
    if (chartType == 0 || chartType == 1) {
        QAbstractBarSeries *barSeries = chartType == 0
            ? static_cast<QAbstractBarSeries *>(new QBarSeries)
            : static_cast<QAbstractBarSeries *>(new QHorizontalBarSeries);
        for (int column = 0; column < seriesValues.size(); ++column) {
            auto *set = new QBarSet(data->horizontalHeaderItem(column + 1)->text());
            for (qreal value : seriesValues[column]) *set << value;
            barSeries->append(set);
        }
        chart->addSeries(barSeries);
        auto *categoryAxis = new QBarCategoryAxis(chart); categoryAxis->append(categoryNames);
        auto *valueAxis = new QValueAxis(chart); valueAxis->setRange(0, qMax(1.0, maximum * 1.15));
        valueAxis->setLabelFormat("%.1f");
        valueAxis->setTickCount(6);
        if (chartType == 0) {
            chart->addAxis(categoryAxis, Qt::AlignBottom);
            chart->addAxis(valueAxis, Qt::AlignLeft);
        } else {
            chart->addAxis(valueAxis, Qt::AlignBottom);
            chart->addAxis(categoryAxis, Qt::AlignLeft);
        }
        barSeries->attachAxis(categoryAxis);
        barSeries->attachAxis(valueAxis);
    } else if (chartType == 4) {
        auto *pie = new QPieSeries;
        for (int row = 0; row < categoryNames.size(); ++row)
            pie->append(categoryNames[row], seriesValues[0][row]);
        pie->setLabelsVisible(true);
        chart->addSeries(pie);
    } else {
        auto *categoryAxis = new QBarCategoryAxis(chart); categoryAxis->append(categoryNames);
        auto *valueAxis = new QValueAxis(chart); valueAxis->setRange(0, qMax(1.0, maximum * 1.15));
        valueAxis->setLabelFormat("%.1f");
        valueAxis->setTickCount(6);
        chart->addAxis(categoryAxis, Qt::AlignBottom);
        chart->addAxis(valueAxis, Qt::AlignLeft);
        for (int column = 0; column < seriesValues.size(); ++column) {
            auto *line = new QLineSeries;
            line->setName(data->horizontalHeaderItem(column + 1)->text());
            for (int row = 0; row < seriesValues[column].size(); ++row)
                line->append(row, seriesValues[column][row]);
            if (chartType == 3) {
                auto *area = new QAreaSeries(line);
                area->setName(line->name());
                area->setOpacity(0.65);
                chart->addSeries(area); area->attachAxis(categoryAxis); area->attachAxis(valueAxis);
            } else {
                line->setPointsVisible(true);
                chart->addSeries(line); line->attachAxis(categoryAxis); line->attachAxis(valueAxis);
            }
        }
    }
    chart->legend()->setVisible(legend->isChecked());
    chart->legend()->setAlignment(Qt::AlignBottom);
    const QWidget *paper = editor->parentWidget();
    const qreal usefulPageWidth = paper->width() - 2 * paper->property("margin").toDouble();
    const qreal usefulPageHeight = paper->height() - 2 * paper->property("margin").toDouble();
    const qreal displayedWidth = qMax(600.0, usefulPageWidth - 8.0);
    const qreal displayedHeight = qMin(usefulPageHeight * 0.50, displayedWidth * 0.68);
    const int renderWidth = qBound(1400, qRound(displayedWidth * 2.5), 2800);
    const int renderHeight = qRound(renderWidth * displayedHeight / displayedWidth);
    QChartView chartView(chart);
    chartView.resize(renderWidth, renderHeight);
    chartView.setRenderHint(QPainter::Antialiasing);
    chartView.ensurePolished();
    chart->resize(renderWidth, renderHeight);
    QImage image(chartView.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter imagePainter(&image);
    imagePainter.setRenderHint(QPainter::Antialiasing);
    // Render the chart scene directly in logical pixels. QWidget::render can
    // apply the monitor DPR twice on Retina displays, leaving chart content at
    // one quarter of the image while the image frame keeps the full size.
    chartView.scene()->render(&imagePainter, QRectF(image.rect()), chart->scene()->sceneRect());
    imagePainter.end();
    QTextImageFormat imageFormat;
    imageFormat.setName("nebi-chart-" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".png");
    imageFormat.setWidth(displayedWidth);
    imageFormat.setHeight(displayedHeight);
    imageFormat.setProperty(QTextFormat::ImageAltText,
                            tr("Grafico: %1").arg(title->text().trimmed()));
    editor->document()->addResource(QTextDocument::ImageResource, QUrl(imageFormat.name()), image);
    editor->textCursor().insertImage(imageFormat);
}

void SuiteWindow::playImpaginationVideo(const QString &reference)
{
    if (reference.startsWith("nebi-video:youtube:")) {
        const QString id = reference.mid(QString("nebi-video:youtube:").size());
        const QUrl videoPage("https://www.youtube.com/watch?v=" + id);
#ifdef NEBIPAD_HAS_WEBENGINE
        QDialog dialog(this);
        dialog.setWindowTitle(tr("Video YouTube"));
        dialog.resize(900, 560);
        auto *layout = new QVBoxLayout(&dialog);
        layout->setContentsMargins(8, 8, 8, 8);
        auto *view = new QWebEngineView(&dialog);
        layout->addWidget(view, 1);
        auto *status = new QLabel(tr("Caricamento di YouTube..."), &dialog);
        auto *retry = new QPushButton(tr("Riprova"), &dialog);
        auto *openBrowser = new QPushButton(tr("Apri su YouTube"), &dialog);
        auto *footer = new QHBoxLayout;
        footer->addWidget(status, 1);
        footer->addWidget(retry);
        footer->addWidget(openBrowser);
        layout->addLayout(footer);
        connect(openBrowser, &QPushButton::clicked, &dialog, [id] {
            QDesktopServices::openUrl(QUrl("https://www.youtube.com/watch?v=" + id));
        });
        connect(view->page(), &QWebEnginePage::loadingChanged, &dialog,
                [view](const QWebEngineLoadingInfo &info) {
            if (info.status() == QWebEngineLoadingInfo::LoadFailedStatus)
                view->setProperty("nebiLoadError", info.errorString());
        });
        connect(retry, &QPushButton::clicked, view, [view, videoPage] {
            view->load(videoPage);
        });
        connect(view, &QWebEngineView::loadFinished, &dialog,
                [view, status](bool loaded) {
            if (loaded)
                status->setText(QObject::tr("YouTube caricato nel lettore integrato."));
            else
                status->setText(QObject::tr("Caricamento non riuscito: %1")
                    .arg(view->property("nebiLoadError").toString()));
        });
        view->load(videoPage);
        dialog.exec();
#else
        QDesktopServices::openUrl(embed);
#endif
        return;
    }
    if (!reference.startsWith("nebi-video:local:")) return;
    const QByteArray encoded = reference.mid(QString("nebi-video:local:").size()).toLatin1();
    QString path = QString::fromUtf8(QByteArray::fromBase64(encoded, QByteArray::Base64UrlEncoding));
    if (QFileInfo(path).isRelative()) {
        const QString documentPath = pathFor(tabs_->currentIndex());
        path = QFileInfo(documentPath).dir().filePath(path);
    }
    if (!QFileInfo::exists(path)) {
        QMessageBox::warning(this, tr("Video"), tr("File video non trovato: %1").arg(path));
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle(QFileInfo(path).fileName());
    dialog.resize(900, 560);
    auto *layout = new QVBoxLayout(&dialog);
    auto *video = new QVideoWidget(&dialog);
    layout->addWidget(video, 1);
    QMediaPlayer player(&dialog);
    QAudioOutput audio(&dialog);
    player.setAudioOutput(&audio);
    player.setVideoOutput(video);
    player.setSource(QUrl::fromLocalFile(path));
    auto *controls = new QHBoxLayout;
    auto *play = new QPushButton(tr("Riproduci / Pausa"), &dialog);
    auto *stop = new QPushButton(tr("Stop"), &dialog);
    auto *volume = new QSlider(Qt::Horizontal, &dialog);
    volume->setRange(0, 100); volume->setValue(80); volume->setFixedWidth(120);
    audio.setVolume(0.8);
    controls->addWidget(play); controls->addWidget(stop);
    controls->addStretch(); controls->addWidget(new QLabel(tr("Volume"), &dialog));
    controls->addWidget(volume);
    layout->addLayout(controls);
    connect(play, &QPushButton::clicked, &dialog, [&player] {
        if (player.playbackState() == QMediaPlayer::PlayingState) player.pause();
        else player.play();
    });
    connect(stop, &QPushButton::clicked, &player, &QMediaPlayer::stop);
    connect(volume, &QSlider::valueChanged, &dialog, [&audio](int value) {
        audio.setVolume(value / 100.0);
    });
    player.play();
    dialog.exec();
    player.stop();
}

void SuiteWindow::newDocument()
{
    auto *container = new QWidget(tabs_);
    container->setStyleSheet("background: #e9eef5;");
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    if (mode_ == SuiteMode::MindNet) {
        auto *scene = new QGraphicsScene(container);
        scene->setBackgroundBrush(QColor("#e9eef5"));
        const QSizeF paper = orientedPaperDimensions(paperSize_->currentText(),
            orientationAction_ && orientationAction_->isChecked());
        scene->addItem(new PageItem(QRectF(QPointF(0, 0), paper)));
        for (const QString &role : {QStringLiteral("header"), QStringLiteral("footer")}) {
            auto *text = new EditableBoxText;
            scene->addItem(text);
            text->setData(0, role);
            text->setDefaultTextColor(QColor("#536b84"));
            text->setFont(QFont("Arial", 11));
            text->setTextWidth(paper.width() - 120);
            text->setPos(60, role == "header" ? 28 : paper.height() - 54);
            text->setTextInteractionFlags(Qt::TextEditorInteraction);
            text->setZValue(2);
        }
        auto *view = new DiagramView(scene, container);
        view->setObjectName("diagramView");
        view->setRenderHint(QPainter::Antialiasing);
        view->setDragMode(QGraphicsView::RubberBandDrag);
        view->setRulerSettings(rulerUnit_->currentText(), rulerStep_->value());
        view->setGridSettings(gridUnit_->currentText(), gridStep_->value());
        view->setRulersVisible(rulersAction_->isChecked());
        view->setGridVisible(gridAction_->isChecked());
        configureDiagramPage(view, paperSize_->currentText(), orientationAction_->isChecked());
        QTimer::singleShot(0, view, [view] {
            configureDiagramPage(view, view->property("paperFormat").toString(),
                                 view->property("landscape").toBool());
        });
        layout->addWidget(view);
        connect(scene, &QGraphicsScene::changed, this, [this, container] {
            if (container->property("loading").toBool()) return;
            container->setProperty("modified", true);
            updateTitle(tabs_->indexOf(container));
        });
        connect(scene, &QGraphicsScene::selectionChanged, this,
                &SuiteWindow::updateMindTextControls);
    } else {
        auto *scroll = new QScrollArea(container);
        scroll->setWidgetResizable(false);
        scroll->setAlignment(Qt::AlignHCenter);
        scroll->setStyleSheet("QScrollArea { border: 0; background: #e9eef5; }");
        auto *paper = new QWidget;
        paper->setObjectName("pagePaper");
        paper->setProperty("paperFormat", paperSize_->currentText());
        paper->setProperty("landscape", false);
        paper->setProperty("margin", 78);
        paper->setProperty("headerVisible", false);
        paper->setProperty("footerVisible", false);
        paper->setProperty("pageNumbersVisible", false);
        paper->setFixedSize(paperSize_->currentText() == "A3" ? QSize(1123, 1587) : QSize(794, 1123));
        paper->setStyleSheet("background: white; border: 1px solid #cad5e2;");
        auto *paperLayout = new QVBoxLayout(paper);
        paperLayout->setContentsMargins(78, 72, 78, 72);
        if (mode_ == SuiteMode::Impagination) {
            auto *header = new QLineEdit(paper);
            header->setObjectName("impHeaderEdit");
            header->setPlaceholderText(tr("Scrivi l'intestazione"));
            header->setStyleSheet("QLineEdit { border: 0; border-bottom: 1px solid #c7d6e6; background: transparent; color: #526b85; padding: 5px 2px; }");
            paperLayout->addWidget(header);
        }
        auto *editor = new PageEdit(paper);
        editor->document()->setProperty("endnoteCount", 0);
        if (mode_ == SuiteMode::Impagination)
            editor->editImage = [this](const QTextCursor &cursor, bool crop) {
                editImpaginationImage(cursor, crop);
            };
        if (mode_ == SuiteMode::Impagination)
            editor->wrapImage = [this](const QTextCursor &cursor, int mode) {
                setImpaginationImageWrap(cursor, mode);
            };
        if (mode_ == SuiteMode::Impagination)
            editor->moveImage = [this](const QTextCursor &cursor, const QPoint &position) {
                moveImpaginationImage(cursor, position);
            };
        if (mode_ == SuiteMode::Impagination)
            editor->playVideo = [this](const QString &reference) {
                playImpaginationVideo(reference);
            };
        if (mode_ == SuiteMode::Impagination)
            editor->playAudio = [this, editor](const QString &target) {
                int index = -1;
                for (int i = 0; i < tabs_->count(); ++i)
                    if (editorAt(tabs_, i) == editor) { index = i; break; }
                const QString document = pathFor(index);
                QString error;
                recording_->playPause(resolveAudioTarget(document, target), &error);
                if (!error.isEmpty()) statusBar()->showMessage(error, 6000);
            };
        editor->setObjectName("pageEditor");
        editor->setFrameShape(QFrame::NoFrame);
        editor->setStyleSheet("background: white; border: 0; color: #172b47;");
        editor->setFont(QFont("Arial", 12));
        editor->setAcceptRichText(true);
        paperLayout->addWidget(editor);
        if (mode_ == SuiteMode::Impagination) {
            auto *footerRow = new QWidget(paper);
            footerRow->setObjectName("impFooterRow");
            auto *footerLayout = new QHBoxLayout(footerRow);
            footerLayout->setContentsMargins(0, 0, 0, 0);
            auto *footer = new QLineEdit(footerRow);
            footer->setObjectName("impFooterEdit");
            footer->setPlaceholderText(tr("Scrivi il piè di pagina"));
            footer->setStyleSheet("QLineEdit { border: 0; border-top: 1px solid #c7d6e6; background: transparent; color: #526b85; padding: 5px 2px; }");
            footerLayout->addWidget(footer, 1);
            auto *number = new QLabel(tr("Pagina 1"), footerRow);
            number->setObjectName("impPageNumberLabel");
            number->setStyleSheet("color: #526b85; padding: 5px 2px;");
            footerLayout->addWidget(number);
            paperLayout->addWidget(footerRow);
            refreshImpaginationPageElements(paper);
            for (QLineEdit *field : {paper->findChild<QLineEdit *>("impHeaderEdit"), footer})
                connect(field, &QLineEdit::textChanged, editor->document(),
                        [document = editor->document()] { document->setModified(true); });
        }
        scroll->setWidget(paper);
        layout->addWidget(scroll);
        connect(editor->document(), &QTextDocument::modificationChanged, this,
                [this, container] { updateTitle(tabs_->indexOf(container)); });
        connect(editor, &QTextEdit::textChanged, this, &SuiteWindow::updateStatus);
        connect(editor, &QTextEdit::cursorPositionChanged, this, &SuiteWindow::updateStatus);
    }
    container->setProperty("filePath", QString());
    container->setProperty("modified", false);
    container->setProperty("tabPinned", false);
    container->setProperty("tabReadOnly", false);
    int index = tabs_->addTab(container, tr("Senza titolo"));
    if (mode_ == SuiteMode::Impagination) installImpaginationTabControls(index);
    tabs_->setCurrentIndex(index);
    updateStatus();
}

QString SuiteWindow::pathFor(int index) const
{
    QWidget *tab = tabs_->widget(index);
    return tab ? tab->property("filePath").toString() : QString();
}

void SuiteWindow::updateTitle(int index)
{
    if (index < 0) return;
    const QString path = pathFor(index);
    const bool modified = editorAt(tabs_, index)
        ? editorAt(tabs_, index)->document()->isModified()
        : tabs_->widget(index)->property("modified").toBool();
    tabs_->setTabText(index, (path.isEmpty() ? tr("Senza titolo") : QFileInfo(path).fileName())
                          + (modified ? " *" : ""));
    if (mode_ == SuiteMode::Impagination) updateImpaginationTabControls(index);
}

bool SuiteWindow::openFile(const QString &path)
{
#ifndef NEBIPAD_HAS_QTPDF
    if (mode_ == SuiteMode::Impagination && path.endsWith(".pdf", Qt::CaseInsensitive)) {
        QMessageBox::warning(this, tr("PDF non disponibile"),
            tr("Questa compilazione di NebiPad+ non include il modulo Qt PDF."));
        return false;
    }
#else
    if (mode_ == SuiteMode::Impagination && path.endsWith(".pdf", Qt::CaseInsensitive)) {
        QPdfDocument pdf(this);
        const QPdfDocument::Error error = pdf.load(path);
        if (error != QPdfDocument::Error::None || pdf.pageCount() < 1) {
            QMessageBox::warning(this, tr("Apertura PDF fallita"),
                error == QPdfDocument::Error::IncorrectPassword
                    ? tr("Il PDF è protetto da password.")
                    : tr("Il PDF non è leggibile o è danneggiato."));
            return false;
        }
        newDocument();
        QTextEdit *editor = textEditor();
        QTextCursor cursor(editor->document());
        cursor.movePosition(QTextCursor::Start);
        const qreal displayWidth = 620.0;
        for (int page = 0; page < pdf.pageCount(); ++page) {
            const QSizeF points = pdf.pagePointSize(page);
            if (points.isEmpty()) continue;
            QSize renderSize = (points * 2.0).toSize();
            if (renderSize.width() > 2400 || renderSize.height() > 2400)
                renderSize.scale(2400, 2400, Qt::KeepAspectRatio);
            const QImage image = pdf.render(page, renderSize);
            if (image.isNull()) continue;
            const QUrl resource(QStringLiteral("nebipad-pdf-page://%1/%2")
                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)).arg(page + 1));
            editor->document()->addResource(QTextDocument::ImageResource, resource, image);
            QTextImageFormat format;
            format.setName(resource.toString());
            format.setWidth(displayWidth);
            format.setHeight(displayWidth * points.height() / points.width());
            cursor.insertImage(format);
            if (page + 1 < pdf.pageCount()) {
                cursor.insertBlock();
                QTextBlockFormat block = cursor.blockFormat();
                block.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
                cursor.setBlockFormat(block);
            }
        }
        editor->document()->setModified(false);
        QWidget *tab = tabs_->currentWidget();
        tab->setProperty("filePath", path);
        tab->setProperty("importedPdf", true);
        tab->setProperty("modified", false);
        updateTitle(tabs_->currentIndex());
        updateStatus();
        rememberRecentFile(path, QStringLiteral("Impagination"));
        return true;
    }
#endif
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Apertura fallita"), file.errorString());
        return false;
    }
    const QByteArray bytes = file.readAll();
    if (mode_ == SuiteMode::MindNet) {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            QMessageBox::warning(this, tr("Formato non valido"), tr("Il diagramma deve essere un file MindNet JSON."));
            return false;
        }
        newDocument();
        auto *scene = diagramScene();
        tabs_->currentWidget()->setProperty("loading", true);
        const QString savedPaper = document.object().value("paper").toString();
        const QString format = savedPaper == "A3" ? "A3" : "A4";
        auto *loadedView = dynamic_cast<DiagramView *>(diagramView());
        configureDiagramPage(loadedView, format,
                             document.object().value("landscape").toBool());
        const QString rulerUnit = document.object().value("rulerUnit").toString("mm");
        const QString gridUnit = document.object().value("gridUnit").toString("mm");
        loadedView->setRulerSettings(rulerUnit,
            document.object().value("rulerStep").toDouble(10));
        loadedView->setGridSettings(gridUnit,
            document.object().value("gridStep").toDouble(10));
        loadedView->setRulersVisible(document.object().value("rulersVisible").toBool());
        loadedView->setGridVisible(document.object().value("gridVisible").toBool());
        syncMindViewControls();
        for (QGraphicsItem *item : scene->items()) {
            const QString role = item->data(0).toString();
            if (role == "header" || role == "footer")
                if (auto *text = dynamic_cast<QGraphicsTextItem *>(item))
                    text->setPlainText(document.object().value(role).toString());
        }
        for (const QJsonValue &value : document.object().value("nodes").toArray()) {
            const QJsonObject node = value.toObject();
            auto *item = new ShapeItem(node.value("shape").toString(),
                QColor(node.value("color").toString("#e7f4ff")),
                node.value("width").toDouble(160), node.value("height").toDouble(70));
            scene->addItem(item);
            item->setPos(node.value("x").toDouble(), node.value("y").toDouble());
            item->setRotation(node.value("rotation").toDouble());
            addNodeLabel(item, node.value("label").toString(), node);
        }
        for (const QJsonValue &value : document.object().value("textBoxes").toArray()) {
            const QJsonObject saved = value.toObject();
            auto *box = new TextBoxItem(saved.value("width").toDouble(250),
                                        saved.value("height").toDouble(105));
            scene->addItem(box);
            box->setPos(saved.value("x").toDouble(), saved.value("y").toDouble());
            box->text()->setHtml(saved.value("html").toString());
            box->text()->setDefaultTextColor(QColor(saved.value("color").toString("#172b47")));
            box->setRotation(saved.value("rotation").toDouble());
        }
        for (const QJsonValue &value : document.object().value("connections").toArray()) {
            const QJsonObject line = value.toObject();
            QVector<QPointF> points;
            for (const QJsonValue &value : line.value("points").toArray()) {
                const QJsonArray pair = value.toArray();
                if (pair.size() == 2) points.append(QPointF(pair[0].toDouble(), pair[1].toDouble()));
            }
            if (points.size() < 2) points = {
                QPointF(line.value("x1").toDouble(), line.value("y1").toDouble()),
                QPointF(line.value("x2").toDouble(), line.value("y2").toDouble())};
            scene->addItem(new ConnectorItem(points,
                line.value("type").toInt(), QColor(line.value("color").toString("#4a74a0")),
                line.value("width").toInt(2)));
        }
        QTimer::singleShot(0, tabs_->currentWidget(), [page = tabs_->currentWidget()] {
            page->setProperty("loading", false);
            page->setProperty("modified", false);
        });
    } else {
        newDocument();
        auto *editor = textEditor();
        QJsonObject pageLayout;
        if (mode_ == SuiteMode::Impagination && bytes.startsWith("<!--NEBIPAD_LAYOUT:")) {
            const int end = bytes.indexOf("-->");
            if (end > 19) pageLayout = QJsonDocument::fromJson(bytes.mid(19, end - 19)).object();
        }
        if (suiteRichDocument(path))
            editor->setHtml(QString::fromUtf8(bytes));
        else editor->setPlainText(QString::fromUtf8(bytes));
        if (mode_ == SuiteMode::Impagination) {
            const QJsonArray imageWrap = pageLayout.value("imageWrap").toArray();
            const QJsonArray imageMargins = pageLayout.value("imageMargins").toArray();
            struct WrappedImage { int position; int mode; QString name; QJsonObject margins; };
            QVector<WrappedImage> wrappedImages;
            int imageIndex = 0;
            for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next())
                for (auto it = block.begin(); !it.atEnd(); ++it) {
                    const auto fragment = it.fragment();
                    if (!fragment.isValid() || !fragment.charFormat().isImageFormat()) continue;
                    const int mode = imageIndex < imageWrap.size() ? imageWrap.at(imageIndex).toInt() : 0;
                    ++imageIndex;
                    if (mode == 1 || mode == 2)
                        wrappedImages.append({fragment.position(), mode,
                            fragment.charFormat().toImageFormat().name(),
                            imageIndex <= imageMargins.size()
                                ? imageMargins.at(imageIndex - 1).toObject() : QJsonObject()});
                }
            for (auto it = wrappedImages.crbegin(); it != wrappedImages.crend(); ++it) {
                QTextCursor imageCursor(editor->document());
                imageCursor.setPosition(it->position);
                imageCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                setImpaginationImageWrap(imageCursor, it->mode);
                if (it->margins.isEmpty()) continue;
                for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next())
                    for (auto fragment = block.begin(); !fragment.atEnd(); ++fragment)
                        if (fragment.fragment().isValid() && fragment.fragment().charFormat().isImageFormat() &&
                            fragment.fragment().charFormat().toImageFormat().name() == it->name) {
                            QTextCursor located(editor->document());
                            located.setPosition(fragment.fragment().position());
                            QTextFrame *frame = located.currentFrame();
                            if (frame == editor->document()->rootFrame() || dynamic_cast<QTextTable *>(frame))
                                continue;
                            QTextFrameFormat format = frame->frameFormat();
                            format.setLeftMargin(it->margins.value("left").toDouble());
                            format.setRightMargin(it->margins.value("right").toDouble());
                            format.setTopMargin(it->margins.value("top").toDouble());
                            frame->setFrameFormat(format);
                        }
            }
        }
        if (!pageLayout.isEmpty()) {
            QWidget *paper = editor->parentWidget();
            const QString format = pageLayout.value("paper").toString() == "A3" ? "A3" : "A4";
            const bool landscape = pageLayout.value("landscape").toBool();
            const int margin = qBound(20, pageLayout.value("margin").toInt(78), 160);
            paper->setProperty("paperFormat", format);
            paper->setProperty("landscape", landscape);
            paper->setProperty("margin", margin);
            paper->setProperty("headerVisible", pageLayout.value("headerVisible").toBool());
            paper->setProperty("footerVisible", pageLayout.value("footerVisible").toBool());
            paper->setProperty("pageNumbersVisible", pageLayout.value("pageNumbersVisible").toBool());
            if (auto *header = paper->findChild<QLineEdit *>("impHeaderEdit"))
                header->setText(pageLayout.value("headerText").toString());
            if (auto *footer = paper->findChild<QLineEdit *>("impFooterEdit"))
                footer->setText(pageLayout.value("footerText").toString());
            refreshImpaginationPageElements(paper);
            paper->layout()->setContentsMargins(margin, margin, margin, margin);
            QSize dimensions = format == "A3" ? QSize(1123, 1587) : QSize(794, 1123);
            if (landscape) dimensions.transpose();
            paper->setFixedSize(dimensions);
            const QSignalBlocker sizeBlock(paperSize_);
            paperSize_->setCurrentText(format);
            if (auto *action = findChild<QAction *>("impLandscapeAction")) {
                const QSignalBlocker orientationBlock(action);
                action->setChecked(landscape);
            }
            for (const auto &entry : {qMakePair("impHeaderToggle", "headerVisible"),
                                      qMakePair("impFooterToggle", "footerVisible"),
                                      qMakePair("impPageNumberToggle", "pageNumbersVisible")})
                if (auto *action = findChild<QAction *>(entry.first)) {
                    const QSignalBlocker blocker(action);
                    action->setChecked(paper->property(entry.second).toBool());
                }
            editor->document()->setProperty("endnoteCount", pageLayout.value("endnotes").toInt());
        }
        editor->document()->setModified(false);
    }
    tabs_->currentWidget()->setProperty("filePath", path);
    tabs_->currentWidget()->setProperty("modified", false);
    updateTitle(tabs_->currentIndex());
    updateStatus();
    const QString recentType = mode_ == SuiteMode::MindNet ? QStringLiteral("MindNet")
        : mode_ == SuiteMode::Impagination ? QStringLiteral("Impagination") : QStringLiteral("Math");
    rememberRecentFile(path, recentType);
    return true;
}

bool SuiteWindow::save(int index, bool saveAs)
{
    if (index < 0) return false;
    QString path = pathFor(index);
    const QString previousPath = path;
    if (tabs_->widget(index)->property("importedPdf").toBool() && !saveAs) {
        saveAs = true;
        const QFileInfo source(path);
        path = source.dir().filePath(source.completeBaseName() + ".html");
    }
    if (path.isEmpty() || saveAs) {
        const QString defaultExtension = suiteDefaultExtension(mode_);
        QString selectedFilter = suiteSaveFilters(mode_).section(";;", 0, 0);
        path = QFileDialog::getSaveFileName(this, tr("Salva documento"),
            path.isEmpty() ? tr("Senza titolo") + "." + defaultExtension : path,
            suiteSaveFilters(mode_), &selectedFilter);
        if (path.isEmpty()) return false;
        if (QFileInfo(path).suffix().isEmpty())
            path += "." + extensionFromFilter(selectedFilter, defaultExtension);
    }
    QByteArray bytes;
    if (mode_ == SuiteMode::MindNet) {
        QJsonArray nodes;
        QJsonArray connections;
        QJsonArray textBoxes;
        QString header, footer;
        for (QGraphicsItem *item : viewAt(tabs_, index)->scene()->items()) {
            const QString role = item->data(0).toString();
            if (auto *box = dynamic_cast<TextBoxItem *>(item); box && role == "textBox") {
                textBoxes.append(QJsonObject{{"x", box->pos().x()}, {"y", box->pos().y()},
                    {"width", box->rect().width()}, {"height", box->rect().height()},
                    {"rotation", box->rotation()},
                    {"html", box->text()->toHtml()},
                    {"color", box->text()->defaultTextColor().name()}});
                continue;
            }
            if (role == "header" || role == "footer") {
                if (auto *text = dynamic_cast<QGraphicsTextItem *>(item)) {
                    if (role == "header") header = text->toPlainText();
                    else footer = text->toPlainText();
                }
                continue;
            }
            if (role == "pageBoundary" || role == "nodeLabel") continue;
            if (auto *line = dynamic_cast<ConnectorItem *>(item)) {
                QJsonArray points;
                for (const QPointF &point : line->points())
                    points.append(QJsonArray{point.x(), point.y()});
                connections.append(QJsonObject{{"points", points}, {"type", line->lineType()},
                    {"width", line->pen().width()}, {"color", line->pen().color().name()}});
                continue;
            }
            const QString shape = role;
            if (shape.isEmpty()) continue;
            QString label;
            QJsonObject labelBox;
            QString labelHtml, labelColor;
            int labelFontSize = 12;
            for (QGraphicsItem *child : item->childItems())
                if (auto *box = dynamic_cast<TextBoxItem *>(child)) {
                    label = box->text()->toPlainText();
                    labelHtml = box->text()->toHtml();
                    labelColor = box->text()->defaultTextColor().name();
                    labelFontSize = box->text()->font().pointSize();
                    labelBox = QJsonObject{{"x", box->pos().x()}, {"y", box->pos().y()},
                        {"width", box->rect().width()}, {"height", box->rect().height()},
                        {"rotation", box->rotation()}};
                }
            auto *pathItem = dynamic_cast<ShapeItem *>(item);
            nodes.append(QJsonObject{{"shape", shape}, {"x", item->pos().x()},
                {"y", item->pos().y()}, {"label", label},
                {"color", pathItem ? pathItem->brush().color().name() : QString("#e7f4ff")},
                {"width", pathItem ? pathItem->shapeWidth() : 160},
                {"height", pathItem ? pathItem->shapeHeight() : 70},
                {"rotation", item->rotation()}, {"labelBox", labelBox},
                {"labelHtml", labelHtml}, {"labelFontSize", labelFontSize},
                {"labelColor", labelColor}});
        }
        auto *view = dynamic_cast<DiagramView *>(viewAt(tabs_, index));
        bytes = QJsonDocument(QJsonObject{{"format", "NebiPad+ MindNet"},
            {"paper", view->property("paperFormat").toString()},
            {"landscape", view->property("landscape").toBool()},
            {"rulersVisible", view->rulersVisible()}, {"gridVisible", view->gridVisible()},
            {"rulerUnit", view->rulerUnit()}, {"rulerStep", view->rulerValue()},
            {"gridUnit", view->gridUnit()}, {"gridStep", view->gridValue()},
            {"header", header}, {"footer", footer},
            {"nodes", nodes}, {"connections", connections}, {"textBoxes", textBoxes}}).toJson();
    } else {
        if (mode_ == SuiteMode::Impagination && !path.endsWith(".txt", Qt::CaseInsensitive)) {
            QTextDocument *document = editorAt(tabs_, index)->document();
            QVector<QPair<int, QString>> localVideos;
            for (QTextBlock block = document->begin(); block.isValid(); block = block.next())
                for (auto it = block.begin(); !it.atEnd(); ++it)
                    if (it.fragment().isValid() && it.fragment().charFormat().isImageFormat()) {
                        const QString reference = it.fragment().charFormat().anchorHref();
                        if (reference.startsWith("nebi-video:local:"))
                            localVideos.append({it.fragment().position(), reference});
                    }
            const QFileInfo documentInfo(path);
            const QString mediaFolder = documentInfo.completeBaseName() + "_media";
            const QString mediaDirectory = documentInfo.dir().filePath(mediaFolder);
            for (auto it = localVideos.crbegin(); it != localVideos.crend(); ++it) {
                QString source = QString::fromUtf8(QByteArray::fromBase64(
                    it->second.mid(QString("nebi-video:local:").size()).toLatin1(),
                    QByteArray::Base64UrlEncoding));
                if (QFileInfo(source).isRelative()) {
                    const QString previousPath = pathFor(index);
                    source = QFileInfo(previousPath).dir().filePath(source);
                }
                if (!QFileInfo::exists(source)) {
                    QMessageBox::warning(this, tr("Salvataggio fallito"),
                        tr("File video non trovato: %1").arg(source));
                    return false;
                }
                if (!QDir().mkpath(mediaDirectory)) {
                    QMessageBox::warning(this, tr("Salvataggio fallito"),
                        tr("Impossibile creare la cartella dei video: %1").arg(mediaDirectory));
                    return false;
                }
                QString destination = source;
                if (QFileInfo(source).absolutePath() != QFileInfo(mediaDirectory).absoluteFilePath()) {
                    destination = QDir(mediaDirectory).filePath(
                        QUuid::createUuid().toString(QUuid::WithoutBraces) + "." +
                        QFileInfo(source).suffix());
                    if (!QFile::copy(source, destination)) {
                        QMessageBox::warning(this, tr("Salvataggio fallito"),
                            tr("Impossibile copiare il video: %1").arg(source));
                        return false;
                    }
                }
                const QString relative = QDir(documentInfo.absolutePath()).relativeFilePath(destination);
                const QString newReference = "nebi-video:local:" + QString::fromLatin1(
                    relative.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
                if (newReference == it->second) continue;
                QTextCursor imageCursor(document);
                imageCursor.setPosition(it->first);
                imageCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                QTextImageFormat format = imageCursor.charFormat().toImageFormat();
                format.setAnchorHref(newReference);
                imageCursor.insertImage(format);
            }
        }
        const bool plain = path.endsWith(".txt", Qt::CaseInsensitive);
        bytes = (plain ? editorAt(tabs_, index)->toPlainText()
                       : mode_ == SuiteMode::Impagination
                           ? portableHtml(editorAt(tabs_, index)->document())
                           : editorAt(tabs_, index)->toHtml()).toUtf8();
        if (mode_ == SuiteMode::Impagination && !plain) {
            QWidget *paper = editorAt(tabs_, index)->parentWidget();
            QJsonArray imageWrap;
            QJsonArray imageMargins;
            QTextDocument *document = editorAt(tabs_, index)->document();
            for (QTextBlock block = document->begin(); block.isValid(); block = block.next())
                for (auto it = block.begin(); !it.atEnd(); ++it)
                    if (it.fragment().isValid() && it.fragment().charFormat().isImageFormat()) {
                        QTextCursor imageCursor(document);
                        imageCursor.setPosition(it.fragment().position());
                        QTextFrame *frame = imageCursor.currentFrame();
                        const int placement = frame && frame != document->rootFrame() &&
                            !dynamic_cast<QTextTable *>(frame)
                                ? int(frame->frameFormat().position())
                                : it.fragment().charFormat().property(QTextFormat::CssFloat).toInt();
                        imageWrap.append(placement == int(QTextFrameFormat::FloatLeft) ? 1
                            : placement == int(QTextFrameFormat::FloatRight) ? 2 : 0);
                        imageMargins.append(frame && frame != document->rootFrame() &&
                            !dynamic_cast<QTextTable *>(frame)
                            ? QJsonObject{{"left", frame->frameFormat().leftMargin()},
                                {"right", frame->frameFormat().rightMargin()},
                                {"top", frame->frameFormat().topMargin()}}
                            : QJsonObject());
                    }
            const QJsonObject pageLayout{{"paper", paper->property("paperFormat").toString()},
                {"landscape", paper->property("landscape").toBool()},
                {"margin", paper->property("margin").toInt()},
                {"headerVisible", paper->property("headerVisible").toBool()},
                {"footerVisible", paper->property("footerVisible").toBool()},
                {"pageNumbersVisible", paper->property("pageNumbersVisible").toBool()},
                {"imageWrap", imageWrap},
                {"imageMargins", imageMargins},
                {"headerText", paper->findChild<QLineEdit *>("impHeaderEdit")->text()},
                {"footerText", paper->findChild<QLineEdit *>("impFooterEdit")->text()},
                {"endnotes", editorAt(tabs_, index)->document()->property("endnoteCount").toInt()}};
            bytes.prepend("<!--NEBIPAD_LAYOUT:" + QJsonDocument(pageLayout).toJson(QJsonDocument::Compact) + "-->\n");
        }
    }
    if (mode_ == SuiteMode::Impagination && !previousPath.isEmpty() &&
        QFileInfo(previousPath).absoluteFilePath() != QFileInfo(path).absoluteFilePath()) {
        QString audioError;
        if (!copyRecordingLinks(previousPath, path, editorAt(tabs_, index)->toPlainText(), &audioError)) {
            QMessageBox::warning(this, tr("Registrazione audio"), audioError);
            return false;
        }
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        QMessageBox::warning(this, tr("Salvataggio fallito"), file.errorString());
        return false;
    }
    tabs_->widget(index)->setProperty("filePath", path);
    tabs_->widget(index)->setProperty("modified", false);
    tabs_->widget(index)->setProperty("importedPdf", false);
    if (auto *editor = editorAt(tabs_, index)) editor->document()->setModified(false);
    updateTitle(index);
    const QString recentType = mode_ == SuiteMode::MindNet ? QStringLiteral("MindNet")
        : mode_ == SuiteMode::Impagination ? QStringLiteral("Impagination") : QStringLiteral("Math");
    rememberRecentFile(path, recentType);
    return true;
}

bool SuiteWindow::maybeSave(int index)
{
    if (index < 0) return true;
    const bool modified = editorAt(tabs_, index) ? editorAt(tabs_, index)->document()->isModified()
        : tabs_->widget(index)->property("modified").toBool();
    if (!modified) return true;
    auto answer = QMessageBox::question(this, tr("Modifiche non salvate"),
        tr("Salvare le modifiche a %1?").arg(tabs_->tabText(index)),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    return answer == QMessageBox::Discard || (answer == QMessageBox::Save && save(index));
}

QTextEdit *SuiteWindow::textEditor() const { return editorAt(tabs_, tabs_->currentIndex()); }
QGraphicsView *SuiteWindow::diagramView() const { return viewAt(tabs_, tabs_->currentIndex()); }
QGraphicsScene *SuiteWindow::diagramScene() const
{
    auto *view = diagramView();
    return view ? view->scene() : nullptr;
}

void SuiteWindow::addNode(const QString &shape)
{
    auto *scene = diagramScene();
    if (!scene) return;
    auto *item = new ShapeItem(shape, shapeColor_);
    scene->addItem(item);
    item->setPos(80 + scene->items().size() * 24, 80 + scene->items().size() * 24);
    scene->clearSelection();
    item->setSelected(true);
    bool accepted = false;
    const QString label = QInputDialog::getText(this, tr("Testo della forma"), tr("Etichetta:"),
        QLineEdit::Normal, {}, &accepted);
    if (accepted && !label.isEmpty()) {
        auto *text = addNodeLabel(item, label);
        text->text()->setDefaultTextColor(QColor("#173652"));
    }
    tabs_->currentWidget()->setProperty("modified", true);
    updateTitle(tabs_->currentIndex());
    updateStatus();
}

void SuiteWindow::addDiagramTemplate(const QString &type)
{
    auto *scene = diagramScene();
    if (!scene) return;
    const QRectF paper = scene->sceneRect().adjusted(kMindPageMargin, kMindPageMargin,
                                                      -kMindPageMargin, -kMindPageMargin);
    const qreal left = 60;
    const qreal center = paper.width() / 2 - 80;
    const qreal right = paper.width() - 220;
    struct Node { QString shape; QString label; QPointF position; };
    QVector<Node> nodes;
    QVector<QPair<int, int>> edges;
    if (type == "flow") {
        nodes = {{"terminal", tr("Inizio"), {center, 110}},
                 {"process", tr("Processo"), {center, 265}},
                 {"decision", tr("Decisione"), {center, 420}},
                 {"terminal", tr("Fine"), {center, 575}}};
        edges = {{0, 1}, {1, 2}, {2, 3}};
    } else if (type == "mindmap") {
        nodes = {{"terminal", tr("Idea centrale"), {center, 310}},
                 {"process", tr("Tema 1"), {left, 135}},
                 {"process", tr("Tema 2"), {right, 135}},
                 {"process", tr("Tema 3"), {left, 490}},
                 {"process", tr("Tema 4"), {right, 490}}};
        edges = {{0, 1}, {0, 2}, {0, 3}, {0, 4}};
    } else if (type == "org") {
        nodes = {{"terminal", tr("Direzione"), {center, 125}},
                 {"process", tr("Reparto A"), {left, 350}},
                 {"process", tr("Reparto B"), {center, 350}},
                 {"process", tr("Reparto C"), {right, 350}}};
        edges = {{0, 1}, {0, 2}, {0, 3}};
    } else if (type == "decision") {
        nodes = {{"decision", tr("Condizione?"), {center, 125}},
                 {"process", tr("Sì"), {left, 350}},
                 {"process", tr("No"), {right, 350}},
                 {"terminal", tr("Risultato"), {center, 570}}};
        edges = {{0, 1}, {0, 2}, {1, 3}, {2, 3}};
    } else if (type == "process") {
        nodes = {{"input", tr("Input"), {left, 300}},
                 {"process", tr("Elaborazione"), {center, 300}},
                 {"database", tr("Dati"), {right, 300}},
                 {"document", tr("Output"), {center, 520}}};
        edges = {{0, 1}, {1, 2}, {1, 3}};
    } else if (type == "network") {
        nodes = {{"connector", tr("Nodo centrale"), {center, 305}},
                 {"connector", tr("Nodo A"), {left, 125}},
                 {"connector", tr("Nodo B"), {right, 125}},
                 {"connector", tr("Nodo C"), {left, 505}},
                 {"connector", tr("Nodo D"), {right, 505}}};
        edges = {{0, 1}, {0, 2}, {0, 3}, {0, 4}};
    } else if (type == "uml") {
        nodes = {{"predefined", tr("Classe base"), {center, 125}},
                 {"predefined", tr("Classe A"), {left, 355}},
                 {"predefined", tr("Classe B"), {right, 355}}};
        edges = {{0, 1}, {0, 2}};
    } else {
        nodes = {{"process", tr("Blocco A"), {left, 300}},
                 {"process", tr("Blocco B"), {center, 300}},
                 {"process", tr("Blocco C"), {right, 300}}};
        edges = {{0, 1}, {1, 2}};
    }
    QVector<ShapeItem *> created;
    scene->clearSelection();
    for (const Node &node : nodes) {
        auto *item = new ShapeItem(node.shape, shapeColor_);
        scene->addItem(item);
        item->setPos(node.position);
        addNodeLabel(item, node.label);
        created.append(item);
    }
    for (const auto &edge : edges) {
        const QPointF start = created[edge.first]->sceneBoundingRect().center();
        const QPointF end = created[edge.second]->sceneBoundingRect().center();
        scene->addItem(new ConnectorItem({start, end}, type == "network" ? 0 : 2,
                                          lineColor_, lineWidth_));
    }
    if (!created.isEmpty()) created.first()->setSelected(true);
    tabs_->currentWidget()->setProperty("modified", true);
    updateTitle(tabs_->currentIndex());
}

void SuiteWindow::addTextBox()
{
    auto *scene = diagramScene();
    auto *view = dynamic_cast<DiagramView *>(diagramView());
    if (!scene || !view) return;
    view->stopLineTool();
    auto *box = new TextBoxItem;
    scene->addItem(box);
    const QRectF paper = scene->sceneRect().adjusted(kMindPageMargin, kMindPageMargin,
                                                      -kMindPageMargin, -kMindPageMargin);
    const QPointF center = view->mapToScene(view->viewport()->rect().center());
    box->setPos(qBound(paper.left() + 20, center.x() - 125, paper.right() - 270),
                qBound(paper.top() + 20, center.y() - 52, paper.bottom() - 125));
    scene->clearSelection();
    box->setSelected(true);
    box->text()->setFocus(Qt::OtherFocusReason);
    updateMindTextControls();
}

QGraphicsTextItem *SuiteWindow::currentMindText() const
{
    auto *scene = diagramScene();
    if (!scene) return nullptr;
    for (QGraphicsItem *item : scene->selectedItems())
        if (auto *box = dynamic_cast<TextBoxItem *>(item)) return box->text();
    for (QGraphicsItem *item : scene->selectedItems())
        if (auto *shape = dynamic_cast<ShapeItem *>(item))
            for (QGraphicsItem *child : shape->childItems())
                if (auto *box = dynamic_cast<TextBoxItem *>(child)) return box->text();
    if (auto *focused = dynamic_cast<EditableBoxText *>(scene->focusItem()))
        if (dynamic_cast<TextBoxItem *>(focused->parentItem())) return focused;
    return nullptr;
}

void SuiteWindow::applyMindTextFormat(const QTextCharFormat &format)
{
    auto *text = currentMindText();
    if (!text) return;
    const QTextCursor original = text->textCursor();
    QTextCursor cursor = original;
    if (!cursor.hasSelection() && !text->toPlainText().isEmpty())
        cursor.select(QTextCursor::Document);
    cursor.mergeCharFormat(format);
    text->setTextCursor(original.hasSelection() ? cursor : original);
    text->setFocus(Qt::OtherFocusReason);
    updateMindTextControls();
}

void SuiteWindow::updateMindTextControls()
{
    if (!mindFontCombo_) return;
    auto *text = currentMindText();
    const bool enabled = text != nullptr;
    mindFontCombo_->setEnabled(enabled);
    mindFontSize_->setEnabled(enabled);
    mindGrowAction_->setEnabled(enabled);
    mindShrinkAction_->setEnabled(enabled);
    for (QAction *action : {mindBoldAction_, mindItalicAction_, mindUnderlineAction_,
                            mindStrikeAction_, mindColorAction_})
        action->setEnabled(enabled);
    if (!text) return;
    const QFont font = text->font();
    {
        const QSignalBlocker blocker(mindFontCombo_);
        mindFontCombo_->setCurrentFont(font);
    }
    {
        const QSignalBlocker blocker(mindFontSize_);
        mindFontSize_->setValue(qBound(8, font.pointSize() > 0 ? font.pointSize() : 12, 96));
    }
    const QTextCharFormat format = text->textCursor().charFormat();
    mindBoldAction_->setChecked(format.fontWeight() >= QFont::Bold || font.bold());
    mindItalicAction_->setChecked(format.fontItalic() || font.italic());
    mindUnderlineAction_->setChecked(format.fontUnderline() || font.underline());
    mindStrikeAction_->setChecked(format.fontStrikeOut() || font.strikeOut());
}

void SuiteWindow::syncMindViewControls()
{
    if (!orientationAction_) return;
    auto *view = dynamic_cast<DiagramView *>(diagramView());
    if (!view) return;
    const QSignalBlocker paperBlock(paperSize_);
    const QSignalBlocker orientationBlock(orientationAction_);
    const QSignalBlocker rulerBlock(rulersAction_);
    const QSignalBlocker gridBlock(gridAction_);
    const QSignalBlocker rulerUnitBlock(rulerUnit_);
    const QSignalBlocker gridUnitBlock(gridUnit_);
    const QSignalBlocker rulerStepBlock(rulerStep_);
    const QSignalBlocker gridStepBlock(gridStep_);
    paperSize_->setCurrentText(view->property("paperFormat").toString());
    orientationAction_->setChecked(view->property("landscape").toBool());
    rulersAction_->setChecked(view->rulersVisible());
    gridAction_->setChecked(view->gridVisible());
    rulerUnit_->setCurrentText(view->rulerUnit());
    gridUnit_->setCurrentText(view->gridUnit());
    rulerStep_->setValue(view->rulerValue());
    gridStep_->setValue(view->gridValue());
    updateStatus();
}

void SuiteWindow::addConnection(int type)
{
    auto *view = dynamic_cast<DiagramView *>(diagramView());
    if (!view) return;
    lineType_ = type;
    view->setLineTool(type, lineColor_, lineWidth_);
    statusBar()->showMessage(tr("Trascina sul foglio per disegnare la linea. Esc per tornare alla selezione."), 5000);
}

void SuiteWindow::setShapeColor()
{
    const QColor chosen = QColorDialog::getColor(shapeColor_, this, tr("Colore delle forme"));
    if (!chosen.isValid()) return;
    shapeColor_ = chosen;
    if (auto *scene = diagramScene()) {
        for (QGraphicsItem *item : scene->selectedItems())
            if (auto *shape = dynamic_cast<ShapeItem *>(item))
                shape->setBrush(chosen);
    }
}

void SuiteWindow::setLineColor()
{
    const QColor chosen = QColorDialog::getColor(lineColor_, this, tr("Colore dei collegamenti"));
    if (!chosen.isValid()) return;
    lineColor_ = chosen;
    if (auto *view = dynamic_cast<DiagramView *>(diagramView()))
        view->updateLineSettings(lineColor_, lineWidth_);
    if (auto *scene = diagramScene()) {
        for (QGraphicsItem *item : scene->selectedItems()) {
            if (auto *line = dynamic_cast<ConnectorItem *>(item)) {
                QPen pen = line->pen();
                pen.setColor(chosen);
                line->setPen(pen);
            }
        }
    }
}

void SuiteWindow::editPageText(const QString &role)
{
    auto *scene = diagramScene();
    if (!scene) return;
    for (QGraphicsItem *item : scene->items()) {
        if (item->data(0).toString() != role) continue;
        auto *text = dynamic_cast<QGraphicsTextItem *>(item);
        if (!text) return;
        bool accepted = false;
        const QString value = QInputDialog::getText(this,
            role == "header" ? tr("Intestazione") : tr("Piè di pagina"), tr("Testo:"),
            QLineEdit::Normal, text->toPlainText(), &accepted);
        if (accepted) text->setPlainText(value);
        return;
    }
}

void SuiteWindow::zoom(qreal factor)
{
    if (auto *view = diagramView()) view->scale(factor, factor);
    else if (auto *editor = textEditor()) {
        if (factor > 1) editor->zoomIn(); else editor->zoomOut();
    }
}

void SuiteWindow::updateStatus()
{
    if (auto *editor = textEditor()) {
        const QTextCursor cursor = editor->textCursor();
        if (mode_ == SuiteMode::Impagination) {
            const int words = editor->toPlainText().split(QRegularExpression("\\s+"),
                Qt::SkipEmptyParts).size();
            status_->setText(tr("%1   |   Parole: %2   Caratteri: %3   Paragrafi: %4   |   Paragrafo %5")
                .arg(modeName(mode_)).arg(words).arg(editor->document()->characterCount() - 1)
                .arg(editor->document()->blockCount()).arg(cursor.blockNumber() + 1));
            return;
        }
        status_->setText(tr("%1   |   Lunghezza: %2   Righe: %3   Linea: %4   Colonna: %5   |   UTF-8")
            .arg(modeName(mode_)).arg(editor->document()->characterCount() - 1)
            .arg(editor->document()->blockCount()).arg(cursor.blockNumber() + 1)
            .arg(cursor.positionInBlock() + 1));
    } else status_->setText(tr("%1   |   %2 %3   |   Trascina le forme; Ctrl+rotella per lo zoom")
        .arg(modeName(mode_), paperSize_->currentText(),
             orientationAction_ && orientationAction_->isChecked() ? tr("orizzontale") : tr("verticale")));
}

void SuiteWindow::closeEvent(QCloseEvent *event)
{
    if (dictation_) dictation_->stop();
    if (recording_) recording_->stop();
    for (int i = tabs_->count() - 1; i >= 0; --i) {
        if (!maybeSave(i)) { event->ignore(); return; }
    }
    event->accept();
}
