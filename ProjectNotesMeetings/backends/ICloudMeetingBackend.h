// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesMeetings/MeetingBackend.h"
#include "ProjectNotesIntegrations/HttpTransport.h"
#include "ProjectNotesIntegrations/ICloudCredentials.h"

#include <QObject>
#include <QHash>
#include <QSet>

namespace PN::Comm {

// Creates an iCloud calendar event through CalDAV. Attendees are deliberately
// omitted so the provider cannot send invitations during creation; the review
// window puts the selected addresses on the clipboard for the user to paste.
class ICloudMeetingBackend final : public QObject, public MeetingBackend {
public:
    explicit ICloudMeetingBackend(ICloudCredentialsProvider credentials, QObject *parent = nullptr,
                                  HttpTransport *transport = nullptr);
    ~ICloudMeetingBackend() override;

    void handoff(MeetingRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    struct Operation;
    using DavCompletion = std::function<void(HttpResponse)>;

    void discover(Operation *operation, int step, QUrl url);
    void createEvent(Operation *operation, QUrl calendarUrl, QString calendarName);
    void dav(Operation *operation, QByteArray method, QUrl url, QByteArray body, int depth,
             DavCompletion completion, int redirects = 0);
    void fail(Operation *operation, QString code, QString displayText = {}, bool uncertain = false);
    void finish(Operation *operation, MeetingHandoffResult result);

    ICloudCredentialsProvider m_credentials;
    HttpTransport *m_transport = nullptr;
    QHash<QUuid, Operation *> m_operations;
};

} // namespace PN::Comm
