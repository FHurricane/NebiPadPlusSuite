#include "suitewindow.h"
#include "imageeditdialog.h"
#include "pdfpodofo.h"

#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QContextMenuEvent>
#include <QFile>
#include <QFileDialog>
#include <QDockWidget>
#include <QDir>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QFontComboBox>
#include <QInputDialog>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLabel>
#include <QGraphicsView>
#include <QMenuBar>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPdfWriter>
#include <QPushButton>
#include <QTabWidget>
#include <QTabBar>
#include <QTextEdit>
#include <QTextList>
#include <QTextDocument>
#include <QTextFrame>
#include <QToolButton>
#include <QToolBar>
#include <QSpinBox>
#include <QTimer>
#include <QTemporaryDir>

static bool hasVisiblePixels(const QIcon &icon)
{
    const QImage image = icon.pixmap(QSize(24, 24)).toImage();
    if (image.isNull()) return false;
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (qAlpha(image.pixel(x, y)) > 0) return true;
    return false;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    for (auto mode : {SuiteMode::MindNet, SuiteMode::Impagination, SuiteMode::Math}) {
        SuiteWindow window(mode);
        window.show();
        app.processEvents();
        auto *tabs = window.findChild<QTabWidget *>();
        if (!tabs || tabs->count() != 1) return 1;
        for (QAction *action : window.menuBar()->actions())
            if (action->text().contains("Linguaggio")) return 2;
        if (window.menuBar()->actions().last()->text() != "?") return 96;
        if (!window.findChild<QMenu *>("googleDriveMenu") ||
            !window.findChild<QAction *>("googleDriveOpenAction") ||
            !window.findChild<QAction *>("googleDriveSaveAction") ||
            !window.findChild<QToolButton *>("googleDriveAvatar") ||
            !window.findChild<QWidget *>("googleDriveAccountWidget")) return 136;
        if (mode == SuiteMode::Impagination) {
            if (!window.findChild<QAction *>("impExportPdfAction")) return 137;
        } else if (!window.findChild<QAction *>("suiteExportPdfAction")) {
            return 138;
        }
        if (mode == SuiteMode::MindNet) {
            auto *diagram = tabs->currentWidget()->findChild<QGraphicsView *>("diagramView");
            auto *dock = window.findChild<QDockWidget *>("mindNetShapesDock");
            auto *connection = window.findChild<QToolButton *>("connectionButton");
            auto *lines = window.findChild<QMenu *>("linesMenu");
            if (!diagram || !dock || !connection || !connection->menu() || !lines) return 3;
            if (lines->actions().isEmpty() || lines->actions().first()->text() != "Linea" ||
                !hasVisiblePixels(lines->actions().first()->icon())) return 13;
            if (connection->toolButtonStyle() != Qt::ToolButtonIconOnly ||
                !hasVisiblePixels(connection->icon()) ||
                !hasVisiblePixels(connection->menu()->actions().first()->icon())) return 14;
            for (QToolBar *bar : window.findChildren<QToolBar *>())
                if (bar->toolButtonStyle() != Qt::ToolButtonIconOnly) return 15;
            auto *orientation = window.findChild<QAction *>("mindOrientationAction");
            auto *rulers = window.findChild<QAction *>("mindRulersAction");
            auto *grid = window.findChild<QAction *>("mindGridAction");
            auto *pageFormat = window.findChild<QComboBox *>("paperSizeCombo");
            auto *rulerUnit = window.findChild<QComboBox *>("rulerUnit");
            auto *gridUnit = window.findChild<QComboBox *>("gridUnit");
            auto *rulerStep = window.findChild<QDoubleSpinBox *>("rulerStep");
            auto *gridStep = window.findChild<QDoubleSpinBox *>("gridStep");
            auto *templates = window.findChild<QMenu *>("diagramTemplatesMenu");
            if (!orientation || !rulers || !grid || !pageFormat || !rulerUnit ||
                !gridUnit || !rulerStep || !gridStep || !templates ||
                templates->actions().size() < 8 || !hasVisiblePixels(grid->icon()) ||
                !hasVisiblePixels(rulers->icon())) return 27;
            pageFormat->setCurrentText("A3");
            orientation->trigger();
            rulers->trigger();
            grid->trigger();
            rulerUnit->setCurrentText("cm");
            gridUnit->setCurrentText("px");
            rulerStep->setValue(2.5);
            gridStep->setValue(25);
            bool landscapeA3 = false;
            for (QGraphicsItem *item : diagram->scene()->items())
                if (item->data(0).toString() == "pageBoundary")
                    if (auto *rect = dynamic_cast<QGraphicsRectItem *>(item))
                        landscapeA3 = rect->rect().size() == QSizeF(1587, 1123);
            if (!landscapeA3) return 28;
            if (!dock->findChild<QToolButton *>("shape_process") ||
                !dock->findChild<QToolButton *>("shape_decision") ||
                !dock->findChild<QToolButton *>("shape_net_router") ||
                !dock->findChild<QToolButton *>("shape_net_switch_l3") ||
                !dock->findChild<QPushButton *>("shapeColorButton") ||
                !dock->findChild<QPushButton *>("headerButton") ||
                !dock->findChild<QPushButton *>("footerButton")) return 8;
            auto *category = dock->findChild<QComboBox *>("mindShapeCategory");
            if (!category || category->count() != 2) return 37;
            category->setCurrentIndex(1);
            if (!dock->findChild<QToolButton *>("shape_net_router")->isVisible() ||
                dock->findChild<QToolButton *>("shape_process")->isVisible()) return 38;
            QTimer::singleShot(0, [] {
                auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
                if (dialog) dialog->reject();
            });
            dock->findChild<QToolButton *>("shape_net_router")->click();
            QGraphicsItem *router = nullptr;
            for (QGraphicsItem *item : diagram->scene()->items())
                if (item->data(0).toString() == "net_router") router = item;
            if (!router || !router->isSelected() ||
                !dynamic_cast<QGraphicsPathItem *>(router)) return 39;
            router->setPos(0, 0);
            auto *routerPath = dynamic_cast<QGraphicsPathItem *>(router);
            if (!routerPath->boundingRect().contains(QPointF(160, 70)) ||
                !routerPath->shape().contains(QPointF(151, 61))) return 40;
            const qreal routerWidth = routerPath->path().boundingRect().width();
            QGraphicsSceneMouseEvent routerPress(QEvent::GraphicsSceneMousePress);
            routerPress.setPos(QPointF(151, 61));
            routerPress.setScenePos(QPointF(151, 61));
            routerPress.setButton(Qt::LeftButton);
            routerPress.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(router, &routerPress);
            QGraphicsSceneMouseEvent routerMove(QEvent::GraphicsSceneMouseMove);
            routerMove.setPos(QPointF(200, 100));
            routerMove.setScenePos(QPointF(200, 100));
            routerMove.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(router, &routerMove);
            QGraphicsSceneMouseEvent routerRelease(QEvent::GraphicsSceneMouseRelease);
            routerRelease.setButton(Qt::LeftButton);
            diagram->scene()->sendEvent(router, &routerRelease);
            if (routerPath->path().boundingRect().width() <= routerWidth) return 41;
            category->setCurrentIndex(0);
            bool hasPaper = false;
            for (auto *item : diagram->scene()->items())
                if (item->data(0).toString() == "pageBoundary") hasPaper = true;
            if (!hasPaper || connection->menu()->actions().size() < 4) return 9;
            QTimer::singleShot(0, [] {
                auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
                if (dialog) { dialog->setTextValue("Processo"); dialog->accept(); }
            });
            dock->findChild<QToolButton *>("shape_process")->click();
            QGraphicsItem *shape = nullptr;
            for (QGraphicsItem *item : diagram->scene()->items())
                if (item->data(0).toString() == "process") shape = item;
            if (!shape || !shape->isSelected()) return 10;
            shape->setPos(0, 0);
            QGraphicsSceneMouseEvent press(QEvent::GraphicsSceneMousePress);
            press.setPos(QPointF(160, 70));
            press.setScenePos(QPointF(160, 70));
            press.setButton(Qt::LeftButton);
            press.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(shape, &press);
            QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
            move.setPos(QPointF(190, 90));
            move.setScenePos(QPointF(190, 90));
            move.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(shape, &move);
            QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
            release.setPos(QPointF(190, 90));
            release.setScenePos(QPointF(190, 90));
            release.setButton(Qt::LeftButton);
            diagram->scene()->sendEvent(shape, &release);
            if (dynamic_cast<QGraphicsPathItem *>(shape)->path().boundingRect().width() <= 160) return 11;
            QGraphicsSceneMouseEvent rotatePress(QEvent::GraphicsSceneMousePress);
            rotatePress.setPos(QPointF(95, -18));
            rotatePress.setScenePos(QPointF(95, -18));
            rotatePress.setButton(Qt::LeftButton);
            rotatePress.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(shape, &rotatePress);
            QGraphicsSceneMouseEvent rotateMove(QEvent::GraphicsSceneMouseMove);
            rotateMove.setPos(QPointF(140, 35));
            rotateMove.setScenePos(QPointF(140, 35));
            rotateMove.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(shape, &rotateMove);
            QGraphicsSceneMouseEvent rotateRelease(QEvent::GraphicsSceneMouseRelease);
            rotateRelease.setPos(QPointF(140, 35));
            rotateRelease.setScenePos(QPointF(140, 35));
            rotateRelease.setButton(Qt::LeftButton);
            diagram->scene()->sendEvent(shape, &rotateRelease);
            if (qAbs(shape->rotation()) < 1) return 12;
            QGraphicsRectItem *nodeLabel = nullptr;
            for (QGraphicsItem *child : shape->childItems())
                if (child->data(0).toString() == "nodeLabel")
                    nodeLabel = dynamic_cast<QGraphicsRectItem *>(child);
            if (!nodeLabel || nodeLabel->childItems().isEmpty()) return 42;
            nodeLabel->setSelected(true);
            const qreal labelWidth = nodeLabel->rect().width();
            const QPointF labelCorner(labelWidth, nodeLabel->rect().height());
            QGraphicsSceneMouseEvent labelPress(QEvent::GraphicsSceneMousePress);
            labelPress.setPos(labelCorner);
            labelPress.setScenePos(nodeLabel->mapToScene(labelCorner));
            labelPress.setButton(Qt::LeftButton);
            labelPress.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(nodeLabel, &labelPress);
            QGraphicsSceneMouseEvent labelMove(QEvent::GraphicsSceneMouseMove);
            labelMove.setPos(labelCorner + QPointF(30, 15));
            labelMove.setScenePos(nodeLabel->mapToScene(labelCorner + QPointF(30, 15)));
            labelMove.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(nodeLabel, &labelMove);
            QGraphicsSceneMouseEvent labelRelease(QEvent::GraphicsSceneMouseRelease);
            labelRelease.setButton(Qt::LeftButton);
            diagram->scene()->sendEvent(nodeLabel, &labelRelease);
            if (nodeLabel->rect().width() <= labelWidth) return 43;
            nodeLabel->setPos(23, 14);
            nodeLabel->setRotation(27);
            auto *labelText = dynamic_cast<QGraphicsTextItem *>(nodeLabel->childItems().first());
            if (!labelText) return 47;
            const auto dragLabelText = [&](Qt::KeyboardModifiers modifiers, const QPointF &delta) {
                const QPointF origin = labelText->mapToScene(QPointF(12, 10));
                QGraphicsSceneMouseEvent pressEvent(QEvent::GraphicsSceneMousePress);
                pressEvent.setPos(QPointF(12, 10));
                pressEvent.setScenePos(origin);
                pressEvent.setButton(Qt::LeftButton);
                pressEvent.setButtons(Qt::LeftButton);
                pressEvent.setModifiers(modifiers);
                diagram->scene()->sendEvent(labelText, &pressEvent);
                QGraphicsSceneMouseEvent moveEvent(QEvent::GraphicsSceneMouseMove);
                moveEvent.setScenePos(origin + delta);
                moveEvent.setButtons(Qt::LeftButton);
                moveEvent.setModifiers(modifiers);
                diagram->scene()->sendEvent(labelText, &moveEvent);
                QGraphicsSceneMouseEvent releaseEvent(QEvent::GraphicsSceneMouseRelease);
                releaseEvent.setButton(Qt::LeftButton);
                releaseEvent.setModifiers(modifiers);
                diagram->scene()->sendEvent(labelText, &releaseEvent);
            };
            const QPointF initialShapePos = shape->pos();
            const QPointF initialLabelPos = nodeLabel->pos();
            dragLabelText(Qt::NoModifier, QPointF(25, 10));
            if (shape->pos() != initialShapePos + QPointF(25, 10) ||
                nodeLabel->pos() != initialLabelPos) return 48;
            const QPointF movedShapePos = shape->pos();
            dragLabelText(Qt::AltModifier, QPointF(20, 5));
            if (shape->pos() != movedShapePos || nodeLabel->pos() == initialLabelPos) return 49;
            nodeLabel->setPos(23, 14);
            diagram->scene()->clearSelection();
            shape->setSelected(true);
            auto *growText = window.findChild<QAction *>("mindGrowTextAction");
            auto *shrinkText = window.findChild<QAction *>("mindShrinkTextAction");
            auto *diagramFontSize = window.findChild<QSpinBox *>("mindFontSize");
            if (!growText || !shrinkText || !diagramFontSize ||
                !hasVisiblePixels(growText->icon()) || !hasVisiblePixels(shrinkText->icon()) ||
                !growText->isEnabled()) return 50;
            const int originalTextSize = labelText->font().pointSize();
            growText->trigger();
            if (labelText->font().pointSize() != originalTextSize + 1) return 51;
            shrinkText->trigger();
            if (labelText->font().pointSize() != originalTextSize) return 52;
            diagramFontSize->setValue(20);
            if (labelText->font().pointSize() != 20) return 53;
            diagram->scene()->clearSelection();
            const int beforeLines = diagram->scene()->items().size();
            lines->actions().first()->trigger();
            const QPoint start = diagram->mapFromScene(QPointF(300, 300));
            const QPoint end = diagram->mapFromScene(QPointF(450, 400));
            QMouseEvent linePress(QEvent::MouseButtonPress, QPointF(start),
                QPointF(diagram->viewport()->mapToGlobal(start)),
                Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(diagram->viewport(), &linePress);
            QMouseEvent lineMove(QEvent::MouseMove, QPointF(end),
                QPointF(diagram->viewport()->mapToGlobal(end)),
                Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(diagram->viewport(), &lineMove);
            QMouseEvent lineRelease(QEvent::MouseButtonRelease, QPointF(end),
                QPointF(diagram->viewport()->mapToGlobal(end)),
                Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(diagram->viewport(), &lineRelease);
            if (diagram->scene()->items().size() != beforeLines + 1) return 16;
            QGraphicsPathItem *firstLine = nullptr;
            for (QGraphicsItem *item : diagram->scene()->selectedItems())
                if (auto *path = dynamic_cast<QGraphicsPathItem *>(item))
                    if (item != router && item != shape) firstLine = path;
            if (!firstLine || firstLine->path().elementCount() < 2) return 45;
            const QPointF lineAnchor = firstLine->path().elementAt(1);
            const QPointF routerAnchor = router->mapToScene(routerPath->path().boundingRect().bottomRight());
            const QPoint snappedStart = diagram->mapFromScene(lineAnchor) + QPoint(4, -3);
            const QPoint snappedEnd = diagram->mapFromScene(routerAnchor) + QPoint(-4, 3);
            QMouseEvent snapPress(QEvent::MouseButtonPress, QPointF(snappedStart),
                QPointF(diagram->viewport()->mapToGlobal(snappedStart)),
                Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(diagram->viewport(), &snapPress);
            QMouseEvent snapMove(QEvent::MouseMove, QPointF(snappedEnd),
                QPointF(diagram->viewport()->mapToGlobal(snappedEnd)),
                Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(diagram->viewport(), &snapMove);
            QMouseEvent snapRelease(QEvent::MouseButtonRelease, QPointF(snappedEnd),
                QPointF(diagram->viewport()->mapToGlobal(snappedEnd)),
                Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(diagram->viewport(), &snapRelease);
            QGraphicsPathItem *snappedLine = nullptr;
            for (QGraphicsItem *item : diagram->scene()->selectedItems())
                if (auto *path = dynamic_cast<QGraphicsPathItem *>(item))
                    if (item != router && item != shape) snappedLine = path;
            if (!snappedLine || snappedLine->path().elementCount() < 2 ||
                QLineF(snappedLine->path().elementAt(0), lineAnchor).length() > 0.01 ||
                QLineF(snappedLine->path().elementAt(1), routerAnchor).length() > 0.01) return 45;
            window.findChild<QAction *>("selectDiagramTool")->trigger();
            bool foundDeleteAction = false;
            QTimer::singleShot(0, [&foundDeleteAction] {
                auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                if (!menu) return;
                for (QAction *action : menu->actions())
                    if (action->text() == "Elimina oggetto") {
                        foundDeleteAction = true;
                        menu->setActiveAction(action);
                        QKeyEvent key(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
                        QApplication::sendEvent(menu, &key);
                        return;
                    }
                menu->close();
            });
            QGraphicsSceneContextMenuEvent objectMenu(QEvent::GraphicsSceneContextMenu);
            objectMenu.setPos(QPointF(80, 39));
            objectMenu.setScenePos(router->mapToScene(QPointF(80, 39)));
            objectMenu.setScreenPos(diagram->mapToGlobal(diagram->mapFromScene(objectMenu.scenePos())));
            diagram->scene()->sendEvent(router, &objectMenu);
            app.processEvents();
            if (!foundDeleteAction || diagram->scene()->items().contains(router)) return 46;
            auto *addText = window.findChild<QAction *>("addMindTextBoxAction");
            auto *fontCombo = window.findChild<QFontComboBox *>("mindFontCombo");
            auto *fontSize = window.findChild<QSpinBox *>("mindFontSize");
            auto *bold = window.findChild<QAction *>("mindBoldAction");
            auto *italic = window.findChild<QAction *>("mindItalicAction");
            auto *underline = window.findChild<QAction *>("mindUnderlineAction");
            auto *strike = window.findChild<QAction *>("mindStrikeAction");
            auto *textColor = window.findChild<QAction *>("mindTextColorAction");
            if (!addText || !fontCombo || !fontSize || !bold || !italic ||
                !underline || !strike || !textColor || !hasVisiblePixels(addText->icon())) return 17;
            addText->trigger();
            QGraphicsItem *textBox = nullptr;
            for (QGraphicsItem *item : diagram->scene()->items())
                if (item->data(0).toString() == "textBox") textBox = item;
            if (!textBox || !textBox->isSelected() || !fontCombo->isEnabled()) return 18;
            textBox->setPos(0, 0);
            auto *boxRect = dynamic_cast<QGraphicsRectItem *>(textBox);
            if (!boxRect) return 32;
            const qreal originalWidth = boxRect->rect().width();
            const qreal originalHeight = boxRect->rect().height();
            QGraphicsSceneMouseEvent boxPress(QEvent::GraphicsSceneMousePress);
            boxPress.setPos(QPointF(originalWidth, originalHeight));
            boxPress.setScenePos(QPointF(originalWidth, originalHeight));
            boxPress.setButton(Qt::LeftButton);
            boxPress.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(textBox, &boxPress);
            QGraphicsSceneMouseEvent boxMove(QEvent::GraphicsSceneMouseMove);
            boxMove.setPos(QPointF(originalWidth + 40, originalHeight + 25));
            boxMove.setScenePos(QPointF(originalWidth + 40, originalHeight + 25));
            boxMove.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(textBox, &boxMove);
            QGraphicsSceneMouseEvent boxRelease(QEvent::GraphicsSceneMouseRelease);
            boxRelease.setButton(Qt::LeftButton);
            diagram->scene()->sendEvent(textBox, &boxRelease);
            if (boxRect->rect().width() <= originalWidth ||
                boxRect->rect().height() <= originalHeight) return 33;
            QGraphicsSceneMouseEvent boxRotatePress(QEvent::GraphicsSceneMousePress);
            const qreal boxCenterX = boxRect->rect().width() / 2;
            boxRotatePress.setPos(QPointF(boxCenterX, -18));
            boxRotatePress.setScenePos(QPointF(boxCenterX, -18));
            boxRotatePress.setButton(Qt::LeftButton);
            boxRotatePress.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(textBox, &boxRotatePress);
            QGraphicsSceneMouseEvent boxRotateMove(QEvent::GraphicsSceneMouseMove);
            boxRotateMove.setPos(QPointF(boxCenterX + 45, 35));
            boxRotateMove.setScenePos(QPointF(boxCenterX + 45, 35));
            boxRotateMove.setButtons(Qt::LeftButton);
            diagram->scene()->sendEvent(textBox, &boxRotateMove);
            QGraphicsSceneMouseEvent boxRotateRelease(QEvent::GraphicsSceneMouseRelease);
            boxRotateRelease.setButton(Qt::LeftButton);
            diagram->scene()->sendEvent(textBox, &boxRotateRelease);
            if (qAbs(textBox->rotation()) < 1) return 34;
            QGraphicsTextItem *boxText = nullptr;
            for (QGraphicsItem *child : textBox->childItems())
                boxText = dynamic_cast<QGraphicsTextItem *>(child);
            if (!boxText) return 19;
            boxText->setPlainText("Testo MindNet");
            fontSize->setValue(18);
            bold->trigger();
            italic->trigger();
            underline->trigger();
            strike->trigger();
            if (fontSize->value() != 18 || boxText->toPlainText() != "Testo MindNet" ||
                !boxText->font().bold() || !boxText->font().italic() ||
                !boxText->font().underline() || !boxText->font().strikeOut()) return 20;
            window.findChild<QAction *>("template_flow")->trigger();
            app.processEvents();
            QTemporaryDir directory;
            if (!directory.isValid()) return 21;
            const QString savedPath = directory.filePath("text-box.mindnet.json");
            tabs->currentWidget()->setProperty("filePath", savedPath);
            QAction *saveDocument = nullptr;
            for (QAction *action : window.menuBar()->actions().first()->menu()->actions())
                if (action->text() == "Salva") saveDocument = action;
            if (!saveDocument) return 22;
            saveDocument->trigger();
            QFile saved(savedPath);
            if (!saved.open(QIODevice::ReadOnly)) return 23;
            const QJsonDocument json = QJsonDocument::fromJson(saved.readAll());
            if (json.object().value("textBoxes").toArray().size() != 1 ||
                !json.object().value("textBoxes").toArray().first().toObject()
                    .value("html").toString().contains("Testo MindNet")) return 24;
            const QJsonObject savedBox = json.object().value("textBoxes").toArray().first().toObject();
            if (savedBox.value("width").toDouble() <= originalWidth ||
                savedBox.value("height").toDouble() <= originalHeight ||
                qAbs(savedBox.value("rotation").toDouble()) < 1) return 35;
            if (json.object().value("paper").toString() != "A3" ||
                !json.object().value("landscape").toBool() ||
                !json.object().value("rulersVisible").toBool() ||
                !json.object().value("gridVisible").toBool() ||
                json.object().value("rulerUnit").toString() != "cm" ||
                json.object().value("gridUnit").toString() != "px" ||
                json.object().value("rulerStep").toDouble() != 2.5 ||
                json.object().value("gridStep").toDouble() != 25 ||
                json.object().value("nodes").toArray().size() < 4) return 29;
            bool savedLabelGeometry = false;
            for (const QJsonValue &value : json.object().value("nodes").toArray()) {
                const QJsonObject label = value.toObject().value("labelBox").toObject();
                if (qAbs(label.value("rotation").toDouble() - 27) < 0.01 &&
                    label.value("width").toDouble() > labelWidth &&
                    qAbs(label.value("x").toDouble() - 23) < 0.01 &&
                    value.toObject().value("labelFontSize").toInt() == 20)
                    savedLabelGeometry = true;
            }
            if (!savedLabelGeometry) return 44;
            if (!window.openFile(savedPath)) return 25;
            if (pageFormat->currentText() != "A3" || !orientation->isChecked() ||
                !rulers->isChecked() || !grid->isChecked() ||
                rulerUnit->currentText() != "cm" || gridUnit->currentText() != "px") return 30;
            auto *reopenedView = tabs->currentWidget()->findChild<QGraphicsView *>("diagramView");
            bool foundReopenedText = false;
            bool foundReopenedLabel = false;
            for (QGraphicsItem *item : reopenedView->scene()->items()) {
                if (item->data(0).toString() == "nodeLabel" &&
                    qAbs(item->rotation() - 27) < 0.01 &&
                    qAbs(item->pos().x() - 23) < 0.01)
                    for (QGraphicsItem *child : item->childItems())
                        if (auto *text = dynamic_cast<QGraphicsTextItem *>(child))
                            foundReopenedLabel = text->font().pointSize() == 20;
                if (item->data(0).toString() != "textBox") continue;
                auto *reopenedBox = dynamic_cast<QGraphicsRectItem *>(item);
                if (!reopenedBox || qAbs(item->rotation() - savedBox.value("rotation").toDouble()) > 0.01 ||
                    qAbs(reopenedBox->rect().width() - savedBox.value("width").toDouble()) > 0.01)
                    return 36;
                for (QGraphicsItem *child : item->childItems())
                    if (auto *text = dynamic_cast<QGraphicsTextItem *>(child))
                        foundReopenedText = text->toPlainText() == "Testo MindNet";
            }
            if (!foundReopenedText || !foundReopenedLabel) return 26;
            app.processEvents();
            if (argc > 1 && !reopenedView->grab().save(QString::fromLocal8Bit(argv[1]))) return 31;
        } else {
            auto *editor = tabs->currentWidget()->findChild<QTextEdit *>("pageEditor");
            if (!editor) return 4;
            editor->insertPlainText("NebiPad+ suite");
            if (editor->toPlainText() != "NebiPad+ suite") return 5;
            if (mode == SuiteMode::Impagination) {
                auto *dictation = window.findChild<QAction *>("impDictationAction");
                auto *dictationOverlay = window.findChild<QWidget *>("dictationOverlay");
                if (!dictation || !dictationOverlay ||
                    dictation->shortcut() != QKeySequence(Qt::Key_F12) ||
                    dictation->shortcutContext() != Qt::ApplicationShortcut ||
                    !hasVisiblePixels(dictation->icon())) return 128;
                auto *recordAudio = window.findChild<QAction *>("impRecordAudioAction");
                auto *playAudio = window.findChild<QAction *>("impPlayPauseAudioAction");
                auto *stopAudio = window.findChild<QAction *>("impStopAudioAction");
                auto *gain = window.findChild<QSpinBox *>("impRecordingGainControl");
                if (!recordAudio || !playAudio || !stopAudio || !gain ||
                    !hasVisiblePixels(recordAudio->icon()) || !hasVisiblePixels(playAudio->icon()) ||
                    !hasVisiblePixels(stopAudio->icon())) return 129;
                auto *readOnlyTab = qobject_cast<QToolButton *>(
                    tabs->tabBar()->tabButton(tabs->currentIndex(), QTabBar::LeftSide));
                auto *tabControls = tabs->tabBar()->tabButton(tabs->currentIndex(), QTabBar::RightSide);
                auto *pinTab = tabControls ? tabControls->findChild<QToolButton *>("impTabPinButton") : nullptr;
                auto *closeTab = tabControls ? tabControls->findChild<QToolButton *>("impTabCloseButton") : nullptr;
                auto *initialPageEditor = tabs->currentWidget()->findChild<QTextEdit *>("pageEditor");
                if (!readOnlyTab || !pinTab || !closeTab || !initialPageEditor || pinTab->x() >= closeTab->x()) return 130;
                readOnlyTab->click(); if (!initialPageEditor->isReadOnly()) return 131;
                readOnlyTab->click(); if (initialPageEditor->isReadOnly()) return 132;
                pinTab->click(); const int protectedCount = tabs->count(); closeTab->click();
                if (tabs->count() != protectedCount) return 133;
                pinTab->click();
                auto *pdfMenu = window.findChild<QMenu *>("pdfMenu");
                if (!pdfMenu || !window.findChild<QAction *>("pdfAddTextAction") ||
                    !window.findChild<QAction *>("pdfAddImageAction") ||
                    !window.findChild<QAction *>("pdfInsertPageAction") ||
                    !window.findChild<QAction *>("pdfDeletePageAction") ||
                    !window.findChild<QAction *>("pdfRotatePageAction") ||
                    !window.findChild<QAction *>("pdfMovePageAction") ||
                    !window.findChild<QAction *>("pdfMergeAction") ||
                    !window.findChild<QAction *>("pdfMetadataAction")) return 119;
                QImage sample(40, 30, QImage::Format_ARGB32);
                sample.fill(Qt::red);
                ImageEditDialog imageDialog(sample);
                imageDialog.setCropRect(QRect(0, 0, 20, 20));
                imageDialog.setImageSize(QSize(80, 40));
                imageDialog.setAngle(90);
                if (imageDialog.resultImage().size() != QSize(40, 80)) return 66;
                ImageEditDialog interactiveImage(sample);
                auto *ratioLock = interactiveImage.findChild<QCheckBox *>("impImageKeepAspectRatio");
                auto *imageWidth = interactiveImage.findChild<QSpinBox *>("impImageWidth");
                auto *imageHeight = interactiveImage.findChild<QSpinBox *>("impImageHeight");
                if (!ratioLock || !ratioLock->isChecked() || !imageWidth || !imageHeight) return 91;
                imageWidth->setValue(80);
                if (interactiveImage.imageSize() != QSize(80, 60)) return 92;
                imageHeight->setValue(30);
                if (interactiveImage.imageSize() != QSize(40, 30)) return 93;
                ratioLock->setChecked(false);
                imageWidth->setValue(60);
                if (interactiveImage.imageSize() != QSize(60, 30)) return 94;
                ratioLock->setChecked(true);
                interactiveImage.setImageSize(QSize(40, 30));
                interactiveImage.show();
                app.processEvents();
                auto *preview = interactiveImage.findChild<QWidget *>("impImagePreview");
                if (!preview) return 69;
                const auto previewMouse = [preview](QEvent::Type type, QPointF point,
                    Qt::MouseButton button, Qt::MouseButtons buttons) {
                    QMouseEvent event(type, point,
                        QPointF(preview->mapToGlobal(point.toPoint())),
                        button, buttons, Qt::NoModifier);
                    QApplication::sendEvent(preview, &event);
                };
                const QPointF imageCenter(preview->width() / 2.0, preview->height() / 2.0);
                const QPointF resizeHandle = imageCenter + QPointF(20, 15);
                previewMouse(QEvent::MouseButtonPress, resizeHandle, Qt::LeftButton, Qt::LeftButton);
                previewMouse(QEvent::MouseMove, resizeHandle + QPointF(20, 10), Qt::NoButton, Qt::LeftButton);
                previewMouse(QEvent::MouseButtonRelease, resizeHandle + QPointF(20, 10), Qt::LeftButton, Qt::NoButton);
                if (interactiveImage.imageSize().width() <= 40) return 70;
                const QPointF rotateHandle = imageCenter + QPointF(0, -interactiveImage.imageSize().height() / 2.0 - 28);
                previewMouse(QEvent::MouseButtonPress, rotateHandle, Qt::LeftButton, Qt::LeftButton);
                previewMouse(QEvent::MouseMove, imageCenter + QPointF(80, 0), Qt::NoButton, Qt::LeftButton);
                previewMouse(QEvent::MouseButtonRelease, imageCenter + QPointF(80, 0), Qt::LeftButton, Qt::NoButton);
                if (qAbs(interactiveImage.angle()) < 45) return 71;
                interactiveImage.findChild<QPushButton *>("impCropImageAction")->click();
                const QPointF cropStart = imageCenter - QPointF(18, 12);
                previewMouse(QEvent::MouseButtonPress, cropStart, Qt::LeftButton, Qt::LeftButton);
                previewMouse(QEvent::MouseMove, imageCenter, Qt::NoButton, Qt::LeftButton);
                previewMouse(QEvent::MouseButtonRelease, imageCenter, Qt::LeftButton, Qt::NoButton);
                interactiveImage.findChild<QPushButton *>("impApplyCropAction")->click();
                if (interactiveImage.cropRect().width() >= sample.width() ||
                    interactiveImage.cropRect().height() >= sample.height()) return 72;
                interactiveImage.close();
                auto *insert = window.findChild<QMenu *>("impInsertMenu");
                auto *layout = window.findChild<QMenu *>("impLayoutMenu");
                auto *style = window.findChild<QComboBox *>("impStyleCombo");
                auto *fontSize = window.findChild<QSpinBox *>("impFontSize");
                auto *bullets = window.findChild<QAction *>("impBulletAction");
                auto *landscape = window.findChild<QAction *>("impLandscapeAction");
                auto *tableMenu = window.findChild<QMenu *>("impTableMenu");
                auto *tableAction = window.findChild<QAction *>("impInsertTableAction");
                if (!insert || !layout || !style || !fontSize || !bullets || !landscape ||
                    !tableAction || !tableMenu || !tableMenu->actions().contains(tableAction) ||
                    !window.findChild<QAction *>("impInsertImageAction") ||
                    !window.findChild<QAction *>("impInsertLinkAction") ||
                    !window.findChild<QAction *>("impFindAction") ||
                    !window.findChild<QAction *>("impReplaceAction") ||
                    !window.findChild<QAction *>("impExportPdfAction")) return 54;
                for (const char *name : {"impDocumentToolbar", "impAudioToolbar", "impPageToolbar",
                                         "impTextToolbar", "impParagraphToolbar", "impInsertToolbar",
                                         "impLayoutToolbar"})
                    if (!window.findChild<QToolBar *>(name)) return 134;
                style->setCurrentIndex(3);
                if (editor->textCursor().blockFormat().headingLevel() != 1) return 55;
                fontSize->setValue(19);
                if (editor->currentCharFormat().fontPointSize() != 19) return 56;
                QTextCursor cursor = editor->textCursor();
                cursor.movePosition(QTextCursor::End);
                cursor.insertBlock();
                editor->setTextCursor(cursor);
                bullets->trigger();
                if (!editor->textCursor().currentList()) return 57;
                editor->textCursor().insertImage(sample, "embedded.png");
                auto *editImageAction = window.findChild<QAction *>("impEditImageAction");
                auto *cropImageAction = window.findChild<QAction *>("impCropImageMenuAction");
                if (!editImageAction || !cropImageAction) return 67;
                bool openedImageDialog = false;
                QTimer::singleShot(0, [&openedImageDialog] {
                    auto *dialog = dynamic_cast<ImageEditDialog *>(QApplication::activeModalWidget());
                    if (!dialog) return;
                    openedImageDialog = true;
                    dialog->setCropRect(QRect(0, 0, 20, 20));
                    dialog->setImageSize(QSize(80, 40));
                    dialog->setAngle(90);
                    dialog->accept();
                });
                editImageAction->trigger();
                if (!openedImageDialog) return 68;
                auto *wrapLeft = window.findChild<QAction *>("impWrapLeftAction");
                auto *wrapRight = window.findChild<QAction *>("impWrapRightAction");
                auto *wrapInline = window.findChild<QAction *>("impWrapInlineAction");
                if (!wrapLeft || !wrapRight || !wrapInline) return 83;
                const auto focusImage = [editor] {
                    for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next())
                        for (auto it = block.begin(); !it.atEnd(); ++it)
                            if (it.fragment().isValid() && it.fragment().charFormat().isImageFormat()) {
                                QTextCursor imageCursor(editor->document());
                                imageCursor.setPosition(it.fragment().position() + 1);
                                editor->setTextCursor(imageCursor);
                                return;
                            }
                };
                wrapLeft->trigger();
                focusImage();
                if (editor->toHtml().count("<img") != 1) return 95;
                if (editor->textCursor().currentFrame()->frameFormat().position() != QTextFrameFormat::FloatLeft) return 84;
                wrapRight->trigger();
                focusImage();
                if (editor->textCursor().currentFrame()->frameFormat().position() != QTextFrameFormat::FloatRight) return 85;
                wrapInline->trigger();
                focusImage();
                if (editor->textCursor().currentFrame()->frameFormat().position() != QTextFrameFormat::InFlow) return 86;
                wrapLeft->trigger();
                editor->insertPlainText(QString("Testo accanto all'immagine. ").repeated(70));
                const QTextBlock wrappedText = editor->textCursor().block();
                if (wrappedText.layout()->lineCount() < 4 ||
                    wrappedText.layout()->lineAt(0).position().x() < 20 ||
                    wrappedText.layout()->lineAt(1).position().x() < 20 ||
                    wrappedText.layout()->lineAt(wrappedText.layout()->lineCount() - 1).position().x() > 1)
                    return 97;
                auto *references = window.findChild<QMenu *>("impReferencesMenu");
                auto *toc = window.findChild<QAction *>("impTableOfContentsAction");
                auto *bookmark = window.findChild<QAction *>("impInsertBookmarkAction");
                auto *endnote = window.findChild<QAction *>("impInsertEndnoteAction");
                auto *findNext = window.findChild<QAction *>("impFindNextAction");
                auto *findPrevious = window.findChild<QAction *>("impFindPreviousAction");
                if (!references || !toc || !bookmark || !endnote ||
                    !findNext || !findPrevious ||
                    !window.findChild<QAction *>("impCellColorAction") ||
                    !window.findChild<QAction *>("impTableBorderAction")) return 73;
                toc->trigger();
                if (!editor->toPlainText().contains("Indice") ||
                    !editor->toHtml().contains("nebi-heading-")) return 74;
                auto *refreshToc = window.findChild<QAction *>("impRefreshTableOfContentsAction");
                if (!refreshToc) return 81;
                refreshToc->trigger();
                if (editor->toPlainText().count("Indice") != 1) return 82;
                QTimer::singleShot(0, [] {
                    auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
                    if (dialog) { dialog->setTextValue("inizio"); dialog->accept(); }
                });
                bookmark->trigger();
                if (!editor->toHtml().contains("inizio")) return 75;
                QTimer::singleShot(0, [] {
                    auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
                    if (dialog) { dialog->setTextValue("Nota di prova"); dialog->accept(); }
                });
                endnote->trigger();
                if (!editor->toPlainText().contains("Nota di prova") ||
                    editor->document()->property("endnoteCount").toInt() != 1) return 76;
                editor->setProperty("findQuery", "NebiPad+");
                findNext->trigger();
                if (editor->textCursor().selectedText() != "NebiPad+") return 77;
                landscape->trigger();
                auto *paperWidget = editor->parentWidget();
                auto *headerToggle = window.findChild<QAction *>("impHeaderToggle");
                auto *footerToggle = window.findChild<QAction *>("impFooterToggle");
                auto *pageNumberToggle = window.findChild<QAction *>("impPageNumberToggle");
                auto *header = paperWidget->findChild<QLineEdit *>("impHeaderEdit");
                auto *footer = paperWidget->findChild<QLineEdit *>("impFooterEdit");
                auto *pageNumber = paperWidget->findChild<QLabel *>("impPageNumberLabel");
                if (!headerToggle || !footerToggle || !pageNumberToggle || !header || !footer || !pageNumber) return 87;
                headerToggle->trigger(); footerToggle->trigger(); pageNumberToggle->trigger();
                header->setText("Titolo documento");
                footer->setText("Costa Fabio");
                if (!header->isVisible() || !footer->isVisible() || !pageNumber->isVisible()) return 88;
                footerToggle->trigger();
                if (footer->isVisible() || !header->isVisible() || !pageNumber->isVisible()) return 89;
                footerToggle->trigger();
                if (!paperWidget->property("landscape").toBool() ||
                    paperWidget->size() != QSize(1123, 794)) return 58;
                layout->actions().at(1)->menu()->actions().first()->trigger();
                if (paperWidget->property("margin").toInt() != 40) return 59;
                QTemporaryDir directory;
                if (!directory.isValid()) return 60;
                const QString path = directory.filePath("impagination.html");
                tabs->currentWidget()->setProperty("filePath", path);
                QAction *saveDocument = nullptr;
                for (QAction *action : window.menuBar()->actions().first()->menu()->actions())
                    if (action->text() == "Salva") saveDocument = action;
                if (!saveDocument) return 61;
                saveDocument->trigger();
                QFile saved(path);
                if (!saved.open(QIODevice::ReadOnly)) return 62;
                const QByteArray html = saved.readAll();
                if (!html.startsWith("<!--NEBIPAD_LAYOUT:") ||
                    !html.contains("NebiPad+ suite") ||
                    !html.contains("data:image/png;base64,") ||
                    !html.contains("float: left") ||
                    !html.contains("Titolo documento") ||
                    !html.contains("Costa Fabio") ||
                    html.count("<img") != 1) return 63;
                if (!window.openFile(path)) return 64;
                editor = tabs->currentWidget()->findChild<QTextEdit *>("pageEditor");
                if (!editor || !editor->parentWidget()->property("landscape").toBool() ||
                    editor->parentWidget()->property("margin").toInt() != 40 ||
                    !editor->toPlainText().contains("NebiPad+ suite") ||
                    editor->toHtml().count("<img") != 1 ||
                    editor->document()->find(QString(QChar::ObjectReplacementCharacter)).isNull() ||
                    !editor->parentWidget()->findChild<QLineEdit *>("impHeaderEdit")->isVisible() ||
                    !editor->parentWidget()->findChild<QLineEdit *>("impFooterEdit")->isVisible() ||
                    !editor->parentWidget()->findChild<QLabel *>("impPageNumberLabel")->isVisible() ||
                    editor->document()->property("endnoteCount").toInt() != 1 ||
                    !editor->toPlainText().contains("Nota di prova")) return 65;
                bool restoredFloat = false;
                for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next())
                    for (auto it = block.begin(); !it.atEnd(); ++it)
                        if (it.fragment().isValid() && it.fragment().charFormat().isImageFormat())
                        {
                            QTextCursor imageCursor(editor->document());
                            imageCursor.setPosition(it.fragment().position());
                            restoredFloat = imageCursor.currentFrame()->frameFormat().position()
                                == QTextFrameFormat::FloatLeft;
                        }
                if (!restoredFloat) return 90;
                auto *goTo = window.findChild<QMenu *>("impGoToBookmarkMenu");
                if (!goTo) return 78;
                goTo->popup(window.mapToGlobal(QPoint(30, 30)));
                app.processEvents();
                QAction *savedBookmark = nullptr;
                for (QAction *action : goTo->actions())
                    if (action->text() == "inizio") savedBookmark = action;
                goTo->hide();
                if (!savedBookmark) return 79;
                savedBookmark->trigger();
                if (editor->textCursor().position() <= 0) return 80;
                auto *localVideo = window.findChild<QAction *>("impInsertLocalVideoAction");
                auto *youtubeVideo = window.findChild<QAction *>("impInsertYouTubeVideoAction");
                if (!localVideo || !youtubeVideo) return 98;
                QTimer::singleShot(0, [] {
                    auto *dialog = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
                    if (dialog) { dialog->setTextValue("https://youtu.be/M7lc1UVf-VE"); dialog->accept(); }
                });
                youtubeVideo->trigger();
                if (!editor->toHtml().contains("nebi-video:youtube:M7lc1UVf-VE")) return 99;
                const QString videoPath = directory.filePath("clip.mp4");
                QFile videoFile(videoPath);
                if (!videoFile.open(QIODevice::WriteOnly) || videoFile.write("video-test") != 10) return 100;
                videoFile.close();
                QTextImageFormat localPoster;
                localPoster.setName("test-video-poster.png");
                localPoster.setWidth(80); localPoster.setHeight(60);
                localPoster.setAnchor(true);
                localPoster.setAnchorHref("nebi-video:local:" + QString::fromLatin1(
                    videoPath.toUtf8().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
                editor->document()->addResource(QTextDocument::ImageResource, QUrl(localPoster.name()), sample);
                editor->textCursor().insertImage(localPoster);
                saveDocument->trigger();
                if (QDir(directory.filePath("impagination_media")).entryList(QDir::Files).isEmpty()) return 101;
                QFile updated(path);
                if (!updated.open(QIODevice::ReadOnly) || !updated.readAll().contains("nebi-video:local:")) return 102;
                QTextCursor movable(editor->document());
                for (QTextBlock block = editor->document()->begin(); block.isValid() && movable.position() == 0;
                     block = block.next())
                    for (auto it = block.begin(); !it.atEnd(); ++it)
                        if (it.fragment().isValid() && it.fragment().charFormat().isImageFormat() &&
                            it.fragment().charFormat().anchorHref().isEmpty()) {
                            movable.setPosition(it.fragment().position());
                            movable.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                            break;
                        }
                if (!movable.hasSelection()) return 103;
                editor->setTextCursor(movable);
                editor->ensureCursorVisible();
                app.processEvents();
                QTextCursor imageStart = movable;
                imageStart.setPosition(movable.selectionStart());
                const QPoint imagePoint = editor->cursorRect(imageStart).topLeft() + QPoint(8, 8);
                bool moveMenuFound = false;
                QTimer::singleShot(0, [&moveMenuFound] {
                    auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
                    if (!menu) return;
                    if (auto *move = menu->findChild<QAction *>("impMoveImageAction")) {
                        moveMenuFound = true;
                        move->trigger();
                    }
                    menu->close();
                });
                QContextMenuEvent context(QContextMenuEvent::Mouse, imagePoint,
                                          editor->viewport()->mapToGlobal(imagePoint));
                QApplication::sendEvent(editor->viewport(), &context);
                if (!moveMenuFound) return 104;
                auto imageMouse = [editor](QEvent::Type type, QPoint point,
                    Qt::MouseButton button, Qt::MouseButtons buttons) {
                    QMouseEvent event(type, point, editor->viewport()->mapToGlobal(point),
                                      button, buttons, Qt::NoModifier);
                    QApplication::sendEvent(editor->viewport(), &event);
                };
                imageMouse(QEvent::MouseButtonPress, imagePoint, Qt::LeftButton, Qt::LeftButton);
                imageMouse(QEvent::MouseMove, imagePoint + QPoint(100, 100), Qt::NoButton, Qt::LeftButton);
                imageMouse(QEvent::MouseButtonRelease, imagePoint + QPoint(100, 100), Qt::LeftButton, Qt::NoButton);
                if (editor->toHtml().count("<img") != 3) return 105;
                bool moved = false;
                for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next())
                    for (auto it = block.begin(); !it.atEnd(); ++it)
                        if (it.fragment().isValid() && it.fragment().charFormat().isImageFormat() &&
                            it.fragment().charFormat().anchorHref().isEmpty()) {
                            QTextCursor imageCursor(editor->document());
                            imageCursor.setPosition(it.fragment().position());
                            const auto frame = imageCursor.currentFrame();
                            moved = frame != editor->document()->rootFrame() &&
                                (frame->frameFormat().leftMargin() > 20 ||
                                 frame->frameFormat().rightMargin() > 20);
                        }
                if (!moved) return 106;
                saveDocument->trigger();
                if (!window.openFile(path)) return 107;
                editor = tabs->currentWidget()->findChild<QTextEdit *>("pageEditor");
                bool restoredPosition = false;
                for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next())
                    for (auto it = block.begin(); !it.atEnd(); ++it)
                        if (it.fragment().isValid() && it.fragment().charFormat().isImageFormat() &&
                            it.fragment().charFormat().anchorHref().isEmpty()) {
                            QTextCursor imageCursor(editor->document());
                            imageCursor.setPosition(it.fragment().position());
                            restoredPosition = imageCursor.currentFrame()->frameFormat().leftMargin() > 20;
                        }
                if (!restoredPosition) return 108;
                if (!editor->toHtml().contains("nebi-video:youtube:M7lc1UVf-VE") ||
                    !editor->toHtml().contains("nebi-video:local:")) return 109;
                auto *chartAction = window.findChild<QAction *>("impInsertChartAction");
                if (!chartAction) return 112;
                bool chartDialogOpened = false;
                QTimer::singleShot(0, [&chartDialogOpened] {
                    auto *dialog = QApplication::activeModalWidget();
                    if (!dialog || dialog->objectName() != "impChartDialog") return;
                    chartDialogOpened = true;
                    if (auto *type = dialog->findChild<QComboBox *>("impChartType"))
                        type->setCurrentIndex(4);
                    if (auto *title = dialog->findChild<QLineEdit *>("impChartTitle"))
                        title->setText("Vendite annuali");
                    if (auto *buttons = dialog->findChild<QDialogButtonBox *>())
                        buttons->button(QDialogButtonBox::Ok)->click();
                });
                const int imagesBeforeChart = editor->toHtml().count("<img");
                chartAction->trigger();
                if (!chartDialogOpened || editor->toHtml().count("<img") != imagesBeforeChart + 1 ||
                    !editor->toHtml().contains("Grafico: Vendite annuali")) return 113;
                QTextCursor chartCursor(editor->document());
                qreal initialChartWidth = 0;
                qreal initialChartHeight = 0;
                for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next())
                    for (auto it = block.begin(); !it.atEnd(); ++it)
                        if (it.fragment().isValid() && it.fragment().charFormat().isImageFormat() &&
                            it.fragment().charFormat().toImageFormat().name().startsWith("nebi-chart-")) {
                            chartCursor.setPosition(it.fragment().position());
                            chartCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
                            initialChartWidth = it.fragment().charFormat().toImageFormat().width();
                            initialChartHeight = it.fragment().charFormat().toImageFormat().height();
                        }
                const qreal writablePageHeight = editor->parentWidget()->height() -
                    2 * editor->parentWidget()->property("margin").toDouble();
                if (!chartCursor.hasSelection() || initialChartWidth < 600 ||
                    initialChartHeight < writablePageHeight * 0.45 ||
                    initialChartWidth > editor->parentWidget()->width()) return 114;
                QTextCursor chartEnd = chartCursor;
                chartEnd.setPosition(chartCursor.selectionEnd());
                QTextCursor chartBeginning = chartCursor;
                chartBeginning.setPosition(chartCursor.selectionStart());
                const QRect chartBefore = editor->cursorRect(chartBeginning);
                const QRect chartAfter = editor->cursorRect(chartEnd);
                if (qAbs((chartAfter.left() - chartBefore.left()) - initialChartWidth) > 2 ||
                    qAbs(chartBefore.height() - initialChartHeight) > 2) return 135;
                editor->setTextCursor(chartCursor);
                editor->ensureCursorVisible(); app.processEvents();
                QTextCursor chartStart = chartCursor;
                chartStart.setPosition(chartCursor.selectionStart());
                const QPoint chartTopLeft = editor->cursorRect(chartStart).topLeft();
                auto chartMouse = [editor](QEvent::Type eventType, QPoint point,
                    Qt::MouseButton button, Qt::MouseButtons pressed) {
                    QMouseEvent event(eventType, point, editor->viewport()->mapToGlobal(point),
                                      button, pressed, Qt::NoModifier);
                    QApplication::sendEvent(editor->viewport(), &event);
                };
                chartMouse(QEvent::MouseButtonPress, chartTopLeft + QPoint(100, 100),
                           Qt::LeftButton, Qt::LeftButton);
                chartMouse(QEvent::MouseButtonRelease, chartTopLeft + QPoint(100, 100),
                           Qt::LeftButton, Qt::NoButton);
                const QPoint bottomRight = chartTopLeft +
                    QPoint(qRound(initialChartWidth), qRound(initialChartHeight));
                chartMouse(QEvent::MouseButtonPress, bottomRight, Qt::LeftButton, Qt::LeftButton);
                chartMouse(QEvent::MouseMove, bottomRight + QPoint(80, 48), Qt::NoButton, Qt::LeftButton);
                chartMouse(QEvent::MouseButtonRelease, bottomRight + QPoint(80, 48),
                           Qt::LeftButton, Qt::NoButton);
                bool chartResized = false;
                for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next())
                    for (auto it = block.begin(); !it.atEnd(); ++it)
                        if (it.fragment().isValid() && it.fragment().charFormat().isImageFormat() &&
                            it.fragment().charFormat().toImageFormat().name().startsWith("nebi-chart-"))
                            chartResized = it.fragment().charFormat().toImageFormat().width()
                                > initialChartWidth + 50;
                if (!chartResized) return 115;
                const qreal enlargedWidth = initialChartWidth + 80;
                const qreal enlargedHeight = enlargedWidth * initialChartHeight / initialChartWidth;
                const QPoint enlargedBottomRight = chartTopLeft +
                    QPoint(qRound(enlargedWidth), qRound(enlargedHeight));
                chartMouse(QEvent::MouseButtonPress, enlargedBottomRight, Qt::LeftButton, Qt::LeftButton);
                const qreal reducedWidth = initialChartWidth - 80;
                const QPoint reducedBottomRight = chartTopLeft + QPoint(qRound(reducedWidth),
                    qRound(reducedWidth * initialChartHeight / initialChartWidth));
                chartMouse(QEvent::MouseMove, reducedBottomRight, Qt::NoButton, Qt::LeftButton);
                chartMouse(QEvent::MouseButtonRelease, reducedBottomRight,
                           Qt::LeftButton, Qt::NoButton);
                bool chartReduced = false;
                for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next())
                    for (auto it = block.begin(); !it.atEnd(); ++it)
                        if (it.fragment().isValid() && it.fragment().charFormat().isImageFormat() &&
                            it.fragment().charFormat().toImageFormat().name().startsWith("nebi-chart-"))
                            chartReduced = it.fragment().charFormat().toImageFormat().width()
                                < initialChartWidth - 50;
                if (!chartReduced) return 116;
#ifdef NEBIPAD_HAS_QTPDF
                const QString pdfPath = directory.filePath("import.pdf");
                {
                    QPdfWriter writer(pdfPath);
                    writer.setResolution(72);
                    QPainter painter(&writer);
                    painter.drawText(QPoint(80, 100), "Pagina PDF 1");
                    writer.newPage();
                    painter.drawText(QPoint(80, 100), "Pagina PDF 2");
                }
                QString pdfError;
                if (!podofoAvailable() || podofoPageCount(pdfPath, &pdfError) != 2) return 120;
                if (!podofoAddText(pdfPath, {1, 50, 700, 14, "Test PoDoFo"}, &pdfError)) return 122;
                if (!podofoInsertPage(pdfPath, 2, &pdfError) || podofoPageCount(pdfPath) != 3) return 123;
                if (!podofoRotatePage(pdfPath, 2, 90, &pdfError)) return 124;
                if (!podofoMovePage(pdfPath, 2, 3, &pdfError)) return 125;
                if (!podofoDeletePage(pdfPath, 3, &pdfError) || podofoPageCount(pdfPath) != 2) return 126;
                if (!podofoSetMetadata(pdfPath, {"Titolo", "Autore", "Oggetto", "test,pdf"}, &pdfError)) return 127;
                const QString overlayPath = directory.filePath("pdf-overlay.png");
                QImage overlay(80, 40, QImage::Format_ARGB32_Premultiplied); overlay.fill(Qt::blue);
                if (!overlay.save(overlayPath) ||
                    !podofoAddImage(pdfPath, {1, 30, 30, 80, 40, overlayPath}, &pdfError)) return 121;
                SuiteWindow pdfWindow(SuiteMode::Impagination);
                if (!pdfWindow.openFile(pdfPath)) return 117;
                auto *pdfTabs = pdfWindow.findChild<QTabWidget *>();
                auto *pdfEditor = pdfTabs ? pdfTabs->currentWidget()->findChild<QTextEdit *>("pageEditor") : nullptr;
                if (!pdfEditor || pdfEditor->toHtml().count("<img") != 2 ||
                    !pdfTabs->currentWidget()->property("importedPdf").toBool()) return 118;
                pdfEditor->document()->setModified(false);
#endif
            }
        }
        auto *paper = window.findChild<QComboBox *>();
        if (!paper || paper->currentText() != (mode == SuiteMode::MindNet ? "A3" : "A4")) return 6;
        if (mode == SuiteMode::Math && paper->isEnabled()) return 7;
        // Avoid the unsaved-changes dialog at destruction.
        if (auto *editor = tabs->currentWidget()->findChild<QTextEdit *>("pageEditor"))
            editor->document()->setModified(false);
    }
    return 0;
}
