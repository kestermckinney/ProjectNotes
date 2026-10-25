// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/ReportRenderer.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QSet>

namespace PN::Comm {

// Prints report HTML to PDF with QTextDocument and QPdfWriter. Rendering runs
// on a QThreadPool worker; the document and writer are created, used and
// destroyed there, and only the RenderResult is queued back to this object's
// thread. Resource loading follows rendererResourceRequestAllowed(): embedded
// data: images load, file/qrc/network resources never do.
class TextDocumentReportRenderer final : public QObject, public ReportRenderer {
    Q_OBJECT
public:
    explicit TextDocumentReportRenderer(QObject *parent = nullptr);
    ~TextDocumentReportRenderer() override;

    void render(RenderRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

    // Synchronous core, also used by tests. Writes request.outputPdfPath and
    // returns the error code ("" on success).
    static QString renderToFile(const RenderRequest &request);

private:
    void finish(const RenderRequest &request, const QString &errorCode);

    QMutex m_mutex;
    QSet<QUuid> m_cancelled;
    QHash<QUuid, Completion> m_pending;
};

} // namespace PN::Comm
