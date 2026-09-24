#include "nebimagewindow.h"
#include "recentfiles.h"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDir>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFormLayout>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QImageReader>
#include <QImageWriter>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPrintDialog>
#include <QPrinter>
#include <QProcess>
#include <QRubberBand>
#include <QSignalBlocker>
#include <QScrollBar>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QTreeView>
#include <QTimer>
#include <QWheelEvent>
#include <QVBoxLayout>

#include <cmath>

namespace {
QString imageFilter()
{
    QStringList patterns;
    for (const QByteArray &format : QImageReader::supportedImageFormats())
        patterns << QStringLiteral("*.%1").arg(QString::fromLatin1(format));
    patterns.removeDuplicates();
    return QObject::tr("Immagini (%1);;Tutti i file (*)").arg(patterns.join(' '));
}

bool isImage(const QString &path)
{
    QImageReader reader(path);
    return reader.canRead();
}

class ImageView final : public QGraphicsView
{
public:
    std::function<void(double)> wheelZoom;
    explicit ImageView(QGraphicsScene *scene, QWidget *parent = nullptr) : QGraphicsView(scene, parent)
    {
        setDragMode(ScrollHandDrag);
        setTransformationAnchor(AnchorUnderMouse);
        setResizeAnchor(AnchorViewCenter);
        setBackgroundBrush(QColor("#20242b"));
        setFrameShape(NoFrame);
    }
protected:
    void wheelEvent(QWheelEvent *event) override
    {
        if (wheelZoom) wheelZoom(event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15);
        event->accept();
    }
};

class HistogramWidget final : public QWidget
{
public:
    QImage image;
    explicit HistogramWidget(QWidget *parent = nullptr) : QWidget(parent) { setMinimumHeight(150); }
    void setImage(const QImage &value) { image = value.scaled(512, 512, Qt::KeepAspectRatio, Qt::FastTransformation); update(); }
protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this); p.fillRect(rect(), QColor("#18202a"));
        if (image.isNull()) return;
        QVector<int> r(256), g(256), b(256), l(256);
        const QImage rgb = image.convertToFormat(QImage::Format_RGB32);
        for (int y = 0; y < rgb.height(); ++y) for (int x = 0; x < rgb.width(); ++x) {
            const QRgb px = rgb.pixel(x, y); ++r[qRed(px)]; ++g[qGreen(px)]; ++b[qBlue(px)];
            ++l[qBound(0, int(.2126*qRed(px)+.7152*qGreen(px)+.0722*qBlue(px)), 255)];
        }
        int maximum = 1; for (int i = 0; i < 256; ++i) maximum = qMax(maximum, qMax(l[i], qMax(r[i], qMax(g[i], b[i]))));
        auto draw = [&](const QVector<int> &bins, QColor color) {
            QPainterPath path; path.moveTo(0, height());
            for (int i = 0; i < 256; ++i) path.lineTo(i * width() / 255.0, height() - bins[i] * (height()-8.0) / maximum);
            p.setPen(QPen(color, 1.2)); p.drawPath(path);
        };
        draw(l, QColor(225,225,225,170)); draw(r, QColor(255,75,75,190)); draw(g, QColor(70,225,130,190)); draw(b, QColor(70,145,255,210));
    }
};

class CropLabel final : public QLabel
{
public:
    QRect selection;
    QRubberBand band{QRubberBand::Rectangle, this};
    QPoint origin;
    explicit CropLabel(const QImage &image, QWidget *parent=nullptr) : QLabel(parent)
    {
        setPixmap(QPixmap::fromImage(image).scaled(900,650,Qt::KeepAspectRatio,Qt::SmoothTransformation));
        setFixedSize(pixmap().size()); setCursor(Qt::CrossCursor);
    }
protected:
    void mousePressEvent(QMouseEvent *e) override { origin=e->position().toPoint(); selection=QRect(origin,QSize()); band.setGeometry(selection); band.show(); }
    void mouseMoveEvent(QMouseEvent *e) override { selection=QRect(origin,e->position().toPoint()).normalized().intersected(rect()); band.setGeometry(selection); }
    void mouseReleaseEvent(QMouseEvent *e) override { mouseMoveEvent(e); }
};

