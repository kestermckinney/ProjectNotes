// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/EmailBackend.h"

#include <QSet>
#include <QUrl>

namespace PN::Comm {

// Writes the message as an .eml file in the cache and opens it with whatever
// mail application the desktop has registered for that type, where the user
// reviews the draft and sends it.  The counterpart of IcsFileMeetingBackend,
// and the successor to the mailto: URL, which could carry neither HTML nor
// attachments.
class EmlFileEmailBackend final : public EmailBackend {
public:
    using FileOpener = std::function<bool(const QUrl &)>;
    explicit EmlFileEmailBackend(QString cacheDirectory, FileOpener opener = {});
    void handoff(EmailRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    QString m_cacheDirectory;
    FileOpener m_opener;
    QSet<QUuid> m_cancelled;
    // Opening a file with the desktop's handler is unobservable and may already
    // have created a compose window even when the opener reports failure; never
    // repeat it.
    QSet<QUuid> m_launchAttempted;
};

} // namespace PN::Comm
