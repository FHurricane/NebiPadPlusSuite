#include "pdfpodofo.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUuid>

#ifdef NEBIPAD_HAS_PODOFO
#include <podofo/podofo.h>

using namespace PoDoFo;

namespace {
QString message(const PdfError &error) { return QString::fromUtf8(error.what()); }

bool replacePdf(const QString &path, PdfMemDocument &document, QString *error)
{
    const QFileInfo info(path);
    const QString temporary = info.dir().filePath("." + info.fileName() + ".nebipad-" +
        QUuid::createUuid().toString(QUuid::WithoutBraces) + ".pdf");
    const QString backup = temporary + ".backup";
    try { document.Save(temporary.toStdString()); }
    catch (const PdfError &exception) { if (error) *error = message(exception); return false; }
    if (!QFile::rename(path, backup)) {
        QFile::remove(temporary); if (error) *error = QObject::tr("Impossibile creare la copia di sicurezza del PDF."); return false;
    }
    if (!QFile::rename(temporary, path)) {
        QFile::rename(backup, path); if (error) *error = QObject::tr("Impossibile sostituire il PDF modificato."); return false;
    }
    QFile::remove(backup);
    return true;
}

template<typename Operation>
bool edit(const QString &path, Operation operation, QString *error)
{
    try {
        PdfMemDocument document;
        document.Load(path.toStdString());
        operation(document);
        return replacePdf(path, document, error);
    } catch (const PdfError &exception) {
        if (error) *error = message(exception);
        return false;
    } catch (const std::exception &exception) {
        if (error) *error = QString::fromUtf8(exception.what());
        return false;
    }
}
}

bool podofoAvailable() { return true; }
int podofoPageCount(const QString &path, QString *error)
{
    try { PdfMemDocument document; document.Load(path.toStdString()); return int(document.GetPages().GetCount()); }
    catch (const PdfError &exception) { if (error) *error = message(exception); return 0; }
}
bool podofoAddText(const QString &path, const PdfTextOverlay &o, QString *error)
{
    return edit(path, [&](PdfMemDocument &d) {
        if (o.page < 1 || unsigned(o.page) > d.GetPages().GetCount()) throw std::out_of_range("Pagina non valida");
        auto &page = d.GetPages().GetPageAt(unsigned(o.page - 1));
        PdfPainter painter; painter.SetCanvas(page);
        PdfFont *font = d.GetFonts().SearchFont("Arial");
        if (!font) font = &d.GetFonts().GetStandard14Font(PdfStandard14FontType::Helvetica);
        painter.TextState.SetFont(*font, o.size);
        painter.DrawText(o.text.toUtf8().toStdString(), o.x, o.y);
        painter.FinishDrawing();
    }, error);
}
bool podofoAddImage(const QString &path, const PdfImageOverlay &o, QString *error)
{
    return edit(path, [&](PdfMemDocument &d) {
        if (o.page < 1 || unsigned(o.page) > d.GetPages().GetCount()) throw std::out_of_range("Pagina non valida");
        auto image = d.CreateImage(); const PdfImageInfo info = image->Load(o.imagePath.toStdString());
        PdfPainter painter; painter.SetCanvas(d.GetPages().GetPageAt(unsigned(o.page - 1)));
        painter.DrawImage(*image, o.x, o.y, o.width / info.Width, o.height / info.Height);
        painter.FinishDrawing();
    }, error);
}
bool podofoInsertPage(const QString &path, int page, QString *error)
{ return edit(path, [&](PdfMemDocument &d) { d.GetPages().CreatePageAt(unsigned(qBound(0, page - 1, int(d.GetPages().GetCount()))), PdfPageSize::A4); }, error); }
bool podofoDeletePage(const QString &path, int page, QString *error)
{ return edit(path, [&](PdfMemDocument &d) { if (d.GetPages().GetCount() <= 1) throw std::runtime_error("Il PDF deve conservare almeno una pagina"); d.GetPages().RemovePageAt(unsigned(page - 1)); }, error); }
bool podofoRotatePage(const QString &path, int page, int degrees, QString *error)
{ return edit(path, [&](PdfMemDocument &d) { auto &p = d.GetPages().GetPageAt(unsigned(page - 1)); p.SetRotation(int(p.GetRotation()) + degrees); }, error); }
bool podofoMovePage(const QString &path, int from, int to, QString *error)
{ return edit(path, [&](PdfMemDocument &d) { if (!d.GetPages().GetPageAt(unsigned(from - 1)).MoveTo(unsigned(to - 1))) throw std::runtime_error("Spostamento pagina non riuscito"); }, error); }
bool podofoMergePdf(const QString &path, const QString &other, QString *error)
{ return edit(path, [&](PdfMemDocument &d) { PdfMemDocument source; source.Load(other.toStdString()); d.GetPages().AppendDocumentPages(source); }, error); }
bool podofoSetMetadata(const QString &path, const PdfMetadataValues &v, QString *error)
{
    return edit(path, [&](PdfMemDocument &d) {
        d.GetMetadata().SetTitle(PdfString(v.title.toUtf8().toStdString()));
        d.GetMetadata().SetAuthor(PdfString(v.author.toUtf8().toStdString()));
        d.GetMetadata().SetSubject(PdfString(v.subject.toUtf8().toStdString()));
        std::vector<std::string> keywords;
        for (const QString &keyword : v.keywords.split(',', Qt::SkipEmptyParts))
            keywords.push_back(keyword.trimmed().toUtf8().toStdString());
        d.GetMetadata().SetKeywords(std::move(keywords));
        d.GetMetadata().SetCreator(PdfString("NebiPad+ Impagination"));
    }, error);
}
#else
bool podofoAvailable() { return false; }
int podofoPageCount(const QString &, QString *error) { if (error) *error = "PoDoFo non disponibile"; return 0; }
#define STUB(name, signature) bool name signature { if (error) *error = "PoDoFo non disponibile"; return false; }
STUB(podofoAddText, (const QString &, const PdfTextOverlay &, QString *error))
STUB(podofoAddImage, (const QString &, const PdfImageOverlay &, QString *error))
STUB(podofoInsertPage, (const QString &, int, QString *error))
STUB(podofoDeletePage, (const QString &, int, QString *error))
STUB(podofoRotatePage, (const QString &, int, int, QString *error))
STUB(podofoMovePage, (const QString &, int, int, QString *error))
STUB(podofoMergePdf, (const QString &, const QString &, QString *error))
STUB(podofoSetMetadata, (const QString &, const PdfMetadataValues &, QString *error))
#endif
