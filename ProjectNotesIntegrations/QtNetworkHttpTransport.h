// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "HttpTransport.h"

#include <QObject>
#include <QHash>
#include <QPointer>

class QNetworkAccessManager;
class QNetworkReply;

namespace PN::Comm {

// Small production implementation of the token-free HTTP seam.  OAuth and
// Graph own authorization decisions; this class only performs the already
// validated request and returns its exact transport result.
class QtNetworkHttpTransport final : public QObject, public HttpTransport
{
public:
    explicit QtNetworkHttpTransport(QObject *parent = nullptr, int timeoutMs = 30000);

    void send(HttpRequest request, Completion completion) override;
    void cancel(const QUuid &operationId) override;

private:
    struct Pending {
        QPointer<QNetworkReply> reply;
        bool timedOut = false;
    };

    QNetworkAccessManager *m_network = nullptr;
    QHash<QUuid, Pending> m_pending;
    int m_timeoutMs = 30000;
};

} // namespace PN::Comm
