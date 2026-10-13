// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#include "TextDocumentReportRenderer.h"
#include "RendererResourcePolicy.h"

#include <QAbstractTextDocumentLayout>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextFrameFormat>
#include <QTextOption>
#include <QThreadPool>
#include <QUrl>

namespace PN::Comm {
namespace {

// Device resolution for the PDF. High enough that layout rounding of 1px
// borders and small fonts does not show; the PDF itself stays vector.
constexpr int kPdfResolution = 1200;

// Emulates the reports' `body { padding: .2in }`, which QTextDocument ignores,
// when a workflow's page layout has no margins of its own.
constexpr qreal kFallbackMarginInches = 0.2;

QByteArray decodeDataUrl(const QUrl &url)
{
    // data:[<mediatype>][;base64],<payload>
    const QString text = url.toString(QUrl::FullyEncoded);
    const qsizetype comma = text.indexOf(u',');
    if (comma < 0)
        return {};
    const QString header = text.mid(5, comma - 5);   // after "data:"
    const QByteArray payload = text.mid(comma + 1).toLatin1();
    if (header.endsWith(QLatin1String(";base64"), Qt::CaseInsensitive))
        return QByteArray::fromBase64(QByteArray::fromPercentEncoding(payload));
    return QByteArray::fromPercentEncoding(payload);
}

// QTextDocument would otherwise resolve image and stylesheet URLs against the
// local file system and qrc. Apply RendererResourcePolicy instead.
//
// Returning nothing is not enough for images: when a resource is null, Qt's
// image handler falls back to QImage::load() on the raw name, which reads local
// files and qrc paths directly. Blocked images therefore resolve to a
// transparent placeholder, which also suppresses the broken-image icon.
class RestrictedTextDocument final : public QTextDocument {
protected:
    QVariant loadResource(int type, const QUrl &name) override
    {
        QByteArray bytes;
        if (name.scheme() == QLatin1String("data") && rendererResourceRequestAllowed(name, true))
            bytes = decodeDataUrl(name);
        if (type == QTextDocument::ImageResource) {
            QImage image;
            if (!bytes.isEmpty())
                image.loadFromData(bytes);
            if (image.isNull()) {
                image = QImage(1, 1, QImage::Format_ARGB32);
                image.fill(Qt::transparent);
            }
            return image;
        }
        if (bytes.isEmpty())
            return QString();   // non-null: no stylesheet or other resource
        return bytes;
    }
};


} // namespace

TextDocumentReportRenderer::TextDocumentReportRenderer(QObject *parent)
    : QObject(parent)
{
}

TextDocumentReportRenderer::~TextDocumentReportRenderer()
{
    // Workers queue their result to this object. Waiting here guarantees no
    // worker posts to a destroyed renderer; events already queued are dropped
    // by Qt when the object is deleted.
    QThreadPool::globalInstance()->waitForDone();
}

QString TextDocumentReportRenderer::renderToFile(const RenderRequest &request)
{
    const QPageLayout layout = request.pageLayout.isValid()
        ? request.pageLayout
        : QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, {});

    QPdfWriter writer(request.outputPdfPath);
    writer.setResolution(kPdfResolution);
    writer.setPageLayout(layout);
    writer.setTitle(request.outputStem);
    writer.setCreator(QStringLiteral("Project Notes"));

    RestrictedTextDocument document;
    // Lay out against the PDF device so CSS pixel lengths, point sizes and
    // borders scale by the device resolution, and the document is paginated
    // to the page's paint rectangle. Without an explicit page size,
    // QTextDocument::print() adds its own 2 cm margins and page numbers.
    document.documentLayout()->setPaintDevice(&writer);
    QTextOption option = document.defaultTextOption();
    option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    document.setDefaultTextOption(option);
    document.setHtml(request.html);

    const QMarginsF margins = layout.margins(QPageLayout::Inch);
    const bool hasMargins = margins.left() > 0 || margins.right() > 0
        || margins.top() > 0 || margins.bottom() > 0;
    QTextFrameFormat root = document.rootFrame()->frameFormat();
    // Frame lengths are CSS pixels (96 per inch); the layout scales them to the
    // paint device, so the margin is given in those units, not device pixels.
    root.setMargin(hasMargins ? 0 : kFallbackMarginInches * 96.0);
    document.rootFrame()->setFrameFormat(root);

    // Table rows may break across pages: QTextDocument has no equivalent of
    // `page-break-inside: avoid`, and repeating header rows (<thead>) keep a
    // continued table readable. See docs/TextDocumentPdfRendererPlan.md.
    document.setPageSize(QSizeF(writer.width(), writer.height()));
    document.print(&writer);

    if (QFileInfo(request.outputPdfPath).size() <= 0)
        return QStringLiteral("renderer-pdf-failed");
    return {};
}

void TextDocumentReportRenderer::render(RenderRequest request, Completion completion)
{
    RenderResult invalid{request.operationId, request.previewRevision};
    const QFileInfo output(request.outputPdfPath);
    if (request.operationId.isNull() || request.html.isEmpty() || !output.isAbsolute()
        || output.exists() || output.suffix().compare(QStringLiteral("pdf"), Qt::CaseInsensitive) != 0
        || !output.dir().exists()) {
        invalid.error = {QStringLiteral("renderer-input-invalid"), {}};
        completion(std::move(invalid));
        return;
    }
    {
        QMutexLocker locker(&m_mutex);
        if (m_cancelled.remove(request.operationId)) {
            locker.unlock();
            invalid.error = {QStringLiteral("operation-cancelled"), {}};
            completion(std::move(invalid));
            return;
        }
        m_pending.insert(request.operationId, std::move(completion));
    }

    QThreadPool::globalInstance()->start([this, request]() {
        const QString error = renderToFile(request);
        QMetaObject::invokeMethod(this, [this, request, error]() { finish(request, error); },
                                  Qt::QueuedConnection);
    });
}

void TextDocumentReportRenderer::finish(const RenderRequest &request, const QString &errorCode)
{
    Completion completion;
    bool cancelled = false;
    {
        QMutexLocker locker(&m_mutex);
        completion = m_pending.take(request.operationId);
        cancelled = m_cancelled.remove(request.operationId);
    }
    if (!completion)
        return;

    RenderResult result{request.operationId, request.previewRevision};
    if (cancelled) {
        QFile::remove(request.outputPdfPath);
        result.error = {QStringLiteral("operation-cancelled"), {}};
    } else if (!errorCode.isEmpty()) {
        result.error = {errorCode, {}};
    } else {
        const QFileInfo info(request.outputPdfPath);
        result.pdf = {QUuid::createUuid(), request.outputPdfPath, info.fileName(),
                      QStringLiteral("application/pdf"), info.size(), {}, true};
    }
    completion(std::move(result));
}

void TextDocumentReportRenderer::cancel(const QUuid &operationId)
{
    // print() cannot be interrupted; the result is discarded when it arrives.
    QMutexLocker locker(&m_mutex);
    m_cancelled.insert(operationId);
}

} // namespace PN::Comm
