// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesMeetings/MeetingBackend.h"

#include <QSet>

namespace PN::Comm {

// Writes an .ics file to the cache and opens it with the desktop's default
// calendar app, where the user reviews the event and sends the invitations.
// The calendar counterpart of EmlFileEmailBackend.
class IcsFileMeetingBackend final : public MeetingBackend {
public:
    using FileOpener = std::function<bool(const QUrl &)>;
    // `openFailureCode` names the failure when `opener` returns false, so a
    // backend that opens one specific application can say which.
    explicit IcsFileMeetingBackend(QString cacheDirectory, FileOpener opener = {},
                                   QString openFailureCode = QStringLiteral("calendar-open-failed"));
    void handoff(MeetingRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    QString m_cacheDirectory;
    FileOpener m_opener;
    QString m_openFailureCode;
    QSet<QUuid> m_cancelled;
};

} // namespace PN::Comm
