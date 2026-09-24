// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/EmailBackend.h"

namespace PN::Comm {
class Office365Service;

class GraphEmailBackend final : public EmailBackend {
public:
    explicit GraphEmailBackend(Office365Service *service);
    void handoff(EmailRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

    static constexpr qint64 maximumSimpleAttachmentBytes = 3 * 1024 * 1024;
    static constexpr qint64 uploadChunkBytes = 320 * 1024;
    static bool isAllowedPresentationUrl(const QUrl &url);

private:
    Office365Service *m_service = nullptr;
    QSet<QUuid> m_cancelled;
};
} // namespace PN::Comm
