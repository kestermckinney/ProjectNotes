// Copyright (C) 2026 Paul McKinney
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "CommunicationTypes.h"

#include <QByteArray>
#include <QHash>
#include <QUuid>
#include <QUrl>

#include <functional>

namespace PN::Comm {

struct HttpRequest {
    QUuid operationId;
    QByteArray method;
    QUrl url;
    QHash<QByteArray, QByteArray> headers;
    QByteArray body;
};

struct HttpResponse {
    QUuid operationId;
    int statusCode = 0;
    QHash<QByteArray, QByteArray> headers;
    QByteArray body;
    ServiceError error;
};

class HttpTransport {
public:
    using Completion = std::function<void(HttpResponse)>;
    virtual ~HttpTransport() = default;
    virtual void send(HttpRequest request, Completion completion) = 0;
    virtual void cancel(const QUuid &operationId) = 0;
};

} // namespace PN::Comm

Q_DECLARE_METATYPE(PN::Comm::HttpRequest)
Q_DECLARE_METATYPE(PN::Comm::HttpResponse)