QImage autoContrast(QImage image)
{
    image = image.convertToFormat(QImage::Format_ARGB32);
    int low = 255, high = 0;
    for (int y=0;y<image.height();++y) for(int x=0;x<image.width();++x) { const int v=qGray(image.pixel(x,y)); low=qMin(low,v); high=qMax(high,v); }
    if (high <= low) return image;
    for (int y=0;y<image.height();++y) { QRgb *line=reinterpret_cast<QRgb*>(image.scanLine(y)); for(int x=0;x<image.width();++x) {
        const QColor c=line[x]; auto scale=[&](int v){return qBound(0,(v-low)*255/(high-low),255);}; line[x]=qRgba(scale(c.red()),scale(c.green()),scale(c.blue()),c.alpha());
    }} return image;
}
}

NebImageWindow::NebImageWindow(QWidget *parent) : QMainWindow(parent),
    scene_(new QGraphicsScene(this)), view_(new ImageView(scene_, this)),
    filmstrip_(new QListWidget(this)), folders_(new QTreeView(this)),
    folderModel_(new QFileSystemModel(this)), folderDock_(new QDockWidget(tr("Cartelle"), this)),
    filmstripDock_(new QDockWidget(tr("Pellicola"), this)), propertiesDock_(new QDockWidget(tr("Proprietà"), this)),
    histogramDock_(new QDockWidget(tr("Istogramma"), this)), properties_(new QLabel(this)),
    histogram_(new HistogramWidget(this)), statusFile_(new QLabel(this)), statusInfo_(new QLabel(this)),
    statusZoom_(new QLabel(this)), slideTimer_(new QTimer(this))
{
    setObjectName("nebImageWindow"); setWindowTitle("NebImage+"); resize(1320, 840); setAcceptDrops(true);
    view_->setObjectName("imageView"); filmstrip_->setObjectName("imageFilmstrip"); folders_->setObjectName("imageFolders"); properties_->setObjectName("imageProperties"); histogram_->setObjectName("imageHistogram");
    setCentralWidget(view_); static_cast<ImageView*>(view_)->wheelZoom = [this](double value){ setZoom(zoom_ * value); };
    folderModel_->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Drives); folderModel_->setRootPath(QDir::rootPath());
    folders_->setModel(folderModel_); for(int i=1;i<folderModel_->columnCount();++i) folders_->hideColumn(i);
    folderDock_->setWidget(folders_); addDockWidget(Qt::LeftDockWidgetArea, folderDock_);
    filmstrip_->setViewMode(QListView::IconMode); filmstrip_->setIconSize(QSize(110,80)); filmstrip_->setResizeMode(QListView::Adjust);
    filmstrip_->setMovement(QListView::Static); filmstrip_->setSpacing(7); filmstripDock_->setWidget(filmstrip_); addDockWidget(Qt::BottomDockWidgetArea, filmstripDock_);
    properties_->setAlignment(Qt::AlignTop|Qt::AlignLeft); properties_->setWordWrap(true); properties_->setMargin(12); propertiesDock_->setWidget(properties_);
    addDockWidget(Qt::RightDockWidgetArea, propertiesDock_); histogramDock_->setWidget(histogram_); addDockWidget(Qt::RightDockWidgetArea, histogramDock_);
    tabifyDockWidget(propertiesDock_, histogramDock_); propertiesDock_->raise();
    statusBar()->addWidget(statusFile_, 1); statusBar()->addPermanentWidget(statusInfo_); statusBar()->addPermanentWidget(statusZoom_);
    buildMenus(); buildToolBar();
    connect(folders_, &QTreeView::clicked, this, [this](const QModelIndex &index){ const QString p=folderModel_->filePath(index); if(QFileInfo(p).isDir()) setFolder(p); });
    connect(filmstrip_, &QListWidget::itemActivated, this, [this](QListWidgetItem *item){ openFile(item->data(Qt::UserRole).toString()); });
    connect(filmstrip_, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){ openFile(item->data(Qt::UserRole).toString()); });
    connect(slideTimer_, &QTimer::timeout, this, [this]{ step(1); });
    setStyleSheet("QMainWindow{background:#20242b} QToolBar{background:#f4f7fa;border-bottom:1px solid #cad4df;spacing:3px} QDockWidget{font-weight:600} QListWidget{background:#252b33;color:#e8edf3;border:0} QListWidget::item:selected{background:#356a9c} QStatusBar{background:#edf3f8;color:#294158}");
}

