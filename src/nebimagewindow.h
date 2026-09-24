#pragma once

#include <QImage>
#include <QMainWindow>

class QFileSystemModel;
class QGraphicsPixmapItem;
class QGraphicsScene;
class QGraphicsView;
class QLabel;
class QListWidget;
class QTreeView;
class QDockWidget;
class QTimer;

class NebImageWindow final : public QMainWindow
{
public:
    explicit NebImageWindow(QWidget *parent = nullptr);
    bool openFile(const QString &path);
    void openImages();

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QGraphicsScene *scene_;
    QGraphicsView *view_;
    QGraphicsPixmapItem *pixmapItem_ = nullptr;
    QListWidget *filmstrip_;
    QTreeView *folders_;
    QFileSystemModel *folderModel_;
    QDockWidget *folderDock_;
    QDockWidget *filmstripDock_;
    QDockWidget *propertiesDock_;
    QDockWidget *histogramDock_;
    QLabel *properties_;
    QWidget *histogram_;
    QLabel *statusFile_;
    QLabel *statusInfo_;
    QLabel *statusZoom_;
    QTimer *slideTimer_;
    QString path_;
    QStringList files_;
    QImage original_;
    QImage image_;
    double zoom_ = 1.0;
    bool fitMode_ = true;
    bool modified_ = false;

    void buildMenus();
    void buildToolBar();
    void setFolder(const QString &directory, const QString &selected = {});
    void setImage(const QImage &image, bool modified);
    void refreshView();
    void updatePanels();
    void setZoom(double factor);
    void fitImage();
    void fitWidth();
    void actualSize();
    void step(int delta);
    void rotate(int degrees);
    void flip(bool horizontal);
    void crop();
    void resizeImage();
    void adjustImage();
    void applyEffect(const QString &effect);
    void copyImage();
    bool save(bool saveAs = false);
    bool maybeSave();
    void printImage();
    void showImageInfo();
    void renameFile();
    void deleteFile();
    void revealFile();
    void batchConvert();
    void batchResize();
    void contactSheet();
    void toggleSlideshow();
};
