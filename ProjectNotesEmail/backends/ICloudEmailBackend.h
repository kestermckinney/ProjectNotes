// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "ProjectNotesEmail/EmailBackend.h"
#include "ProjectNotesIntegrations/ICloudCredentials.h"

#include <QObject>
#include <QHash>

class QSslSocket;
class QTimer;

namespace PN::Comm {

// Stores an unsent RFC 5322 message in iCloud Mail's Drafts mailbox using
// IMAP APPEND. iCloud does not return a browser URL for the new draft.
class ICloudEmailBackend final : public QObject, public EmailBackend {
public:
    explicit ICloudEmailBackend(ICloudCredentialsProvider credentials, QObject *parent = nullptr);
    ~ICloudEmailBackend() override;

    void handoff(EmailRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    struct Session;
    void consume(Session *session);
    void finish(Session *session, EmailHandoffResult result);
    void fail(Session *session, QString code, QString displayText = {}, bool uncertain = false);

    ICloudCredentialsProvider m_credentials;
    QHash<QUuid, Session *> m_sessions;
};

} // namespace PN::Comm