void NebImageWindow::buildMenus()
{
    auto *file=menuBar()->addMenu(tr("&File"));
    file->addAction(tr("Apri immagini..."), QKeySequence::Open, this, &NebImageWindow::openImages);
    file->addAction(tr("Apri cartella..."), this, [this]{ const QString p=QFileDialog::getExistingDirectory(this,tr("Apri cartella")); if(!p.isEmpty()) setFolder(p); });
    file->addSeparator(); file->addAction(tr("Salva"), QKeySequence::Save, this, [this]{save();}); file->addAction(tr("Salva con nome..."), QKeySequence::SaveAs, this,[this]{save(true);});
    file->addAction(tr("Stampa..."), QKeySequence::Print, this, &NebImageWindow::printImage); file->addSeparator();
    file->addAction(tr("Rinomina..."), Qt::Key_F2, this, &NebImageWindow::renameFile); file->addAction(tr("Mostra nel gestore file"), this, &NebImageWindow::revealFile);
    file->addAction(tr("Elimina file"), QKeySequence::Delete, this, &NebImageWindow::deleteFile); file->addSeparator(); file->addAction(tr("Chiudi"), this, &QWidget::close);
    auto *edit=menuBar()->addMenu(tr("&Modifica")); edit->addAction(tr("Copia immagine"), QKeySequence::Copy, this,&NebImageWindow::copyImage);
    edit->addAction(tr("Ripristina originale"), this,[this]{if(!original_.isNull())setImage(original_,false);}); edit->addSeparator();
    edit->addAction(tr("Ruota a sinistra"), QKeySequence("Ctrl+L"), this,[this]{rotate(-90);}); edit->addAction(tr("Ruota a destra"), QKeySequence("Ctrl+R"), this,[this]{rotate(90);});
    edit->addAction(tr("Rifletti orizzontalmente"), this,[this]{flip(true);}); edit->addAction(tr("Rifletti verticalmente"), this,[this]{flip(false);});
    edit->addAction(tr("Ritaglia..."), this,&NebImageWindow::crop); edit->addAction(tr("Ridimensiona..."), this,&NebImageWindow::resizeImage); edit->addAction(tr("Luce e colore..."), this,&NebImageWindow::adjustImage);
    auto *effects=edit->addMenu(tr("Effetti")); for(const QString &e:{tr("Scala di grigi"),tr("Seppia"),tr("Negativo"),tr("Contrasto automatico")}) effects->addAction(e,this,[this,e]{applyEffect(e);});
    auto *view=menuBar()->addMenu(tr("&Visualizza")); view->addAction(tr("Immagine successiva"), QKeySequence(Qt::Key_Right), this,[this]{step(1);}); view->addAction(tr("Immagine precedente"), QKeySequence(Qt::Key_Left), this,[this]{step(-1);});
    view->addSeparator(); view->addAction(tr("Adatta immagine"), QKeySequence("Ctrl+0"), this,&NebImageWindow::fitImage); view->addAction(tr("Adatta larghezza"), this,&NebImageWindow::fitWidth); view->addAction(tr("Dimensioni reali"), QKeySequence("Ctrl+1"), this,&NebImageWindow::actualSize);
    view->addAction(tr("Ingrandisci"), QKeySequence::ZoomIn, this,[this]{setZoom(zoom_*1.2);}); view->addAction(tr("Riduci"), QKeySequence::ZoomOut, this,[this]{setZoom(zoom_/1.2);});
    view->addSeparator(); view->addAction(folderDock_->toggleViewAction()); view->addAction(filmstripDock_->toggleViewAction()); view->addAction(propertiesDock_->toggleViewAction()); view->addAction(histogramDock_->toggleViewAction());
    view->addAction(tr("Schermo intero"), QKeySequence(Qt::Key_F), this,[this]{isFullScreen()?showNormal():showFullScreen();}); view->addAction(tr("Presentazione"), QKeySequence(Qt::Key_S), this,&NebImageWindow::toggleSlideshow);
    auto *batch=menuBar()->addMenu(tr("&Batch")); batch->addAction(tr("Converti formato..."),this,&NebImageWindow::batchConvert); batch->addAction(tr("Ridimensiona..."),this,&NebImageWindow::batchResize); batch->addAction(tr("Crea provino a contatto..."),this,&NebImageWindow::contactSheet);
    auto *tools=menuBar()->addMenu(tr("&Strumenti")); tools->addAction(tr("Informazioni immagine..."),this,&NebImageWindow::showImageInfo);
    auto *bg=tools->addAction(tr("Colore sfondo...")); connect(bg,&QAction::triggered,this,[this]{ QColor c=QColorDialog::getColor(view_->backgroundBrush().color(),this); if(c.isValid())view_->setBackgroundBrush(c); });
    auto *help=menuBar()->addMenu(tr("?")); help->addAction(tr("Informazioni su NebImage+"),this,[this]{QMessageBox::about(this,tr("Informazioni su NebImage+"),tr("NebImage+ — visualizzatore e gestore fotografico della NebiPad+ Suite."));});
}

