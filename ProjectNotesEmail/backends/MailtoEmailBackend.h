// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/EmailBackend.h"

#include <QUrl>
#include <QtGlobal>

namespace PN::Comm {

class MailtoEmailBackend final : public EmailBackend {
public:
    using UrlOpener = std::function<bool(const QUrl &)>;
    explicit MailtoEmailBackend(UrlOpener opener = {});

    // A mailto: URL is handed to the platform, so the ceiling is the platform's,
    // not the protocol's. Windows routes it through ShellExecute, which truncates
    // near 2048 characters without reporting it, so stay well under that. Other
    // desktops pass the URL as a process argument and tolerate far more, which is
    // the difference between a normal meeting note succeeding and failing.
#if defined(Q_OS_WIN)
    static constexpr qsizetype maximumEncodedLength = 1900;
#else
    static constexpr qsizetype maximumEncodedLength = 16000;
#endif
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
