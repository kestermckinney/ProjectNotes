// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "EmailTypes.h"

#include <QPageLayout>

#include <functional>

namespace PN::Comm {

struct RenderRequest {
    QUuid operationId;
    quint64 previewRevision = 0;
    QString html;
    QString outputStem;
    // Captured from ReportDocument at preparation time. The renderer must not
    // substitute a generic page size/orientation for a report workflow.
    QPageLayout pageLayout;
    // An absolute, non-existent PDF path inside the caller's operation-owned
    // staging directory.  The renderer never chooses a shared temp location.
    QString outputPdfPath;
};

struct RenderResult {
    QUuid operationId;
    quint64 previewRevision = 0;
    Artifact pdf;
    ServiceError error;
};

class ReportRenderer {
public:
    using Completion = std::function<void(RenderResult)>;
    virtual ~ReportRenderer() = default;
    virtual void render(RenderRequest request, Completion completion) = 0;
    virtual void cancel(const QUuid &operationId) = 0;
};

} // namespace PN::Comm