void NebImageWindow::buildToolBar()
{
    auto *bar=addToolBar(tr("Visualizzazione immagini")); bar->setObjectName("nebImageToolbar"); bar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    bar->addAction(QIcon(":/icons/open.svg"),tr("Apri"),this,&NebImageWindow::openImages); bar->addAction(QIcon(":/icons/save.svg"),tr("Salva"),this,[this]{save();}); bar->addAction(QIcon(":/icons/print.svg"),tr("Stampa"),this,&NebImageWindow::printImage); bar->addSeparator();
    bar->addAction(tr("◀"),this,[this]{step(-1);}); bar->addAction(tr("▶"),this,[this]{step(1);}); bar->addSeparator();
    bar->addAction(QIcon(":/icons/zoom-in.svg"),tr("Zoom +"),this,[this]{setZoom(zoom_*1.2);}); bar->addAction(QIcon(":/icons/zoom-out.svg"),tr("Zoom −"),this,[this]{setZoom(zoom_/1.2);});
    bar->addAction(tr("Adatta"),this,&NebImageWindow::fitImage); bar->addAction(tr("1:1"),this,&NebImageWindow::actualSize); bar->addSeparator();
    bar->addAction(tr("↶"),this,[this]{rotate(-90);}); bar->addAction(tr("↷"),this,[this]{rotate(90);}); bar->addAction(tr("Ritaglia"),this,&NebImageWindow::crop); bar->addAction(tr("Regola"),this,&NebImageWindow::adjustImage); bar->addSeparator();
    bar->addAction(tr("Presentazione"),this,&NebImageWindow::toggleSlideshow); bar->addAction(tr("Schermo intero"),this,[this]{isFullScreen()?showNormal():showFullScreen();});
}

void NebImageWindow::openImages()
{
    const QStringList selected=QFileDialog::getOpenFileNames(this,tr("Apri immagini"),path_.isEmpty()?QDir::homePath():QFileInfo(path_).absolutePath(),imageFilter());
    if(selected.isEmpty())return; setFolder(QFileInfo(selected.first()).absolutePath(),selected.first()); openFile(selected.first());
}

bool NebImageWindow::openFile(const QString &path)
{
    if(!maybeSave())return false; QImageReader reader(path); reader.setAutoTransform(true); const QImage loaded=reader.read();
    if(loaded.isNull()){QMessageBox::warning(this,tr("Apertura fallita"),tr("Impossibile aprire l'immagine:\n%1").arg(reader.errorString()));return false;}
    path_=QFileInfo(path).absoluteFilePath(); original_=loaded; setImage(loaded,false); setFolder(QFileInfo(path_).absolutePath(),path_); rememberRecentFile(path_,QStringLiteral("NebImage+")); return true;
}

