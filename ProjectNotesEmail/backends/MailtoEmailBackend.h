// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/EmailBackend.h"

#include <QUrl>

namespace PN::Comm {

class MailtoEmailBackend final : public EmailBackend {
public:
    using UrlOpener = std::function<bool(const QUrl &)>;
    explicit MailtoEmailBackend(UrlOpener opener = {});

    static constexpr qsizetype maximumEncodedLength = 1900;
    static std::optional<QUrl> buildUrl(const EmailRequest &request, ValidationResult *validation = nullptr);
    void handoff(EmailRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    UrlOpener m_opener;
    QSet<QUuid> m_cancelled;
    // A mailto launch is unobservable and may already have created a compose
    // window even if the platform opener reports failure; never repeat it.
    QSet<QUuid> m_launchAttempted;
};

} // namespace PN::Comm