void NebImageWindow::setFolder(const QString &directory,const QString &selected)
{
    QDir dir(directory); if(!dir.exists())return; folders_->setCurrentIndex(folderModel_->index(directory)); files_.clear(); filmstrip_->clear();
    const QFileInfoList entries=dir.entryInfoList(QDir::Files,QDir::Name|QDir::IgnoreCase);
    for(const QFileInfo &info:entries) if(isImage(info.absoluteFilePath())) { files_<<info.absoluteFilePath(); auto *item=new QListWidgetItem(info.fileName(),filmstrip_); item->setData(Qt::UserRole,info.absoluteFilePath()); QImageReader r(info.absoluteFilePath()); r.setScaledSize(QSize(110,80)); item->setIcon(QPixmap::fromImage(r.read())); if(info.absoluteFilePath()==selected)filmstrip_->setCurrentItem(item); }
}

void NebImageWindow::setImage(const QImage &image,bool modified){image_=image;modified_=modified;refreshView();updatePanels();}
void NebImageWindow::refreshView(){scene_->clear();pixmapItem_=scene_->addPixmap(QPixmap::fromImage(image_));scene_->setSceneRect(pixmapItem_->boundingRect());if(fitMode_)fitImage();else setZoom(zoom_);setWindowTitle(QString("%1%2 — NebImage+").arg(QFileInfo(path_).fileName(),modified_?" *":""));}
void NebImageWindow::updatePanels()
{
    QFileInfo f(path_); statusFile_->setText(f.fileName()); statusInfo_->setText(tr("%1 × %2 px  ·  %3 MB").arg(image_.width()).arg(image_.height()).arg(f.exists()?f.size()/1048576.0:image_.sizeInBytes()/1048576.0,0,'f',2)); statusZoom_->setText(QString::number(qRound(zoom_*100))+"%");
    QImageReader reader(path_); QString text=tr("Nome: %1\nPercorso: %2\nDimensioni: %3 × %4 px\nProfondità: %5 bit\nFormato: %6\nDimensione file: %7 KB\nModificato: %8")
        .arg(f.fileName(),f.absolutePath()).arg(image_.width()).arg(image_.height()).arg(image_.depth()).arg(QString::fromLatin1(reader.format()).toUpper()).arg(f.size()/1024.0,0,'f',1).arg(QLocale().toString(f.lastModified(), QLocale::ShortFormat));
    for(const QString &key:reader.textKeys())text+=QString("\n%1: %2").arg(key,reader.text(key)); properties_->setText(text); static_cast<HistogramWidget*>(histogram_)->setImage(image_);
}
void NebImageWindow::setZoom(double factor){if(image_.isNull())return;fitMode_=false;zoom_=qBound(.02,factor,32.0);view_->resetTransform();view_->scale(zoom_,zoom_);statusZoom_->setText(QString::number(qRound(zoom_*100))+"%");}
void NebImageWindow::fitImage(){if(image_.isNull())return;fitMode_=true;view_->fitInView(scene_->sceneRect(),Qt::KeepAspectRatio);zoom_=view_->transform().m11();statusZoom_->setText(QString::number(qRound(zoom_*100))+"%");}
void NebImageWindow::fitWidth(){if(image_.isNull())return;setZoom(qMax(.02,(view_->viewport()->width()-4.0)/image_.width()));}
void NebImageWindow::actualSize(){setZoom(1.0);}
void NebImageWindow::step(int delta){if(files_.isEmpty())return;int i=files_.indexOf(path_);if(i<0)i=0;openFile(files_[(i+delta+files_.size())%files_.size()]);}
void NebImageWindow::rotate(int degrees){if(image_.isNull())return;QTransform t;t.rotate(degrees);setImage(image_.transformed(t,Qt::SmoothTransformation),true);}
void NebImageWindow::flip(bool horizontal){if(!image_.isNull())setImage(image_.mirrored(horizontal,!horizontal),true);}

void NebImageWindow::crop()
{
    if(image_.isNull())return; QDialog d(this); d.setWindowTitle(tr("Ritaglia immagine")); QVBoxLayout layout(&d);
    auto *hint=new QLabel(tr("Trascina sull'immagine per scegliere l'area da conservare."),&d); auto *preview=new CropLabel(image_,&d); auto *scroll=new QScrollArea(&d); scroll->setWidget(preview); scroll->setWidgetResizable(false); layout.addWidget(hint); layout.addWidget(scroll);
    QDialogButtonBox box(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&d); layout.addWidget(&box); connect(&box,&QDialogButtonBox::accepted,&d,&QDialog::accept); connect(&box,&QDialogButtonBox::rejected,&d,&QDialog::reject);
    d.resize(qMin(960,preview->width()+40),qMin(760,preview->height()+100)); if(d.exec()!=QDialog::Accepted||preview->selection.width()<2||preview->selection.height()<2)return;
    const double sx=image_.width()/double(preview->width()), sy=image_.height()/double(preview->height()); QRect area(qRound(preview->selection.x()*sx),qRound(preview->selection.y()*sy),qRound(preview->selection.width()*sx),qRound(preview->selection.height()*sy)); setImage(image_.copy(area.intersected(image_.rect())),true);
}
void NebImageWindow::resizeImage()
{
    if(image_.isNull())return;QDialog d(this);d.setWindowTitle(tr("Ridimensiona immagine"));QFormLayout l(&d);QSpinBox w,h;w.setRange(1,50000);h.setRange(1,50000);w.setValue(image_.width());h.setValue(image_.height());QCheckBox keep(tr("Mantieni proporzioni"));keep.setChecked(true);l.addRow(tr("Larghezza (px)"),&w);l.addRow(tr("Altezza (px)"),&h);l.addRow(&keep);QDialogButtonBox box(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);l.addRow(&box);connect(&box,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(&box,&QDialogButtonBox::rejected,&d,&QDialog::reject);connect(&w,&QSpinBox::valueChanged,&d,[&](int v){if(keep.isChecked()){QSignalBlocker b(h);h.setValue(qRound(v*image_.height()/double(image_.width())));}});if(d.exec()==QDialog::Accepted)setImage(image_.scaled(w.value(),h.value(),keep.isChecked()?Qt::KeepAspectRatio:Qt::IgnoreAspectRatio,Qt::SmoothTransformation),true);
}
void NebImageWindow::adjustImage()
{
    if(image_.isNull())return;QDialog d(this);d.setWindowTitle(tr("Luce e colore"));QFormLayout l(&d);QSpinBox bright,contrast,saturation;for(auto *s:{&bright,&contrast,&saturation})s->setRange(-100,100);l.addRow(tr("Luminosità"),&bright);l.addRow(tr("Contrasto"),&contrast);l.addRow(tr("Saturazione"),&saturation);QDialogButtonBox box(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);l.addRow(&box);connect(&box,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(&box,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;QImage out=image_.convertToFormat(QImage::Format_ARGB32);double contrastFactor=(100.0+contrast.value())/100.0;contrastFactor*=contrastFactor;for(int y=0;y<out.height();++y){QRgb *line=reinterpret_cast<QRgb*>(out.scanLine(y));for(int x=0;x<out.width();++x){QColor col=line[x];auto ch=[&](int n){double z=n/255.0;z=((z-.5)*contrastFactor+.5)*255+bright.value()*2.55;return qBound(0,int(z),255);};QColor adjusted(ch(col.red()),ch(col.green()),ch(col.blue()),col.alpha());int h=adjusted.hsvHue(),s=adjusted.hsvSaturation(),v=adjusted.value();s=qBound(0,int(s*(1+saturation.value()/100.0)),255);adjusted.setHsv(h,s,v,col.alpha());line[x]=adjusted.rgba();}}setImage(out,true);
}
void NebImageWindow::applyEffect(const QString &effect){if(image_.isNull())return;QImage out=image_.convertToFormat(QImage::Format_ARGB32);if(effect.contains(tr("automatico"),Qt::CaseInsensitive)){setImage(autoContrast(out),true);return;}for(int y=0;y<out.height();++y){QRgb *line=reinterpret_cast<QRgb*>(out.scanLine(y));for(int x=0;x<out.width();++x){QColor c=line[x];if(effect==tr("Negativo"))c.setRgb(255-c.red(),255-c.green(),255-c.blue(),c.alpha());else {int gray=qGray(c.rgb());if(effect==tr("Seppia"))c.setRgb(qMin(255,int(gray*1.18)),qMin(255,int(gray*1.02)),qMin(255,int(gray*.78)),c.alpha());else c.setRgb(gray,gray,gray,c.alpha());}line[x]=c.rgba();}}setImage(out,true);}
void NebImageWindow::copyImage(){if(!image_.isNull())QApplication::clipboard()->setImage(image_);}

bool NebImageWindow::save(bool saveAs)
{
    if(image_.isNull())return false;QString target=path_;if(saveAs||target.isEmpty())target=QFileDialog::getSaveFileName(this,tr("Salva immagine"),target,tr("JPEG (*.jpg *.jpeg);;PNG (*.png);;WebP (*.webp);;TIFF (*.tif *.tiff);;BMP (*.bmp)"));if(target.isEmpty())return false;QImageWriter writer(target);writer.setQuality(95);if(!writer.write(image_)){QMessageBox::warning(this,tr("Salvataggio fallito"),writer.errorString());return false;}path_=QFileInfo(target).absoluteFilePath();original_=image_;modified_=false;rememberRecentFile(path_,"NebImage+");setFolder(QFileInfo(path_).absolutePath(),path_);updatePanels();return true;
}
bool NebImageWindow::maybeSave(){if(!modified_)return true;auto a=QMessageBox::warning(this,tr("Modifiche non salvate"),tr("Salvare le modifiche all'immagine corrente?"),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel);return a==QMessageBox::Discard||(a==QMessageBox::Save&&save());}
void NebImageWindow::printImage(){if(image_.isNull())return;QPrinter printer(QPrinter::HighResolution);QPrintDialog dialog(&printer,this);if(dialog.exec()!=QDialog::Accepted)return;QPainter p(&printer);QRectF page=printer.pageLayout().paintRectPixels(printer.resolution());QSizeF size=image_.size();size.scale(page.size(),Qt::KeepAspectRatio);p.drawImage(QRectF(page.center()-QPointF(size.width()/2,size.height()/2),size),image_);}
void NebImageWindow::showImageInfo(){propertiesDock_->show();propertiesDock_->raise();}
void NebImageWindow::renameFile(){if(path_.isEmpty())return;bool ok=false;QString name=QInputDialog::getText(this,tr("Rinomina"),tr("Nuovo nome:"),QLineEdit::Normal,QFileInfo(path_).fileName(),&ok);if(!ok||name.isEmpty())return;QString target=QFileInfo(path_).dir().filePath(name);if(QFile::rename(path_,target)){path_=target;setFolder(QFileInfo(path_).absolutePath(),path_);updatePanels();}}
void NebImageWindow::deleteFile(){if(path_.isEmpty()||QMessageBox::question(this,tr("Elimina file"),tr("Spostare questa immagine nel cestino?"))!=QMessageBox::Yes)return;QString old=path_;if(QFile::moveToTrash(old)){forgetRecentFile(old);modified_=false;int i=files_.indexOf(old);files_.removeAll(old);path_.clear();if(!files_.isEmpty())openFile(files_[qBound(0,i,files_.size()-1)]);else{image_=QImage();scene_->clear();updatePanels();}}}
void NebImageWindow::revealFile(){if(path_.isEmpty())return;
#ifdef Q_OS_MAC
    QProcess::startDetached("open",{"-R",path_});
#elif defined(Q_OS_WIN)
    QProcess::startDetached("explorer",{"/select,",QDir::toNativeSeparators(path_)});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path_).absolutePath()));
#endif
}
void NebImageWindow::batchConvert(){QStringList input=QFileDialog::getOpenFileNames(this,tr("Seleziona immagini"),{},imageFilter());if(input.isEmpty())return;QString dir=QFileDialog::getExistingDirectory(this,tr("Cartella di destinazione"));if(dir.isEmpty())return;QString format=QInputDialog::getItem(this,tr("Converti formato"),tr("Formato:"),{"jpg","png","webp","bmp","tiff"},0,false);for(const QString&p:input){QImageReader r(p);QImage im=r.read();if(!im.isNull()){QString out=QDir(dir).filePath(QFileInfo(p).completeBaseName()+"."+format);QImageWriter w(out,format.toLatin1());w.setQuality(92);w.write(im);}}QMessageBox::information(this,tr("Batch"),tr("Conversione completata."));}
void NebImageWindow::batchResize(){QStringList input=QFileDialog::getOpenFileNames(this,tr("Seleziona immagini"),{},imageFilter());if(input.isEmpty())return;bool ok=false;int width=QInputDialog::getInt(this,tr("Ridimensiona"),tr("Larghezza massima (px):"),1920,32,50000,1,&ok);if(!ok)return;QString dir=QFileDialog::getExistingDirectory(this,tr("Cartella di destinazione"));if(dir.isEmpty())return;for(const QString&p:input){QImage im(p);if(!im.isNull())im.scaledToWidth(width,Qt::SmoothTransformation).save(QDir(dir).filePath(QFileInfo(p).fileName()),nullptr,94);}QMessageBox::information(this,tr("Batch"),tr("Ridimensionamento completato."));}
void NebImageWindow::contactSheet(){QStringList input=QFileDialog::getOpenFileNames(this,tr("Seleziona immagini"),{},imageFilter());if(input.isEmpty())return;const int cellW=300,cellH=230,cols=4,rows=(input.size()+cols-1)/cols;QImage sheet(cols*cellW,rows*cellH,QImage::Format_RGB32);sheet.fill(Qt::white);QPainter p(&sheet);for(int i=0;i<input.size();++i){QImage im(input[i]);QRect cell((i%cols)*cellW,(i/cols)*cellH,cellW,cellH);QImage thumb=im.scaled(cellW-20,cellH-45,Qt::KeepAspectRatio,Qt::SmoothTransformation);p.drawImage(QPoint(cell.x()+(cellW-thumb.width())/2,cell.y()+8),thumb);p.setPen(Qt::black);p.drawText(QRect(cell.x()+8,cell.bottom()-30,cellW-16,24),Qt::AlignCenter,QFileInfo(input[i]).fileName());}p.end();QString target=QFileDialog::getSaveFileName(this,tr("Salva provino"),"contact-sheet.jpg",tr("JPEG (*.jpg);;PNG (*.png)"));if(!target.isEmpty())sheet.save(target,nullptr,94);}
void NebImageWindow::toggleSlideshow(){if(slideTimer_->isActive()){slideTimer_->stop();if(isFullScreen())showNormal();statusBar()->showMessage(tr("Presentazione terminata"),2000);}else if(!files_.isEmpty()){bool ok=false;int seconds=QInputDialog::getInt(this,tr("Presentazione"),tr("Secondi per immagine:"),4,1,60,1,&ok);if(ok){showFullScreen();slideTimer_->start(seconds*1000);}}}
void NebImageWindow::closeEvent(QCloseEvent *e){if(maybeSave())e->accept();else e->ignore();}
void NebImageWindow::dragEnterEvent(QDragEnterEvent *e){if(e->mimeData()->hasUrls())e->acceptProposedAction();}
void NebImageWindow::dropEvent(QDropEvent *e){for(const QUrl &u:e->mimeData()->urls()){QFileInfo f(u.toLocalFile());if(f.isDir()){setFolder(f.absoluteFilePath());break;}if(f.isFile()&&isImage(f.absoluteFilePath())){openFile(f.absoluteFilePath());break;}}}
bool NebImageWindow::eventFilter(QObject *watched,QEvent *event){return QMainWindow::eventFilter(watched,event);}
